#ifndef ARITHMETIC_H
#define ARITHMETIC_H
// ****************************************************************************
//  arithmetic.h                                                  DB48X project
// ****************************************************************************
//
//   File Description:
//
//     Basic arithmetic operations
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
#include "runtime.h"

struct arithmetic : algebraic
// ----------------------------------------------------------------------------
//   Shared logic for all arithmetic operations
// ----------------------------------------------------------------------------
{
    arithmetic(id i): algebraic(i) {}

    static bool real_promotion(gcobj &x, object::id type);
    // -------------------------------------------------------------------------
    //   Promote the value x to the given type, return true if successful
    // -------------------------------------------------------------------------

    static bool real_promotion(gcobj &x, gcobj &y);
    // -------------------------------------------------------------------------
    //   Promote x or y to the largest of both types, return true if successful
    // -------------------------------------------------------------------------


protected:
    template <typename Op> result evaluate();
    // ------------------------------------------------------------------------
    //   The actual evaluation for all binary operators
    // ------------------------------------------------------------------------
};


#define ARITHMETIC_DECLARE(derived, Arity, Precedence)                  \
/* ----------------------------------------------------------------- */ \
/*  Macro to define an arithmetic command                            */ \
/* ----------------------------------------------------------------- */ \
struct derived : arithmetic                                             \
{                                                                       \
    derived(id i = ID_##derived) : arithmetic(i) {}                     \
                                                                        \
    static uint arity()             { return Arity; }                   \
    static uint precedence()        { return Precedence; }              \
                                                                        \
    static bool integer_ok(id &xt, id &yt, ularge &xv, ularge &yv);     \
    static bool non_numeric(gcobj &x, gcobj &y, id &xt, id &yt);        \
    static constexpr auto bid32_op = bid32_##derived;                   \
    static constexpr auto bid64_op = bid64_##derived;                   \
    static constexpr auto bid128_op = bid128_##derived;                 \
                                                                        \
    OBJECT_HANDLER(derived)                                             \
    {                                                                   \
        if (op == EVAL)                                                 \
        {                                                               \
            RT.command(long_name[ID_##derived]);                        \
            if (!Arity || RT.stack(Arity-1))                            \
                return ((derived *) obj)->evaluate();                   \
            return ERROR;                                               \
        }                                                               \
        return DELEGATE(arithmetic);                                    \
    }                                                                   \
    result evaluate()                                                   \
    {                                                                   \
        return arithmetic::evaluate<derived>();                         \
    }                                                                   \
}

ARITHMETIC_DECLARE(add, 2, 5);
ARITHMETIC_DECLARE(sub, 2, 5);
ARITHMETIC_DECLARE(mul, 2, 7);
ARITHMETIC_DECLARE(div, 2, 7);

#endif // ARITHMETIC
