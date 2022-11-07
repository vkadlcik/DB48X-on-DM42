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

#include "decimal-32.h"
#include "decimal-64.h"
#include "decimal128.h"
#include "integer.h"


object::result function::evaluate(bid128_fn op128, arg_check_fn check)
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
        if (!check(xv))
            return ERROR;
        bid128 res;
        op128(&res.value, &xv.value);
        x = rt.make<decimal128>(ID_decimal128, res);
        rt.top(x);
        return OK;
    }

    // All other cases: report an error
    rt.error("Invalid number type");
    return ERROR;
}


static bool log_arg_check(bid128 &x)
// ----------------------------------------------------------------------------
//   Log cannot take a negative value as input
// ----------------------------------------------------------------------------
{
    if (decimal128::is_negative_or_zero(x))
    {
        runtime::RT.error("Argument outside domain");
        return false;
    }
    return true;
}


template<> bool arg_check<struct log>  (bid128 &x) { return log_arg_check(x); }
template<> bool arg_check<struct log2> (bid128 &x) { return log_arg_check(x); }
template<> bool arg_check<struct log10>(bid128 &x) { return log_arg_check(x); }


static bool sqrt_arg_check(bid128 &x)
// ----------------------------------------------------------------------------
//   Log cannot take a negative value as input
// ----------------------------------------------------------------------------
{
    if (decimal128::is_negative(x))
    {
        runtime::RT.error("Argument outside domain");
        return false;
    }
    return true;
}


template<> bool arg_check<struct sqrt>(bid128 &x) { return sqrt_arg_check(x); }


template<typename Func>
object::result function::evaluate()
// ----------------------------------------------------------------------------
//   Evaluation for a given function
// ----------------------------------------------------------------------------
{
    return evaluate(Func::bid128_op, arg_check<Func>);
}


template<>
object::result function::evaluate<struct abs>()
// ----------------------------------------------------------------------------
//   Special case for abs
// ----------------------------------------------------------------------------
{
    gcobj x = RT.stack(0);
    if (!x)
        return ERROR;

    id xt = x->type();
    if (xt == ID_neg_integer)
    {
        integer_p i = integer_p(object_p(x));
        ularge magnitude = i->value<ularge>();
        integer_p ai = RT.make<integer>(ID_integer, magnitude);
        if (!ai)
            return ERROR;
        RT.top(ai);
        return OK;
    }
    else if (is_integer(xt))
    {
        // No-op
        return OK;
    }

    // Fall-back to floating-point abs
    return evaluate(bid128_abs, arg_check<struct abs>);
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
