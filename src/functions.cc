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
#include "list.h"



bool function::should_be_symbolic(id type)
// ----------------------------------------------------------------------------
//   Check if we should treat the type symbolically
// ----------------------------------------------------------------------------
{
    return is_strictly_symbolic(type) || is_fraction(type);
}


algebraic_p function::symbolic(id op, algebraic_r x)
// ----------------------------------------------------------------------------
//    Check if we should process this function symbolically
// ----------------------------------------------------------------------------
{
    if (!x.Safe())
        return nullptr;
    return rt.make<equation>(ID_equation, op, x);
}


object::result function::evaluate(id op, bid128_fn op128, complex_fn zop)
// ----------------------------------------------------------------------------
//   Shared code for evaluation of all common math functions
// ----------------------------------------------------------------------------
{
    algebraic_g x = algebraic_p(rt.top());
    if (!x)
        return ERROR;
    x = evaluate(x, op ,op128, zop);
    if (x && rt.top(x))
        return OK;
    return ERROR;
}


algebraic_p function::evaluate(algebraic_r xr,
                               id          op,
                               bid128_fn   op128,
                               complex_fn  zop)
// ----------------------------------------------------------------------------
//   Shared code for evaluation of all common math functions
// ----------------------------------------------------------------------------
{
    if (!xr.Safe())
        return nullptr;

    id xt = xr->type();
    if (should_be_symbolic(xt))
        return symbolic(op, xr);
    if (is_complex(xt))
    {
        complex_r z = (complex_r) xr;
        return algebraic_p(zop(z));
    }

    algebraic_g x = xr;
    if (is_integer(xt))
    {
        // Do not accept sin(#123h)
        if (!is_real(xt))
        {
            rt.type_error();
            return nullptr;
        }

        // Promote to a floating-point type
        xt = real_promotion(x);
    }

    // Call the right function
    // We need to only call the bid128 functions here, because the 32 and 64
    // variants are not in the DM42's QSPI, and take too much space here
    if (real_promotion(x, ID_decimal128))
    {
        bid128 xv = decimal128_p(algebraic_p(x))->value();
        bid128 res;
        op128(&res.value, &xv.value);
        int finite = false;
        bid128_isFinite(&finite, &res.value);
        if (!finite)
        {
            rt.domain_error();
            return nullptr;
        }
        x = rt.make<decimal128>(ID_decimal128, res);
        return x;
    }

    // If things did not work with real number, try an equation
    if (x->is_strictly_symbolic())
    {
        x = rt.make<equation>(ID_equation, op, x);
        return x;
    }

    // All other cases: report an error
    rt.type_error();
    return nullptr;
}


object::result function::evaluate(algebraic_fn op)
// ----------------------------------------------------------------------------
//   Perform the operation from the stack, using a C++ operation
// ----------------------------------------------------------------------------
{
    algebraic_g x = algebraic_p(rt.top());
    x = op(x);
    if (x && rt.top(x))
        return OK;
    return ERROR;
}



FUNCTION_BODY(abs)
// ----------------------------------------------------------------------------
//   Implementation of 'abs'
// ----------------------------------------------------------------------------
//   Special case where we don't need to promote argument to decimal128
{
    if (!x.Safe())
        return nullptr;

    id xt = x->type();
    if (should_be_symbolic(xt))
        return symbolic(ID_abs, x);

    if (xt == ID_neg_integer  ||
        xt == ID_neg_bignum   ||
        xt == ID_neg_fraction ||
        xt == ID_neg_big_fraction)
    {
        // We can keep the object, just changing the type
        id absty = id(xt - 1);
        algebraic_p clone = algebraic_p(rt.clone(x));
        byte *tp = (byte *) clone;
        *tp = absty;
        return clone;
    }
    else if (is_integer(xt) || is_bignum(xt) || is_fraction(xt))
    {
        // No-op
        return x;
    }
    else if (is_complex(xt))
    {
        return complex_p(algebraic_p(x))->mod();
    }

    // Fall-back to floating-point abs
    return function::evaluate(x, ID_abs, bid128_abs, nullptr);
}


FUNCTION_BODY(norm)
// ----------------------------------------------------------------------------
//   Implementation of 'norm'
// ----------------------------------------------------------------------------
{
    if (!x.Safe())
        return nullptr;

    id xt = x->type();
    if (should_be_symbolic(xt))
        return symbolic(ID_norm, x);

    return abs::evaluate(x);
}


FUNCTION_BODY(sign)
// ----------------------------------------------------------------------------
//   Implementation of 'sign'
// ----------------------------------------------------------------------------
{
    if (!x.Safe())
        return nullptr;

    id xt = x->type();
    if (should_be_symbolic(xt))
        return symbolic(ID_sign, x);

    if (x->is_negative(false))
    {
        return integer::make(-1);
    }
    else if (x->is_zero(false))
    {
        return integer::make(0);
    }
    else if (is_integer(xt) || is_bignum(xt) || is_fraction(xt) || is_real(xt))
    {
        return integer::make(1);
    }
    else if (is_complex(xt))
    {
        return rt.make<polar>(integer::make(1), complex_p(algebraic_p(x))->arg());
    }

    rt.type_error();
    return nullptr;
}


FUNCTION_BODY(inv)
// ----------------------------------------------------------------------------
//   Invert is implemented as 1/x
// ----------------------------------------------------------------------------
{
    if (!x.Safe())
        return nullptr;
    if (x->is_strictly_symbolic())
        return symbolic(ID_inv, x);

    // Apparently there is a div function getting in the way, see man div(3)
    algebraic_g one = rt.make<integer>(ID_integer, 1);
    return one / x;
}


FUNCTION_BODY(neg)
// ----------------------------------------------------------------------------
//   Negate is implemented as 0-x
// ----------------------------------------------------------------------------
{
    if (!x.Safe())
        return nullptr;
    if (x->is_strictly_symbolic())
        return symbolic(ID_neg, x);

    algebraic_g zero = rt.make<integer>(ID_integer, 0);
    return zero - x;
}


FUNCTION_BODY(sq)
// ----------------------------------------------------------------------------
//   Square is implemented using a multiplication
// ----------------------------------------------------------------------------
{
    if (!x.Safe())
        return nullptr;
    if (x->is_strictly_symbolic())
        return rt.make<equation>(ID_equation, ID_sq, x);
    return x * x;
}


FUNCTION_BODY(cubed)
// ----------------------------------------------------------------------------
//   Cubed is implemented as "dup dup mul mul"
// ----------------------------------------------------------------------------
{
    if (!x.Safe())
        return nullptr;
    if (x->is_strictly_symbolic())
        return rt.make<equation>(ID_equation, ID_cubed, x);
    return x * x * x;
}
