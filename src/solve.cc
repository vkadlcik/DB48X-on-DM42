// ****************************************************************************
//  solve.cc                                                      DB48X project
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

#include "solve.h"

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

RECORDER(solve,         16, "Numerical solver");
RECORDER(solve_error,   16, "Numerical solver");


static inline bool smaller(algebraic_r x, algebraic_r y)
// ----------------------------------------------------------------------------
//   Compare magnitude
// ----------------------------------------------------------------------------
{
    algebraic_p cmp = abs::run(x) < abs::run(y);
    return cmp && cmp->as_truth(false);
}


COMMAND_BODY(Root)
// ----------------------------------------------------------------------------
//   Numerical solver
// ----------------------------------------------------------------------------
{
    if (!rt.args(3))
        return ERROR;

    object_g eq = rt.stack(2);
    object_g variable = rt.stack(1);
    object_g guess = rt.stack(0);
    if (!eq || !variable || !guess)
        return ERROR;

    record(solve,
           "Solving %t for variable %t with guess %t",
           eq.Safe(), variable.Safe(), guess.Safe());

    // Check that we have a variable name on stack level 1 and
    // a proram or equation on level 2
    symbol_g name = variable->as_quoted<symbol>();
    id eqty = eq->type();
    if (eqty != ID_program && eqty != ID_equation)
        name = nullptr;
    if (!name)
    {
        rt.type_error();
        return ERROR;
    }
    if (eqty == ID_equation)
        eq = equation_p(eq.Safe())->as_difference_for_solve();

    // Drop input parameters
    rt.drop(3);

    // Actual solving
    if (algebraic_g x = solve(eq, name, guess))
    {
        size_t nlen = 0;
        gcutf8 ntxt = name->value(&nlen);
        object_g top = tag::make(ntxt, nlen, x.Safe());
        if (rt.push(top))
            return rt.error() ? ERROR : OK;
    }

    return ERROR;
}



algebraic_p solve(object_g eq, symbol_g name, object_g guess)
// ----------------------------------------------------------------------------
//   The core of the solver
// ----------------------------------------------------------------------------
{
    // Check if the guess is an algebraic or if we need to extract one
    algebraic_g x, dx, lx, hx;
    algebraic_g y, dy, ly, hy;
    object::id gty = guess->type();
    if (object::is_real(gty) || object::is_complex(gty))
    {
        lx = algebraic_p(guess.Safe());
        hx = algebraic_p(guess.Safe());
        y = integer::make(1000);
        hx = hx->is_zero() ? inv::run(y) : hx + hx / y;
    }
    else if (gty == object::ID_list || gty == object::ID_array)
    {
        lx = guess->algebraic_child(0);
        hx = guess->algebraic_child(1);
        if (!lx || !hx)
            return nullptr;
    }
    x = lx;
    record(solve, "Initial range %t-%t", lx.Safe(), hx.Safe());

    // Set independent variable
    save<symbol_g *> iref(equation::independent, &name);
    save<object_g *> ival(equation::independent_value, (object_g *) &x);
    int              prec = -Settings.solveprec;
    algebraic_g      eps = rt.make<decimal128>(object::ID_decimal128,
                                               prec, true);

    bool is_constant = true;
    bool is_valid = false;
    uint max = Settings.maxsolve;
    for (uint i = 0; i < max && !program::interrupted(); i++)
    {
        // Evaluate equation
        size_t depth = rt.depth();
        if (!rt.push(x.Safe()))
            return nullptr;
        record(solve, "[%u] x=%t", i, x.Safe());

        object::result err    = eq->execute();
        size_t         dnow   = rt.depth();
        bool           jitter = false;
        if (dnow != depth + 1 && dnow != depth + 2)
        {
            record(solve_error, "Depth moved from %u to %u", depth, dnow);
            rt.invalid_solve_function_error();
            return nullptr;
        }
        if (err != object::OK)
        {
            // Error on last function evaluation, try again
            record(solve_error, "Got error %+s", rt.error());
            if (!ly || !hy)
            {
                rt.bad_guess_error();
                return nullptr;
            }
            jitter = true;
        }
        else
        {
            is_valid = true;
            y = algebraic_p(rt.pop());
            if (dnow == depth + 2)
                rt.drop();
            record(solve, "[%u] x=%t y=%t", i, x.Safe(), y.Safe());
            if (!y || !y->is_algebraic())
            {
                rt.invalid_solve_function_error();
                return nullptr;
            }
            if (y->is_zero() || smaller(y, eps))
            {
                record(solve, "[%u] Solution=%t value=%t",
                       i, x.Safe(), y.Safe());
                return x;
            }

            if (!ly)
            {
                record(solve, "Setting low");
                ly = y;
                lx = x;
                x = hx;
                continue;
            }
            else if (!hy)
            {
                record(solve, "Setting high");
                hy = y;
                hx = x;
            }
            else if (smaller(y, ly))
            {
                // Smaller than the smallest
                record(solve, "Smallest");
                hx = lx;
                hy = ly;
                lx = x;
                ly = y;
            }
            else if (smaller(y, hy))
            {
                record(solve, "Improvement");
                // Between smaller and biggest
                hx = x;
                hy = y;
            }
            else if (smaller(hy, y))
            {
                // y became bigger, try to get closer to low
                bool crosses = (ly * hy)->is_negative(false);
                record(solve, "New value is worse");
                is_constant = false;

                // Try to bisect
                dx = integer::make(2);
                x = (lx + x) / dx;
                if (!x)
                    return nullptr;
                if (crosses)    // For positive and negative values, as is
                    continue;

                // Otherwise, try to jitter around
                jitter = true;
            }
            else
            {
                // y is constant - Try a random spot
                record(solve, "Unmoving");
                jitter = true;
            }

            if (!jitter)
            {
                dx = hx - lx;
                if (!dx)
                    return nullptr;
                if (dx->is_zero() ||
                    smaller(abs::run(dx) / (abs::run(hx) + abs::run(lx)), eps))
                {
                    x = lx;
                    if ((ly * hy)->is_negative(false))
                    {
                        record(solve, "[%u] Cross solution=%t value=%t",
                               i, x.Safe(), y.Safe());
                    }
                    else
                    {
                        record(solve, "[%u] Minimum=%t value=%t",
                               i, x.Safe(), y.Safe());
                        rt.no_solution_error();
                    }
                    return x;
                }

                dy = hy - ly;
                if (!dy)
                    return nullptr;
                if (dy->is_zero())
                {
                    record(solve,
                           "[%u] unmoving %t between %t and %t",
                           hy.Safe(), lx.Safe(), hx.Safe());
                    jitter = true;
                }
                else
                {
                    record(solve, "[%u] Moving to %t - %t / %t",
                           i, lx.Safe(), dy.Safe(), dx.Safe());
                    is_constant = false;
                    x = lx - y * dx / dy;
                }
            }

            // Check if there are unresolved symbols
            if (x->is_strictly_symbolic())
            {
                rt.invalid_solve_function_error();
                return x;
            }

            // If we are starting to use really big numbers, approximate
            if (x->is_big())
            {
                if (!algebraic::to_decimal(x))
                {
                    rt.invalid_solve_function_error();
                    return x;
                }
            }
        }

        // If we have some issue improving things, shake it a bit
        if (jitter)
        {
            int s = (i & 2)- 1;
            if (x->is_complex())
                dx = polar::make(integer::make(997 * s * i),
                                 integer::make(421 * s * i * i),
                                 settings::DEGREES);
            else
                dx = integer::make(0x1081 * s * i);
            dx = dx * eps;
            if (x->is_zero())
                x = dx;
            else
                x = x + x * dx;
            if (!x)
                return nullptr;
            record(solve, "Jitter x=%t", x.Safe());
        }
    }

    record(solve, "Exited after too many loops, x=%t y=%t lx=%t ly=%t",
           x.Safe(), y.Safe(), lx.Safe(), ly.Safe());

    if (!is_valid)
        rt.invalid_solve_function_error();
    else if (is_constant)
        rt.constant_value_error();
    else
        rt.no_solution_error();
    return lx;
}
