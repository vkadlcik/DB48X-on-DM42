#ifndef EQUATION_H
#define EQUATION_H
// ****************************************************************************
//  equation.h                                                    DB48X project
// ****************************************************************************
//
//   File Description:
//
//     Implementation of algebraic equations
//
//     Equations are simply programs that are rendered and parsed specially
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


#include "program.h"
#include "symbol.h"

GCP(equation);

struct equation : program
// ----------------------------------------------------------------------------
//   An equation is a program with ' and ' as delimiters
// ----------------------------------------------------------------------------
//   We also need special parsing and rendering of algebraic objects
{
    equation(gcbytes bytes, size_t len, id type = ID_equation)
        : program(bytes, len, type) {}
    static size_t required_memory(id i, gcbytes UNUSED bytes, size_t len)
    {
        return program::required_memory(i, bytes, len);
    }

    // Building an equation from an object
    equation(algebraic_r arg, id type = ID_equation);
    static size_t required_memory(id i, algebraic_r arg);

    // Building equations from one or two arguments
    equation(id op, algebraic_r arg, id type = ID_equation);
    static size_t required_memory(id i, id op, algebraic_r arg);
    equation(id op, algebraic_r x, algebraic_r y, id type = ID_equation);
    static size_t required_memory(id i, id op, algebraic_r x, algebraic_r y);

    object_p quoted(id type) const;
    static size_t size_in_equation(object_p obj);

    static equation_p make(algebraic_r x,
                           id type = ID_equation)
    {
        if (!x.Safe())
            return nullptr;
        return rt.make<equation>(type, x);
    }

    static equation_p make(id op, algebraic_r x,
                           id type = ID_equation)
    {
        if (!x.Safe())
            return nullptr;
        return rt.make<equation>(type, op, x);
    }

    static equation_p make(id op, algebraic_r x, algebraic_r y,
                           id type = ID_equation)
    {
        if (!x.Safe() || !y.Safe())
            return nullptr;
        return rt.make<equation>(type, op, x, y);
    }

    equation_p rewrite(equation_r from, equation_r to) const;
    static equation_p rewrite(equation_r eq, equation_r from, equation_r to)
    {
        return eq->rewrite(from, to);
    }

protected:
    static symbol_g render(uint depth, int &precedence, bool edit);
    static symbol_g parentheses(symbol_g what);
    static symbol_g space(symbol_g what);

public:
    OBJECT_DECL(equation);
    PARSE_DECL(equation);
    RENDER_DECL(equation);
};


COMMAND_DECLARE(Rewrite);

#endif // EQUATION_H
