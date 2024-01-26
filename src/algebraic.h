#ifndef ALGEBRAIC_H
#define ALGEBRAIC_H
// ****************************************************************************
//  algebraic.h                                                  DB48X project
// ****************************************************************************
//
//   File Description:
//
//     RPL algebraic objects
//
//     RPL algebraics are objects that can be placed in algebraic expression
//     (between quotes). They are defined by a precedence and an arity.
//     Items with higher precedence are grouped, a.g. * has higher than +
//     Arity is the number of arguments the command takes
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
//
//     Unlike traditional RPL, algebraics are case-insensitive, i.e. you can
//     use either "DUP" or "dup". There is a setting to display them as upper
//     or lowercase. The reason is that on the DM42, lowercases look good.
//
//     Additionally, many algebraics also have a long form. There is also an
//     option to prefer displaying as long form. This does not impact encoding,
//     and when typing programs, you can always use the short form

#include "command.h"

GCP(algebraic);
GCP(program);
GCP(decimal);

struct algebraic : command
// ----------------------------------------------------------------------------
//   Shared logic for all algebraics
// ----------------------------------------------------------------------------
{
    algebraic(id i): command(i) {}

    // Promotion of integer / fractions / hwfp to decimal
    static bool decimal_promotion(algebraic_g &x);

    // Promotion of integer / fractions / decimal to hwfp
    static bool hwfp_promotion(algebraic_g &x);

    // Promotion of integer, real or fraction to complex
    static bool complex_promotion(algebraic_g &x, id type = ID_rectangular);

    // Promotion of integer to bignum
    static id   bignum_promotion(algebraic_g &x);

    // Promotion to based numbers
    static id   based_promotion(algebraic_g &x);

    // Convert to a fraction
    static bool decimal_to_fraction(algebraic_g &x);

    // Convert to decimal number
    static bool to_decimal(algebraic_g &x, bool weak = false);

    // Convert to decimal if this is a big value
    static bool to_decimal_if_big(algebraic_g &x)
    {
        return !x->is_big() || to_decimal(x);
    }

    // Numerical value of pi
    static algebraic_g pi();

    // Evaluate an object as a function
    static algebraic_p evaluate_function(program_r eq, algebraic_r x);
    algebraic_p evaluate_function(program_r eq)
    {
        algebraic_g x = this;
        return evaluate_function(eq, x);
    }

    // Evaluate an algebraic as an algebraic
    algebraic_p evaluate() const;

    // Function pointers used by generic evaluation code
    typedef decimal_p (*decimal_fn)(decimal_r x);

    template<typename value>
    static algebraic_p as_hwfp(value x);
    // -------------------------------------------------------------------------
    //   Return a hardware floating-point value if possible
    // -------------------------------------------------------------------------

    INSERT_DECL(algebraic);
};

typedef algebraic_p (*algebraic_fn)(algebraic_r x);
typedef algebraic_p (*arithmetic_fn)(algebraic_r x, algebraic_r y);


#define CONSTANT_DECL(derived)                                          \
struct derived : algebraic                                              \
/* ----------------------------------------------------------------- */ \
/* Define an algenraic constant                                      */ \
/* ----------------------------------------------------------------- */ \
{                                                                       \
    derived(id type = ID_##derived): algebraic(type) {}                 \
                                                                        \
    OBJECT_DECL(derived);                                               \
    EVAL_DECL(derived);                                                 \
    PREC_DECL(NONE);                                                    \
}


CONSTANT_DECL(ImaginaryUnit);
CONSTANT_DECL(pi);

#endif // ALGEBRAIC_H
