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
//
// Payload format:
//
//   A copy of the 128-bit representation of the object follows the type
//
//   Since it is unclear that Intel's library is robust to misaligned data,
//   We presently copy that payload when operating on objects. This may be
//   unnecessary.


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

    decimal128(uint32_t value, id type = ID_decimal128): object(type)
    {
        bid128 num;
        // Bug in the BID library, which uses int and not int32_t
        bid128_from_uint32(&num.value, (uint *) &value);
        byte *p = payload();
        memcpy(p, &num, sizeof(num));
    }

    decimal128(int32_t value, id type = ID_decimal128): object(type)
    {
        bid128 num;
        // Bug in the BID library, which uses int and not int32_t
        bid128_from_int32(&num.value, (int *) &value);
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

    bid128 value() const
    {
        bid128 result;
        byte *p = payload();
        memcpy(&result, p, sizeof(result));
        return result;
    }

    enum class_type
    // ------------------------------------------------------------------------
    //   Class type for bid128 numbers
    // ------------------------------------------------------------------------
    //   This should really be exported in the header, since it's the result of
    //   the bid128_class function. Lifted from Inte's source code
    {
        signalingNaN,
        quietNaN,
        negativeInfinity,
        negativeNormal,
        negativeSubnormal,
        negativeZero,
        positiveZero,
        positiveSubnormal,
        positiveNormal,
        positiveInfinity
    };

    static class_type fpclass(BID_UINT128 *b)
    {
        int c = 0;
        bid128_class(&c, b);
        return (class_type) c;
    }

    static class_type fpclass(bid128 &x)
    {
        return fpclass(&x.value);
    }

    static bool is_negative(BID_UINT128 *x)
    {
        class_type c = fpclass(x);
        return c >= negativeInfinity && c <= negativeZero;
    }

    static bool is_negative(bid128 &x)
    {
        return is_negative(&x.value);
    }

    static bool is_negative_or_zero(BID_UINT128 *x)
    {
        class_type c = fpclass(x);
        return c >= negativeInfinity && c <= positiveZero;
    }

    static bool is_negative_or_zero(bid128 &x)
    {
        return is_negative_or_zero(&x.value);
    }

    OBJECT_HANDLER(decimal128);
    OBJECT_PARSER(decimal128);
    OBJECT_RENDERER(decimal128);
};

typedef const decimal128 *decimal128_p;

// Functions used by arithmetic.h
void bid128_mod(BID_UINT128 *pres, BID_UINT128 *px, BID_UINT128 *py);
void bid128_rem(BID_UINT128 *pres, BID_UINT128 *px, BID_UINT128 *py);

// Utlity common to all formats to format a number for display
void decimal_format(char *out, size_t len, int digits);

#endif // DECIMAL128_H
