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
#include "utf8.h"

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
    case EXEC:
    case EVAL:
        // Integer values evaluate as self
        return rt.push(obj) ? OK : ERROR;
    case SIZE:
        return ptrdiff(payload, obj) + leb128size(payload);
    case PARSE:
        return object_parser(OBJECT_PARSER_ARG(), rt);
    case RENDER:
        return obj->object_renderer(OBJECT_RENDERER_ARG(), rt);
    case HELP:
        return (intptr_t) "integer";

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

    record(integer, "Parsing [%s]", (utf8) p.source);

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
            // The HP syntax takes A-F as digits, and b/d as bases
            // Prefer to accept B and D suffixes, but only if no
            // digit above was found in the base
            base = Settings.base;

            uint max = 0;
            for (byte_p e = s; e < endp - 1; e++)
                if (max < value[*e])
                    max = value[*e];

            switch(endp[-1])
            {
            case 'b':
            case 'B':
                if (max < 2)
                    base = 2;
                else
                    endp++;
                break;
            case 'O':
            case 'o':
                base = 8;
                break;
            case 'd':
            case 'D':
                if (max < 10)
                    base = 10;
                else
                    endp++;
                break;
            case 'H':
            case 'h':
                base = 16;
                break;
            default:
                // Use current default base
                endp++;
                break;
            }
            switch(base)
            {
            case  2: type = ID_bin_integer; break;
            case  8: type = ID_oct_integer; break;
            case 10: type = ID_dec_integer; break;
            case 16: type = ID_hex_integer; break;
            }
            endp--;
            if (s >= endp)
            {
                rt.based_number_error(). source(s);
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
            rt.based_digit_error().source(s-1);
            return ERROR;
        }
        ularge next = result * base + v;
        record(integer, "Digit %c value %u value=%llu next=%llu",
               s[-1], v, result, next);

        // If the value does not fit in an integer, defer to bignum / real
        if ((next << shift) <  result)
        {
            rt.based_range_error().source(s);
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
    if (*s == Settings.decimalDot || utf8_codepoint(s) == Settings.exponentChar)
        return SKIP;

    // Record output
    p.end = (utf8) s - (utf8) p.source;
    if (sign < 0)
        p.out = rt.make<neg_integer>(type, result);
    else
        p.out = rt.make<integer>(type, result);

    return OK;
}


static size_t render_num(renderer &r,
                         integer_p num,
                         uint      base,
                         cstring   fmt)
// ----------------------------------------------------------------------------
//   Convert an integer value to the proper format
// ----------------------------------------------------------------------------
//   This is necessary because the arm-none-eabi-gcc printf can't do 64-bit
//   I'm getting non-sensible output
{
    // Copy the '#' or '-' sign
    if (*fmt)
        r.put(*fmt++);

    // Get denominator for the base
    size_t findex = r.size();
    ularge n = num->value<ularge>();

    // Keep dividing by the base until we get 0
    do
    {
        ularge digit = n % base;
        n /= base;
        char c = (digit < 10) ? digit + '0' : digit + ('A' - 10);
        r.put(c);
    } while (n);

    // Revert the digits
    char *dest = (char *) r.text();
    char *first = dest + findex;
    char *last = dest + r.size() - 1;
    while (first < last)
    {
        char tmp = *first;
        *first = *last;
        *last = tmp;
        last--;
        first++;
    }

    // Add suffix
    if (*fmt)
        r.put(*fmt++);

    return r.size();
}


OBJECT_RENDERER_BODY(integer)
// ----------------------------------------------------------------------------
//   Render the integer into the given string buffer
// ----------------------------------------------------------------------------
{
    size_t result = render_num(r, this, 10, "");
    return result;
}


template<>
OBJECT_RENDERER_BODY(neg_integer)
// ----------------------------------------------------------------------------
//   Render the negative integer value into the given string buffer
// ----------------------------------------------------------------------------
{
    return render_num(r, this, 10, "-");
}


template<>
OBJECT_RENDERER_BODY(hex_integer)
// ----------------------------------------------------------------------------
//   Render the hexadecimal integer value into the given string buffer
// ----------------------------------------------------------------------------
{
    return render_num(r, this, 16, "#h");
}

template<>
OBJECT_RENDERER_BODY(dec_integer)
// ----------------------------------------------------------------------------
//   Render the decimal based number
// ----------------------------------------------------------------------------
{
    return render_num(r, this, 10, "#d");
}

template<>
OBJECT_RENDERER_BODY(oct_integer)
// ----------------------------------------------------------------------------
//   Render the octal integer value into the given string buffer
// ----------------------------------------------------------------------------
{
    return render_num(r, this, 8, "#o");
}

template<>
OBJECT_RENDERER_BODY(bin_integer)
// ----------------------------------------------------------------------------
//   Render the binary integer value into the given string buffer
// ----------------------------------------------------------------------------
{
    return render_num(r, this, 2, "#b");
}
