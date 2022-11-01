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
#include "decimal-32.h"
#include "decimal-64.h"
#include "decimal128.h"
#include "runtime.h"


struct arithmetic : algebraic
// ----------------------------------------------------------------------------
//   Shared logic for all arithmetic operations
// ----------------------------------------------------------------------------
{
    arithmetic(id i): algebraic(i) {}

    static bool real_promotion(gcobj &x, gcobj &y);
    // -------------------------------------------------------------------------
    //   Promote x or y to the largest of both types, return true if successful
    // -------------------------------------------------------------------------

    static bool real_promotion(gcobj &x, object::id type)
    {
        return algebraic::real_promotion(x, type);
    }
    static id   real_promotion(gcobj &x)
    {
        return algebraic::real_promotion(x);
    }



protected:
    typedef bool (*integer_fn)(id &xt, id &yt, ularge &xv, ularge &yv);
    typedef bool (*non_numeric_fn)(gcobj &x, gcobj &y, id &xt, id &yt);

    // Function pointers used by generic evaluation code
    typedef void (*bid128_fn)(BID_UINT128 *res, BID_UINT128 *x, BID_UINT128 *y);
    typedef void (*bid64_fn) (BID_UINT64  *res, BID_UINT64  *x, BID_UINT64  *y);
    typedef void (*bid32_fn) (BID_UINT32  *res, BID_UINT32  *x, BID_UINT32  *y);
    static result evaluate(bid128_fn      op128,
                           bid64_fn       op64,
                           bid32_fn       op32,
                           integer_fn     integer_ok,
                           non_numeric_fn non_numeric);

    template <typename Op> static result evaluate();
    // ------------------------------------------------------------------------
    //   The actual evaluation for all binary operators
    // ------------------------------------------------------------------------
};


template <typename Alg>
inline bool non_numeric(gcobj &UNUSED      x,
                        gcobj &UNUSED      y,
                        object::id &UNUSED xt,
                        object::id &UNUSED yt)
// ----------------------------------------------------------------------------
//   Return true if we can process non-numeric objects of the type
// ----------------------------------------------------------------------------
{
    return false;
}


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
    static constexpr auto bid32_op = bid32_##derived;                   \
    static constexpr auto bid64_op = bid64_##derived;                   \
    static constexpr auto bid128_op = bid128_##derived;                 \
                                                                        \
    OBJECT_HANDLER(derived)                                             \
    {                                                                   \
        if (op == EVAL || op == EXEC)                                   \
        {                                                               \
            RT.command(fancy(ID_##derived));                            \
            if (!Arity || RT.stack(Arity-1))                            \
                return derived::evaluate();                             \
            return ERROR;                                               \
        }                                                               \
        return DELEGATE(arithmetic);                                    \
    }                                                                   \
    static result evaluate()                                            \
    {                                                                   \
        return arithmetic::evaluate<derived>();                         \
    }                                                                   \
}


ARITHMETIC_DECLARE(add, 2, 5);
ARITHMETIC_DECLARE(sub, 2, 5);
ARITHMETIC_DECLARE(mul, 2, 7);
ARITHMETIC_DECLARE(div, 2, 7);
ARITHMETIC_DECLARE(mod, 2, 7);
ARITHMETIC_DECLARE(rem, 2, 7);
ARITHMETIC_DECLARE(pow, 2, 9);
ARITHMETIC_DECLARE(hypot, 2, 15);

void bid64_hypot(BID_UINT64 *pres, BID_UINT64 *px, BID_UINT64 *py);
void bid32_hypot(BID_UINT32 *pres, BID_UINT32 *px, BID_UINT32 *py);
void bid64_pow(BID_UINT64 *pres, BID_UINT64 *px, BID_UINT64 *py);
void bid32_pow(BID_UINT32 *pres, BID_UINT32 *px, BID_UINT32 *py);


#endif // ARITHMETIC
