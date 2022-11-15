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


static size_t render_num(char     *dest,
                         size_t    size,
                         integer_p num,
                         uint      base,
                         cstring   fmt)
// ----------------------------------------------------------------------------
//   Convert an integer value to the proper format
// ----------------------------------------------------------------------------
//   This is necessary because the arm-none-eabi-gcc printf can't do 64-bit
//   I'm getting non-sensible output
{
    char *p = dest;
    char *end = p + size;

    // copy the '#' or '-' sign
    if (*fmt)
    {
        if (p < end)
            *p = *fmt;
        p++;
        fmt++;
    }

    // Get denominator for the base
    runtime &rt = runtime::RT;
    object::id ntype = num->type();
    char *first = p;
    integer_g b = rt.make<integer>(ntype, base);
    integer_g n = (integer *) num;

    // Keep dividing by the base until we get 0
    do
    {
        integer_g r = nullptr;
        if (!integer::quorem(n, b, n, r))
            break;
        uint digit = r->value<uint>();
        if (p < end)
            *p = (digit < 10) ? digit + '0' : digit + ('A' - 10);
        p++;
    } while (!n->zero());

    // Revert the digits
    char *last = (p < end ? p : end) - 1;
    while (first < last)
    {
        char tmp = *first;
        *first = *last;
        *last = tmp;
        last--;
        first++;
    }

    // add suffix
    char *tail = p;
    if (*fmt)
    {
        if (tail < end)
            *tail = *fmt;
        tail++;
    }
    if (tail < end)
        *tail = 0;

    size_t result = tail - (char *) dest;
    return result;
}


OBJECT_RENDERER_BODY(integer)
// ----------------------------------------------------------------------------
//   Render the integer into the given string buffer
// ----------------------------------------------------------------------------
{
    size_t result = render_num(r.target, r.length, this, 10, "");
    return result;
}


template<>
OBJECT_RENDERER_BODY(neg_integer)
// ----------------------------------------------------------------------------
//   Render the negative integer value into the given string buffer
// ----------------------------------------------------------------------------
{
    return render_num(r.target, r.length, this, 10, "-");
}


template<>
OBJECT_RENDERER_BODY(hex_integer)
// ----------------------------------------------------------------------------
//   Render the hexadecimal integer value into the given string buffer
// ----------------------------------------------------------------------------
{
    return render_num(r.target, r.length, this, 16, "#h");
}

template<>
OBJECT_RENDERER_BODY(dec_integer)
// ----------------------------------------------------------------------------
//   Render the decimal based number
// ----------------------------------------------------------------------------
{
    return render_num(r.target, r.length, this, 10, "#d");
}

template<>
OBJECT_RENDERER_BODY(oct_integer)
// ----------------------------------------------------------------------------
//   Render the octal integer value into the given string buffer
// ----------------------------------------------------------------------------
{
    return render_num(r.target, r.length, this, 8, "#o");
}

template<>
OBJECT_RENDERER_BODY(bin_integer)
// ----------------------------------------------------------------------------
//   Render the binary integer value into the given string buffer
// ----------------------------------------------------------------------------
{
    return render_num(r.target, r.length, this, 2, "#b");
}



// ============================================================================
//
//    Big integer comparisons
//
// ============================================================================

int integer::compare(byte_p x, byte_p y)
// ----------------------------------------------------------------------------
//   Compare the two byte sequences
// ----------------------------------------------------------------------------
{
    int result = 0;
    byte c = 0;
    bool xmore, ymore;
    do
    {
        byte xd = *x++;
        byte yd = *y++;
        xmore = xd & 0x80;
        ymore = yd & 0x80;
        result = int(xd & 0x7F) - int(yd & 0x7F);
        c = byte(result) - c;
        c >>= 7;
    }
    while (xmore && ymore);
    if (xmore)
        return 1;
    else if (ymore)
        return -1;
    else if (c)
        return -1;
    else
        return result;
}


int integer::compare(integer_g x, integer_g y)
// ----------------------------------------------------------------------------
//   Compare two integer values
// ----------------------------------------------------------------------------
{
    id xt = x->type();
    id yt = y->type();

    // Negative integers are always smaller than positive integers
    if (xt == ID_neg_integer && yt != ID_neg_integer)
        return -2;
    else if (yt == ID_neg_integer && xt != ID_neg_integer)
        return 2;

    byte_p xp = x->payload();
    byte_p yp = y->payload();
    int result = compare(xp, yp);
    if (xt == ID_neg_integer)
        result = -result;
    return result;
}



// ============================================================================
//
//    Big integer arithmetic
//
// ============================================================================

static inline byte add_op(byte x, byte y, byte c)       { return x + y + c; }
static inline byte sub_op(byte x, byte y, byte c)       { return x - y - c; }
static inline byte neg_op(byte x, byte c)               { return -x - c; }
static inline byte not_op(byte x, byte  )               { return ~x; }
static inline byte and_op(byte x, byte y, byte  )       { return x & y; }
static inline byte or_op (byte x, byte y, byte  )       { return x | y; }
static inline byte xor_op(byte x, byte y, byte  )       { return x ^ y; }


inline object::id integer::opposite_type(id type)
// ----------------------------------------------------------------------------
//   Return the type of the opposite
// ----------------------------------------------------------------------------
{
    switch(type)
    {
    case ID_integer:            return ID_neg_integer;
    case ID_neg_integer:        return ID_integer;
    default:                    return type;
    }
}


inline object::id integer::product_type(id yt, id xt)
// ----------------------------------------------------------------------------
//   Return the type of the product of x and y
// ----------------------------------------------------------------------------
{
    switch(xt)
    {
    case ID_integer:
        if (yt == ID_neg_integer)
            return ID_neg_integer;
        return ID_integer;
    case ID_neg_integer:
        if (yt == ID_neg_integer)
            return ID_integer;
        return ID_neg_integer;
    default:
        return xt;
    }
}


integer_g operator-(integer_g x)
// ----------------------------------------------------------------------------
//   Negate the input value
// ----------------------------------------------------------------------------
{
    runtime &rt = runtime::RT;
    object::id xt = x->type();
    byte_p p = x->payload();

    // Deal with simple case where we can simply copy the payload
    if (xt == object::ID_integer)
        return rt.make<integer>(object::ID_neg_integer, p, leb128size(p));
    else if (xt == object::ID_neg_integer)
        return rt.make<integer>(object::ID_integer, p, leb128size(p));

    // Complicated case: need to actually compute the opposite
    size_t ws = integer::wordsize(xt);
    size_t size = integer::unary<true>(neg_op, x->payload(), ws);
    gcbytes bytes = rt.allocate(size);
    integer_g result = rt.make<integer>(xt, bytes, size);
    rt.free(size);
    return result;
}


integer_g operator~(integer_g x)
// ----------------------------------------------------------------------------
//   Boolean not
// ----------------------------------------------------------------------------
{
    runtime &rt = runtime::RT;
    object::id xt = x->type();
    byte_p p = x->payload();

    // For integer and neg_integer, do a 0/1 not
    if (xt == object::ID_integer || xt == object::ID_neg_integer)
    {
        return rt.make<integer>(object::ID_integer, *p == 0);
    }

    // For hex_integer and other based numbers, do a binary not
    size_t ws = integer::wordsize(xt);
    size_t size = integer::unary<true>(not_op, x->payload(), ws);
    gcbytes bytes = rt.allocate(size);
    integer_g result = rt.make<integer>(xt, bytes, size);
    rt.free(size);
    return result;
}


integer_g integer::add_sub(integer_g y, integer_g x, bool issub)
// ----------------------------------------------------------------------------
//   Add the two integer values, result has type of x
// ----------------------------------------------------------------------------
{
    runtime &rt = RT;
    id yt = y->type();
    id xt = x->type();
    byte_p yp = y->payload();
    byte_p xp = x->payload();
    size_t ws = wordsize(xt);

    // Check if we have opposite signs
    bool samesgn = (xt == ID_neg_integer) == (yt == ID_neg_integer);
    if (samesgn == issub)
    {
        int cmp = compare(yp, xp);
        if (cmp >= 0)
        {
            // abs Y > abs X: result has opposite type of X
            size_t size = binary<false>(sub_op, yp, xp, ws);
            gcbytes bytes = rt.allocate(size);
            id ty = cmp == 0 ? ID_integer: issub ? xt : opposite_type(xt);
            integer_g result = rt.make<integer>(ty, bytes, size);
            rt.free(size);
            return result;
        }
        else
        {
            // abs Y < abs X: result has type of X
            size_t size = binary<false>(sub_op, xp, yp, ws);
            gcbytes bytes = rt.allocate(size);
            id ty = issub ? opposite_type(xt) : xt;
            integer_g result = rt.make<integer>(ty, bytes, size);
            rt.free(size);
            return result;
        }
    }

    // We have the same sign, add items
    size_t size = binary<false>(add_op, yp, xp, ws);
    gcbytes bytes = rt.allocate(size);
    id ty = issub ? opposite_type(xt) : xt;
    integer_g result = rt.make<integer>(ty, bytes, size);
    rt.free(size);
    return result;
}



integer_g operator+(integer_g y, integer_g x)
// ----------------------------------------------------------------------------
//   Add the two integer values, result has type of x
// ----------------------------------------------------------------------------
{
    return integer::add_sub(y, x, false);
}


integer_g operator-(integer_g y, integer_g x)
// ----------------------------------------------------------------------------
//   Subtract two integer values, result has type of x
// ----------------------------------------------------------------------------
{
    return integer::add_sub(y, x, true);
}


integer_g operator&(integer_g y, integer_g x)
// ----------------------------------------------------------------------------
//   Perform a binary and operation
// ----------------------------------------------------------------------------
{
    return integer::binary<false>(and_op, x, y);
}


integer_g operator|(integer_g y, integer_g x)
// ----------------------------------------------------------------------------
//   Perform a binary or operation
// ----------------------------------------------------------------------------
{
    return integer::binary<false>(or_op, x, y);
}


integer_g operator^(integer_g y, integer_g x)
// ----------------------------------------------------------------------------
//   Perform a binary xor operation
// ----------------------------------------------------------------------------
{
    return integer::binary<false>(xor_op, x, y);
}


size_t integer::multiply(byte_p x, byte_p y, size_t maxbits)
// ----------------------------------------------------------------------------
//   Perform multiply operation op on leb128 numbers x and y
// ----------------------------------------------------------------------------
//   Result is placed in scratchpad, the function returns the size in bytes
{
    runtime &rt = runtime::RT;
    byte *buffer = rt.scratchpad();
    byte *p = buffer;
    bool xmore;
    uint shift = 0;
    size_t available = rt.available();
    if (available > (maxbits + 6) / 7)
        available = (maxbits + 6) / 7;
    *p = 0;
    byte *highest = p;

    do
    {
        uint xs = 0;
        byte xd = *x++;
        xmore = xd & 0x80;
        xd &= 0x7F;
        while (xd)
        {
            if (xd & 1)
            {
                size_t a = available;
                bool ymore = true;
                byte_p yp = y;
                uint c = 0;
                uint s = shift + xs;
                uint word = s / 7;
                p = buffer + word;
                if (a > word)
                    a -= word;
                else
                    a = 0;
                s %= 7;
                while (highest < p)
                    *highest++ = 0x80;
                if (p > buffer)
                    p[-1] |= 0x80;
                while (ymore && a)
                {
                    byte yd = *yp++;
                    ymore = yd & 0x80;
                    byte pd = p < highest ? *p & 0x7F : 0;
                    c = pd + ((yd & 0x7F) << s) + c;
                    *p++ = byte(c & 0x7F) | 0x80;
                    c >>= 7;
                    a--;
                }
                while (c && a)
                {
                    byte pd = p < highest ? *p & 0x7F : 0;
                    c = pd + c;
                    *p++ = byte(c & 0x7F) | 0x80;
                    c >>= 7;
                    a--;
                }
                p[-1] &= ~0x80;
                highest = p;
            }
            xd >>= 1;
            xs++;
        }
        shift += 7;
    }
    while (xmore);

    size_t written = p - buffer;
    if (maxbits < 7 * written)
    {
        size_t bits = 7 * written - maxbits;
        p[-1] = byte(p[-1] << bits) >> bits;
    }
    return p - buffer;
}


integer_g operator*(integer_g y, integer_g x)
// ----------------------------------------------------------------------------
//   Multiplication of integers
// ----------------------------------------------------------------------------
{
    runtime &rt = runtime::RT;
    object::id yt = y->type();
    object::id xt = x->type();
    byte_p yp = y->payload();
    byte_p xp = x->payload();
    size_t ws = integer::wordsize(xt);
    size_t size = integer::multiply(yp, xp, ws);
    gcbytes bytes = rt.allocate(size);
    object::id prodtype = integer::product_type(yt, xt);
    integer_g result = rt.make<integer>(prodtype, bytes, size);
    rt.free(size);
    return result;

}


size_t integer::divide(byte_p y, byte_p x, size_t maxbits,
                       byte_p &qptr, size_t &qsize,
                       byte_p &rptr, size_t &rsize)
// ----------------------------------------------------------------------------
//   Perform divide  operation op on leb128 numbers x and y
// ----------------------------------------------------------------------------
//   Result is placed in scratchpad, the function returns the size in bytes
{
    runtime &rt = runtime::RT;
    if (*x == 0)
    {
        rt.zero_divide_error();
        return 0;
    }

    // Allocate size for the remainder (smaller than x), initialize to 0
    byte_p yp = y;
    do { } while (*yp++ & 0x80);
    size_t qsz = yp - y;
    byte *quotient = rt.scratchpad();
    *quotient = 0;

    // Pointer to the remainder, initialize to 0
    size_t rsz = 0;
    byte *remainder = quotient + qsz;
    *remainder = 0;

    size_t available = rt.available();
    if (available > (maxbits + 6) / 7)
        available = (maxbits + 6) / 7;
    for (int word = qsz-1; word >= 0; word--)
    {
        quotient[word] = 0x80;
        for (int bit = 6; bit >= 0; bit--)
        {
            // Shift remainder left by one bit, add numerator bit
            byte_p xp = x;
            int delta = 0;      // >= 0 if remainder >= denominator
            byte *r = remainder;
            byte c = (y[word] >> bit) & 1;
            bool rmore, xmore;

            do
            {
                byte rd = *r;
                rmore = rd & 0x80;
                byte xd = *xp++;
                xmore = xd & 0x80;
                c = (byte(rd & 0x7F) << 1) | c;
                delta = int(c & 0x7F) - int(xd & 0x7F);
                *r++ = c | 0x80;
                c >>= 7;
            }
            while(rmore && xmore);

            if (rmore || xmore)
                delta = rmore - xmore;

            while (rmore)
            {
                byte rd = *r;
                rmore = rd & 0x80;
                c = (byte(rd & 0x7F) << 1) | c;
                *r++ = c | 0x80;
                c >>= 7;
            }

            if (c)
                *r++ = c | 0x80;

            // If remainder >= denominator, add to quotient, subtract from rem
            if (delta >= 0)
            {
                quotient[word] |= (1 << bit);

                // Strip zeroes at top of remainder
                while (r > remainder + 1 && r[-1] == 0x80)
                    r--;
                r[-1] &= ~0x80;

                r = remainder;
                xp = x;

                c = 0;
                do
                {
                    byte rd = *r;
                    rmore = rd & 0x80;
                    byte xd = *xp++;
                    xmore = xd & 0x80;
                    c = byte(rd & 0x7F) - byte(xd & 0x7F) - c;
                    *r++ = (c & 0x7F) | 0x80;
                    c >>= 7;
                }
                while (rmore && xmore);

                while (rmore)
                {
                    byte rd = *r;
                    rmore = rd & 0x80;
                    c = (byte(rd & 0x7F) << 1) - c;
                    *r++ = c | 0x80;
                    c >>= 7;
                }

            }

            // Strip zeroes at top of remainder
            while (r > remainder + 1 && r[-1] == 0x80)
                r--;
            r[-1] &= ~0x80;
            rsz = r - remainder;
        } // numerator bit loop

    } // numerator word loop

    // Strip zeroes from the quotient
    byte *q = quotient + qsz;
    while (q > quotient + 1 && q[-1] == 0x80)
        q--;
    q[-1] &= ~0x80;
    qsz = q - quotient;

    // Write out result
    qptr  = quotient;
    qsize = qsz;
    rptr  = remainder;
    rsize = rsz;

    // Return total size used in scratchpad
    return remainder + rsz - quotient;
}


integer_g operator/(integer_g y, integer_g x)
// ----------------------------------------------------------------------------
//   Perform long division of y by x
// ----------------------------------------------------------------------------
{
    runtime &rt = runtime::RT;
    object::id yt = y->type();
    object::id xt = x->type();
    byte_p yp = y->payload();
    byte_p xp = x->payload();
    size_t ws = integer::wordsize(xt);
    byte_p qp = nullptr;
    byte_p rp = nullptr;
    size_t qs = 0;
    size_t rs = 0;
    if (size_t size = integer::divide(yp, xp, ws, qp, qs, rp, rs))
    {
        gcbytes bytes = rt.allocate(size);
        object::id prodtype = integer::product_type(yt, xt);
        gcbytes qbytes = qp;
        integer_g result = rt.make<integer>(prodtype, qbytes, qs);
        rt.free(size);
        return result;
    }
    return y;
}


integer_g operator%(integer_g y, integer_g x)
// ----------------------------------------------------------------------------
//  Perform long-remainder of y by x
// ----------------------------------------------------------------------------
{
    runtime &rt = runtime::RT;
    object::id yt = y->type();
    object::id xt = x->type();
    byte_p yp = y->payload();
    byte_p xp = x->payload();
    size_t ws = integer::wordsize(xt);
    byte_p qp = nullptr;
    byte_p rp = nullptr;
    size_t qs = 0;
    size_t rs = 0;
    if (size_t size = integer::divide(yp, xp, ws, qp, qs, rp, rs))
    {
        gcbytes bytes = rt.allocate(size);
        object::id prodtype = integer::product_type(yt, xt);
        gcbytes rbytes = rp;
        integer_g result = rt.make<integer>(prodtype, rbytes, rs);
        rt.free(size);
        return result;
    }
    return y;
}


bool integer::quorem(integer_g y, integer_g x, integer_g &q, integer_g &r)
// ----------------------------------------------------------------------------
//  Perform long-remainder of y by x
// ----------------------------------------------------------------------------
{
    runtime &rt = runtime::RT;
    object::id yt = y->type();
    object::id xt = x->type();
    byte_p yp = y->payload();
    byte_p xp = x->payload();
    size_t ws = integer::wordsize(xt);
    byte_p qp = nullptr;
    byte_p rp = nullptr;
    size_t qs = 0;
    size_t rs = 0;
    if (size_t size = integer::divide(yp, xp, ws, qp, qs, rp, rs))
    {
        gcbytes bytes = rt.allocate(size);
        object::id prodtype = integer::product_type(yt, xt);
        gcbytes qbytes = qp;
        gcbytes rbytes = rp;
        q = rt.make<integer>(prodtype, qbytes, qs);
        r = rt.make<integer>(prodtype, rbytes, rs);
        rt.free(size);
        return true;
    }
    return false;
}


integer_g integer::pow(integer_g y, integer_g x)
// ----------------------------------------------------------------------------
//    Compute y^abs(x)
// ----------------------------------------------------------------------------
//   Note that the case where x is negative should be filtered by caller
{
    runtime &rt = RT;
    integer_g r = rt.make<integer>(ID_integer, 1);

    byte_p xp = x->payload();
    bool xmore;
    do
    {
        byte xv = *xp++;
        xmore = xv & 0x80;
        xv &= 0x7F;
        for (uint bit = 0; bit < 7; bit++)
        {
            if (xv & 1)
                r = r * y;
            xv >>= 1;
            if (xv || xmore)
                y = y * y;
        }
    }
    while (xmore);
    return r;
}
