#ifndef FUNCTIONS_H
#define FUNCTIONS_H
// ****************************************************************************
//  functions.h                                                  DB48X project
// ****************************************************************************
//
//   File Description:
//
//     Standard mathematical functions
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

struct function : algebraic
// ----------------------------------------------------------------------------
//   Shared logic for all standard functions
// ----------------------------------------------------------------------------
{
    function(id i): algebraic(i) {}


protected:
    static result evaluate(id op, bid128_fn op128);
    // ------------------------------------------------------------------------
    //   The actual evaluation for all binary operators
    // ------------------------------------------------------------------------

    template <typename Op> static result evaluate();
    // ------------------------------------------------------------------------
    //   The actual evaluation for all binary operators
    // ------------------------------------------------------------------------
};


#define STANDARD_FUNCTION(derived)                                      \
/* ----------------------------------------------------------------- */ \
/*  Macro to define a standard mathematical function (in library)    */ \
/* ----------------------------------------------------------------- */ \
struct derived : function                                               \
{                                                                       \
    derived(id i = ID_##derived) : function(i) {}                       \
                                                                        \
    static constexpr auto bid128_op = bid128_##derived;                 \
                                                                        \
    OBJECT_HANDLER(derived)                                             \
    {                                                                   \
        switch(op)                                                      \
        {                                                               \
        case EVAL:                                                      \
        case EXEC:                                                      \
            RT.command(fancy(ID_##derived));                            \
            return evaluate<derived>();                                 \
        case SIZE:                                                      \
            return byte_p(payload) - byte_p(obj);                       \
        case ARITY:                                                     \
            return 1;                                                   \
        case PRECEDENCE:                                                \
            return algebraic::FUNCTION;                                 \
        default:                                                        \
            return DELEGATE(function);                                  \
        }                                                               \
    }                                                                   \
}

STANDARD_FUNCTION(sqrt);
STANDARD_FUNCTION(cbrt);

STANDARD_FUNCTION(sin);
STANDARD_FUNCTION(cos);
STANDARD_FUNCTION(tan);
STANDARD_FUNCTION(asin);
STANDARD_FUNCTION(acos);
STANDARD_FUNCTION(atan);

STANDARD_FUNCTION(sinh);
STANDARD_FUNCTION(cosh);
STANDARD_FUNCTION(tanh);
STANDARD_FUNCTION(asinh);
STANDARD_FUNCTION(acosh);
STANDARD_FUNCTION(atanh);

STANDARD_FUNCTION(log1p);
STANDARD_FUNCTION(expm1);
STANDARD_FUNCTION(log);
STANDARD_FUNCTION(log10);
STANDARD_FUNCTION(log2);
STANDARD_FUNCTION(exp);
STANDARD_FUNCTION(exp10);
STANDARD_FUNCTION(exp2);
STANDARD_FUNCTION(erf);
STANDARD_FUNCTION(erfc);
STANDARD_FUNCTION(tgamma);
STANDARD_FUNCTION(lgamma);


#define FUNCTION(derived)                                               \
struct derived : function                                               \
/* ----------------------------------------------------------------- */ \
/*  Macro to define a mathematical function not from the library     */ \
/* ----------------------------------------------------------------- */ \
{                                                                       \
    derived(id i = ID_##derived) : function(i) {}                       \
                                                                        \
    static result evaluate();                                           \
                                                                        \
    OBJECT_HANDLER(derived)                                             \
    {                                                                   \
        switch(op)                                                      \
        {                                                               \
        case EVAL:                                                      \
        case EXEC:                                                      \
            RT.command(fancy(ID_##derived));                            \
            return evaluate();                                          \
        case SIZE:                                                      \
            return byte_p(payload) - byte_p(obj);                       \
        case ARITY:                                                     \
            return 1;                                                   \
        case PRECEDENCE:                                                \
            return algebraic::FUNCTION;                                 \
        default:                                                        \
            return DELEGATE(function);                                  \
        }                                                               \
    }                                                                   \
};                                                                      \
template<> object::result function::evaluate<struct derived>()

#define FUNCTION_BODY(derived)                  \
object::result derived::evaluate()

FUNCTION(abs);
FUNCTION(norm);
FUNCTION(inv);
FUNCTION(neg);
FUNCTION(sq);
FUNCTION(cubed);

#endif // FUNCTIONS_H
