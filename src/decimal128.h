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
#include "runtime.h"

#include <bid_conf.h>
#include <bid_functions.h>
#include <cstring>

struct decimal128 : object
// ----------------------------------------------------------------------------
//    Floating-point numbers in 128-bit decimal128 representation
// ----------------------------------------------------------------------------
{
    decimal128(gcstring value, id type = ID_decimal128): object(type)
    {
        bid128 num;
        bid128_from_string(&num.value, (cstring) value);
        byte *p = payload();
        memcpy(p, &num, sizeof(num));
    }

    decimal128(const bid128 &value, id type = ID_decimal128): object(type)
    {
        byte *p = payload();
        memcpy(p, &value, sizeof(value));
    }

    decimal128(uint64_t value, id type = ID_decimal128): object(type)
    {
        bid128 num;
        bid128_from_uint64(&num.value, &value);
        byte *p = payload();
        memcpy(p, &num, sizeof(num));
    }

    decimal128(int64_t value, id type = ID_decimal128): object(type)
    {
        bid128 num;
        bid128_from_int64(&num.value, &value);
        byte *p = payload();
        memcpy(p, &num, sizeof(num));
    }

#if 128 > 64
    decimal128(const bid64 &value, id type = ID_decimal128): object(type)
    {
        bid128 num;
        bid64_to_bid128(&num.value, (BID_UINT64 *) &value.value);
        byte *p = payload();
        memcpy(p, &num, sizeof(num));
    }
#endif

#if 128 > 32
    decimal128(const bid32 &value, id type = ID_decimal128): object(type)
    {
        bid128 num;
        bid32_to_bid128(&num.value, (BID_UINT32 *) &value.value);
        byte *p = payload();
        memcpy(p, &num, sizeof(num));
    }
#endif

    template <typename Value>
    static size_t required_memory(id i, Value UNUSED value)
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


// Utlity common to all formats to format a number for display
void decimal_format(char *out, size_t len);

#endif // DECIMAL128_H
