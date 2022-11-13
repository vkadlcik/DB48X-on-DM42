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
    object_p y = RT.stack(1);
    object_p x = RT.stack(0);
    if (!x || !y)
        return ERROR;

    id xt = x->type();
    switch(xt)
    {
    case ID_True:
    case ID_False:
    case ID_integer:
    case ID_neg_integer:
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
        RT.pop();
        RT.top(command::static_object(r ? ID_True : ID_False));
        return OK;
    }
    case ID_bin_integer:
    case ID_oct_integer:
    case ID_dec_integer:
    case ID_hex_integer:
    {
        integer_p xi = integer_p(x);
        if (y->is_integer())
        {
            integer_p yi = integer_p(y);
            if (Settings.wordsize <= 64 && yi->native() && xi->native())
            {
                // Short-enough integers to fit as native machine type
                ularge xv = xi->value<ularge>();
                ularge yv = yi->value<ularge>();
                ularge value = native(yv, xv);
                if (Settings.wordsize < 64)
                    value &= (1ULL << Settings.wordsize) - 1ULL;
                RT.pop();
                RT.top(RT.make<integer>(xt, value));
                return OK;
            }
            integer_g xv = (integer *) xi;
            integer_g yv = (integer *) yi;
            integer_g result = big(yv, xv);
            RT.pop();
            RT.top(integer_p(result));
            return OK;
        }
    }
    default:
        RT.type_error();
        break;
    }

    return ERROR;
}


object::result logical::evaluate(unary_fn native, big_unary_fn big)
// ----------------------------------------------------------------------------
//   Evaluation for unary logical operations
// ----------------------------------------------------------------------------
{
    object_p x = RT.stack(0);
    if (!x)
        return ERROR;

    id xt = x->type();
    switch(xt)
    {
    case ID_True:
    case ID_False:
    case ID_integer:
    case ID_neg_integer:
    case ID_decimal128:
    case ID_decimal64:
    case ID_decimal32:
    {
        int xv = x->as_truth();
        if (xv < 0)
            return ERROR;
        RT.top(command::static_object((native(xv) & 1) ? ID_True : ID_False));
        return OK;
    }
    case ID_bin_integer:
    case ID_oct_integer:
    case ID_dec_integer:
    case ID_hex_integer:
    {
        integer_p xi = integer_p(x);
        if (Settings.wordsize <= 64 && xi->native())
        {
            ularge xv = xi->value<ularge>();
            ularge value = native(xv);
            if (Settings.wordsize < 64)
                value &= (1ULL << Settings.wordsize) - 1ULL;
            RT.top(RT.make<integer>(xt, value));
            return OK;
        }
        integer_g xv = (integer *) xi;
        integer_g result = big(xv);
        RT.top(integer_p(result));
        return OK;
    }
    default:
        RT.type_error();
        break;
    }

    return ERROR;
}
