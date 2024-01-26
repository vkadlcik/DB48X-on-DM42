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

#include "array.h"
#include "bignum.h"
#include "compare.h"
#include "decimal.h"
#include "expression.h"
#include "fraction.h"
#include "functions.h"
#include "integer.h"
#include "list.h"
#include "runtime.h"
#include "settings.h"
#include "tag.h"
#include "text.h"
#include "unit.h"

#include <bit>
#include <bitset>


RECORDER(arithmetic,            16, "Arithmetic");
RECORDER(arithmetic_error,      16, "Errors from arithmetic code");

bool arithmetic::complex_promotion(algebraic_g &x, algebraic_g &y)
// ----------------------------------------------------------------------------
//   Return true if one type is complex and the other can be promoted
// ----------------------------------------------------------------------------
{
    if (!x || !y)
        return false;

    id xt = x->type();
    id yt = y->type();

    // If both are complex, we do not do anything: Complex ops know best how
    // to handle mixed inputs (mix of rectangular and polar). We should leave
    // it to them to handle the different representations.
    if (is_complex(xt) && is_complex(yt))
        return true;

    // Try to convert both types to the same complex type
    if (is_complex(xt))
        return complex_promotion(y, xt);
    if (is_complex(yt))
        return complex_promotion(x, yt);

    // Neither type is complex, no point to promote
    return false;
}


fraction_p arithmetic::fraction_promotion(algebraic_g &x)
// ----------------------------------------------------------------------------
//  Check if we can promote the number to a fraction
// ----------------------------------------------------------------------------
{
    id ty = x->type();
    if (is_fraction(ty))
        return fraction_g((fraction *) object_p(x));
    if (ty >= ID_integer && ty <= ID_neg_integer)
    {
        integer_g n = integer_p(object_p(x));
        integer_g d = integer::make(1);
        fraction_p f = fraction::make(n, d);
        return f;
    }
    if (ty >= ID_bignum && ty <= ID_neg_bignum)
    {
        bignum_g n = bignum_p(object_p(x));
        bignum_g d = bignum::make(1);
        fraction_p f = big_fraction::make(n, d);
        return f;
    }
    return nullptr;
}


template<>
algebraic_p arithmetic::non_numeric<add>(algebraic_r x, algebraic_r y)
// ----------------------------------------------------------------------------
//   Deal with non-numerical data types for addition
// ----------------------------------------------------------------------------
//   This deals with:
//   - Text + text: Concatenation of text
//   - Text + object: Concatenation of text + object text
//   - Object + text: Concatenation of object text + text
{
    // Deal with basic auto-simplifications rules
    if (Settings.AutoSimplify() && x->is_algebraic() && y->is_algebraic())
    {
        if (x->is_zero(false))                  // 0 + X = X
            return y;
        if (y->is_zero(false))                  // X + 0 = X
            return x;
    }

    // Check addition of unit objects
    if (unit_p xu = x->as<unit>())
    {
        if (unit_p yu = y->as<unit>())
        {
            unit_g xc = xu;
            if (yu->convert(xc))
            {
                algebraic_g xv = xc->value();
                algebraic_g yv = yu->value();
                algebraic_g ye = yu->uexpr();
                xv = xv + yv;
                return unit::simple(xv, ye);
            }
            return nullptr;
        }
        rt.inconsistent_units_error();
        return nullptr;
    }
    else if (y->type() == ID_unit)
    {
        rt.inconsistent_units_error();
        return nullptr;
    }

    // list + ...
    if (list_g xl = x->as<list>())
    {
        if (list_g yl = y->as<list>())
            return xl + yl;
        if (list_g yl = rt.make<list>(byte_p(+y), y->size()))
            return xl + yl;
    }
    else if (list_g yl = y->as<list>())
    {
        if (list_g xl = rt.make<list>(byte_p(+x), x->size()))
            return xl + yl;
    }

    // text + ...
    if (text_g xs = x->as<text>())
    {
        // text + text
        if (text_g ys = y->as<text>())
            return xs + ys;
        // text + object
        if (text_g ys = y->as_text())
            return xs + ys;
    }
    // ... + text
    else if (text_g ys = y->as<text>())
    {
        // object + text
        if (text_g xs = x->as_text())
            return xs + ys;
    }

    // vector + vector or matrix + matrix
    if (array_g xa = x->as<array>())
    {
        if (array_g ya = y->as<array>())
            return xa + ya;
        return xa->map(add::evaluate, y);
    }
    else if (array_g ya = y->as<array>())
    {
        return ya->map(x, add::evaluate);
    }

    // Not yet implemented
    return nullptr;
}


inline bool add::integer_ok(object::id &xt, object::id &yt,
                            ularge &xv, ularge &yv)
// ----------------------------------------------------------------------------
//   Check if adding two integers works or if we need to promote to real
// ----------------------------------------------------------------------------
{
    // For integer types of the same sign, promote to real if we overflow
    if ((xt == ID_neg_integer) == (yt == ID_neg_integer))
    {
        ularge sum = xv + yv;

        // Do not promot to real if we have based numbers as input
        if ((sum < xv || sum < yv) && is_real(xt) && is_real(yt))
            return false;

        xv = sum;
        // Here, the type of x is the type of the result
        return true;
    }

    // Opposite sign: the difference in magnitude always fit in an integer type
    if (!is_real(xt))
    {
        // Based numbers keep the base of the number in X
        xv = xv - yv;
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


inline bool add::bignum_ok(bignum_g &x, bignum_g &y)
// ----------------------------------------------------------------------------
//   We can always add two big integers (memory permitting)
// ----------------------------------------------------------------------------
{
    x = x + y;
    return true;
}


inline bool add::fraction_ok(fraction_g &x, fraction_g &y)
// ----------------------------------------------------------------------------
//   We can always add two fractions
// ----------------------------------------------------------------------------
{
    x = x + y;
    return true;
}


inline bool add::complex_ok(complex_g &x, complex_g &y)
// ----------------------------------------------------------------------------
//   Add complex numbers if we have them
// ----------------------------------------------------------------------------
{
    x = x + y;
    return true;
}


template <>
algebraic_p arithmetic::non_numeric<sub>(algebraic_r x, algebraic_r y)
// ----------------------------------------------------------------------------
//   Deal with non-numerical data types for multiplication
// ----------------------------------------------------------------------------
//   This deals with vector and matrix operations
{
    // Deal with basic auto-simplifications rules
    if (Settings.AutoSimplify() && x->is_algebraic() && y->is_algebraic())
    {
        if (y->is_zero(false))                  // X - 0 = X
            return x;
        if (x->is_same_as(y))                   // X - X = 0
            return integer::make(0);
        if (x->is_zero(false) && y->is_symbolic())
            return neg::run(y);                 // 0 - X = -X
    }

    // Check subtraction of unit objects
    if (unit_p xu = x->as<unit>())
    {
        if (unit_p yu = y->as<unit>())
        {
            unit_g xc = xu;
            if (yu->convert(xc))
            {
                algebraic_g xv = xc->value();
                algebraic_g yv = yu->value();
                algebraic_g ye = yu->uexpr();
                xv = xv - yv;
                return unit::simple(xv, ye);
            }
        }
        rt.inconsistent_units_error();
        return nullptr;
    }
    else if (y->type() == ID_unit)
    {
        rt.inconsistent_units_error();
        return nullptr;
    }

    // vector + vector or matrix + matrix
    if (array_g xa = x->as<array>())
    {
        if (array_g ya = y->as<array>())
            return xa - ya;
        return xa->map(sub::evaluate, y);
    }
    else if (array_g ya = y->as<array>())
    {
        return ya->map(x, sub::evaluate);
    }

    // Not yet implemented
    return nullptr;
}


inline bool sub::integer_ok(object::id &xt, object::id &yt,
                            ularge &xv, ularge &yv)
// ----------------------------------------------------------------------------
//   Check if subtracting two integers works or if we need to promote to real
// ----------------------------------------------------------------------------
{
    // For integer types of opposite sign, promote to real if we overflow
    if ((xt == ID_neg_integer) != (yt == ID_neg_integer))
    {
        ularge sum = xv + yv;
        if ((sum < xv || sum < yv) && is_real(xt) && is_real(yt))
            return false;
        xv = sum;

        // The type of x gives us the correct sign for the difference:
        //   -2 - 3 is -5, 2 - (-3) is 5:
        return true;
    }

    // Same sign: the difference in magnitude always fit in an integer type
    if (!is_real(xt))
    {
        // Based numbers keep the base of the number in X
        xv = xv - yv;
    }
    else if (yv >= xv)
    {
        // Case of (+3) - (+4) or (-3) - (-4): Change the sign of X
        xv = yv - xv;
        xt = (xv == 0 || xt == ID_neg_integer) ? ID_integer : ID_neg_integer;
    }
    else
    {
        // Case of (-3) - (-2) or (+3) - (+2): Keep the sign of X
        xv = xv - yv;
    }
    return true;
}


inline bool sub::bignum_ok(bignum_g &x, bignum_g &y)
// ----------------------------------------------------------------------------
//   We can always subtract two big integers (memory permitting)
// ----------------------------------------------------------------------------
{
    x = x - y;
    return true;
}


inline bool sub::fraction_ok(fraction_g &x, fraction_g &y)
// ----------------------------------------------------------------------------
//   We can always subtract two fractions (memory permitting)
// ----------------------------------------------------------------------------
{
    x = x - y;
    return true;
}


inline bool sub::complex_ok(complex_g &x, complex_g &y)
// ----------------------------------------------------------------------------
//   Subtract complex numbers if we have them
// ----------------------------------------------------------------------------
{
    x = x - y;
    return true;
}


template <>
algebraic_p arithmetic::non_numeric<mul>(algebraic_r x, algebraic_r y)
// ----------------------------------------------------------------------------
//   Deal with non-numerical data types for multiplication
// ----------------------------------------------------------------------------
//   This deals with:
//   - Text * integer: Repeat the text
//   - Integer * text: Repeat the text
{
    // Deal with basic auto-simplifications rules
    if (Settings.AutoSimplify() && x->is_algebraic() && y->is_algebraic())
    {
        if (x->is_zero(false))                  // 0 * X = 0
            return x;
        if (y->is_zero(false))                  // X * 0 = Y
            return y;
        if (x->is_one(false))                   // 1 * X = X
            return y;
        if (y->is_one(false))                   // X * 1 = X
            return x;
        if (x->type() == ID_ImaginaryUnit)
        {
            if (y->type() == ID_ImaginaryUnit)
                return integer::make(-1);
            if (y->is_real())
                return rectangular::make(integer::make(0), y);
        }
        if (y->type() == ID_ImaginaryUnit)
            if (x->is_real())
                return rectangular::make(integer::make(0), x);
        if (x->is_symbolic() && x->is_same_as(y))
            return sq::run(x);                  // X * X = X²
    }

    // Check multiplication of unit objects
    if (unit_p xu = x->as<unit>())
    {
        algebraic_g xv = xu->value();
        algebraic_g xe = xu->uexpr();
        if (unit_p yu = y->as<unit>())
        {
            algebraic_g yv = yu->value();
            algebraic_g ye = yu->uexpr();
            xv = xv * yv;
            xe = xe * ye;
            return unit::simple(xv, xe);
        }
        else
        {
            xv = xv * y;
            return unit::simple(xv, xe);
        }
    }
    else if (unit_p yu = y->as<unit>())
    {
        algebraic_g yv = yu->value();
        algebraic_g ye = yu->uexpr();
        yv = x * yv;
        return unit::simple(yv, ye);
    }

    // Text multiplication
    if (text_g xs = x->as<text>())
        if (integer_g yi = y->as<integer>())
            return xs * yi->value<uint>();
    if (text_g ys = y->as<text>())
        if (integer_g xi = x->as<integer>())
            return ys * xi->value<uint>();
    if (list_g xl = x->as<list>())
        if (integer_g yi = y->as<integer>())
            return xl * yi->value<uint>();
    if (list_g yl = y->as<list>())
        if (integer_g xi = x->as<integer>())
            return yl * xi->value<uint>();

    // vector + vector or matrix + matrix
    if (array_g xa = x->as<array>())
    {
        if (array_g ya = y->as<array>())
            return xa * ya;
        return xa->map(mul::evaluate, y);
    }
    else if (array_g ya = y->as<array>())
    {
        return ya->map(x, mul::evaluate);
    }

    // Not yet implemented
    return nullptr;
}


inline bool mul::integer_ok(object::id &xt, object::id &yt,
                            ularge &xv, ularge &yv)
// ----------------------------------------------------------------------------
//   Check if multiplying two integers works or if we need to promote to real
// ----------------------------------------------------------------------------
{
    // If one of the two objects is a based number, always use integer mul
    if (!is_real(xt) || !is_real(yt))
    {
        xv = xv * yv;
        return true;
    }

    // Check if there is an overflow
    // Can's use std::countl_zero yet (-std=c++20 breaks DMCP)
    if (std::__countl_zero(xv) + std::__countl_zero(yv) < int(8*sizeof(ularge)))
        return false;

    // Check if the multiplication generates a larger result. Is this correct?
    ularge product = xv * yv;

    // Check the sign of the product
    xt = (xt == ID_neg_integer) == (yt == ID_neg_integer)
        ? ID_integer
        : ID_neg_integer;
    xv = product;
    return true;
}


inline bool mul::bignum_ok(bignum_g &x, bignum_g &y)
// ----------------------------------------------------------------------------
//   We can always multiply two big integers (memory permitting)
// ----------------------------------------------------------------------------
{
    x = x * y;
    return true;
}


inline bool mul::fraction_ok(fraction_g &x, fraction_g &y)
// ----------------------------------------------------------------------------
//   We can always multiply two fractions (memory permitting)
// ----------------------------------------------------------------------------
{
    x = x * y;
    return true;
}


inline bool mul::complex_ok(complex_g &x, complex_g &y)
// ----------------------------------------------------------------------------
//   Multiply complex numbers if we have them
// ----------------------------------------------------------------------------
{
    x = x * y;
    return true;
}


template <>
algebraic_p arithmetic::non_numeric<struct div>(algebraic_r x, algebraic_r y)
// ----------------------------------------------------------------------------
//   Deal with non-numerical data types for division
// ----------------------------------------------------------------------------
//   This deals with vector and matrix operations
{
    // Deal with basic auto-simplifications rules
    if (Settings.AutoSimplify() && x->is_algebraic() && y->is_algebraic())
    {
        if (x->is_zero(false))                  // 0 / X = 0
        {
            if (y->is_zero(false))
            {
                rt.zero_divide_error();
                return nullptr;
            }
            return x;
        }
        if (y->is_one(false))                   // X / 1 = X
            return x;
        if (x->is_one(false) && y->is_symbolic())
            return inv::run(y);                 // 1 / X = X⁻¹
        if (x->is_same_as(y))
            return integer::make(1);            // X / X = 1
    }


    // Check division of unit objects
    if (unit_p xu = x->as<unit>())
    {
        algebraic_g xv = xu->value();
        algebraic_g xe = xu->uexpr();
        if (unit_p yu = y->as<unit>())
        {
            algebraic_g yv = yu->value();
            algebraic_g ye = yu->uexpr();
            xv = xv / yv;
            xe = xe / ye;
            return unit::simple(xv, xe);
        }
        else
        {
            xv = xv / y;
            return unit::simple(xv, xe);
        }
    }
    else if (unit_p yu = y->as<unit>())
    {
        algebraic_g yv = yu->value();
        algebraic_g ye = yu->uexpr();
        yv = x / yv;
        ye = inv::run(ye);
        return unit::simple(yv, ye);
    }

    // vector + vector or matrix + matrix
    if (array_g xa = x->as<array>())
    {
        if (array_g ya = y->as<array>())
            return xa / ya;
        return xa->map(div::evaluate, y);
    }
    else if (array_g ya = y->as<array>())
    {
        return ya->map(x, div::evaluate);
    }

    // Not yet implemented
    return nullptr;
}


inline bool div::integer_ok(object::id &xt, object::id &yt,
                            ularge &xv, ularge &yv)
// ----------------------------------------------------------------------------
//   Check if dividing two integers works or if we need to promote to real
// ----------------------------------------------------------------------------
{
    // Check divide by zero
    if (yv == 0)
    {
        rt.zero_divide_error();
        return false;
    }

    // If one of the two objects is a based number, always used integer div
    if (!is_real(xt) || !is_real(yt))
    {
        xv = xv / yv;
        return true;
    }

    // Check if there is a remainder - If so, switch to fraction
    if (xv % yv)
        return false;

    // Perform the division
    xv = xv / yv;

    // Check the sign of the ratio
    xt = (xt == ID_neg_integer) == (yt == ID_neg_integer)
        ? ID_integer
        : ID_neg_integer;
    return true;
}


inline bool div::bignum_ok(bignum_g &x, bignum_g &y)
// ----------------------------------------------------------------------------
//   Division works if there is no remainder
// ----------------------------------------------------------------------------
{
    if (!y)
    {
        rt.zero_divide_error();
        return false;
    }
    bignum_g q = nullptr;
    bignum_g r = nullptr;
    id type = bignum::product_type(x->type(), y->type());
    bool result = bignum::quorem(x, y, type, &q, &r);
    if (result)
        result = bignum_p(r) != nullptr;
    if (result)
    {
        if (is_based(type) || r->is_zero())
            x = q;                  // Integer result
        else
            x = bignum_p(fraction_p(big_fraction::make(x, y))); // Wrong-cast
    }
    return result;
}


inline bool div::fraction_ok(fraction_g &x, fraction_g &y)
// ----------------------------------------------------------------------------
//   Division of fractions, except division by zero
// ----------------------------------------------------------------------------
{
    if (!y->numerator())
    {
        rt.zero_divide_error();
        return false;
    }
    x = x / y;
    return true;
}


inline bool div::complex_ok(complex_g &x, complex_g &y)
// ----------------------------------------------------------------------------
//   Divide complex numbers if we have them
// ----------------------------------------------------------------------------
{
    if (y->is_zero())
    {
        rt.zero_divide_error();
        return false;
    }
    x = x / y;
    return true;
}


inline bool mod::integer_ok(object::id &xt, object::id &yt,
                            ularge &xv, ularge &yv)
// ----------------------------------------------------------------------------
//   The modulo of two integers is always an integer
// ----------------------------------------------------------------------------
{
    // Check divide by zero
    if (yv == 0)
    {
        rt.zero_divide_error();
        return false;
    }

    // If one of the two objects is a based number, always used integer mod
    if (!is_real(xt) || !is_real(yt))
    {
        xv = xv % yv;
        return true;
    }

    // Perform the modulo
    xv = xv % yv;
    if (xt == ID_neg_integer && xv)
        xv = yv - xv;

    // The resulting type is always positive
    xt = ID_integer;
    return true;
}


inline bool mod::bignum_ok(bignum_g &x, bignum_g &y)
// ----------------------------------------------------------------------------
//   Modulo always works except divide by zero
// ----------------------------------------------------------------------------
{
    bignum_g r = x % y;
    if (byte_p(r) == nullptr)
        return false;
    if (x->type() == ID_neg_bignum && !r->is_zero())
        x = y->type() == ID_neg_bignum ? r - y : r + y;
    else
        x = r;
    return true;
}


inline bool mod::fraction_ok(fraction_g &x, fraction_g &y)
// ----------------------------------------------------------------------------
//   Modulo of fractions, except division by zero
// ----------------------------------------------------------------------------
{
    if (!y->numerator())
    {
        rt.zero_divide_error();
        return false;
    }
    x = x % y;
    if (x->is_negative() && !x->is_zero())
        x = y->is_negative() ? x - y : x + y;
    return true;
}


inline bool mod::complex_ok(complex_g &, complex_g &)
// ----------------------------------------------------------------------------
//   No modulo on complex numbers
// ----------------------------------------------------------------------------
{
    return false;
}


inline bool rem::integer_ok(object::id &UNUSED xt, object::id &UNUSED yt,
                            ularge &xv, ularge &yv)
// ----------------------------------------------------------------------------
//   The reminder of two integers is always an integer
// ----------------------------------------------------------------------------
{
    // Check divide by zero
    if (yv == 0)
    {
        rt.zero_divide_error();
        return false;
    }

    // The type of the result is always the type of x
    xv = xv % yv;
    return true;
}


inline bool rem::bignum_ok(bignum_g &x, bignum_g &y)
// ----------------------------------------------------------------------------
//   Remainder always works except divide by zero
// ----------------------------------------------------------------------------
{
    x = x % y;
    return true;
}


inline bool rem::fraction_ok(fraction_g &x, fraction_g &y)
// ----------------------------------------------------------------------------
//   Modulo of fractions, except division by zero
// ----------------------------------------------------------------------------
{
    if (!y->numerator())
    {
        rt.zero_divide_error();
        return false;
    }
    x = x % y;
    return true;
}


inline bool rem::complex_ok(complex_g &, complex_g &)
// ----------------------------------------------------------------------------
//   No remainder on complex numbers
// ----------------------------------------------------------------------------
{
    return false;
}


template <>
algebraic_p arithmetic::non_numeric<struct pow>(algebraic_r x, algebraic_r y)
// ----------------------------------------------------------------------------
//   Deal with non-numerical data types for multiplication
// ----------------------------------------------------------------------------
{
    if (!x || !y)
        return nullptr;

    // Deal with the case of units
    if (unit_p xu = x->as<unit>())
    {
        algebraic_g xv = xu->value();
        algebraic_g xe = xu->uexpr();
        save<bool> save(unit::mode, false);
        return unit::simple(pow(xv, y), pow(xe, y));
    }

    // Check 0^0 (but check compatibility flag, since HPs return 1)
    // See https://www.hpcalc.org/hp48/docs/faq/48faq-5.html#ss5.2 as
    // to rationale on why HP calculators compute 0^0 as 1.
    if (x->is_zero(false) && y->is_zero(false))
    {
        if (Settings.ZeroPowerZeroIsUndefined())
            rt.undefined_operation_error();
        return integer::make(1);
    }

    // Deal with X^N where N is a positive  or negative integer
    id   yt   = y->type();
    bool negy = yt == ID_neg_integer;
    bool posy = yt == ID_integer;
    if (negy || posy)
    {
        // Defer computations for integer values to integer_ok
        if (x->is_integer() && !negy)
            return nullptr;

        // Auto-simplify x^0 = 1 and x^1 = x (we already tested 0^0)
        if (Settings.AutoSimplify())
        {
            if (y->is_zero(false))
                return integer::make(1);
            if (y->is_one())
                return x;
        }

        // Do not expand X^3 or integers when y>=0
        if (x->is_symbolic())
            return expression::make(ID_pow, x, y);

        // Deal with X^N where N is a positive integer
        ularge yv = integer_p(+y)->value<ularge>();
        algebraic_g r = integer::make(1);
        algebraic_g xx = x;
        while (yv)
        {
            if (yv & 1)
                r = r * xx;
            yv /= 2;
            xx = xx * xx;
        }
        if (negy)
            r = inv::run(r);
        return r;
    }

    // Not yet implemented
    return nullptr;
}


inline bool pow::integer_ok(object::id &xt, object::id &yt,
                            ularge &xv, ularge &yv)
// ----------------------------------------------------------------------------
//   Compute Y^X
// ----------------------------------------------------------------------------
{
    // Cannot raise to a negative power as integer
    if (yt == ID_neg_integer)
        return false;

    // Check the type of the result
    if (xt == ID_neg_integer)
        xt = (yv & 1) ? ID_neg_integer : ID_integer;

    // Compute result, check that it does not overflow
    ularge r = 1;
    enum { MAXBITS = 8 * sizeof(ularge) };
    while (yv)
    {
        if (yv & 1)
        {
            if (std::__countl_zero(xv) + std::__countl_zero(r) < MAXBITS)
                return false;   // Integer overflow
            ularge p = r * xv;
            r = p;
        }
        yv /= 2;

        if (std::__countl_zero(xv) * 2 < MAXBITS)
            return false;   // Integer overflow
        ularge nxv = xv * xv;
        xv = nxv;
    }

    xv = r;
    return true;
}


inline bool pow::bignum_ok(bignum_g &x, bignum_g &y)
// ----------------------------------------------------------------------------
//   Compute y^x, works if x >= 0
// ----------------------------------------------------------------------------
{
    // Compute result, check that it does not overflow
    if (y->type() == ID_neg_bignum)
        return false;
    x = bignum::pow(x, y);
    return true;
}


inline bool pow::complex_ok(complex_g &x, complex_g &y)
// ----------------------------------------------------------------------------
//   Implement x^y as exp(y * log(x))
// ----------------------------------------------------------------------------
{
    x = complex::exp(y * complex::log(x));
    return true;
}


inline bool pow::fraction_ok(fraction_g &UNUSED x, fraction_g &UNUSED y)
// ----------------------------------------------------------------------------
//   Compute y^x, works if x >= 0
// ----------------------------------------------------------------------------
{
    return false;
}


inline bool hypot::integer_ok(object::id &UNUSED xt, object::id &UNUSED yt,
                              ularge &UNUSED xv, ularge &UNUSED yv)
// ----------------------------------------------------------------------------
//   hypot() involves a square root, so not working on integers
// ----------------------------------------------------------------------------
//   Not trying to optimize the few cases where it works, e.g. 3^2+4^2=5^2
{
    return false;
}


inline bool hypot::bignum_ok(bignum_g &UNUSED x, bignum_g &UNUSED y)
// ----------------------------------------------------------------------------
//   Hypot never works with big integers
// ----------------------------------------------------------------------------
{
    return false;
}


inline bool hypot::fraction_ok(fraction_g &UNUSED x, fraction_g &UNUSED y)
// ----------------------------------------------------------------------------
//   Hypot never works with big integers
// ----------------------------------------------------------------------------
{
    return false;
}


inline bool hypot::complex_ok(complex_g &, complex_g &)
// ----------------------------------------------------------------------------
//   No hypot on complex yet, to be defined as sqrt(x^2+y^2)
// ----------------------------------------------------------------------------
{
    return false;
}



// ============================================================================
//
//   atan2: Optimize exact cases when dealing with fractions of pi
//
// ============================================================================

inline bool atan2::integer_ok(object::id &UNUSED xt, object::id &UNUSED yt,
                              ularge &UNUSED xv, ularge &UNUSED yv)
// ----------------------------------------------------------------------------
//   Optimized for integers on the real axis
// ----------------------------------------------------------------------------
{
    return false;
}


inline bool atan2::bignum_ok(bignum_g &UNUSED x, bignum_g &UNUSED y)
// ----------------------------------------------------------------------------
//   Optimize for bignums on the real axis
// ----------------------------------------------------------------------------
{
    return false;
}


inline bool atan2::fraction_ok(fraction_g &UNUSED x, fraction_g &UNUSED y)
// ----------------------------------------------------------------------------
//   Optimize for fractions on the real and complex axis and for diagonals
// ----------------------------------------------------------------------------
{
    return false;
}


inline bool atan2::complex_ok(complex_g &, complex_g &)
// ----------------------------------------------------------------------------
//   No atan2 on complex numbers yet
// ----------------------------------------------------------------------------
{
    return false;
}


template <>
algebraic_p arithmetic::non_numeric<struct atan2>(algebraic_r y, algebraic_r x)
// ----------------------------------------------------------------------------
//   Deal with various exact angle optimizations for atan2
// ----------------------------------------------------------------------------
//   Note that the first argument to atan2 is traditionally called y,
//   and represents the imaginary axis for complex numbers
{
    auto angle_mode = Settings.AngleMode();
    if (angle_mode != object::ID_Rad)
    {
        // Deal with special cases without rounding
        if (y->is_zero(false))
        {
            if (x->is_negative(false))
                return integer::make(1);
            return integer::make(0);
        }
        if (x->is_zero(false))
        {
            return fraction::make(integer::make(y->is_negative() ? -1 : 1),
                                  integer::make(2));
        }
        algebraic_g s = x + y;
        algebraic_g d = x - y;
        if (!s || !d)
            return nullptr;
        bool posdiag = d->is_zero(false);
        bool negdiag = s->is_zero(false);
        if (posdiag || negdiag)
        {
            bool xneg = x->is_negative();
            int  num  = posdiag ? (xneg ? -3 : 1) : (xneg ? 3 : -1);
            switch (angle_mode)
            {
            case object::ID_PiRadians:
                return fraction::make(integer::make(num), integer::make(4));
            case object::ID_Deg:
                return integer::make(num * 45);
            case object::ID_Grad:
                return integer::make(num * 50);
            default:
                break;
            }
        }
    }
    return nullptr;
}



// ============================================================================
//
//   Shared evaluation code
//
// ============================================================================

algebraic_p arithmetic::evaluate(id          op,
                                 algebraic_r xr,
                                 algebraic_r yr,
                                 ops_t       ops)
// ----------------------------------------------------------------------------
//   Shared code for all forms of evaluation, does not use the RPL stack
// ----------------------------------------------------------------------------
{
    if (!xr || !yr)
        return nullptr;

    algebraic_g x   = xr;
    algebraic_g y   = yr;
    utf8        err = rt.error();

    // Convert arguments to numeric if necessary
    if (Settings.NumericalResults())
    {
        (void) to_decimal(x, true);          // May fail silently
        (void) to_decimal(y, true);
    }

    id xt = x->type();
    id yt = y->type();

    // All non-numeric cases, e.g. string concatenation
    // Must come first, e.g. for optimization of X^3 or list + tagged object
    while(true)
    {
        if (algebraic_p result = ops.non_numeric(x, y))
            return result;
        if (rt.error() != err)
            return nullptr;

        if (xt == ID_tag)
        {
            x = algebraic_p(tag_p(+x)->tagged_object());
            xt = x->type();
        }
        else if (yt == ID_tag)
        {
            y = algebraic_p(tag_p(+y)->tagged_object());
            yt = y->type();
        }
        else
        {
            break;
        }
    }

    // Integer types
    if (is_integer(xt) && is_integer(yt))
    {
        bool based = is_based(xt) || is_based(yt);
        if (based)
        {
            xt = algebraic::based_promotion(x);
            yt = algebraic::based_promotion(y);
        }

        if (!is_bignum(xt) && !is_bignum(yt))
        {
            // Perform conversion of integer values to the same base
            integer_p xi = integer_p(object_p(+x));
            integer_p yi = integer_p(object_p(+y));
            uint      ws = Settings.WordSize();
            if (xi->native() && yi->native() && (ws < 64 || !based))
            {
                ularge xv = xi->value<ularge>();
                ularge yv = yi->value<ularge>();
                if (ops.integer_ok(xt, yt, xv, yv))
                {
                    if (based)
                        xv &= (1UL << ws) - 1UL;
                    return rt.make<integer>(xt, xv);
                }
            }
        }

        algebraic_g xb = x;
        algebraic_g yb = y;
        if (!is_bignum(xt))
            xt = bignum_promotion(xb);
        if (!is_bignum(yt))
            yt = bignum_promotion(yb);

        // Proceed with big integers if native did not fit
        bignum_g xg = bignum_p(+xb);
        bignum_g yg = bignum_p(+yb);
        if (ops.bignum_ok(xg, yg))
        {
            x = +xg;
            if (Settings.NumericalResults())
                (void) to_decimal(x, true);
            return x;
        }
    }

    // Fraction types
    if ((x->is_fraction() || y->is_fraction() ||
         (op == ID_div && x->is_fractionable() && y->is_fractionable())))
    {
        if (fraction_g xf = fraction_promotion(x))
        {
            if (fraction_g yf = fraction_promotion(y))
            {
                if (ops.fraction_ok(xf, yf))
                {
                    x = algebraic_p(fraction_p(xf));
                    if (x)
                    {
                        bignum_g d = xf->denominator();
                        if (d->is(1))
                            return algebraic_p(bignum_p(xf->numerator()));
                    }
                    if (Settings.NumericalResults())
                        (void) to_decimal(x, true);
                    return x;
                }
            }
        }
    }

    // Hardware-accelerated floating-point data types
    if (hwfp_promotion(x, y))
    {
        if (hwfloat_g fx = x->as<hwfloat>())
            if (hwfloat_g fy = y->as<hwfloat>())
                return ops.fop(fx, fy);
        if (hwdouble_g dx = x->as<hwdouble>())
            if (hwdouble_g dy = y->as<hwdouble>())
                return ops.dop(dx, dy);
    }


    // Real data types
    if (decimal_promotion(x, y))
    {
        // Here, x and y have the same type, a decimal type
        decimal_g xv = decimal_p(+x);
        decimal_g yv = decimal_p(+y);
        x = ops.decop(xv, yv);
        if (xv && !xv->is_normal())
        {
            rt.domain_error();
            return nullptr;
        }
        return x;
    }

    // Complex data types
    if (complex_promotion(x, y))
    {
        complex_g xc = complex_p(algebraic_p(x));
        complex_g yc = complex_p(algebraic_p(y));
        if (ops.complex_ok(xc, yc))
            return xc;
    }

    if (!x || !y)
        return nullptr;

    if (x->is_symbolic_arg() && y->is_symbolic_arg())
    {
        x = expression::make(op, x, y);
        return x;
    }

    // Default error is "Bad argument type", unless we got something else
    if (rt.error() == err)
        rt.type_error();
    return nullptr;
}


object::result arithmetic::evaluate(id op, ops_t ops)
// ----------------------------------------------------------------------------
//   Shared code for all forms of evaluation using the RPL stack
// ----------------------------------------------------------------------------
{
    if (!rt.args(2))
        return ERROR;

    // Fetch arguments from the stack
    // Possibly wrong type, i.e. it migth not be an algebraic on the stack,
    // but since we tend to do extensive type checking later, don't overdo it
    algebraic_g y = algebraic_p(rt.stack(1));
    if (!y)
        return ERROR;
    algebraic_g x = algebraic_p(rt.stack(0));
    if (!x)
        return ERROR;

    // Evaluate the operation
    algebraic_g r = evaluate(op, y, x, ops);

    // If result is valid, drop second argument and push result on stack
    if (r)
    {
        rt.drop();
        if (rt.top(r))
            return OK;
    }

    // Default error is "Bad argument type", unless we got something else
    if (!rt.error())
        rt.type_error();
    return ERROR;
}



// ============================================================================
//
//   Instantiations
//
// ============================================================================

template object::result arithmetic::evaluate<struct add>();
template object::result arithmetic::evaluate<struct sub>();
template object::result arithmetic::evaluate<struct mul>();
template object::result arithmetic::evaluate<struct div>();
template object::result arithmetic::evaluate<struct mod>();
template object::result arithmetic::evaluate<struct rem>();
template object::result arithmetic::evaluate<struct pow>();
template object::result arithmetic::evaluate<struct hypot>();
template object::result arithmetic::evaluate<struct atan2>();
template object::result arithmetic::evaluate<struct Min>();
template object::result arithmetic::evaluate<struct Max>();

template algebraic_p arithmetic::evaluate<struct mod>(algebraic_r x, algebraic_r y);
template algebraic_p arithmetic::evaluate<struct rem>(algebraic_r x, algebraic_r y);
template algebraic_p arithmetic::evaluate<struct hypot>(algebraic_r x, algebraic_r y);
template algebraic_p arithmetic::evaluate<struct atan2>(algebraic_r x, algebraic_r y);


template <typename Op>
arithmetic::ops_t arithmetic::Ops()
// ----------------------------------------------------------------------------
//   Return the operations for the given Op
// ----------------------------------------------------------------------------
{
    static const ops result =
    {
        Op::decop,
        hwfloat_fn(Op::fop),
        hwdouble_fn(Op::dop),
        Op::integer_ok,
        Op::bignum_ok,
        Op::fraction_ok,
        Op::complex_ok,
        non_numeric<Op>
    };
    return result;
}


template <typename Op>
algebraic_p arithmetic::evaluate(algebraic_r x, algebraic_r y)
// ----------------------------------------------------------------------------
//   Evaluate the operation for C++ use (not using RPL stack)
// ----------------------------------------------------------------------------
{
    return evaluate(Op::static_id, x, y, Ops<Op>());
}


template <typename Op>
object::result arithmetic::evaluate()
// ----------------------------------------------------------------------------
//   The stack-based evaluator for arithmetic operations
// ----------------------------------------------------------------------------
{
    return evaluate(Op::static_id, Ops<Op>());
}


// ============================================================================
//
//   C++ wrappers
//
// ============================================================================

algebraic_g operator-(algebraic_r x)
// ----------------------------------------------------------------------------
//   Negation
// ----------------------------------------------------------------------------
{
    return neg::evaluate(x);
}


algebraic_g operator+(algebraic_r x, algebraic_r y)
// ----------------------------------------------------------------------------
//   Addition
// ----------------------------------------------------------------------------
{
    return add::evaluate(x, y);
}


algebraic_g operator-(algebraic_r x, algebraic_r y)
// ----------------------------------------------------------------------------
//   Subtraction
// ----------------------------------------------------------------------------
{
    return sub::evaluate(x, y);
}


algebraic_g operator*(algebraic_r x, algebraic_r y)
// ----------------------------------------------------------------------------
//   Multiplication
// ----------------------------------------------------------------------------
{
    return mul::evaluate(x, y);
}


algebraic_g operator/(algebraic_r x, algebraic_r y)
// ----------------------------------------------------------------------------
//   Division
// ----------------------------------------------------------------------------
{
    return div::evaluate(x, y);
}


algebraic_g operator%(algebraic_r x, algebraic_r y)
// ----------------------------------------------------------------------------
//   Modulo
// ----------------------------------------------------------------------------
{
    return mod::evaluate(x, y);
}


algebraic_g pow(algebraic_r x, algebraic_r y)
// ----------------------------------------------------------------------------
//   Power
// ----------------------------------------------------------------------------
{
    return pow::evaluate(x, y);
}


INSERT_BODY(arithmetic)
// ----------------------------------------------------------------------------
//   Arithmetic objects do not insert parentheses
// ----------------------------------------------------------------------------
{
    if (o->type() == ID_mul && Settings.UseDotForMultiplication())
    {
        auto mode = ui.editing_mode();
        if (mode == ui.ALGEBRAIC || mode == ui.PARENTHESES)
            return ui.edit(utf8("·"), ui.ALGEBRAIC);
    }
    return ui.edit(o->fancy(), ui.INFIX);
}



// ============================================================================
//
//    Percentage operations
//
// ============================================================================

EVAL_BODY(Percent)
// ----------------------------------------------------------------------------
//   Evaluate percentage operation
// ----------------------------------------------------------------------------
{
    if (!rt.args(2))
        return object::ERROR;

    // Fetch arguments from the stack
    // Possibly wrong type, i.e. it migth not be an algebraic on the stack,
    // but since we tend to do extensive type checking later, don't overdo it
    algebraic_g y = algebraic_p(rt.stack(1));
    algebraic_g x = algebraic_p(rt.stack(0));
    if (!x ||!y)
        return object::ERROR;

    // Command type
    id          type    = o->type();
    algebraic_g hundred = integer::make(100);
    algebraic_g one     = integer::make(1);

    // Evaluate the operation
    switch(type)
    {
    default:
    case ID_Percent:            x = y * (x / hundred); break;
    case ID_PercentChange:      x = (x/y - one) * hundred; break;
    case ID_PercentTotal:       x = (x/y) * hundred; break;
    }

    // If result is valid, drop second argument and push result on stack
    if (x)
    {
        rt.drop();
        if (rt.top(x))
            return OK;
    }

    return ERROR;
}



// ============================================================================
//
//   Min and Max operations
//
// ============================================================================

bool Min::integer_ok(id &, id &, ularge &, ularge &)    { return false; }
bool Max::integer_ok(id &, id &, ularge &, ularge &)    { return false; }
bool Min::bignum_ok(bignum_g &, bignum_g &)             { return false; }
bool Max::bignum_ok(bignum_g &, bignum_g &)             { return false; }
bool Min::fraction_ok(fraction_g &, fraction_g &)       { return false; }
bool Max::fraction_ok(fraction_g &, fraction_g &)       { return false; }
bool Min::complex_ok(complex_g &, complex_g &)          { return false; }
bool Max::complex_ok(complex_g &, complex_g &)          { return false; }


static algebraic_p min_max(algebraic_r x, algebraic_r y, int sign)
// ----------------------------------------------------------------------------
//   Compute min / max
// ----------------------------------------------------------------------------
{
    if (array_g xa = x->as<array>())
    {
        if (array_g ya = y->as<array>())
        {
            auto xi = xa->begin();
            auto xe = xa->end();
            auto yi = ya->begin();
            auto ye = ya->end();
            array_g ra = array_p(rt.make<array>(nullptr, 0));
            algebraic_g xo, yo;
            while (xi != xe && yi != ye)
            {
                object_p xobj = *xi++;
                if (!xobj->is_algebraic())
                    return nullptr;
                object_p yobj = *yi++;
                if (!yobj->is_algebraic())
                    return nullptr;
                xo = algebraic_p(xobj);
                yo = algebraic_p(yobj);
                xo = min_max(xo, yo, sign);
                if (!xo)
                    return nullptr;
                ra = ra->append(+xo);
            }
            if (xi != xe || yi != ye)
            {
                rt.dimension_error();
                return nullptr;
            }
            return ra;
        }
        return xa->map(Min::evaluate, y);
    }
    else if (array_g ya = y->as<array>())
    {
        return ya->map(x, Min::evaluate);
    }

    int cmp = 0;
    if (comparison::compare(&cmp, x, y))
        return sign * cmp > 0 ? x : y;
    return nullptr;
}


template<>
algebraic_p arithmetic::non_numeric<Min>(algebraic_r x, algebraic_r y)
// ----------------------------------------------------------------------------
//   Process the Min command
// ----------------------------------------------------------------------------
{
    return min_max(x, y, -1);
}


template<>
algebraic_p arithmetic::non_numeric<Max>(algebraic_r x, algebraic_r y)
// ----------------------------------------------------------------------------
//  Process the Max command
// ----------------------------------------------------------------------------
{
    return min_max(x, y, 1);
}
