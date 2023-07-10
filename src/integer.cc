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

#include "bignum.h"
#include "fraction.h"
#include "parser.h"
#include "renderer.h"
#include "runtime.h"
#include "settings.h"
#include "utf8.h"

#include <stdio.h>


RECORDER(integer, 16, "Integers");

SIZE_BODY(integer)
// ----------------------------------------------------------------------------
//   Compute size for all integers
// ----------------------------------------------------------------------------
{
    byte_p p = o->payload();
    return ptrdiff(p, o) + leb128size(p);
}


PARSE_BODY(integer)
// ----------------------------------------------------------------------------
//    Try to parse this as an integer
// ----------------------------------------------------------------------------
//    For simplicity, this deals with all kinds of integers, including bignum
{
    int        base        = 10;
    id         type        = ID_integer;
    const byte NODIGIT     = (byte) -1;
    bool       is_fraction = false;
    object_g   number      = nullptr;
    object_g   numerator   = nullptr;

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

    byte_p s    = (byte_p) p.source;
    byte_p endp = nullptr;

    if (*s == '-')
    {
        // In an equation, '1+3' should interpret '+' as an infix command
        if (p.precedence < 0)
            return SKIP;
        type = ID_neg_integer;
        s++;
    }
    else if (*s == '+')
    {
        if (p.precedence < 0)
            return SKIP;
        s++;
    }
    else if (*s == '#')
    {
        s++;
        for (byte_p e = s; !endp; e++)
            if (value[*e] == NODIGIT && *e != '#')
                endp = e;

        if (endp > s)
        {
            // The HP syntax takes A-F as digits, and b/d as bases
            // Prefer to accept B and D suffixes, but only if no
            // digit above was found in the base
            base     = Settings.base;
            type     = ID_based_integer;

            uint max = 0;
            for (byte_p e = s; e < endp - 1; e++)
                if (max < value[*e])
                    max = value[*e];

            switch (endp[-1])
            {
            case 'b':
            case 'B':
                if (max < 2)
                {
                    base = 2;
                    type = ID_bin_integer;
                }
                else
                    endp++;
                break;
            case 'O':
            case 'o':
                base = 8;
                type = ID_oct_integer;
                break;
            case 'd':
            case 'D':
                if (max < 10)
                {
                    base = 10;
                    type = ID_dec_integer;
                }
                else
                    endp++;
                break;
            case 'H':
            case 'h':
                base = 16;
                type = ID_hex_integer;
                break;
            default:
                // Use current default base
                endp++;
                break;
            }
            endp--;
            if (s >= endp)
            {
                rt.based_number_error().source(s);
                return ERROR;
            }
        }
    }

    // If this is a + or - operator, skip
    if (*s && value[*s] >= base)
        return SKIP;

    do
    {
        // Loop on digits
        ularge result = 0;
        bool   big    = false;
        byte   v;
        if (is_fraction && value[*s] == NODIGIT)
        {
            rt.syntax_error();
            return ERROR;
        }

        while (!endp || s < endp)
        {
            // Check new syntax for based numbers
            if (*s == '#')
            {
                if (result < 2 || result > 36)
                {
                    rt.invalid_base_error().source(s);
                    return ERROR;
                }
                base = result;
                result = 0;
                type = ID_based_integer;
                s++;
                continue;
            }

            v = value[*s++];
            if (v == NODIGIT)
                break;

            if (v >= base)
            {
                rt.based_digit_error().source(s - 1);
                return ERROR;
            }
            ularge next = result * base + v;
            record(integer,
                   "Digit %c value %u value=%llu next=%llu",
                   s[-1],
                   v,
                   result,
                   next);

            // If the value does not fit in an integer, defer to bignum / real
            big = next / base != result;
            if (big)
                break;

            result = next;
        }

        // Check if we need bignum
        bignum_g bresult = nullptr;
        if (big)
        {
            // We may cause garbage collection in bignum arithmetic
            gcbytes gs    = s;
            gcbytes ge    = endp;
            size_t  count = endp - s;

            switch (type)
            {
            case ID_integer:       type = ID_bignum; break;
            case ID_neg_integer:   type = ID_neg_bignum; break;
            case ID_hex_integer:   type = ID_hex_bignum; break;
            case ID_dec_integer:   type = ID_dec_bignum; break;
            case ID_oct_integer:   type = ID_oct_bignum; break;
            case ID_bin_integer:   type = ID_bin_bignum; break;
            case ID_based_integer: type = ID_based_bignum; break;
            default: break;
            }

            // Integrate last digit that overflowed above
            bignum_g bbase  = rt.make<bignum>(ID_bignum, base);
            bignum_g bvalue = rt.make<bignum>(type, v);
            bresult         = rt.make<bignum>(type, result);
            bresult = bvalue + bbase * bresult; // Order matters for types

            while (count--)
            {
                v = value[*gs];
                ++gs;
                if (v == NODIGIT)
                    break;

                if (v >= base)
                {
                    rt.based_digit_error().source(s - 1);
                    return ERROR;
                }
                record(integer, "Digit %c value %u in bignum", s[-1], v);
                bvalue  = rt.make<bignum>(type, v);
                bresult = bvalue + bbase * bresult;
            }

            s    = gs;
            endp = ge;
        }


        // Skip base if one was given, else point at char that got us out
        if (endp && s == endp)
            s++;
        else
            s--;

        // Create the intermediate result, which may GC
        {
            gcutf8 gs = s;
            number = big ? object_p(bresult) : rt.make<integer>(type, result);
            s = gs;
        }
        if (!number)
            return ERROR;

        // Check if we parse a fraction
        if (is_fraction)
        {
            is_fraction = false;
            if (integer_p(object_p(number))->is_zero())
            {
                rt.zero_divide_error();
                return ERROR;
            }
            else if (numerator->is_bignum() || number->is_bignum())
            {
                // We rely here on the fact that an integer can also be read as
                // a bignum (they share the same payload format)
                bignum_g n = (bignum *) (object_p) numerator;
                bignum_g d = (bignum *) (object_p) number;
                number     = (object_p) big_fraction::make(n, d);
            }
            else
            {
                // We rely here on the fact that an integer can also be read as
                // a bignum (they share the same payload format)
                integer_g n = (integer *) (object_p) numerator;
                integer_g d = (integer *) (object_p) number;
                number      = (object_p) fraction::make(n, d);
            }
        }
        else if (*s == '/')
        {
            is_fraction = true;
            numerator   = number;
            number      = nullptr;
            type        = ID_integer;
            s++;
        }
    } while (is_fraction);

    // Check if we finish with something indicative of a fraction or real number
    if (!endp)
    {
        if (*s == Settings.decimal_mark ||
            utf8_codepoint(s) == Settings.exponent_mark)
            return SKIP;
    }

    // Record output
    p.end = (utf8) s - (utf8) p.source;
    p.out = number;

    return OK;
}


static size_t render_num(renderer &r, integer_p num, uint base, cstring fmt)
// ----------------------------------------------------------------------------
//   Convert an integer value to the proper format
// ----------------------------------------------------------------------------
//   This is necessary because the arm-none-eabi-gcc printf can't do 64-bit
//   I'm getting non-sensible output
{
    // If we render to a file, need to first render to scratchpad to be able to
    // revert the digits in memory before writing
    if (r.file_save())
    {
        renderer tmp(r.equation(), r.editing(), r.stack());
        size_t result = render_num(tmp, num, base, fmt);
        r.put(tmp.text(), result);
        return result;
    }

    // Check which kind of spacing to use
    bool based = *fmt == '#';
    uint spacing = based ? Settings.spacing_based : Settings.spacing_mantissa;
    unicode space = based ? Settings.space_based : Settings.space;
    if (r.editing())
        spacing = 0;

    // Copy the '#' or '-' sign
    if (*fmt)
        r.put(*fmt++);

    // Get denominator for the base
    size_t findex = r.size();
    ularge n      = num->value<ularge>();

    // Keep dividing by the base until we get 0
    uint sep = 0;
    do
    {
        ularge digit = n % base;
        n /= base;
        char c = (digit < 10) ? digit + '0' : digit + ('A' - 10);
        r.put(c);

        if (n && ++sep == spacing)
        {
            sep = 0;
            r.put(space);
        }
    } while (n);

    // Revert the digits
    byte *dest  = (byte *) r.text();
    bool multibyte = spacing && space > 0xFF;
    utf8_reverse(dest + findex, dest + r.size(), multibyte);

    // Add suffix
    if (*fmt)
        r.put(*fmt++);

    return r.size();
}


RENDER_BODY(integer)
// ----------------------------------------------------------------------------
//   Render the integer into the given string buffer
// ----------------------------------------------------------------------------
{
    size_t result = render_num(r, o, 10, "");
    return result;
}


template <>
RENDER_BODY(neg_integer)
// ----------------------------------------------------------------------------
//   Render the negative integer value into the given string buffer
// ----------------------------------------------------------------------------
{
    return render_num(r, o, 10, "-");
}


template <>
RENDER_BODY(hex_integer)
// ----------------------------------------------------------------------------
//   Render the hexadecimal integer value into the given string buffer
// ----------------------------------------------------------------------------
{
    return render_num(r, o, 16, "#h");
}

template <>
RENDER_BODY(dec_integer)
// ----------------------------------------------------------------------------
//   Render the decimal based number
// ----------------------------------------------------------------------------
{
    return render_num(r, o, 10, "#d");
}

template <>
RENDER_BODY(oct_integer)
// ----------------------------------------------------------------------------
//   Render the octal integer value into the given string buffer
// ----------------------------------------------------------------------------
{
    return render_num(r, o, 8, "#o");
}

template <>
RENDER_BODY(bin_integer)
// ----------------------------------------------------------------------------
//   Render the binary integer value into the given string buffer
// ----------------------------------------------------------------------------
{
    return render_num(r, o, 2, "#b");
}


template <>
RENDER_BODY(based_integer)
// ----------------------------------------------------------------------------
//   Render the based integer value into the given string buffer
// ----------------------------------------------------------------------------
{
    return render_num(r, o, Settings.base, "#");
}


RENDER_BODY(fraction)
// ----------------------------------------------------------------------------
//   Render the fraction as 'num/den'
// ----------------------------------------------------------------------------
{
    integer_g n = o->numerator(1);
    integer_g d = o->denominator(1);
    render_num(r, n, 10, "");
    r.put('/');
    render_num(r, d, 10, "");
    return r.size();
}


RENDER_BODY(neg_fraction)
// ----------------------------------------------------------------------------
//   Render the fraction as '-num/den'
// ----------------------------------------------------------------------------
{
    integer_g n = o->numerator(1);
    integer_g d = o->denominator(1);
    render_num(r, n, 10, "-/");
    render_num(r, d, 10, "");
    return r.size();
}
