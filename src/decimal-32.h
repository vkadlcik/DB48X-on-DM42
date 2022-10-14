#ifndef DECIMAL32_H
#define DECIMAL32_H
// ****************************************************************************
//  decimal32.h                                                 DB48X project
// ****************************************************************************
//
//   File Description:
//
//      Real numbers in decimal32 representation
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

struct decimal32 : object
// ----------------------------------------------------------------------------
//    Floating-point numbers in 32-bit decimal32 representation
// ----------------------------------------------------------------------------
{
    typedef BID_UINT32 bid32;

    decimal32(cstring value, id type = ID_decimal32): object(type)
    {
        bid32 num;
        bid32_from_string(&num, value);
        byte *p = payload();
        memcpy(p, &num, sizeof(num));
    }

    static size_t required_memory(id i, cstring UNUSED value)
    {
        return leb128size(i) + sizeof(bid32);
    }

    bid32 value()
    {
        bid32 result;
        byte *p = payload();
        memcpy(&result, p, sizeof(result));
        return result;
    }

    OBJECT_HANDLER(decimal32);
    OBJECT_PARSER(decimal32);
    OBJECT_RENDERER(decimal32);
};

#endif // DECIMAL32_H
