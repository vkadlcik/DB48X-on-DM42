// ****************************************************************************
//  integer.cc                                                    DB48X project
// ****************************************************************************
//
//   File Description:
//
//     Implementation of basic integer operations
//
//
//
//
//
//
//
//
// ****************************************************************************
//   (C) 2022 Christophe de Dinechin <christophe@dinechin.org>
//   This software is licensed under the terms outlined in LICENSE.txt
// ****************************************************************************
//   This file is part of DB48X.
//
//   DB48X is free software: you can redistribute it and/or modify
//   it under the terms outlined in the LICENSE.txt file
//
//   DB48X is distributed in the hope that it will be useful,
//   but WITHOUT ANY WARRANTY; without even the implied warranty of
//   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
// ****************************************************************************

#include "integer.h"

#include "parser.h"
#include "renderer.h"
#include "runtime.h"
#include "settings.h"

#include <stdio.h>


RECORDER(integer, 16, "Integers");

OBJECT_HANDLER_BODY(integer)
// ----------------------------------------------------------------------------
//    Handle commands for integers
// ----------------------------------------------------------------------------
{
    record(integer, "Command %+s on %p", name(op), obj);
    switch(op)
    {
    case EVAL:
        // Integer values evaluate as self
        rt.push(obj);
        return 0;
    case SIZE:
        return ptrdiff(payload, obj) + leb128size(payload);
    case PARSE:
        return object_parser(OBJECT_PARSER_ARG(), rt);
    case RENDER:
        return obj->object_renderer(OBJECT_RENDERER_ARG(), rt);

    default:
        // Check if anyone else knows how to deal with it
        return DELEGATE(object);
    }
}


OBJECT_PARSER_BODY(integer)
// ----------------------------------------------------------------------------
//    Try to parse this as an integer
// ----------------------------------------------------------------------------
//    For simplicity, this deals with all kinds of integers
{
    int         sign       = 1;
    int         base       = 10;
    id          type       = ID_integer;
    const byte  NODIGIT    = (byte) -1;

    record(integer, "Parsing [%s]", p.source);

    // Array of values for digits
    static byte value[256] = { 0 };
    if (!value[(byte) 'A'])
    {
        // Initialize value array on first use
        for (int c = 0; c < 256; c++)
            value[c] = NODIGIT;
        for (int c = '0'; c <= '9'; c++)
            value[c] = c - '0';
        for (int c = 'A'; c <= 'Z'; c++)
            value[c] = c - 'A' + 10;
        for (int c = 'a'; c <= 'z'; c++)
            value[c] = c - 'a' + 10;
    }

    byte_p s = (byte_p) p.source;
    byte_p endp = nullptr;

    if (*s == '-')
    {
        sign = -1;
        type = ID_neg_integer;
        s++;
    }
    else if (*s == '+')
    {
        sign = 1;
        s++;
    }
    else if (*s == '#')
    {
        s++;
        for (byte_p e = s; !endp; e++)
            if (value[*e] == NODIGIT)
                endp = e;

        if (endp > s)
        {
            switch(endp[-1])
            {
            case 'b':
            case 'B':
                base = 2;
                type = ID_bin_integer;
                break;
            case 'O':
            case 'o':
                base = 8;
                type = ID_oct_integer;
                break;
            case 'd':
            case 'D':
                base = 10;
                type = ID_dec_integer;
                break;
            case 'H':
            case 'h':
                base = 16;
                type = ID_hex_integer;
                break;
            default:
                // Check if we can use the current default base
                base = Settings.base;
                if (value[(byte) endp[-1]] > base)
                {
                    rt.error("Invalid base", endp-1);
                    return ERROR;
                }
                break;
            }
            endp--;
            if (s >= endp)
            {
                rt.error("Invalid based number", s);
                return ERROR;
            }
        }
    }

    // If this is a + or - operator, skip
    if (*s && value[*s] >= base)
        return SKIP;

    // Loop on digits
    ularge result = 0;
    uint shift = sign < 0 ? 1 : 0;
    byte v;
    while ((!endp || s < endp) && (v = value[*s++]) != NODIGIT)
    {
        if (v >= base)
        {
            rt.error("Invalid digit for base", s-1);
            return ERROR;
        }
        ularge next = result * base + v;
        record(integer, "Digit %c value %u value=%llu next=%llu",
               s[-1], v, result, next);

        // If the value does not fit in an integer, defer to bignum / real
        if ((next << shift) <  result)
        {
            rt.error("Number is too big");
            return WARN;
        }

        result = next;
    }

    // Skip base if one was given, else point at char that got us out
    if (endp && s == endp)
        s++;
    else
        s--;


    // Check if we finish with something indicative of a real number
    if (*s == Settings.decimalDot || *s == (byte) Settings.exponentChar)
        return SKIP;

    // Record output
    p.end = (cstring) s - (cstring) p.source;
    if (sign < 0)
        p.out = rt.make<neg_integer>(type, result);
    else
        p.out = rt.make<integer>(type, result);

    return OK;
}


OBJECT_RENDERER_BODY(integer)
// ----------------------------------------------------------------------------
//   Render the integer into the given string buffer
// ----------------------------------------------------------------------------
{
    ularge v = value<ularge>();
    size_t result = snprintf(r.target, r.length, "%lu", v);
    record(integer, "Render %llu (0x%llX) as [%s]", v, v, (cstring) r.target);
    return result;
}


template<>
OBJECT_RENDERER_BODY(neg_integer)
// ----------------------------------------------------------------------------
//   Render the negative integer value into the given string buffer
// ----------------------------------------------------------------------------
{
    return snprintf(r.target, r.length, "-%lu", value<ularge>());
}


template<>
OBJECT_RENDERER_BODY(hex_integer)
// ----------------------------------------------------------------------------
//   Render the hexadecimal integer value into the given string buffer
// ----------------------------------------------------------------------------
{
    return snprintf(r.target, r.length, "#%lXh", value<ularge>());
}

template<>
OBJECT_RENDERER_BODY(oct_integer)
// ----------------------------------------------------------------------------
//   Render the octal integer value into the given string buffer
// ----------------------------------------------------------------------------
{
    return snprintf(r.target, r.length, "#%loo", value<ularge>());
}

template<>
OBJECT_RENDERER_BODY(dec_integer)
// ----------------------------------------------------------------------------
//   Render the negative integer value into the given string buffer
// ----------------------------------------------------------------------------
{
    return snprintf(r.target, r.length, "#%lud", value<ularge>());
}

template<>
OBJECT_RENDERER_BODY(bin_integer)
// ----------------------------------------------------------------------------
//   Render the binary integer value into the given string buffer
// ----------------------------------------------------------------------------
{
    // Why is there no printf format for binary?
    ularge num = value<ularge>();
    char  *p   = r.target;
    char  *end = p + r.length;
    if (p < end)
        *p++ = '#';
    ularge testbit = num;
    do
    {
        p++;
        testbit >>= 1;
    } while (testbit);
    if (p < end)
        *p = 'b';
    if (p+1 < end)
        p[1] = 0;
    size_t result = p + 1 - (char *) r.target;
    do
    {
        *(--p) = (num & 1) ? '1' : '0';
        num >>= 1;
    } while (num);
    return result;
}
