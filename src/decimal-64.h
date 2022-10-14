#ifndef DECIMAL64_H
#define DECIMAL64_H
// ****************************************************************************
//  decimal64.h                                                 DB48X project
// ****************************************************************************
//
//   File Description:
//
//      Real numbers in decimal64 representation
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

#include "object.h"

#include <bid_conf.h>
#include <bid_functions.h>
#include <cstring>

struct decimal64 : object
// ----------------------------------------------------------------------------
//    Floating-point numbers in 64-bit decimal64 representation
// ----------------------------------------------------------------------------
{
    typedef BID_UINT64 bid64;

    decimal64(cstring value, id type = ID_decimal64): object(type)
    {
        bid64 num;
        bid64_from_string(&num, value);
        byte *p = payload();
        memcpy(p, &num, sizeof(num));
    }

    static size_t required_memory(id i, cstring UNUSED value)
    {
        return leb128size(i) + sizeof(bid64);
    }

    bid64 value()
    {
        bid64 result;
        byte *p = payload();
        memcpy(&result, p, sizeof(result));
        return result;
    }

    OBJECT_HANDLER(decimal64);
    OBJECT_PARSER(decimal64);
    OBJECT_RENDERER(decimal64);
};

#endif // DECIMAL64_H
