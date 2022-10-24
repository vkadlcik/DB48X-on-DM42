// ****************************************************************************
//  arithmetic.cc                                                 DB48X project
// ****************************************************************************
//
//   File Description:
//
//     Implementation of basic arithmetic operations
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

#include "arithmetic.h"

#include "decimal-32.h"
#include "decimal-64.h"
#include "decimal128.h"
#include "integer.h"
#include "runtime.h"
#include "settings.h"


RECORDER(arithmetic,            16, "Arithmetic");
RECORDER(arithmetic_error,      16, "Errors from arithmetic code");

bool arithmetic::real_promotion(gcobj &x, object::id type)
// ----------------------------------------------------------------------------
//   Promote the value x to the given type
// ----------------------------------------------------------------------------
{
    object::id xt = x->type();
    if (xt == type)
        return true;

    record(arithmetic, "Real promotion of %p from %+s to %+s",
           (object_p) x, object::name(xt), object::name(type));
    runtime &rt = runtime::RT;
    switch(xt)
    {
    case ID_integer:
    {
        integer_p i    = x->as<integer>();
        ularge    ival = i->value<ularge>();
        switch (type)
        {
        case ID_decimal32:
            x = rt.make<decimal32>(ID_decimal32, ival);
            return true;
        case ID_decimal64:
            x = rt.make<decimal64>(ID_decimal64, ival);
            return true;
        case ID_decimal128:
            x = rt.make<decimal128>(ID_decimal128, ival);
            return true;
        default:
            break;
        }
        record(arithmetic_error,
               "Cannot promote integer %p (%llu) from %+s to %+s",
               i, ival, object::name(xt), object::name(type));
        rt.error("Invalid real conversion");
        return false;
    }
    case ID_neg_integer:
    {
        integer_p i    = x->as<neg_integer>();
        large     ival = -i->value<large>();
        switch (type)
        {
        case ID_decimal32:
            x = rt.make<decimal32>(ID_decimal32, ival);
            return true;
        case ID_decimal64:
            x = rt.make<decimal64>(ID_decimal64, ival);
            return true;
        case ID_decimal128:
            x = rt.make<decimal128>(ID_decimal128, ival);
            return true;
        default:
            break;
        }
        record(arithmetic_error,
               "Cannot promote neg_integer %p (%lld) from %+s to %+s",
               i, ival, object::name(xt), object::name(type));
        rt.error("Invalid real conversion");
        return false;
    }

    case ID_decimal32:
    {
        decimal32_p d = x->as<decimal32>();
        bid32       dval = d->value();
        switch (type)
        {
        case ID_decimal64:
            x = rt.make<decimal64>(ID_decimal64, dval);
            return true;
        case ID_decimal128:
            x = rt.make<decimal128>(ID_decimal128, dval);
            return true;
        default:
            break;
        }
        record(arithmetic_error,
               "Cannot promote decimal32 %p from %+s to %+s",
               d, object::name(xt), object::name(type));
        rt.error("Invalid real conversion");
        return false;
    }

    case ID_decimal64:
    {
        decimal64_p d = x->as<decimal64>();
        bid64       dval = d->value();
        switch (type)
        {
        case ID_decimal64:
            x = rt.make<decimal64>(ID_decimal64, dval);
            return true;
        case ID_decimal128:
            x = rt.make<decimal128>(ID_decimal128, dval);
            return true;
        default:
            break;
        }
        record(arithmetic_error,
               "Cannot promote decimal64 %p from %+s to %+s",
               d, object::name(xt), object::name(type));
        rt.error("Invalid real conversion");
        return false;
    }
    default:
        break;
    }

    return false;
}


bool arithmetic::real_promotion(gcobj &x, gcobj &y)
// ----------------------------------------------------------------------------
//   Promote x or y to the largest of both types
// ----------------------------------------------------------------------------
{
    id xt = x->type();
    id yt = y->type();
    if (is_integer(xt) && is_integer(yt))
    {
        // If we got here, we failed an integer op, e.g. 2/3
        uint16_t prec = Settings.precision;
        id       target = prec > BID64_MAXDIGITS ? ID_decimal128
            : prec > BID32_MAXDIGITS ? ID_decimal64
            : ID_decimal32;
        real_promotion(x, target);
        real_promotion(y, target);
        return true;
    }

    return xt < yt ? real_promotion(x, yt) : real_promotion(y, xt);
}


inline bool add::non_numeric(gcobj &x, gcobj &y,
                             object::id &xt, object::id &yt)
// ----------------------------------------------------------------------------
//   Deal with non-numerical data types for addition
// ----------------------------------------------------------------------------
{
    // Not yet implemented
    return false;
}


inline bool add::integer_ok(object::id &xt, object::id &yt,
                            ularge &xv, ularge &yv)
// ----------------------------------------------------------------------------
//   Check if adding two integers works or if we need to promote to real
// ----------------------------------------------------------------------------
{
    // If one of the two objects is a based number, always used integer add
    if (!is_real(xt) || !is_real(yt))
    {
        xv = yv + xv;
        return true;
    }

    // For integer types of the same sign, promote to real if we overflow
    if ((xt == object::ID_neg_integer) == (yt == object::ID_neg_integer))
    {
        ularge sum = xv + yv;
        if (sum < xv || sum < yv)
            return false;
        xv = sum;
        // Here, the type of x is the type of the result
        return true;
    }

    // Opposite sign: the difference in magnitude always fit in an integer type
    if (!is_real(xt))
    {
        // Based numbers keep the base of the number in X
        xv = yv - xv;
    }
    else if (yv >= xv)
    {
        // Case of (-3) + (+2) or (+3) + (-2): Change the sign of X
        xv = yv - xv;
        xt = (xv == 0 || xt == ID_neg_integer) ? ID_integer : ID_neg_integer;
    }
    else
    {
        // Case of (-3) + (+4) or (+3) + (-4): Keep the sign of X
        xv = xv - yv;
    }
    return true;
}


inline bool sub::non_numeric(gcobj &x, gcobj &y,
                             object::id &xt, object::id &yt)
// ----------------------------------------------------------------------------
//   Deal with non-numerical data types for subtraction
// ----------------------------------------------------------------------------
{
    // Not yet implemented
    return false;
}


inline bool sub::integer_ok(object::id &xt, object::id &yt,
                            ularge &xv, ularge &yv)
// ----------------------------------------------------------------------------
//   Check if subtracting two integers works or if we need to promote to real
// ----------------------------------------------------------------------------
{
    // If one of the two objects is a based number, always used integer sub
    if (!is_real(xt) || !is_real(yt))
    {
        xv = yv - xv;
        return true;
    }

    // For integer types of opposite sign, promote to real if we overflow
    if ((xt == object::ID_neg_integer) != (yt == object::ID_neg_integer))
    {
        ularge sum = xv + yv;
        if (sum < xv || sum < yv)
            return false;
        xv = sum;
        // The type of yt gives us the correct sign for the difference
        xt = yt;
        return true;
    }

    // Same sign: the difference in magnitude always fit in an integer type
    if (!is_real(xt))
    {
        // Based numbers keep the base of the number in X
        xv = yv - xv;
    }
    else if (yv > xv)
    {
        // Case of (-3) - (-2) or (+3) - (+2): Keep the sign of X
        xv = yv - xv;
    }
    else
    {
        // Case of (+3) - (+4) or (-3) - (-4): Change the sign of X
        xv = xv - yv;
        xt = (xv == 0 || xt == ID_neg_integer) ? ID_integer : ID_neg_integer;
    }
    return true;
}


inline bool mul::non_numeric(gcobj &x, gcobj &y,
                             object::id &xt, object::id &yt)
// ----------------------------------------------------------------------------
//   Deal with non-numerical data types for multiplicatoin
// ----------------------------------------------------------------------------
{
    // Not yet implemented
    return false;
}


inline bool mul::integer_ok(object::id &xt, object::id &yt,
                            ularge &xv, ularge &yv)
// ----------------------------------------------------------------------------
//   Check if multiplying two integers works or if we need to promote to real
// ----------------------------------------------------------------------------
{
    // If one of the two objects is a based number, always used integer mul
    if (!is_real(xt) || !is_real(yt))
    {
        xv = yv * xv;
        return true;
    }

    // Check if the multiplication generates a larger result. Is this correct?
    ularge product = xv * yv;
    if (product < xv || product < yv)
        return false;

    // Check the sign of the product
    xt = (xt == object::ID_neg_integer) == (yt == object::ID_neg_integer)
        ? object::ID_integer
        : object::ID_neg_integer;
    xv = product;
    return true;
}


inline bool div::non_numeric(gcobj &x, gcobj &y,
                             object::id &xt, object::id &yt)
// ----------------------------------------------------------------------------
//   Deal with non-numerical data types for division
// ----------------------------------------------------------------------------
{
    // Not yet implemented
    return false;
}


inline bool div::integer_ok(object::id &xt, object::id &yt,
                            ularge &xv, ularge &yv)
// ----------------------------------------------------------------------------
//   Check if dividing two integers works or if we need to promote to real
// ----------------------------------------------------------------------------
{
    // Check divid by zero
    if (xv == 0)
    {
        runtime::RT.error("Divide by zero");
        return false;
    }

    // If one of the two objects is a based number, always used integer sub
    if (!is_real(xt) || !is_real(yt))
    {
        xv = yv / xv;
        return true;
    }

    // Check if there is a remainder - If so, need to use real numbers
    if (yv % xv)
        return false;

    // Perform the division
    xv = yv / xv;

    // Check the sign of the ratio
    xt = (xt == object::ID_neg_integer) == (yt == object::ID_neg_integer)
        ? object::ID_integer
        : object::ID_neg_integer;
    return true;
}


template <typename Op>
object::result arithmetic::evaluate()
// ----------------------------------------------------------------------------
//   The evaluator for arithmetic operations
// ----------------------------------------------------------------------------
{
    gcobj x = RT.stack(0);
    gcobj y = RT.stack(1);
    if (!x || !y)
        return ERROR;

    id xt = x->type();
    id yt = y->type();
    runtime &rt = runtime::RT;

    /* Integer types */
    bool ok = false;
    if (is_integer(xt) && is_integer(yt))
    {
        /* Perform conversion of integer values to the same base */
        integer *xi = (integer *) (object_p) x;
        integer *yi = (integer *) (object_p) y;
        ularge xv = xi->value<ularge>();
        ularge yv = yi->value<ularge>();
        if (Op::integer_ok(xt, yt, xv, yv))
        {
            x = rt.make<integer>(xt, xv);
            ok = true;
        }
    }

    /* Real data types */
    if (!ok && real_promotion(x, y))
    {
        /* Here, x and y have the same type, a decimal type */
        xt = x->type();
        switch(xt)
        {
        case ID_decimal32:
        {
            bid32 xv = x->as<decimal32>()->value();
            bid32 yv = y->as<decimal32>()->value();
            bid32 res;
            Op::bid32_op(&res.value, &yv.value, &xv.value);
            x = rt.make<decimal32>(ID_decimal32, res);
            ok = true;
            break;
        }
        case ID_decimal64:
        {
            bid64 xv = x->as<decimal64>()->value();
            bid64 yv = y->as<decimal64>()->value();
            bid64 res;
            Op::bid64_op(&res.value, &yv.value, &xv.value);
            x = rt.make<decimal64>(ID_decimal64, res);
            ok = true;
            break;
        }
        case ID_decimal128:
        {
            bid128 xv = x->as<decimal128>()->value();
            bid128 yv = y->as<decimal128>()->value();
            bid128 res;
            Op::bid128_op(&res.value, &yv.value, &xv.value);
            x = rt.make<decimal128>(ID_decimal128, res);
            ok = true;
            break;
        }
        default:
            break;
        }
    }
    if (!ok)
        ok = Op::non_numeric(x, y, xt, yt);

    if (ok)
    {
        RT.drop();
        RT.top(x);
    }
    else
    {
        rt.error("Invalid types");
    }
    return ERROR;
}

// Apparently, there is a function called 'div' on the simulator, see man div(3)
template object::result arithmetic::evaluate<add>();
template object::result arithmetic::evaluate<sub>();
template object::result arithmetic::evaluate<mul>();
template object::result arithmetic::evaluate<struct div>();
