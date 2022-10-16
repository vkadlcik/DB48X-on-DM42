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

#include "runtime.h"
#include "settings.h"

#include <stdio.h>


RECORDER(integer, 16, "Integers");

OBJECT_HANDLER_BODY(integer)
// ----------------------------------------------------------------------------
//    Handle commands for integers
// ----------------------------------------------------------------------------
{
    record(integer, "Command %+s on %p", name(cmd), obj);
    switch(cmd)
    {
    case EVAL:
        // Integer values evaluate as self
        rt.push(obj);
        return 0;
    case SIZE:
        return ptrdiff(payload, obj) + leb128size(payload);
    case PARSE:
    {
        parser *p = (parser *) arg;
        return object_parser(p->begin, &p->end, &p->output, rt);
    }
    case RENDER:
    {
        renderer *r = (renderer *) arg;
        return obj->object_renderer(r->begin, r->end, rt);
    }

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

    record(integer, "Parsing [%s]", begin);

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

    byte_p p = (byte_p) begin;
    byte_p endp = nullptr;

    if (*p == '-')
    {
        sign = -1;
        type = ID_neg_integer;
        p++;
    }
    else if (*p == '+')
    {
        sign = 1;
        p++;
    }
    else if (*p == '#')
    {
        p++;
        for (byte_p e = p; !endp; e++)
            if (value[*e] == NODIGIT)
                endp = e;

        if (endp > p)
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
            if (p >= endp)
            {
                rt.error("Invalid based number", p);
                return ERROR;
            }
        }
    }

    // If this is a + or - operator, skip
    if (value[*p] >= base)
        return SKIP;

    // Loop on digits
    ularge result = 0;
    uint shift = sign < 0 ? 1 : 0;
    byte v;
    while ((!endp || p < endp) && (v = value[*p++]) != NODIGIT)
    {
        if (v >= base)
        {
            rt.error("Invalid digit for base", p-1);
            return ERROR;
        }
        ularge next = result * base + v;

        // If the value does not fit in an integer, defer to bignum / real
        if ((next << shift) <  result)
            return SKIP;

        result = next;
    }

    // Check if we finish with something indicative of a real number
    if (*p == Settings.decimalDot || *p == Settings.exponentChar)
        return SKIP;

    if (end)
        *end = (cstring) p;
    if (sign < 0)
    {
        // Write negative integer
        if (out)
            *out = rt.make<neg_integer>(type, result);
    }
    else
    {
        // Write unsigned value
        if (out)
            *out = rt.make<integer>(type, result);
    }

    return OK;
}


OBJECT_RENDERER_BODY(integer)
// ----------------------------------------------------------------------------
//   Render the integer into the given string buffer
// ----------------------------------------------------------------------------
{
    return snprintf(begin, end - begin, "%llu", value<ularge>());
}


template<>
OBJECT_RENDERER_BODY(neg_integer)
// ----------------------------------------------------------------------------
//   Render the negative integer value into the given string buffer
// ----------------------------------------------------------------------------
{
    return snprintf(begin, end - begin, "-%llu", value<ularge>());
}


template<>
OBJECT_RENDERER_BODY(hex_integer)
// ----------------------------------------------------------------------------
//   Render the hexadecimal integer value into the given string buffer
// ----------------------------------------------------------------------------
{
    return snprintf(begin, end - begin, "#%llXh", value<ularge>());
}

template<>
OBJECT_RENDERER_BODY(oct_integer)
// ----------------------------------------------------------------------------
//   Render the octal integer value into the given string buffer
// ----------------------------------------------------------------------------
{
    return snprintf(begin, end - begin, "#%lloo", value<ularge>());
}

template<>
OBJECT_RENDERER_BODY(dec_integer)
// ----------------------------------------------------------------------------
//   Render the negative integer value into the given string buffer
// ----------------------------------------------------------------------------
{
    return snprintf(begin, end - begin, "#%llud", value<ularge>());
}

template<>
OBJECT_RENDERER_BODY(bin_integer)
// ----------------------------------------------------------------------------
//   Render the binary integer value into the given string buffer
// ----------------------------------------------------------------------------
{
    // Why is there no printf format for binary?
    ularge num = value<ularge>();
    char *p = begin;
    if (p < end)
        *p = '#';
    ularge testbit = num;
    do
    {
        p++;
        testbit >>= 1;
    } while (testbit);
    if (p < end)
        *p = 'b';
    size_t result = p + 1 - begin;
    do
    {
        *(--p) = (num & 1) ? '1' : '0';
        num >>= 1;
    } while (num);
    return result;
}
