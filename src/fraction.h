#ifndef FRACTION_H
#define FRACTION_H
// ****************************************************************************
//  fraction.h                                                    DB48X project
// ****************************************************************************
//
//   File Description:
//
//     Representation of mathematical fractions
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
//   Payload representation:
//   - The ID is one of four formats:
//     + ID_fraction:           Positive ratio of two LEB128-encoded numbers
//     + ID_neg_fraction:       Negative ratio of two LEB128-encoded numbers
//     + ID_big_fraction:       Positive ratio of two bignum-encoded numbers
//     + ID_neg_big_fraction:   Negative ratio of two bignum-encoded numbers
//   - Following the ID are the two payloads for the matching integer type

#include "bignum.h"
#include "integer.h"
#include "object.h"
#include "runtime.h"

struct fraction;
typedef const fraction *fraction_p;
typedef gcp<fraction> fraction_g;

struct big_fraction;
typedef const big_fraction *big_fraction_p;
typedef gcp<big_fraction> big_fraction_g;


struct fraction : object
// ----------------------------------------------------------------------------
//   A fraction is a ratio of two integers
// ----------------------------------------------------------------------------
{
    fraction(integer_g n, integer_g d, id type): object(type)
    {
        byte *p = payload();
        byte_p np = n->payload();
        byte_p dp = d->payload();
        size_t ns = n->skip() - object_p(np);
        size_t ds = d->skip() - object_p(dp);
        memcpy(p, np, ns);
        memcpy(p + ns, dp, ds);
    }

    static size_t required_memory(id i, integer_g n, integer_g d)
    // ------------------------------------------------------------------------
    //  Compute the amount of memory required for an object
    // ------------------------------------------------------------------------
    {
        return leb128size(i)
            + n->size() - leb128size(n->type())
            + d->size() - leb128size(d->type());
    }

    size_t size(byte_p payload) const
    // ------------------------------------------------------------------------
    //   Return the size of a fraction
    // ------------------------------------------------------------------------
    {
        // LEB-128 encoded numerator and denominator
        size_t ns = leb128size(payload);
        payload += ns;
        size_t ds = leb128size(payload);
        payload += ds;
        return payload - byte_p(this);
    }

    integer_g numerator() const
    // ------------------------------------------------------------------------
    //   Return the numerator as an integer
    // ------------------------------------------------------------------------
    {
        id ty = type() == ID_fraction ? ID_integer : ID_neg_integer;
        byte_p p = payload();
        size_t ns = leb128size(p);
        return RT.make<integer>(ty, p, ns);
    }

    integer_g denominator() const
    // ------------------------------------------------------------------------
    //   Return the denominator as an integer (always positive)
    // ------------------------------------------------------------------------
    {
        byte_p p = payload();
        size_t ns = leb128size(p);
        p += ns;
        size_t ds = leb128size(p);
        return RT.make<integer>(ID_integer, p, ds);
    }

    static fraction_g make(integer_g n, integer_g d);
    OBJECT_HANDLER(fraction);
    OBJECT_RENDERER(fraction);
};


struct neg_fraction : fraction
// ----------------------------------------------------------------------------
//   Negative fraction, the numerator is seen as negative
// ----------------------------------------------------------------------------
{
    neg_fraction(integer_g num, integer_g den, id type = ID_neg_fraction)
        : fraction(num, den, type) {}
    OBJECT_HANDLER(neg_fraction)
    {
        if (op == RENDER)
            return obj->object_renderer(OBJECT_RENDERER_ARG(), rt);
        return DELEGATE(fraction);
    }
    OBJECT_RENDERER(neg_fraction);
};


struct big_fraction : object
// ----------------------------------------------------------------------------
//   A fraction where numerator and denominator are bignum
// ----------------------------------------------------------------------------
{
    big_fraction(bignum_g n, bignum_g d, id type): object(type)
    {
        byte *p = payload();
        byte_p np = n->payload();
        byte_p dp = d->payload();
        size_t ns = n->skip() - object_p(np);
        size_t ds = d->skip() - object_p(dp);
        memcpy(p, np, ns);
        memcpy(p + ns, dp, ds);
    }

    static size_t required_memory(id i, bignum_g n, bignum_g d)
    // ------------------------------------------------------------------------
    //  Compute the amount of memory required for an object
    // ------------------------------------------------------------------------
    {
        return leb128size(i)
            + n->object::size() - leb128size(n->type())
            + d->object::size() - leb128size(d->type());
    }

    static big_fraction_g make(bignum_g n, bignum_g d);

    size_t size(byte_p payload) const
    // ------------------------------------------------------------------------
    //   Return the size of a fraction
    // ------------------------------------------------------------------------
    {
        // Bignum-encoded numerator and denominator
        size_t ns = leb128<size_t>(payload);
        payload += ns;
        size_t ds = leb128<size_t>(payload);
        payload += ds;
        return payload - byte_p(this);
    }

    bignum_g numerator() const
    // ------------------------------------------------------------------------
    //   Return the numerator as a bignum
    // ------------------------------------------------------------------------
    {
        id ty = type() == ID_big_fraction ? ID_bignum : ID_neg_bignum;
        byte_p p = payload();
        size_t ns = leb128<size_t>(p);
        return RT.make<bignum>(ty, p, ns);
    }

    bignum_g denominator() const
    // ------------------------------------------------------------------------
    //   Return the denominator as bignum (always positive)
    // ------------------------------------------------------------------------
    {
        byte_p p = payload();
        size_t ns = leb128<size_t>(p);
        p += ns;
        size_t ds = leb128<size_t>(p);
        return RT.make<bignum>(ID_bignum, p, ds);
    }

    OBJECT_HANDLER(big_fraction);
    OBJECT_RENDERER(big_fraction);
};


struct neg_big_fraction : big_fraction
// ----------------------------------------------------------------------------
//   A negative fraction where numerator and denominator are bignum
// ----------------------------------------------------------------------------
{
    neg_big_fraction(bignum_g num, bignum_g den, id type = ID_neg_big_fraction)
        : big_fraction(num, den, type) {}
    OBJECT_HANDLER(neg_big_fraction)
    {
        if (op == RENDER)
            return obj->object_renderer(OBJECT_RENDERER_ARG(), rt);
        return DELEGATE(big_fraction);
    }
    OBJECT_RENDERER(neg_big_fraction);
};

#endif // FRACTION_H
