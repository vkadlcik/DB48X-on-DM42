#ifndef PRECEDENCE_H
#define PRECEDENCE_H
// ****************************************************************************
//  precedence.h                                                 DB48X project
// ****************************************************************************
//
//   File Description:
//
//     Define operator precedence
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

enum precedence
// ----------------------------------------------------------------------------
// Precedence for the various operators
// ----------------------------------------------------------------------------
{
    NONE                = 0,    // No precedence
    LOWEST              = 1,    // Lowest precedence (when parsing parentheses)

    LOGICAL             = 2,    // and, or, xor
    RELATIONAL          = 4,    // <, >, =, etc
    ADDITIVE            = 6,    // +, -
    MULTIPLICATIVE      = 8,    // *, /
    POWER               = 10,   // ^

    UNKNOWN             = 20,   // Unknown operator
    PARENTHESES         = 30,   // Parentheses
    FUNCTION            = 40,   // Functions, e.g. f(x)
    FUNCTION_POWER      = 50,   // X²
    SYMBOL              = 60,   // Names
};

#endif // PRECEDENCE_H
