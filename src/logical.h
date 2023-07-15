#ifndef LOGICAL_H
#define LOGICAL_H
// ****************************************************************************
//  logical.h                                                     DB48X project
// ****************************************************************************
//
//   File Description:
//
//     Define logical operations
//
//     Logical operations can operate bitwise on based integers, or
//     as truth values on integers, real numbers and True/False
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

#include "arithmetic.h"
#include "bignum.h"


struct logical : arithmetic
// ----------------------------------------------------------------------------
//   Shared by all logical operations
// ----------------------------------------------------------------------------
{
    logical(id i): arithmetic(i) {}

    static int as_truth(object_p obj);
    typedef ularge (*binary_fn)(ularge x, ularge y);
    typedef bignum_g (*big_binary_fn)(bignum_g x, bignum_g y);
    static result evaluate(binary_fn opn, big_binary_fn opb);
    typedef ularge (*unary_fn)(ularge x);
    typedef bignum_g (*big_unary_fn)(bignum_g x);
    static result evaluate(unary_fn opn, big_unary_fn opb);

    template <typename Cmp> static result evaluate()
    // ------------------------------------------------------------------------
    //   The actual evaluation for all binary operators
    // ------------------------------------------------------------------------
    {
        return evaluate(Cmp::native, Cmp::bignum);
    }
};


#define BINARY_LOGICAL(derived, code)                                   \
/* ----------------------------------------------------------------- */ \
/*  Macro to define an arithmetic command                            */ \
/* ----------------------------------------------------------------- */ \
struct derived : logical                                                \
{                                                                       \
    derived(id i = ID_##derived) : logical(i) {}                        \
                                                                        \
    OBJECT_DECL(derived);                                               \
    ARITY_DECL(2);                                                      \
    PREC_DECL(LOGICAL);                                                 \
    EVAL_DECL(derived)                                                  \
    {                                                                   \
        rt.command(fancy(ID_##derived));                                \
        return evaluate<derived>();                                     \
    }                                                                   \
                                                                        \
    static ularge    native(ularge Y, ularge X)        { return code; } \
    static bignum_g  bignum(bignum_g Y, bignum_g X)    { return code; } \
}


#define UNARY_LOGICAL(derived, code)                                    \
/* ----------------------------------------------------------------- */ \
/*  Macro to define an arithmetic command                            */ \
/* ----------------------------------------------------------------- */ \
struct derived : logical                                                \
{                                                                       \
    derived(id i = ID_##derived) : logical(i) {}                        \
                                                                        \
    OBJECT_DECL(derived);                                               \
    ARITY_DECL(1);                                                      \
    PREC_DECL(NONE);                                                    \
    EVAL_DECL(derived)                                                  \
    {                                                                   \
        rt.command(fancy(ID_##derived));                                \
        return evaluate<derived>();                                     \
    }                                                                   \
    static ularge    native(ularge X)           { return code; }        \
    static bignum_g  bignum(bignum_g X)         { return code; }        \
}


BINARY_LOGICAL(And,      Y &  X);
BINARY_LOGICAL(Or,       Y |  X);
BINARY_LOGICAL(Xor,      Y ^  X);
BINARY_LOGICAL(NAnd,   ~a(Y &  X));
BINARY_LOGICAL(NOr,    ~a(Y |  X));
BINARY_LOGICAL(Implies, ~Y |  X);
BINARY_LOGICAL(Equiv,  ~a(Y ^  X));
BINARY_LOGICAL(Excludes, Y & ~X); // If Y then X=0
UNARY_LOGICAL (Not,          ~X);

#endif // LOGICAL_H
