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
#include "bignum.h"
#include "decimal-32.h"
#include "decimal-64.h"
#include "decimal128.h"
#include "integer.h"
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
    typedef bool (*bignum_fn)(bignum_g &x, bignum_g &y);
    typedef bool (*non_numeric_fn)(gcobj &x, gcobj &y, id &xt, id &yt);

    // Function pointers used by generic evaluation code
    typedef void (*bid128_fn)(BID_UINT128 *res, BID_UINT128 *x, BID_UINT128 *y);
    typedef void (*bid64_fn) (BID_UINT64  *res, BID_UINT64  *x, BID_UINT64  *y);
    typedef void (*bid32_fn) (BID_UINT32  *res, BID_UINT32  *x, BID_UINT32  *y);
    static result evaluate(id             op,
                           bid128_fn      op128,
                           bid64_fn       op64,
                           bid32_fn       op32,
                           integer_fn     integer_ok,
                           bignum_fn      bignum_ok,
                           non_numeric_fn non_numeric);

    template <typename Op> static result evaluate();
    // ------------------------------------------------------------------------
    //   The actual evaluation for all binary operators
    // ------------------------------------------------------------------------

    template <typename Op>
    static bool non_numeric(gcobj &UNUSED x,
                            gcobj &UNUSED y,
                            id &   UNUSED xt,
                            id &   UNUSED yt)
    // ------------------------------------------------------------------------
    //   Return true if we can process non-numeric objects of the type
    // ------------------------------------------------------------------------
    {
        return false;
    }
};


#define ARITHMETIC_DECLARE(derived, Precedence)                         \
/* ----------------------------------------------------------------- */ \
/*  Macro to define an arithmetic command                            */ \
/* ----------------------------------------------------------------- */ \
struct derived : arithmetic                                             \
{                                                                       \
    derived(id i = ID_##derived) : arithmetic(i) {}                     \
                                                                        \
    static bool integer_ok(id &xt, id &yt, ularge &xv, ularge &yv);     \
    static bool bignum_ok(bignum_g &x, bignum_g &y);                    \
    static constexpr auto bid32_op = bid32_##derived;                   \
    static constexpr auto bid64_op = bid64_##derived;                   \
    static constexpr auto bid128_op = bid128_##derived;                 \
                                                                        \
    OBJECT_HANDLER(derived)                                             \
    {                                                                   \
        switch(op)                                                      \
        {                                                               \
        case EVAL:                                                      \
        case EXEC:                                                      \
            RT.command(fancy(ID_##derived));                            \
            return derived::evaluate();                                 \
        case SIZE:                                                      \
            return byte_p(payload) - byte_p(obj);                       \
        case ARITY:                                                     \
            return 2;                                                   \
        case PRECEDENCE:                                                \
            return Precedence;                                          \
        default:                                                        \
            return DELEGATE(arithmetic);                                \
        }                                                               \
    }                                                                   \
    static result evaluate()                                            \
    {                                                                   \
        return arithmetic::evaluate<derived>();                         \
    }                                                                   \
}


ARITHMETIC_DECLARE(add, algebraic::ADDITIVE);
ARITHMETIC_DECLARE(sub, algebraic::ADDITIVE);
ARITHMETIC_DECLARE(mul, algebraic::MULTIPICATIVE);
ARITHMETIC_DECLARE(div, algebraic::MULTIPICATIVE);
ARITHMETIC_DECLARE(mod, algebraic::MULTIPICATIVE);
ARITHMETIC_DECLARE(rem, algebraic::MULTIPICATIVE);
ARITHMETIC_DECLARE(pow, algebraic::POWER);
ARITHMETIC_DECLARE(hypot, algebraic::FUNCTION);

void bid64_hypot(BID_UINT64 *pres, BID_UINT64 *px, BID_UINT64 *py);
void bid32_hypot(BID_UINT32 *pres, BID_UINT32 *px, BID_UINT32 *py);
void bid64_pow(BID_UINT64 *pres, BID_UINT64 *px, BID_UINT64 *py);
void bid32_pow(BID_UINT32 *pres, BID_UINT32 *px, BID_UINT32 *py);


#endif // ARITHMETIC
