#ifndef UNIT_H
#  define UNIT_H
// ****************************************************************************
//  unit.h                                                        DB48X project
// ****************************************************************************
//
//   File Description:
//
//    Unit objects represent objects such as 1_km/s.
//
//    The representation is an equation where the outermost operator is _
//    which is different from the way the HP48 does it, but simplify
//    many other operations
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

#include "equation.h"

GCP(unit);

struct unit : equation
// ----------------------------------------------------------------------------
//   A unit object is mostly like an equation, except for parsing
// ----------------------------------------------------------------------------
{
    unit(id type, algebraic_r value, algebraic_r uexpr):
        equation(type, ID_mkunit, value, uexpr) {}
    static size_t required_memory(id i, algebraic_r v, algebraic_r u)
    {
        return equation::required_memory(i, ID_mkunit, v, u);
    }

    static unit_p make(algebraic_r v, algebraic_r u, id ty = ID_unit)
    {
        if (!v.Safe() || !u.Safe())
            return nullptr;
        return rt.make<unit>(ty, u, v);
    }

public:
    OBJECT_DECL(unit);
    PARSE_DECL(unit);
    RENDER_DECL(unit);
    HELP_DECL(unit);
};


#endif // UNIT_H
