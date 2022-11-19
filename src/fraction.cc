// ****************************************************************************
//  fraction.cc                                                   DB48X project
// ****************************************************************************
//
//   File Description:
//
//     Representation of fractions
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

#include "fraction.h"

RECORDER(fraction, 16, "Fractions");


static ularge gcd(ularge a, ularge b)
// ----------------------------------------------------------------------------
//   Compute the greatest common denominator between a and b
// ----------------------------------------------------------------------------
{
    while (b)
    {
        ularge na = b;
        b = a % b;
        a = na;
    }
    return a;
}


static bignum_g gcd(bignum_g a, bignum_g b)
// ----------------------------------------------------------------------------
//   Compute the greatest common denominator between a and b
// ----------------------------------------------------------------------------
{
    while (!b->zero())
    {
        bignum_g na = b;
        b = a % b;
        a = na;
    }
    return a;
}


fraction_g fraction::make(integer_g n, integer_g d)
// ----------------------------------------------------------------------------
//   Create a reduced fraction from n and d
// ----------------------------------------------------------------------------
{
    ularge nv = n->value<ularge>();
    ularge dv = d->value<ularge>();
    ularge cd = gcd(nv, dv);
    if (cd > 1)
    {
        n = integer::make(nv / cd);
        d = integer::make(dv / cd);
    }
    runtime &rt = runtime::RT;
    bool negative =
        (n->type() == ID_neg_integer) !=
        (d->type() == ID_neg_integer);
    id ty = negative ? ID_neg_fraction : ID_fraction;
    return rt.make<fraction>(ty, n, d);
}


big_fraction_g big_fraction::make(bignum_g n, bignum_g d)
// ----------------------------------------------------------------------------
//   Create a reduced fraction from n and d
// ----------------------------------------------------------------------------
{
    bignum_g cd = gcd(n, d);
    size_t cds = 0;
    byte_p cdt = cd->value(&cds);
    if (cds != 1 || *cdt != 1)
    {
        n = n / cd;
        d = d / cd;
    }
    runtime &rt = runtime::RT;
    bool negative =
        (n->type() == ID_neg_bignum) !=
        (d->type() == ID_neg_bignum);
    id ty = negative ? ID_neg_big_fraction : ID_big_fraction;
    return rt.make<big_fraction>(ty, n, d);
}


OBJECT_HANDLER_BODY(fraction)
// ----------------------------------------------------------------------------
//  Handle commands for fractions
// ----------------------------------------------------------------------------
{
    record(fraction, "Command %+s on %p", name(op), obj);
    switch(op)
    {
    case EXEC:
    case EVAL:
        // Integer values evaluate as self
        return rt.push(obj) ? OK : ERROR;
    case SIZE:
        return obj->size(byte_p(payload));
    case PARSE:
        return SKIP;            // This is done in integer class
    case RENDER:
        return obj->object_renderer(OBJECT_RENDERER_ARG(), rt);
    case HELP:
        return (intptr_t) "fraction";

    default:
        // Check if anyone else knows how to deal with it
        return DELEGATE(object);
    }
}


OBJECT_HANDLER_BODY(big_fraction)
// ----------------------------------------------------------------------------
//  Handle commands for fractions using bignum as a representation
// ----------------------------------------------------------------------------
{
    record(fraction, "Command %+s on %p", name(op), obj);
    switch(op)
    {
    case EXEC:
    case EVAL:
        // Integer values evaluate as self
        return rt.push(obj) ? OK : ERROR;
    case SIZE:
        return obj->size(byte_p(payload));
    case PARSE:
        return SKIP;            // This is done in integer class
    case RENDER:
        return obj->object_renderer(OBJECT_RENDERER_ARG(), rt);
    case HELP:
        return (intptr_t) "fraction";

    default:
        // Check if anyone else knows how to deal with it
        return DELEGATE(object);
    }
}


// The renderer is in 'integer.cc'
