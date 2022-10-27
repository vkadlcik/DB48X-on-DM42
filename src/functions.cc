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


template<> bool arg_check<log>  (bid128 &x) { return log_arg_check(x); }
template<> bool arg_check<log2> (bid128 &x) { return log_arg_check(x); }
template<> bool arg_check<log10>(bid128 &x) { return log_arg_check(x); }


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


template<> bool arg_check<sqrt>(bid128 &x) { return sqrt_arg_check(x); }


template<typename Func>
object::result function::evaluate()
// ----------------------------------------------------------------------------
//   Evaluation for a given function
// ----------------------------------------------------------------------------
{
    return evaluate(Func::bid128_op, arg_check<Func>);
}



// ============================================================================
//
//   Instatiations
//
// ============================================================================

template object::result function::evaluate<sqrt>();
template object::result function::evaluate<cbrt>();

template object::result function::evaluate<sin>();
template object::result function::evaluate<cos>();
template object::result function::evaluate<tan>();
template object::result function::evaluate<asin>();
template object::result function::evaluate<acos>();
template object::result function::evaluate<atan>();

template object::result function::evaluate<sinh>();
template object::result function::evaluate<cosh>();
template object::result function::evaluate<tanh>();
template object::result function::evaluate<asinh>();
template object::result function::evaluate<acosh>();
template object::result function::evaluate<atanh>();

template object::result function::evaluate<log1p>();
template object::result function::evaluate<expm1>();
template object::result function::evaluate<log>();
template object::result function::evaluate<log10>();
template object::result function::evaluate<log2>();
template object::result function::evaluate<exp>();
template object::result function::evaluate<exp10>();
template object::result function::evaluate<exp2>();
template object::result function::evaluate<erf>();
template object::result function::evaluate<erfc>();
template object::result function::evaluate<tgamma>();
template object::result function::evaluate<lgamma>();
