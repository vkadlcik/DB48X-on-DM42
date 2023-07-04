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
//   While an implementation of computations on larger values was implemented on
//   this LEB128 format, it is now replaced with a bignum format for both
//   computation and memory efficiency reasons:
//   - Computations do not need to mask 7 bits at every step
//   - Memory for 64 bits is 2 (ID + size) + 8 (payload) = 10, whereas LEB128
//     would use 1 (ID) + 10 (64 / 7 > 9), so starting at 63 bits, the LEB128
//     representation is 12.5% less memory-efficient.
//
//   See bignum.h for the 8-bit big-num implementation

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
        byte *p = (byte *) payload(this);
        leb128(p, value);
    }

    template <typename Int>
    static size_t required_memory(id i, Int value)
    {
        return leb128size(i) + leb128size(value);
    }

    integer(gcbytes ptr, size_t size, id type = ID_integer): object(type)
    {
        byte *p = (byte *) payload(this);
        memmove(p, byte_p(ptr), size);
    }

    static size_t required_memory(id i, gcbytes UNUSED ptr, size_t size)
    {
        return leb128size(i) + size;
    }

    template <typename Int>
    Int value() const
    {
        byte *p = (byte *) payload(this);
        return leb128<Int>(p);
    }

    operator bool() const               { return !is_zero(); }
    bool is_zero() const                { return *payload(this) == 0; }
    template<typename Int>
    bool operator==(Int x)              { return value<Int>() == x; }
    template<typename Int>
    bool operator!=(Int x)              { return value<Int>() != x; }
    template<typename Int>
    bool operator< (Int x)              { return value<Int>() <  x; }
    template<typename Int>
    bool operator<=(Int x)              { return value<Int>() <= x; }
    template<typename Int>
    bool operator> (Int x)              { return value<Int>() >  x; }
    template<typename Int>
    bool operator>=(Int x)              { return value<Int>() >= x; }

    template <typename Int>
    static integer *make(Int value);

    // Up to 63 bits, we use native functions, it's faster
    enum { NATIVE = 64 / 7 };
    static bool native(byte_p x)        { return leb128size(x) <= NATIVE; }
    bool native() const                 { return native(payload(this)); }

public:
    OBJECT_DECL(integer);
    PARSE_DECL(integer);
    SIZE_DECL(integer);
    RENDER_DECL(integer);
};


template <object::id Type>
struct special_integer : integer
// ----------------------------------------------------------------------------
//   Representation for other integer types
// ----------------------------------------------------------------------------
{
    template <typename Int>
    special_integer(Int value, id type = Type): integer(value, type) {}

public:
    static id static_type()             { return Type; }
    PARSE_DECL(special_integer)         { return SKIP; }
    RENDER_DECL(special_integer);
};

using neg_integer   = special_integer<object::ID_neg_integer>;
using hex_integer   = special_integer<object::ID_hex_integer>;
using oct_integer   = special_integer<object::ID_oct_integer>;
using bin_integer   = special_integer<object::ID_bin_integer>;
using dec_integer   = special_integer<object::ID_dec_integer>;
using hex_integer   = special_integer<object::ID_hex_integer>;
using based_integer = special_integer<object::ID_based_integer>;

template <typename Int>
integer *integer::make(Int value)
// ----------------------------------------------------------------------------
//   Make an integer with the correct sign
// ----------------------------------------------------------------------------
{
    return value < 0 ? rt.make<neg_integer>(-value) : rt.make<integer>(value);
}

#endif // INTEGER_H
