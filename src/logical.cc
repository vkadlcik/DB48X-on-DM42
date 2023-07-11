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

#include "decimal-32.h"
#include "decimal-64.h"
#include "decimal128.h"
#include "integer.h"


object::result logical::evaluate(binary_fn native, big_binary_fn big)
// ----------------------------------------------------------------------------
//   Evaluation for binary logical operations
// ----------------------------------------------------------------------------
{
    algebraic_g y = algebraic_p(rt.stack(1));
    algebraic_g x = algebraic_p(rt.stack(0));
    if (!x || !y)
        return ERROR;

    id xt = x->type();
    switch(xt)
    {
    case ID_True:
    case ID_False:
    case ID_integer:
    case ID_neg_integer:
    case ID_bignum:
    case ID_neg_bignum:
    case ID_decimal128:
    case ID_decimal64:
    case ID_decimal32:
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
        return ERROR;           // Out of memory
    }
    case ID_bin_integer:
    case ID_oct_integer:
    case ID_dec_integer:
    case ID_hex_integer:
    case ID_based_integer:
    {
        integer_p xi = integer_p(object_p(x));
        if (y->is_integer())
        {
            integer_p yi = integer_p(object_p(y));
            if (Settings.wordsize <= 64 && yi->native() && xi->native())
            {
                // Short-enough integers to fit as native machine type
                ularge xv = xi->value<ularge>();
                ularge yv = yi->value<ularge>();
                ularge value = native(yv, xv);
                if (Settings.wordsize < 64)
                    value &= (1ULL << Settings.wordsize) - 1ULL;
                rt.pop();
                integer_p result = rt.make<integer>(xt, value);
                if (result && rt.top(result))
                    return OK;
                return ERROR;   // Out of memory
            }
        }
        // Fall through to bignum variants
    }

    case ID_bin_bignum:
    case ID_oct_bignum:
    case ID_dec_bignum:
    case ID_hex_bignum:
    case ID_based_bignum:
    {
        id yt = x->type();
        if (!is_bignum(xt))
            xt = bignum_promotion(x);
        if (!is_bignum(yt))
            yt = bignum_promotion(y);

        // Proceed with big integers if native did not fit
        bignum_g xg = (bignum *) object_p(x);
        bignum_g yg = (bignum *) object_p(y);
        rt.pop();
        bignum_g rg = big(yg, xg);
        if (bignum_p(rg) && rt.top(rg))
            return OK;
        return ERROR;           // Out of memory
    }

    default:
        rt.type_error();
        break;
    }

    return ERROR;
}


object::result logical::evaluate(unary_fn native, big_unary_fn big)
// ----------------------------------------------------------------------------
//   Evaluation for unary logical operations
// ----------------------------------------------------------------------------
{
    algebraic_g x = algebraic_p(rt.stack(0));
    if (!x)
        return ERROR;

    id xt = x->type();
    switch(xt)
    {
    case ID_True:
    case ID_False:
    case ID_integer:
    case ID_neg_integer:
    case ID_bignum:
    case ID_neg_bignum:
    case ID_decimal128:
    case ID_decimal64:
    case ID_decimal32:
    {
        int xv = x->as_truth();
        if (xv < 0)
            return ERROR;
        xv = native(xv) & 1;
        if (rt.top(command::static_object(xv ? ID_True : ID_False)))
            return OK;
        return ERROR;           // Out of memory
    }
    case ID_bin_integer:
    case ID_oct_integer:
    case ID_dec_integer:
    case ID_hex_integer:
    case ID_based_integer:
    {
        integer_p xi = integer_p(object_p(x));
        if (Settings.wordsize <= 64 && xi->native())
        {
            ularge xv = xi->value<ularge>();
            ularge value = native(xv);
            if (Settings.wordsize < 64)
                value &= (1ULL << Settings.wordsize) - 1ULL;
            integer_p result = rt.make<integer>(xt, value);
            if (result && rt.top(result))
                return OK;
            return ERROR;       // Out of memory
        }
        // Fall-through to bignum case
    }

    case ID_bin_bignum:
    case ID_oct_bignum:
    case ID_dec_bignum:
    case ID_hex_bignum:
    case ID_based_bignum:
    {
        if (!is_bignum(xt))
            xt = bignum_promotion(x);

        // Proceed with big integers if native did not fit
        bignum_g xg = (bignum *) object_p(x);
        bignum_g rg = big(xg);
        if (bignum_p(rg) && rt.top(rg))
            return OK;
        return ERROR;           // Out of memory
    }

    default:
        rt.type_error();
        break;
    }

    return ERROR;
}
