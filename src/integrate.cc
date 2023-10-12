// ****************************************************************************
//  integrate.cc                                                 DB48X project
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
//   (C) 2023 Christophe de Dinechin <christophe@dinechin.org>
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

#include "integrate.h"

#include "algebraic.h"
#include "arithmetic.h"
#include "compare.h"
#include "equation.h"
#include "functions.h"
#include "integer.h"
#include "recorder.h"
#include "settings.h"
#include "symbol.h"
#include "tag.h"

RECORDER(integrate,         16, "Numerical integration");
RECORDER(integrate_error,   16, "Numerical integrationsol");


COMMAND_BODY(Integrate)
// ----------------------------------------------------------------------------
//   Numerical integration
// ----------------------------------------------------------------------------
{
    if (!rt.args(4))
        return ERROR;

    object_g variable = rt.stack(0);
    object_g eq = rt.stack(1);
    object_g high = rt.stack(2);
    object_g low = rt.stack(3);
    if (!eq || !variable || !high || !low)
        return ERROR;

    record(integrate,
           "Integrating %t for variable %t in range %t-%t",
           eq.Safe(), variable.Safe(), low.Safe(), high.Safe());

    // Check that we have a variable name on stack level 1 and
    // a proram or equation on level 2
    symbol_g name = variable->as_quoted<symbol>();
    id eqty = eq->type();
    if (eqty != ID_program && eqty != ID_equation)
        name = nullptr;
    if (!name || !low->is_algebraic() || !high->is_algebraic())
    {
        rt.type_error();
        return ERROR;
    }

    // Drop input parameters
    rt.drop(4);

    // Actual integration
    algebraic_g intg = integrate(eq, name,
                                 algebraic_p(low.Safe()),
                                 algebraic_p(high.Safe()));
    if (intg.Safe() &&rt.push(intg.Safe()))
        return OK;

    return ERROR;
}


algebraic_p integrate(object_g     eq,
                      symbol_g     name,
                      algebraic_g lx,
                      algebraic_g hx)
// ----------------------------------------------------------------------------
//   The core of the integration function
// ----------------------------------------------------------------------------
{
    // Check if the guess is an algebraic or if we need to extract one
    algebraic_g x, dx, dx2;
    algebraic_g y, dy, sy, sy2;
    algebraic_g two = integer::make(2);
    record(integrate, "Initial range %t-%t", lx.Safe(), hx.Safe());

    // Set independent variable
    save<symbol_g *> iref(equation::independent, &name);
    save<object_g *> ival(equation::independent_value, (object_g *) &x);
    int              prec = -Settings.integprec;
    algebraic_g      eps = rt.make<decimal128>(object::ID_decimal128,
                                               prec, true);

    // Initial integration step and sum
    dx = hx - lx;
    sy = integer::make(0);
    if (!dx || !sy)
        return nullptr;

    // Loop for a maximum number of conversion iterations
    uint max = Settings.maxinteg;
    uint iter = 1;
    for (uint d = 0; iter <= max && !program::interrupted(); d++)
    {
        sy2 = sy;
        dx2 = dx / two;
        x   = lx + dx2;
        sy  = integer::make(0);
        if (!x || !sy)
            return nullptr;

        for (uint i = 0; i < iter; i++)
        {
            // If we are starting to use really big numbers, approximate
            if (x->is_big())
                if (!algebraic::to_decimal(x))
                    return nullptr;

            // Evaluate equation
            size_t depth = rt.depth();
            if (!rt.push(x.Safe()))
                return nullptr;
            record(integrate, "[%u:%u] x=%t", d, i, x.Safe());

            object::result err    = eq->execute();
            size_t         dnow   = rt.depth();
            if (dnow != depth + 1 && dnow != depth + 2)
            {
                record(integrate_error, "Depth moved from %u to %u", depth, dnow);
                rt.invalid_function_error();
                return nullptr;
            }

            if (err != object::OK)
            {
                // Error on last function evaluation, try again
                record(integrate_error, "Got error %+s", rt.error());
                return nullptr;
            }

            y = algebraic_p(rt.pop());
            if (dnow == depth + 2)
                rt.drop();
            record(integrate, "[%u:%u] x=%t y=%t", d, i, x.Safe(), y.Safe());

            if (!y)
                return nullptr;
            if (!y->is_algebraic())
            {
                rt.invalid_function_error();
                return nullptr;
            }

            sy = sy + y;
            x = x + dx;
            record(integrate, "[%u:%u] sy=%t", d, i, sy.Safe());
            if (!sy || !x)
                return nullptr;

            if (sy->is_big())
                if (!algebraic::to_decimal(sy))
                    return nullptr;
        }
        sy = sy * dx;
        record(integrate, "[%u] Sum sy=%t sy2=%t dx=%t",
               d, sy.Safe(), sy2.Safe(), dx.Safe());
        if (!sy)
            return nullptr;
        if (smaller_magnitude(sy - sy2, eps * sy2))
        {
            sy = (sy + sy2) / two;
            break;
        }

        dx = dx2;
        sy = sy + sy2 / two;
        if (!sy)
            return nullptr;

        iter <<= 1;
    }

    return sy;
}
