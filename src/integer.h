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
//   Integer types are disstinguished by their type ID.
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

    template <typename Int>
    Int value() const
    {
        byte *p = payload();
        return leb128<Int>(p);
    }

    template <typename Int>
    static integer *make(Int value);

    OBJECT_HANDLER(integer);
    OBJECT_PARSER(integer);
    OBJECT_RENDERER(integer);
};

typedef const integer *integer_p;

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

#endif // INTEGER_H
