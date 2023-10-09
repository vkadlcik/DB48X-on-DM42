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

    // Check if the guess is an algebraic or if we need to extract one
    algebraic_g x, dx, lx, hx;
    algebraic_g y, dy, ly, hy;
    id gty = guess->type();
    if (object::is_real(gty) || object::is_complex(gty))
    {
        lx = algebraic_p(guess.Safe());
        hx = algebraic_p(guess.Safe());
        y = integer::make(1000);
        hx = hx->is_zero() ? inv::run(y) : hx + hx / y;
    }
    else if (gty == ID_list || gty == ID_array)
    {
        lx = guess->algebraic_child(0);
        hx = guess->algebraic_child(1);
        if (!lx || !hx)
            return ERROR;
    }
    x = lx;
    record(solve, "Initial range %t-%t", lx.Safe(), hx.Safe());

    // Drop input parameters
    rt.drop(3);

    // Set independent variable
    save<symbol_g *> iref(equation::independent, &name);
    save<object_g *> ival(equation::independent_value, (object_g *) &x);
    int              prec = -Settings.solveprec;
    algebraic_g      eps = rt.make<decimal128>(ID_decimal128, prec, true);

    uint errors = 0;
    uint unmoving = 0;
    uint unsuccessful = 0;
    uint max = Settings.maxsolve;
    for (uint i = 0; i < max && !program::interrupted(); i++)
    {
        // Evaluate equation
        size_t depth = rt.depth();
        if (!rt.push(x.Safe()))
            return ERROR;
        record(solve, "[%u] x=%t", i, x.Safe());
        object::result err = eq->execute();
        size_t dnow = rt.depth();
        if (dnow != depth + 1 && dnow != depth + 2)
        {
            record(solve_error, "Depth moved from %u to %u", depth, dnow);
            rt.invalid_solve_function_error();
            return ERROR;
        }
        if (err != object::OK)
        {
            // Error on last function evaluation, try again
            record(solve_error, "Got error %+s", rt.error());
            dx = integer::make(1000);
            x = x->is_zero() ? inv::run(dx) : x + x / dx;
            errors++;
        }
        else
        {
            y = algebraic_p(rt.pop());
            if (dnow == depth + 2)
                rt.drop();
            record(solve, "[%u] x=%t y=%t", i, x.Safe(), y.Safe());
            if (!y || !y->is_algebraic())
            {
                rt.invalid_solve_function_error();
                return ERROR;
            }
            if (y->is_zero())
                break;

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
                hx   = lx;
                hy = ly;
                lx    = x;
                ly = y;
            }
            else if (smaller(y, hy))
            {
                record(solve, "Not largest");
                // Between smaller and biggest
                hx = x;
                hy = y;
            }
            else if (smaller(hy, y))
            {
                // y is bigger, try to get closer to low
                record(solve, "Unsuccessful");
                y = integer::make(2);
                x = (lx + x) / y;
                if (!x)
                    return ERROR;
                unsuccessful++;
                continue;
            }
            else
            {
                record(solve, "Constant?");
                hx = x;
                hy = y;
                unmoving++;
            }

            dy = hy - ly;
            if (!dy)
                return ERROR;
            if (dy->is_zero())
            {
                unmoving++;
                record(solve, "[%u] unmoving=%u", i, unmoving);
                dx = integer::make(1000);
                hx = hx->is_zero() ? dx : hx + hx / dx;
                x = hx;
                if (!x)
                    return ERROR;
            }
            else
            {
                errors = unmoving = unsuccessful = 0;
                dx = hx - lx;
                if (!dx)
                    return ERROR;
                if (dx->is_zero() ||
                    smaller(abs::run(dx) / (abs::run(hx) + abs::run(lx)), eps))
                {
                    record(solve, "[%u] Solution=%t value=%t",
                           i, x.Safe(), y.Safe());
                    x = lx;
                    break;
                }
                x = lx - y * dx / dy;
                record(solve, "[%u] Moving to %t - %t / %t",
                       i, lx.Safe(), dy.Safe(), dx.Safe());
            }

            // If we are starting to use really big numbers, approximate
            if (x->is_big())
            {
                if (!algebraic::to_decimal(x))
                {
                    rt.invalid_solve_function_error();
                    break;
                }
            }
            if (x->is_strictly_symbolic())
            {
                rt.invalid_solve_function_error();
                break;
            }
        }
    }

    if (x)
    {
        size_t nlen = 0;
        gcutf8 ntxt = name->value(&nlen);
        object_g top = tag::make(ntxt, nlen, x.Safe());
        if (errors)
            rt.invalid_solve_function_error();
        else if (unmoving)
            rt.constant_value_error();
        else if (unsuccessful)
            rt.no_solution_error();
        if (top && rt.push(top))
            return rt.error() ? ERROR : OK;
    }

    return OK;
}
