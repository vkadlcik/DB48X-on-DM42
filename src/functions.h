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
    typedef bool (*arg_check_fn)(bid128 &x);

    static result evaluate(id op, bid128_fn op128, arg_check_fn check);
    // ------------------------------------------------------------------------
    //   The actual evaluation for all binary operators
    // ------------------------------------------------------------------------

    template <typename Op> static result evaluate();
    // ------------------------------------------------------------------------
    //   The actual evaluation for all binary operators
    // ------------------------------------------------------------------------
};


template <typename Func>
bool arg_check(bid128 &UNUSED x)
// ----------------------------------------------------------------------------
//   Check the arguments for a given function
// ----------------------------------------------------------------------------
{
    return true;
}


#define FUNCTION(derived)                                               \
/* ----------------------------------------------------------------- */ \
/*  Macro to define a standard mathematical function                 */ \
/* ----------------------------------------------------------------- */ \
struct derived : function                                               \
{                                                                       \
    derived(id i = ID_##derived) : function(i) {}                       \
                                                                        \
    static uint arity()             { return 1; }                       \
    static uint precedence()        { return 15; }                      \
                                                                        \
    static constexpr auto bid128_op = bid128_##derived;                 \
                                                                        \
    OBJECT_HANDLER(derived)                                             \
    {                                                                   \
        if (op == EVAL || op == EXEC)                                   \
        {                                                               \
            RT.command(fancy(ID_##derived));                            \
            return evaluate<derived>();                                 \
        }                                                               \
        return DELEGATE(function);                                      \
    }                                                                   \
}


FUNCTION(sqrt);
FUNCTION(cbrt);

FUNCTION(sin);
FUNCTION(cos);
FUNCTION(tan);
FUNCTION(asin);
FUNCTION(acos);
FUNCTION(atan);

FUNCTION(sinh);
FUNCTION(cosh);
FUNCTION(tanh);
FUNCTION(asinh);
FUNCTION(acosh);
FUNCTION(atanh);

FUNCTION(log1p);
FUNCTION(expm1);
FUNCTION(log);
FUNCTION(log10);
FUNCTION(log2);
FUNCTION(exp);
FUNCTION(exp10);
FUNCTION(exp2);
FUNCTION(erf);
FUNCTION(erfc);
FUNCTION(tgamma);
FUNCTION(lgamma);


struct abs;
template<> object::result function::evaluate<struct abs>();
FUNCTION(abs);



#endif // FUNCTIONS_H
