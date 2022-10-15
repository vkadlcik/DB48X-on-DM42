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

#include "object.h"

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
    Int value()
    {
        byte *p = payload();
        return leb128<byte, Int>(p);
    }

    OBJECT_HANDLER(integer);
    OBJECT_PARSER(integer);
    OBJECT_RENDERER(integer);
};


template <object::id Type>
struct special_integer : integer
// ----------------------------------------------------------------------------
//   Representation for other integer types
// ----------------------------------------------------------------------------
{
    template <typename Int>
    special_integer(Int value, id type = Type): integer(value, type) {}

    OBJECT_HANDLER_NO_ID(special_integer)
    {
        if (cmd == RENDER)
        {
            renderer *r = (renderer *) arg;
            return obj->special_integer::render(r->begin, r->end, rt);
        }
        return DELEGATE(integer);
    }
    OBJECT_RENDERER(special_integer);
};

using neg_integer = special_integer<object::ID_neg_integer>;
using hex_integer = special_integer<object::ID_hex_integer>;
using oct_integer = special_integer<object::ID_oct_integer>;
using bin_integer = special_integer<object::ID_bin_integer>;
using dec_integer = special_integer<object::ID_dec_integer>;

#endif // INTEGER_H
