#ifndef COMPARE_H
#define COMPARE_H
// ****************************************************************************
//  compare.h                                                     DB48X project
// ****************************************************************************
//
//   File Description:
//
//     Comparisons between objects
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

#include "arithmetic.h"

struct comparison : arithmetic
// ----------------------------------------------------------------------------
//   Shared by all comparisons
// ----------------------------------------------------------------------------
{
    comparison(id i): arithmetic(i) {}

    typedef bool (*comparison_fn)(int cmp);
    static result condition(bool &value, object_p cond);
    static result compare(int *cmp, object_p left, object_p right);
    static result compare(comparison_fn cmp);
    static result is_same(bool derefNames);

    template <typename Cmp> static result evaluate();
};


#define COMPARISON_DECLARE(derived, condition)                          \
/* ----------------------------------------------------------------- */ \
/*  Macro to define an arithmetic command                            */ \
/* ----------------------------------------------------------------- */ \
struct derived : comparison                                             \
{                                                                       \
    derived(id i = ID_##derived) : comparison(i) {}                     \
                                                                        \
    ARITY_DECL(2);                                                      \
    PREC_DECL(RELATIONAL);                                              \
                                                                        \
    EVAL_DECL(derived)                                                  \
    {                                                                   \
        rt.command(fancy(ID_##derived));                                \
        return comparison::evaluate<derived>();                         \
    }                                                                   \
    static bool make_result(int cmp)    { return condition; }           \
    static result evaluate()                                            \
    {                                                                   \
        return comparison::evaluate<derived>();                         \
    }                                                                   \
}

COMPARISON_DECLARE(TestLT, cmp <  0 );
COMPARISON_DECLARE(TestLE, cmp <= 0);
COMPARISON_DECLARE(TestEQ, cmp == 0);
COMPARISON_DECLARE(TestGT, cmp >  0);
COMPARISON_DECLARE(TestGE, cmp >= 0);
COMPARISON_DECLARE(TestNE, cmp != 0);

// A special case that requires types to be identical
struct TestSame;
template <> object::result comparison::evaluate<TestSame>();
COMPARISON_DECLARE(TestSame, cmp == 0);
struct same;
template <> object::result comparison::evaluate<same>();
COMPARISON_DECLARE(same, cmp == 0);

#endif // COMPARE_H
