// ****************************************************************************
//  functions.cc                                                  DB48X project
// ****************************************************************************
//
//   File Description:
//
//
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

#include "functions.h"

#include "arithmetic.h"
#include "decimal128.h"
#include "integer.h"
#include "stack-cmds.h"


object::result function::evaluate(id op, bid128_fn op128)
// ----------------------------------------------------------------------------
//   Shared code for evaluation of all common math functions
// ----------------------------------------------------------------------------
{
    gcobj x = RT.stack(0);
    if (!x)
        return ERROR;

    id xt = x->type();
    runtime &rt = runtime::RT;
    if (is_integer(xt))
    {
        // Do not accept sin(#123h)
        if (is_real(xt))
            // Promote to a floating-point type
            xt = real_promotion(x);
    }

    // Call the right function
    // We need to only call the bid128 functions here, because the 32 and 64
    // variants are not in the DM42's QSPI, and take too much space here
    bool ok = real_promotion(x, ID_decimal128);
    if (ok)
    {
        bid128 xv = x->as<decimal128>()->value();
        bid128 res;
        op128(&res.value, &xv.value);
        int finite = false;
        bid128_isFinite(&finite, &res.value);
        if (!finite)
        {
            rt.domain_error();
            return ERROR;
        }
        x = rt.make<decimal128>(ID_decimal128, res);
        if (x && rt.top(x))
            return OK;
        return ERROR;           // Out of memory
    }

    // If things did not work with real number, try an equation
    if (x->is_symbolic())
    {
        gcobj arg[1] = { x };
        x = rt.make<equation>(ID_equation, 1, arg, op);
        if (x && rt.top(x))
            return OK;
        return ERROR;           // Out of memory
    }

    // All other cases: report an error
    rt.type_error();
    return ERROR;
}


template<typename Func>
object::result function::evaluate()
// ----------------------------------------------------------------------------
//   Evaluation for a given function
// ----------------------------------------------------------------------------
{
    return evaluate(Func::static_type(), Func::bid128_op);
}


static object::result symbolic(object::id type)
// ----------------------------------------------------------------------------
//   Check if the function's argument is symbolic, if so process it as is
// ----------------------------------------------------------------------------
{
    runtime &rt = runtime::RT;
    gcobj x = rt.stack(0);
    if (x->is_strictly_symbolic())
    {
        x = rt.make<equation>(object::ID_equation, 1, &x, type);
        if (x && rt.top(x))
            return object::OK;
        return object::ERROR;
    }
    return object::SKIP;
}



FUNCTION_BODY(abs)
// ----------------------------------------------------------------------------
//   Implementation of 'abs'
// ----------------------------------------------------------------------------
{
    gcobj x = RT.stack(0);
    if (!x)
        return ERROR;

    result r = symbolic(ID_abs);
    if (r != SKIP)
        return r;

    id xt = x->type();
    if (xt == ID_neg_integer)
    {
        integer_p i = integer_p(object_p(x));
        ularge magnitude = i->value<ularge>();
        integer_p ai = RT.make<integer>(ID_integer, magnitude);
        if (ai && RT.top(ai))
            return OK;
        return ERROR;           // Out of memory
    }
    else if (is_integer(xt))
    {
        // No-op
        return OK;
    }

    // Fall-back to floating-point abs
    return function::evaluate(ID_abs, bid128_abs);
}


FUNCTION_BODY(norm)
// ----------------------------------------------------------------------------
//   Implementation of 'norm'
// ----------------------------------------------------------------------------
{
    using abs = struct abs;

    result r = symbolic(ID_norm);
    if (r != SKIP)
        return r;

    return (result) run<abs>();
}


FUNCTION_BODY(inv)
// ----------------------------------------------------------------------------
//   Invert is implemented as 1/x
// ----------------------------------------------------------------------------
{
    result r = symbolic(ID_inv);
    if (r != SKIP)
        return r;

    // Apparently there is a div function getting in the way, see man div(3)
    using div = struct div;
    integer_p one = RT.make<integer>(ID_integer, 1);
    if (RT.push(one)             &&
        run<Swap>() == OK        &&
        run<div>()  == OK)
        return OK;
    return ERROR;
}


FUNCTION_BODY(neg)
// ----------------------------------------------------------------------------
//   Negate is implemented as 0-x
// ----------------------------------------------------------------------------
{
    result r = symbolic(ID_neg);
    if (r != SKIP)
        return r;

    integer_p zero = RT.make<integer>(ID_integer, 0);
    if (RT.push(zero)           &&
        run<Swap>() == OK       &&
        run<sub>()  == OK)
        return OK;
    return ERROR;
}


FUNCTION_BODY(sq)
// ----------------------------------------------------------------------------
//   Square is implemented as "dup mul"
// ----------------------------------------------------------------------------
{
    result r = symbolic(ID_sq);
    if (r != SKIP)
        return r;

    runtime &rt = RT;
    gcobj x = rt.stack(0);
    if (x->is_symbolic())
    {
        x = rt.make<equation>(ID_equation, 1, &x, ID_sq);
        if (x && rt.top(x))
            return OK;
        return ERROR;           // Out of memory
    }

    run<Dup>();
    run<mul>();
    return OK;
}


FUNCTION_BODY(cubed)
// ----------------------------------------------------------------------------
//   Cubed is implemented as "dup dup mul mul"
// ----------------------------------------------------------------------------
{
    result r = symbolic(ID_cubed);
    if (r != SKIP)
        return r;

    run<Dup>();
    run<Dup>();
    run<mul>();
    run<mul>();
    return OK;
}




// ============================================================================
//
//   Instatiations
//
// ============================================================================

template object::result function::evaluate<struct sqrt>();
template object::result function::evaluate<struct cbrt>();

template object::result function::evaluate<struct sin>();
template object::result function::evaluate<struct cos>();
template object::result function::evaluate<struct tan>();
template object::result function::evaluate<struct asin>();
template object::result function::evaluate<struct acos>();
template object::result function::evaluate<struct atan>();

template object::result function::evaluate<struct sinh>();
template object::result function::evaluate<struct cosh>();
template object::result function::evaluate<struct tanh>();
template object::result function::evaluate<struct asinh>();
template object::result function::evaluate<struct acosh>();
template object::result function::evaluate<struct atanh>();

template object::result function::evaluate<struct log1p>();
template object::result function::evaluate<struct expm1>();
template object::result function::evaluate<struct log>();
template object::result function::evaluate<struct log10>();
template object::result function::evaluate<struct log2>();
template object::result function::evaluate<struct exp>();
template object::result function::evaluate<struct exp10>();
template object::result function::evaluate<struct exp2>();
template object::result function::evaluate<struct erf>();
template object::result function::evaluate<struct erfc>();
template object::result function::evaluate<struct tgamma>();
template object::result function::evaluate<struct lgamma>();
