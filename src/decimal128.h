#ifndef DECIMAL128_H
#define DECIMAL128_H
// ****************************************************************************
//  decimal128.h                                                 DB48X project
// ****************************************************************************
//
//   File Description:
//
//      Real numbers in decimal128 representation
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

struct decimal128 : object
// ----------------------------------------------------------------------------
//    Floating-point numbers in 128-bit decimal128 representation
// ----------------------------------------------------------------------------
{
    typedef BID_UINT128 bid128;

    decimal128(cstring value, id type = ID_decimal128): object(type)
    {
        bid128 num;
        bid128_from_string(&num, value);
        byte *p = payload();
        memcpy(p, &num, sizeof(num));
    }

    static size_t required_memory(id i, cstring value)
    {
        return leb128size(i) + sizeof(bid128);
    }

    bid128 value()
    {
        bid128 result;
        byte *p = payload();
        memcpy(&result, p, sizeof(result));
        return result;
    }

    OBJECT_HANDLER(decimal128);
    OBJECT_PARSER(decimal128);
    OBJECT_RENDERER(decimal128);
};

#endif // DECIMAL128_H
