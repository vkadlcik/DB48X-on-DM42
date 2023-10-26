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
#include "complex.h"
#include "decimal-32.h"
#include "decimal-64.h"
#include "decimal128.h"
#include "fraction.h"
#include "runtime.h"


struct arithmetic : algebraic
// ----------------------------------------------------------------------------
//   Shared logic for all arithmetic operations
// ----------------------------------------------------------------------------
{
    arithmetic(id i): algebraic(i) {}

    static bool real_promotion(algebraic_g &x, algebraic_g &y);
    // -------------------------------------------------------------------------
    //   Promote x or y to the largest of both types, return true if successful
    // -------------------------------------------------------------------------

    static bool real_promotion(algebraic_g &x, object::id type)
    {
        return algebraic::real_promotion(x, type);
    }
    static id   real_promotion(algebraic_g &x)
    {
        return algebraic::real_promotion(x);
    }

    static bool complex_promotion(algebraic_g &x, id type = ID_rectangular)
    {
        return algebraic::complex_promotion(x, type);
    }
    static bool complex_promotion(algebraic_g &x, algebraic_g &y);


    static fraction_p fraction_promotion(algebraic_g &x);

    // We do not insert parentheses for algebraic values
    INSERT_DECL(arithmetic);

protected:
    typedef bool (*integer_fn)(id &xt, id &yt, ularge &xv, ularge &yv);
    typedef bool (*bignum_fn)(bignum_g &x, bignum_g &y);
    typedef bool (*fraction_fn)(fraction_g &x, fraction_g &y);
    typedef bool (*complex_fn)(complex_g &x, complex_g &y);

    // Function pointers used by generic evaluation code
    typedef void (*bid128_fn)(BID_UINT128 *res, BID_UINT128 *x, BID_UINT128 *y);
    typedef void (*bid64_fn) (BID_UINT64  *res, BID_UINT64  *x, BID_UINT64  *y);
    typedef void (*bid32_fn) (BID_UINT32  *res, BID_UINT32  *x, BID_UINT32  *y);

    // Structure holding the function pointers called by generic code
    struct ops
    {
        bid128_fn      op128;
        bid64_fn       op64;
        bid32_fn       op32;
        integer_fn     integer_ok;
        bignum_fn      bignum_ok;
        fraction_fn    fraction_ok;
        complex_fn     complex_ok;
        arithmetic_fn non_numeric;
    };
    typedef const ops &ops_t;
    template <typename Op> static ops_t Ops();

    static result evaluate(id op, ops_t ops);

    template <typename Op> static result evaluate();
    // ------------------------------------------------------------------------
    //   Stack-based evaluation for all binary operators
    // ------------------------------------------------------------------------

    static algebraic_p evaluate(id op, algebraic_r x, algebraic_r y, ops_t ops);

    template <typename Op>
    static algebraic_p evaluate(algebraic_r x, algebraic_r y);
    // --------------------------------------------------Z----------------------
    //   C++ wrapper for the operation
    // ------------------------------------------------------------------------

    template <typename Op>
    static algebraic_p non_numeric(algebraic_r UNUSED x, algebraic_r UNUSED y)
    // ------------------------------------------------------------------------
    //   Return true if we can process non-numeric objects of the type
    // ------------------------------------------------------------------------
    {
        return nullptr;
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
    static bool fraction_ok(fraction_g &x, fraction_g &y);              \
    static bool complex_ok(complex_g &x, complex_g &y);                 \
    static constexpr auto bid32_op = bid32_##derived;                   \
    static constexpr auto bid64_op = bid64_##derived;                   \
    static constexpr auto bid128_op = bid128_##derived;                 \
                                                                        \
    OBJECT_DECL(derived)                                                \
    ARITY_DECL(2);                                                      \
    PREC_DECL(Precedence);                                              \
    EVAL_DECL(derived)                                                  \
    {                                                                   \
        rt.command(fancy(ID_##derived));                                \
        return arithmetic::evaluate<derived>();                         \
    }                                                                   \
    static algebraic_g run(algebraic_r x, algebraic_r y)                \
    {                                                                   \
        return evaluate(x, y);                                          \
    }                                                                   \
    static algebraic_p evaluate(algebraic_r x, algebraic_r y)           \
    {                                                                   \
        return arithmetic::evaluate<derived>(x,y);                      \
    }                                                                   \
}


ARITHMETIC_DECLARE(add,         ADDITIVE);
ARITHMETIC_DECLARE(sub,         ADDITIVE);
ARITHMETIC_DECLARE(mul,         MULTIPLICATIVE);
ARITHMETIC_DECLARE(div,         MULTIPLICATIVE);
ARITHMETIC_DECLARE(mod,         MULTIPLICATIVE);
ARITHMETIC_DECLARE(rem,         MULTIPLICATIVE);
ARITHMETIC_DECLARE(pow,         POWER);
ARITHMETIC_DECLARE(hypot,       POWER);
ARITHMETIC_DECLARE(atan2,       POWER);

void bid64_hypot(BID_UINT64 *pres, BID_UINT64 *px, BID_UINT64 *py);
void bid32_hypot(BID_UINT32 *pres, BID_UINT32 *px, BID_UINT32 *py);
void bid64_atan2(BID_UINT64 *pres, BID_UINT64 *px, BID_UINT64 *py);
void bid32_atan2(BID_UINT32 *pres, BID_UINT32 *px, BID_UINT32 *py);
void bid64_pow(BID_UINT64 *pres, BID_UINT64 *px, BID_UINT64 *py);
void bid32_pow(BID_UINT32 *pres, BID_UINT32 *px, BID_UINT32 *py);



// ============================================================================
//
//    Arithmetic interface for C++
//
// ============================================================================

algebraic_g operator-(algebraic_r x);
algebraic_g operator+(algebraic_r x, algebraic_r y);
algebraic_g operator-(algebraic_r x, algebraic_r y);
algebraic_g operator*(algebraic_r x, algebraic_r y);
algebraic_g operator/(algebraic_r x, algebraic_r y);
algebraic_g operator%(algebraic_r x, algebraic_r y);
algebraic_g pow(algebraic_r x, algebraic_r y);

#endif // ARITHMETIC
