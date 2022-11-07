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


object::result logical::value(object_p obj, ularge *v)
// ----------------------------------------------------------------------------
//   Get the logical value for an object
// ----------------------------------------------------------------------------
{
    id type = obj->type();
    switch(type)
    {
    case ID_True:
        *v = 1;
        return OK;
    case ID_False:
        *v = 0;
        return OK;
    case ID_integer:
    case ID_neg_integer:
        *v = integer_p(obj)->value<ularge>() != 0;
        return OK;
    case ID_bin_integer:
    case ID_oct_integer:
    case ID_dec_integer:
    case ID_hex_integer:
        *v = integer_p(obj)->value<ularge>();
        return OK;
    case ID_decimal128:
        *v = !decimal128_p(obj)->is_zero();
        return OK;
    case ID_decimal64:
        *v = !decimal64_p(obj)->is_zero();
        return OK;
    case ID_decimal32:
        *v = !decimal32_p(obj)->is_zero();
        return OK;
    default:
        RT.error("Bad argument type");
    }
    return ERROR;
}


object::result logical::evaluate(binary_fn op)
// ----------------------------------------------------------------------------
//   Evaluation for binary logical operations
// ----------------------------------------------------------------------------
{
    object_p y = RT.stack(1);
    object_p x = RT.stack(0);
    if (!x || !y)
        return ERROR;

    id xt = x->type();

    ularge xv = 0;
    ularge yv = 0;
    result r = value(y, &yv);
    if (r != OK)
        return r;
    r = value(x, &xv);
    if (r != OK)
        return r;

    RT.pop();
    RT.pop();
    ularge value = op(yv, xv);

    switch(xt)
    {
    case ID_True:
    case ID_False:
    case ID_integer:
    case ID_neg_integer:
    case ID_decimal128:
    case ID_decimal64:
    case ID_decimal32:
        RT.push(command::static_object(value ? ID_True : ID_False));
        return OK;

    case ID_bin_integer:
    case ID_oct_integer:
    case ID_dec_integer:
    case ID_hex_integer:
        RT.push(RT.make<integer>(xt, value));
        return OK;
    default:
        RT.error("Bad argument type");
        break;
    }

    return ERROR;
}


object::result logical::evaluate(unary_fn op)
// ----------------------------------------------------------------------------
//   Evaluation for unary logical operations
// ----------------------------------------------------------------------------
{
    object_p x = RT.stack(0);
    if (!x)
        return ERROR;

    id xt = x->type();
    ularge xv = 0;
    result r = value(x, &xv);
    if (r != OK)
        return r;

    ularge value = op(xv);

    switch(xt)
    {
    case ID_True:
    case ID_False:
    case ID_integer:
    case ID_neg_integer:
    case ID_decimal128:
    case ID_decimal64:
    case ID_decimal32:
        RT.top(command::static_object(value ? ID_True : ID_False));
        return OK;

    case ID_bin_integer:
    case ID_oct_integer:
    case ID_dec_integer:
    case ID_hex_integer:
        RT.top(RT.make<integer>(xt, value));
        return OK;
    default:
        RT.error("Bad argument type");
        break;
    }

    return ERROR;
}
