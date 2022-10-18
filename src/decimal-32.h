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
#include "runtime.h"

#include <bid_conf.h>
#include <bid_functions.h>
#include <cstring>

struct decimal32 : object
// ----------------------------------------------------------------------------
//    Floating-point numbers in 32-bit decimal32 representation
// ----------------------------------------------------------------------------
{
    decimal32(gcstring value, id type = ID_decimal32): object(type)
    {
        bid32 num;
        bid32_from_string(&num.value, (cstring) value);
        byte *p = payload();
        memcpy(p, &num, sizeof(num));
    }

    decimal32(const bid32 &value, id type = ID_decimal32): object(type)
    {
        byte *p = payload();
        memcpy(p, &value, sizeof(value));
    }

    decimal32(uint64_t value, id type = ID_decimal32): object(type)
    {
        bid32 num;
        bid32_from_uint64(&num.value, &value);
        byte *p = payload();
        memcpy(p, &num, sizeof(num));
    }

    decimal32(int64_t value, id type = ID_decimal32): object(type)
    {
        bid32 num;
        bid32_from_int64(&num.value, &value);
        byte *p = payload();
        memcpy(p, &num, sizeof(num));
    }

    decimal32(uint32_t value, id type = ID_decimal32): object(type)
    {
        bid32 num;
        // Bug in the BID library, which uses int and not int32_t
        bid32_from_uint32(&num.value, (uint *) &value);
        byte *p = payload();
        memcpy(p, &num, sizeof(num));
    }

    decimal32(int32_t value, id type = ID_decimal32): object(type)
    {
        bid32 num;
        // Bug in the BID library, which uses int and not int32_t
        bid32_from_int32(&num.value, (int *) &value);
        byte *p = payload();
        memcpy(p, &num, sizeof(num));
    }

#if 32 > 64
    decimal32(const bid64 &value, id type = ID_decimal32): object(type)
    {
        bid32 num;
        bid64_to_bid32(&num.value, (BID_UINT64 *) &value.value);
        byte *p = payload();
        memcpy(p, &num, sizeof(num));
    }
#endif

#if 32 > 32
    decimal32(const bid32 &value, id type = ID_decimal32): object(type)
    {
        bid32 num;
        bid32_to_bid32(&num.value, (BID_UINT32 *) &value.value);
        byte *p = payload();
        memcpy(p, &num, sizeof(num));
    }
#endif

    template <typename Value>
    static size_t required_memory(id i, Value UNUSED value)
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


// Utlity common to all formats to format a number for display
void decimal_format(char *out, size_t len);

#endif // DECIMAL32_H
