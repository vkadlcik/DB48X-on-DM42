#ifndef INTEGER_H
#define INTEGER_H
// ****************************************************************************
//  integer.h                                                     DB48X project
// ****************************************************************************
//
//   File Description:
//
//      The integer object type
//
//      Operations on integer values
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
//
// Payload format:
//
//   Integer types are distinguished by their type ID.
//   Negative integers are represented by ID_neg_integer.
//   They store their magnitude in LEB128 format.
//
//   The present implementation limits itself to values that fit in 64 bit,
//   and uses native CPU operations to do that (or at least, a fixed number
//   of CPU operations on 32-bit CPUs like on the DM42).
//
//   However, the design allows computations with unlimited precision to be
//   implemented without changing the number storage format. Such
//   variable-precision arithmetic is likely to be implemented some day.

#include "object.h"
#include "runtime.h"
#include "settings.h"

struct integer;
typedef const integer *integer_p;
typedef gcp<integer> integer_g;


struct integer : object
// ----------------------------------------------------------------------------
//    Represent integer objects
// ----------------------------------------------------------------------------
{
    template <typename Int>
    integer(Int value, id type = ID_integer): object(type)
    {
        byte *p = payload();
        leb128(p, value);
    }

    template <typename Int>
    static size_t required_memory(id i, Int value)
    {
        return leb128size(i) + leb128size(value);
    }

    integer(gcbytes ptr, size_t size, id type = ID_integer): object(type)
    {
        byte *p = payload();
        memmove(p, byte_p(ptr), size);
    }

    static size_t required_memory(id i, gcbytes UNUSED ptr, size_t size)
    {
        return leb128size(i) + size;
    }

    template <typename Int>
    Int value() const
    {
        byte *p = payload();
        return leb128<Int>(p);
    }

    operator bool() const               { return !zero(); }
    bool zero() const                   { return *payload() == 0; }

    template <typename Int>
    static integer *make(Int value);

    // Up to 63 bits, we use native functions, it's faster
    enum { NATIVE = 64 / 7 };
    static bool native(byte_p x)        { return leb128size(x) <= NATIVE; }
    bool native() const                 { return native(payload()); }

    OBJECT_HANDLER(integer);
    OBJECT_PARSER(integer);
    OBJECT_RENDERER(integer);

public:
    // Arithmetic internal routines
    static int compare(byte_p x, byte_p y);
    static int compare(integer_g x, integer_g y);

    static size_t wordsize(id type);
    size_t wordsize() const             { return wordsize(type()); }
    static id opposite_type(id type);
    static id product_type(id yt, id xt);

    template<bool extend, typename Op>
    static size_t binary(Op op, byte_p x, byte_p y, size_t maxbits);
    template<bool extend, typename Op>
    static integer_g binary(Op op, integer_g x, integer_g y);
    template<bool extend, typename Op>
    static size_t unary(Op op, byte_p x, size_t maxbits);

    static integer_g add_sub(integer_g y, integer_g x, bool subtract);
    static size_t multiply(byte_p x, byte_p y, size_t maxbits);
    static size_t divide(byte_p y, byte_p x, size_t maxbits,
                         byte_p &qptr, size_t &qsize,
                         byte_p &rptr, size_t &rsize);

    // Compute quotient and reminder together
    static bool quorem(integer_g y, integer_g x, integer_g &q, integer_g &r);
    static integer_g pow(integer_g y, integer_g x);
};


template <object::id Type>
struct special_integer : integer
// ----------------------------------------------------------------------------
//   Representation for other integer types
// ----------------------------------------------------------------------------
{
    template <typename Int>
    special_integer(Int value, id type = Type): integer(value, type) {}

    static id static_type() { return Type; }
    OBJECT_HANDLER_NO_ID(special_integer)
    {
        if (op == RENDER)
            return obj->object_renderer(OBJECT_RENDERER_ARG(), rt);
        return DELEGATE(integer);
    }
    OBJECT_RENDERER(special_integer);
};

using neg_integer = special_integer<object::ID_neg_integer>;
using hex_integer = special_integer<object::ID_hex_integer>;
using oct_integer = special_integer<object::ID_oct_integer>;
using bin_integer = special_integer<object::ID_bin_integer>;
using dec_integer = special_integer<object::ID_dec_integer>;

template <typename Int>
integer *integer::make(Int value)
// ----------------------------------------------------------------------------
//   Make an integer with the correct sign
// ----------------------------------------------------------------------------
{
    return value < 0 ? RT.make<neg_integer>(-value) : RT.make<integer>(value);
}


// ============================================================================
//
//    Big-integer comparisons and arithmetic
//
// ============================================================================

inline bool operator< (integer_g y, integer_g x)
{
    return integer::compare(y,x) <  0;
}

inline bool operator==(integer_g y, integer_g x)
{
    return integer::compare(y,x) == 0;
}

inline bool operator> (integer_g y, integer_g x)
{
    return integer::compare(y,x) >  0;
}

inline bool operator<=(integer_g y, integer_g x)
{
    return integer::compare(y,x) <= 0;
}

inline bool operator>=(integer_g y, integer_g x)
{
    return integer::compare(y,x) >= 0;
}

inline bool operator!=(integer_g y, integer_g x)
{
    return integer::compare(y,x) != 0;
}


integer_g operator- (integer_g x);
integer_g operator~ (integer_g x);
integer_g operator+ (integer_g y, integer_g x);
integer_g operator- (integer_g y, integer_g x);
integer_g operator* (integer_g y, integer_g x);
integer_g operator/ (integer_g y, integer_g x);
integer_g operator% (integer_g y, integer_g x);
integer_g operator& (integer_g y, integer_g x);
integer_g operator| (integer_g y, integer_g x);
integer_g operator^ (integer_g y, integer_g x);




// ============================================================================
//
//   Helper code
//
// ============================================================================

inline size_t integer::wordsize(id type)
// ----------------------------------------------------------------------------
//   Return the word size for an integer type
// ----------------------------------------------------------------------------
{
    if (type == ID_integer || type == ID_neg_integer)
        return size_t(-7); // So that (ws + 6) / 7 is a large value
    return Settings.wordsize;
}


template<bool extend, typename Op>
size_t integer::binary(Op op, byte_p x, byte_p y, size_t maxbits)
// ----------------------------------------------------------------------------
//   Perform binary operation op on leb128 numbers x and y
// ----------------------------------------------------------------------------
//   Result is placed in scratchpad, the function returns the size in bytes
{
    runtime &rt = runtime::RT;
    byte *buffer = rt.scratchpad();
    byte *p = buffer;
    bool xmore, ymore;
    byte c = 0;
    size_t available = rt.available();
    if (available > (maxbits + 6) / 7)
        available = (maxbits + 6) / 7;
    do
    {
        byte xd = *x++;
        byte yd = *y++;
        xmore = xd & 0x80;
        ymore = yd & 0x80;
        c = op(xd & 0x7F, yd & 0x7F, c);
        *p++ = (c & 0x7F) | 0x80;
        c >>= 7;
        available--;
    }
    while (xmore && ymore && available);

    while (xmore && available)
    {
        byte xd = *x++;
        xmore = xd & 0x80;
        c = op(xd & 0x7F, 0, c);
        *p++ = (c & 0x7F) | 0x80;
        c >>= 7;
        available--;
    }
    while (ymore && available)
    {
        byte yd = *y++;
        ymore = yd & 0x80;
        c = op(0, yd & 0x7F, c);
        *p++ = (c & 0x7F) | 0x80;
        c >>= 7;
        available--;
    }
    while (extend && available)
    {
        c = op(0, 0, c);
        *p++ = (c & 0x7F) | 0x80;
        c >>= 7;
        available--;
    }
    if (c & available)
    {
        *p++ = (c & 0x7F) | 0x80;
        available--;
    }
    while (p > buffer + 1 && p[-1] == 0x80)
        p--;
    p[-1] &= ~0x80;
    size_t written = p - buffer;
    if (maxbits < 7 * written)
    {
        size_t bits = 7 * written - maxbits;
        p[-1] = byte(p[-1] << bits) >> bits;
    }
    return p - buffer;
}


template<bool extend, typename Op>
integer_g integer::binary(Op op, integer_g x, integer_g y)
// ----------------------------------------------------------------------------
//   Perform binary operation op on leb128 numbers x and y
// ----------------------------------------------------------------------------
{
    runtime &rt = RT;
    object::id xt = x->type();
    byte_p yp = y->payload();
    byte_p xp = x->payload();
    size_t ws = wordsize(xt);
    size_t size = binary<extend, Op>(op, yp, xp, ws);
    gcbytes bytes = rt.allocate(size);
    integer_g result = rt.make<integer>(xt, bytes, size);
    rt.free(size);
    return result;
}


template<bool extend, typename Op>
size_t integer::unary(Op op, byte_p x, size_t maxbits)
// ----------------------------------------------------------------------------
//   Shift the bytes at x left
// ----------------------------------------------------------------------------
//   Result is placed in the scratchpad, the function returns size in bytes
{
    runtime &rt = runtime::RT;
    byte *buffer = rt.scratchpad();
    byte *p = buffer;
    bool xmore;
    byte c = 0;
    size_t available = rt.available();
    if (available > (maxbits + 6) / 7)
        available = (maxbits + 6) / 7;
    do
    {
        byte xd = *x++;
        xmore = xd & 0x80;
        c = op(xd & 0x7F, c);
        *p++ = (c & 0x7F) | 0x80;
        c >>= 7;
        available--;
    }
    while (xmore && available);

    while (extend && available)
    {
        c = op(0, c);
        *p++ = (c & 0x7F) | 0x80;
        c >>= 7;
        available--;
    }
    if (extend)
        p[-1] &= (1 << (maxbits % 7)) - 1;
    while (p > buffer + 1 && p[-1] == 0x80)
        p--;
    p[-1] &= ~0x80;
    size_t written = p - buffer;
    if (maxbits < 7 * written)
    {
        size_t bits = 7 * written - maxbits;
        p[-1] = byte(p[-1] << bits) >> bits;
    }
    return p - buffer;
}

#endif // INTEGER_H
