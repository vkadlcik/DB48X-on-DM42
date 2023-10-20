// ****************************************************************************
//  algebraic.cc                                                DB48X project
// ****************************************************************************
//
//   File Description:
//
//     Shared code for all algebraic commands
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

#include "algebraic.h"

#include "arithmetic.h"
#include "array.h"
#include "bignum.h"
#include "complex.h"
#include "equation.h"
#include "functions.h"
#include "integer.h"
#include "parser.h"
#include "renderer.h"
#include "runtime.h"
#include "settings.h"
#include "user_interface.h"

#include <ctype.h>
#include <stdio.h>


RECORDER(algebraic,       16, "RPL Algebraics");
RECORDER(algebraic_error, 16, "Errors processing a algebraic");


INSERT_BODY(algebraic)
// ----------------------------------------------------------------------------
//   Enter data in algebraic mode
// ----------------------------------------------------------------------------
{
    return ui.edit(o->fancy(), o->arity() ? ui.ALGEBRAIC : ui.CONSTANT);
}


bool algebraic::real_promotion(algebraic_g &x, object::id type)
// ----------------------------------------------------------------------------
//   Promote the value x to the given type
// ----------------------------------------------------------------------------
{
    if (!x.Safe())
        return false;

    id xt = x->type();
    if (xt == type)
        return true;

    record(algebraic, "Real promotion of %p from %+s to %+s",
           (object_p) x, object::name(xt), object::name(type));
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
            return x.Safe();
        case ID_decimal64:
            x = rt.make<decimal64>(ID_decimal64, ival);
            return x.Safe();
        case ID_decimal128:
            x = rt.make<decimal128>(ID_decimal128, ival);
            return x.Safe();
        default:
            break;
        }
        record(algebraic_error,
               "Cannot promote integer %p (%llu) from %+s to %+s",
               i, ival, object::name(xt), object::name(type));
        break;
    }
    case ID_neg_integer:
    {
        integer_p i    = x->as<neg_integer>();
        ularge    ival = i->value<ularge>();
        switch (type)
        {
        case ID_decimal32:
            x = rt.make<decimal32>(ID_decimal32, ival, true);
            return x.Safe();
        case ID_decimal64:
            x = rt.make<decimal64>(ID_decimal64, ival, true);
            return x.Safe();
        case ID_decimal128:
            x = rt.make<decimal128>(ID_decimal128, ival, true);
            return x.Safe();
        default:
            break;
        }
        record(algebraic_error,
               "Cannot promote neg_integer %p (%lld) from %+s to %+s",
               i, ival, object::name(xt), object::name(type));
        break;
    }

    case ID_bignum:
    case ID_neg_bignum:
    {
        bignum_g i = bignum_p(object_p(x));
        switch (type)
        {
        case ID_decimal32:
            x = rt.make<decimal32>(ID_decimal32, i);
            return x.Safe();
        case ID_decimal64:
            x = rt.make<decimal64>(ID_decimal64, i);
            return x.Safe();
        case ID_decimal128:
            x = rt.make<decimal128>(ID_decimal128, i);
            return x.Safe();
        default:
            break;
        }
        record(algebraic_error,
               "Cannot promote bignum %p from %+s to %+s",
               i.Safe(), object::name(xt), object::name(type));
        break;
    }

    case ID_fraction:
    case ID_neg_fraction:
    case ID_big_fraction:
    case ID_neg_big_fraction:
    {
        fraction_g f = fraction_p(object_p(x));
        switch (type)
        {
        case ID_decimal32:
            x = rt.make<decimal32>(ID_decimal32, f);
            return x.Safe();
        case ID_decimal64:
            x = rt.make<decimal64>(ID_decimal64, f);
            return x.Safe();
        case ID_decimal128:
            x = rt.make<decimal128>(ID_decimal128, f);
            return x.Safe();
        default:
            break;
        }
        record(algebraic_error,
               "Cannot promote fraction %p from %+s to %+s",
               f.Safe(), object::name(xt), object::name(type));
        break;
    }

    case ID_decimal32:
    {
        decimal32_p d = x->as<decimal32>();
        bid32       dval = d->value();
        switch (type)
        {
        case ID_decimal32:
            return true;
        case ID_decimal64:
            x = rt.make<decimal64>(ID_decimal64, dval);
            return x.Safe();
        case ID_decimal128:
            x = rt.make<decimal128>(ID_decimal128, dval);
            return x.Safe();
        default:
            break;
        }
        record(algebraic_error,
               "Cannot promote decimal32 %p from %+s to %+s",
               d, object::name(xt), object::name(type));
        break;
    }

    case ID_decimal64:
    {
        decimal64_p d = x->as<decimal64>();
        bid64       dval = d->value();
        switch (type)
        {
        case ID_decimal32:
        case ID_decimal64:
            return true;
        case ID_decimal128:
            x = rt.make<decimal128>(ID_decimal128, dval);
            return x.Safe();
        default:
            break;
        }
        record(algebraic_error,
               "Cannot promote decimal64 %p from %+s to %+s",
               d, object::name(xt), object::name(type));
        break;
    }

    case ID_decimal128:
    {
        switch(type)
        {
        case ID_decimal32:
        case ID_decimal64:
        case ID_decimal128:
            return x.Safe();
        default:
            break;
        }
        record(algebraic_error,
               "Cannot promote decimal128 %p from %+s to %+s",
               x.Safe(), object::name(xt), object::name(type));
        break;
    }

    default:
        break;
    }

    return false;
}


object::id algebraic::real_promotion(algebraic_g &x)
// ----------------------------------------------------------------------------
//   Promote the value x to a type selected based on preferences
// ----------------------------------------------------------------------------
{
    // Auto-selection of type
    uint16_t prec = Settings.precision;
    id       type = prec > BID64_MAXDIGITS ? ID_decimal128
                  : prec > BID32_MAXDIGITS ? ID_decimal64
                                           : ID_decimal32;
    return real_promotion(x, type) ? type : ID_object;
}


bool algebraic::complex_promotion(algebraic_g &x, object::id type)
// ----------------------------------------------------------------------------
//   Promote the value x to the given complex type
// ----------------------------------------------------------------------------
{
    id xt = x->type();
    if (xt == type)
        return true;

    record(algebraic, "Complex promotion of %p from %+s to %+s",
           (object_p) x, object::name(xt), object::name(type));

    if (!is_complex(type))
    {
        record(algebraic_error, "Complex promotion to invalid type %+s",
               object::name(type));
        return false;
    }

    if (xt == ID_polar)
    {
        // Convert from polar to rectangular
        polar_g z = polar_p(algebraic_p(x));
        x = rectangular_p(z->as_rectangular());
        return x.Safe();
    }
    else if (xt == ID_rectangular)
    {
        // Convert from rectangular to polar
        rectangular_g z = rectangular_p(algebraic_p(x));
        x = polar_p(z->as_polar());
        return x.Safe();
    }
    else if (is_strictly_symbolic(xt))
    {
        // Assume a symbolic value is complex for now
        // TODO: Implement `REALASSUME`
        return false;
    }
    else if (is_integer(xt) || is_real(xt) || is_symbolic(xt) ||
             is_algebraic(xt))
    {
        algebraic_g zero = algebraic_p(integer::make(0));
        if (type == ID_polar)
            x = polar::make(x, zero, settings::PI_RADIANS);
        else
            x = rectangular::make(x, zero);
        return x.Safe();
    }

    return false;
}


object::id algebraic::bignum_promotion(algebraic_g &x)
// ----------------------------------------------------------------------------
//   Promote the value x to the corresponding bignum
// ----------------------------------------------------------------------------
{
    id xt = x->type();
    id ty = xt;

    switch(xt)
    {
#if CONFIG_FIXED_BASED_OBJECTS
    case ID_hex_integer:        ty = ID_hex_bignum;     break;
    case ID_dec_integer:        ty = ID_dec_bignum;     break;
    case ID_oct_integer:        ty = ID_oct_bignum;     break;
    case ID_bin_integer:        ty = ID_bin_bignum;     break;
#endif // CONFIG_FIXED_BASED_OBJECTS
    case ID_based_integer:      ty = ID_based_bignum;   break;
    case ID_neg_integer:        ty = ID_neg_bignum;     break;
    case ID_integer:            ty = ID_bignum;         break;
    default:
        break;
    }
    if (ty != xt)
    {
        integer_g i = (integer *) object_p(x);
        x = rt.make<bignum>(ty, i);
    }
    return ty;
}


bool algebraic::decimal_to_fraction(algebraic_g &x)
// ----------------------------------------------------------------------------
//  Check if we can promote the number to a fraction
// ----------------------------------------------------------------------------
{
    id ty = x->type();
    switch(ty)
    {
    case ID_decimal64:
    case ID_decimal32:
        if (!real_promotion(x, ID_decimal128))
            return false;
    case ID_decimal128:
        x = decimal128_p(x.Safe())->to_fraction();
        return true;
    case ID_fraction:
    case ID_neg_fraction:
    case ID_big_fraction:
    case ID_neg_big_fraction:
        return true;

    case ID_rectangular:
    {
        rectangular_p z = rectangular_p(x.Safe());
        algebraic_g re = z->re();
        algebraic_g im = z->im();
        if (!decimal_to_fraction(re) || !decimal_to_fraction(im))
            return false;
        x = rectangular::make(re, im);
        return true;
    }
    case ID_polar:
    {
        polar_p z = polar_p(x.Safe());
        algebraic_g mod = z->mod();
        algebraic_g arg = z->pifrac();
        if (!decimal_to_fraction(mod) || !decimal_to_fraction(arg))
            return false;
        x = polar::make(mod, arg, settings::PI_RADIANS);
        return true;
    }
    default:
        return false;
    }
}


bool algebraic::to_decimal(algebraic_g &x, bool weak)
// ----------------------------------------------------------------------------
//   Convert a value to decimal
// ----------------------------------------------------------------------------
{
    id xt = x->type();

    switch(xt)
    {
    case ID_rectangular:
    {
        rectangular_p z = rectangular_p(x.Safe());
        algebraic_g re = z->re();
        algebraic_g im = z->im();
        if (to_decimal(re, weak) && to_decimal(im, weak))
        {
            x = rectangular::make(re, im);
            return true;
        }
        break;
    }
    case ID_polar:
    {
        polar_p z = polar_p(x.Safe());
        algebraic_g mod = z->mod();
        algebraic_g arg = z->pifrac();
        if (to_decimal(mod, weak) &&
            (mod->is_fraction() || to_decimal(arg, weak)))
        {
            x = polar::make(mod, arg, settings::PI_RADIANS);
            return true;
        }
        break;
    }
    case ID_integer:
    case ID_neg_integer:
        if (weak)
            return true;
    case ID_bignum:
    case ID_neg_bignum:
    case ID_fraction:
    case ID_neg_fraction:
    case ID_big_fraction:
    case ID_neg_big_fraction:
    case ID_decimal32:
    case ID_decimal64:
    case ID_decimal128:
        return real_promotion(x);
    case ID_pi:
        x = pi();
        return true;
    case ID_ImaginaryUnit:
        x = rectangular::make(integer::make(0),integer::make(1));
        return true;
    case ID_equation:
    {
        bool save = Settings.numeric;
        Settings.numeric = true;
        result r = x->execute();
        Settings.numeric = save;
        if (r == OK)
            if (object_p obj = rt.pop())
                if (algebraic_p alg = obj->as_algebraic())
                    x = alg;
        return !rt.error();
    }
    default:
        if (!weak)
            rt.type_error();
    }
    return false;
}


algebraic_g algebraic::pi()
// ----------------------------------------------------------------------------
//   Return the value of pi
// ----------------------------------------------------------------------------
{
    static bool init = false;
    static byte rep[1+sizeof(bid128)];
    if (!init)
    {
        bid128 pival;
        bid128_from_string(&pival.value,
                           "3.141592653589793238462643383279502884");
        memcpy(rep+1, &pival.value, sizeof(pival.value));
        rep[0] = object::ID_decimal128;
        init = true;
    }
    return decimal128_p(rep);
}


algebraic_p algebraic::evaluate_function(object_r eq, algebraic_r x)
// ----------------------------------------------------------------------------
//   Evaluate the eq object as a function
// ----------------------------------------------------------------------------
//   Equation objects can be one of:
//   - Something that takes value from the stack and returns it on the stack
//     for example << 1 + >>
//   - Something that evaluates using the indep and returns it on the stack,
//     for example 'X + 1' (assuming X is the independent variable)
{
    if (!rt.push(x.Safe()))
        return nullptr;
    save<object_g *> ival(equation::independent_value, (object_g *) &x);
    size_t   depth  = rt.depth();
    result   err    = eq->execute();
    size_t   dnow   = rt.depth();
    object_p result = rt.pop();
    if (dnow == depth + 1)
    {
        object_p indep = rt.pop();
        if (indep != x.Safe())
        {
            rt.invalid_function_error();
            err = ERROR;
        }
    }
    if (!result || !result->is_algebraic())
    {
        rt.type_error();
        err = ERROR;
    }
    if (err != OK || (dnow != depth && dnow != depth + 1))
    {
        if (dnow > depth)
            rt.drop(dnow - depth);
        if (err == OK)
            rt.invalid_function_error();
        return nullptr;
    }
    return algebraic_p(result);
}


EVAL_BODY(ImaginaryUnit)
// ----------------------------------------------------------------------------
//   Push a unit complex number on the stack
// ----------------------------------------------------------------------------
{
    if (!rt.push(o))
        return ERROR;
    return OK;
}


EVAL_BODY(pi)
// ----------------------------------------------------------------------------
//   Push a symbolic representation of π on the stack
// ----------------------------------------------------------------------------
{
    if (!rt.push(o))
        return ERROR;
    return OK;
}
