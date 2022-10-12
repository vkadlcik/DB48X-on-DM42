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

#include <integer.h>

OBJECT_HANDLER_BODY(integer)
// ----------------------------------------------------------------------------
//    Handle commands for integers
// ----------------------------------------------------------------------------
{
    switch(cmd)
    {
    case EVAL:
        // Integer values evaluate as self
        rt.push(this);
        return 0;
    case SIZE:
        return payload - obj + leb128size(payload);
    case PARSE:
    {
        parser *p = (parser *) arg;
        return parse(p->begin, p->end, &r->output, rt);
    }
    case RENDER:
    {
        renderer *r = (renderer *) r;
        return render(r->begin, r->end, r);
    }

    default:
        // Check if anyone else knows how to deal with it
        return SKIP;
    }

}


OBJECT_PARSER(integer)
// ----------------------------------------------------------------------------
//    Try to parse this as an integer
// ----------------------------------------------------------------------------
//    For simplicity, this deals with all kinds of integers
{
    if (begin >= end)
    {
        rt.error("Invalid empty integer", begin);
        return ERROR;
    }

    int sign = 1;
    int base = 10;
    id type = ID_integer;

    cstring p = begin;
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
        if (end > p)
        {
            switch(end[-1])
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
                rt.error("Invalid base", end-1);
                return ERROR;
            }
            end--;
            if (p >= end)
            {
                rt.error("Invalid based number", p);
                return ERROR;
            }
        }
    }

    // If this is a + or - operator
    if (p >= end)
        return SKIP;

    // Array of values for digits
    static byte value[256] = { 0 };
    if (!value['A'])
    {
        // Initialize value array on first use
        for (byte c = 0; c < 256; c++)
            value[c] = (byte) -1;
        for (char c = '0'; c <= '9'; c++)
            value[c] = c - '0';
        for (char c = 'A'; c <= 'Z'; c++)
            value[c] = c - 'A' + 10;
        for (char c = 'a'; c <= 'z'; c++)
            value[c] = c - 'a' + 10;
    }

    // Loop on digits
    ularge result = 0;
    uint shift = sign < 0 ? 1 : 0;
    while (p < end)
    {
        int v = value[*p++];
        if (v >= base)
        {
            rt.error("Invalid digit for base", p-1);
            return ERROR;
        }
        ularge next = result * base + v;
        if ((next << shift) <  result)
        {
            rt.error("Integer constant is too large", p-1);
            return ERROR;
        }
        result = next;
    }

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
}


OBJECT_RENDERER(integer)
// ----------------------------------------------------------------------------
//   Render the integer into the given string buffer
// ----------------------------------------------------------------------------
{
    return snprintf(begin, end - begin, "%llu", value<ull>());
}


OBJECT_RENDERER(neg_integer)
// ----------------------------------------------------------------------------
//   Render the negative integer value into the given string buffer
// ----------------------------------------------------------------------------
{
    return snprintf(begin, end - begin, "-%llu", value<ull>());
}


OBJECT_RENDERER(hex_integer)
// ----------------------------------------------------------------------------
//   Render the hexadecimal integer value into the given string buffer
// ----------------------------------------------------------------------------
{
    return snprintf(begin, end - begin, "#%llXh", value<ull>());
}

OBJECT_RENDERER(oct_integer)
// ----------------------------------------------------------------------------
//   Render the octal integer value into the given string buffer
// ----------------------------------------------------------------------------
{
    return snprintf(begin, end - begin, "#%lloo", value<ull>());
}

OBJECT_RENDERER(dec_integer)
// ----------------------------------------------------------------------------
//   Render the negative integer value into the given string buffer
// ----------------------------------------------------------------------------
{
    return snprintf(begin, end - begin, "#%llud", value<ull>());
}

OBJECT_RENDERER(dec_integer)
// ----------------------------------------------------------------------------
//   Render the binary integer value into the given string buffer
// ----------------------------------------------------------------------------
{
    // Why is there no printf format for binary?
    ull num = value<ull>();
    char *p = begin;
    if (p < end)
        *p = '#';
    ull testbit = num;
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
        numm >>= 1;
    } while (num);
    return result;
}
