// ****************************************************************************
//  logical.cc                                                    DB48X project
// ****************************************************************************
//
//   File Description:
//
//     Logical operations
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

#include "logical.h"

#include "decimal.h"
#include "integer.h"


object::result logical::evaluate(binary_fn native, big_binary_fn big, bool num)
// ----------------------------------------------------------------------------
//   Evaluation for binary logical operations
// ----------------------------------------------------------------------------
{
    if (!rt.args(2))
        return ERROR;

    algebraic_g y = algebraic_p(rt.stack(1));
    algebraic_g x = algebraic_p(rt.stack(0));
    if (!x || !y)
        return ERROR;

    id xt = x->type();
    switch (xt)
    {
    case ID_True:
    case ID_False:
    case ID_hwfloat:
    case ID_hwdouble:
    case ID_decimal:
    case ID_neg_decimal:
        if (num)
        {
            rt.type_error();
            return ERROR;
        }

    case ID_integer:
    case ID_neg_integer:
    case ID_bignum:
    case ID_neg_bignum:
        if (!num)
        {
            // Logical truth
            int xv = x->as_truth();
            int yv = y->as_truth();
            if (xv < 0 || yv < 0)
                return ERROR;
            int r = native(yv, xv) & 1;
            rt.pop();
            if (rt.top(command::static_object(r ? ID_True : ID_False)))
                return OK;
            return ERROR; // Out of memory
        }
#if CONFIG_FIXED_BASED_OBJECTS
    case ID_bin_integer:
    case ID_oct_integer:
    case ID_dec_integer:
    case ID_hex_integer:
#endif // CONFIG_FIXED_BASED_OBJECTS
    case ID_based_integer:
    {
        integer_p xi = integer_p(object_p(x));
        if (y->is_integer())
        {
            integer_p yi = integer_p(object_p(y));
            size_t    ws = Settings.WordSize();
            if (ws <= 64 && yi->native() && xi->native())
            {
                // Short-enough integers to fit as native machine type
                ularge xv    = xi->value<ularge>();
                ularge yv    = yi->value<ularge>();
                ularge value = native(yv, xv);
                if (ws < 64)
                    value &= (1ULL << ws) - 1ULL;
                rt.pop();
                if (!is_based(xt) && y->is_based())
                    xt = y->type();
                integer_p result = rt.make<integer>(xt, value);
                if (result && rt.top(result))
                    return OK;
                return ERROR; // Out of memory
            }
        }
        // Fall through to bignum variants
    }

#if CONFIG_FIXED_BASED_OBJECTS
    case ID_bin_bignum:
    case ID_oct_bignum:
    case ID_dec_bignum:
    case ID_hex_bignum:
#endif // CONFIG_FIXED_BASED_OBJECTS
    case ID_based_bignum:
    {
        id yt = y->type();
        if (!is_bignum(xt))
            xt = bignum_promotion(x);
        if (!is_bignum(yt))
        {
            yt = bignum_promotion(y);
            if (!is_bignum(yt))
            {
                rt.type_error();
                return ERROR;
            }
        }

        // Proceed with big integers if native did not fit
        bignum_g xg = (bignum *) object_p(x);
        bignum_g yg = (bignum *) object_p(y);
        rt.pop();
        bignum_g rg = big(yg, xg);
        if (bignum_p(rg) && rt.top(rg))
            return OK;
        return ERROR; // Out of memory
    }

    default: rt.type_error(); break;
    }

    return ERROR;
}


object::result logical::evaluate(unary_fn native, big_unary_fn big, bool num)
// ----------------------------------------------------------------------------
//   Evaluation for unary logical operations
// ----------------------------------------------------------------------------
{
    if (!rt.args(1))
        return ERROR;
    algebraic_g x = algebraic_p(rt.stack(0));
    if (!x)
        return ERROR;

    id   xt  = x->type();
    bool neg = xt == ID_neg_integer || xt == ID_neg_bignum;
    switch (xt)
    {
    case ID_True:
    case ID_False:
    case ID_hwfloat:
    case ID_hwdouble:
    case ID_decimal:
    case ID_neg_decimal:
        if (num)
        {
            rt.type_error();
            return ERROR;
        }

    case ID_integer:
    case ID_neg_integer:
    case ID_bignum:
    case ID_neg_bignum:
        if (!num)
        {
            int xv = x->as_truth();
            if (xv < 0)
                return ERROR;
            xv = native(xv) & 1;
            if (rt.top(command::static_object(xv ? ID_True : ID_False)))
                return OK;
            return ERROR; // Out of memory
        }
#if CONFIG_FIXED_BASED_OBJECTS
    case ID_bin_integer:
    case ID_oct_integer:
    case ID_dec_integer:
    case ID_hex_integer:
#endif // CONFIG_FIXED_BASED_OBJECTS
    case ID_based_integer:
    {
        integer_p xi = integer_p(object_p(x));
        size_t    ws = Settings.WordSize();
        if (ws <= 64 && xi->native())
        {
            ularge xv    = xi->value<ularge>();
            ularge value = neg ? -native(-xv) : native(xv);
            if (ws < 64)
                value &= (1ULL << ws) - 1ULL;
            integer_p result = rt.make<integer>(xt, value);
            if (result && rt.top(result))
                return OK;
            return ERROR; // Out of memory
        }
        // Fall-through to bignum case
    }

#if CONFIG_FIXED_BASED_OBJECTS
    case ID_bin_bignum:
    case ID_oct_bignum:
    case ID_dec_bignum:
    case ID_hex_bignum:
#endif
    case ID_based_bignum:
    {
        if (!is_bignum(xt))
            xt = bignum_promotion(x);

        // Proceed with big integers if native did not fit
        bignum_g xg = (bignum *) object_p(x);
        if (neg)
            xg = -xg;
        bignum_g rg = big(xg);
        if (neg)
            rg = -rg;
        if (bignum_p(rg) && rt.top(rg))
            return OK;
        return ERROR; // Out of memory
    }

    default: rt.type_error(); break;
    }

    return ERROR;
}


// ============================================================================
//
//   Shifts with the configured size
//
// ============================================================================

ularge logical::rol(ularge x, ularge y)
// ----------------------------------------------------------------------------
//   Rotate left by the given amount
// ----------------------------------------------------------------------------
{
    ularge ws   = Settings.WordSize();
    ularge mask = ws < 64 ? ((1UL << ws) - 1UL) : ~0ULL;
    y %= ws;
    return ((x << y) | (x >> (ws - y))) & mask;
}


bignum_p logical::rol(bignum_r x, uint y)
// ----------------------------------------------------------------------------
//   Rotate left by the given amount
// ----------------------------------------------------------------------------
{
    return bignum::shift(x, int(y), true, false);
}


bignum_p logical::rol(bignum_r x, bignum_r y)
// ----------------------------------------------------------------------------
//   Rotate left by the given amount
// ----------------------------------------------------------------------------
{
    if (!x || !y)
        return nullptr;
    uint shift = y->as_uint32(0, true);
    if (rt.error())
        return nullptr;
    return bignum::shift(x, int(shift), true, false);
}


ularge logical::ror(ularge x, ularge y)
// ----------------------------------------------------------------------------
//   Rotate right by the given amount
// ----------------------------------------------------------------------------
{
    return rol(x, Settings.WordSize() - y);
}


bignum_p logical::ror(bignum_r x, uint y)
// ----------------------------------------------------------------------------
//   Rotate right by the given amount
// ----------------------------------------------------------------------------
{
    return bignum::shift(x, -int(y), true, false);
}


bignum_p logical::ror(bignum_r x, bignum_r y)
// ----------------------------------------------------------------------------
//   Rotate right
// ----------------------------------------------------------------------------
{
    if (!x || !y)
        return nullptr;
    uint shift = y->as_uint32(0, true);
    if (rt.error())
        return nullptr;
    return bignum::shift(x, -int(shift), true, false);
}


ularge logical::asr(ularge x, ularge y)
// ----------------------------------------------------------------------------
//   Arithmetic shift right
// ----------------------------------------------------------------------------
{
    uint ws   = Settings.WordSize();
    bool sbit = x & (1ULL << (ws - 1));
    x >>= y;
    if (sbit)
        x |= ((1 << y) - 1UL) << (ws - y);
    return x;
}


bignum_p logical::asr(bignum_r x, uint y)
// ----------------------------------------------------------------------------
//   Arithmetic shift right
// ----------------------------------------------------------------------------
{
    return bignum::shift(x, -int(y), false, true);
}


bignum_p logical::asr(bignum_r x, bignum_r y)
// ----------------------------------------------------------------------------
//   Arithmetic shift right
// ----------------------------------------------------------------------------
{
    if (!x|| !y)
        return nullptr;
    uint shift = y->as_uint32(0, true);
    if (rt.error())
        return nullptr;
    return bignum::shift(x, -int(shift), false, true);
}
