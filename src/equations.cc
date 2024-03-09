// ****************************************************************************
//  equations.cc                                                  DB48X project
// ****************************************************************************
//
//   File Description:
//
//      Implementation of the equations library
//
//
//
//
//
//
//
//
// ****************************************************************************
//   (C) 2024 Christophe de Dinechin <christophe@dinechin.org>
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

#include "equations.h"

#include "renderer.h"

RECORDER(equations,         16, "Equation objects");
RECORDER(equations_error,   16, "Error on equation objects");


// ============================================================================
//
//   Equation definitions
//
// ============================================================================

static const cstring basic_equations[] =
// ----------------------------------------------------------------------------
//   List of basic equations
// ----------------------------------------------------------------------------
//   clang-format off
{

    // ------------------------------------------------------------------------
    //   Physics
    // ------------------------------------------------------------------------

    "Phys",     nullptr,

    "RelativityMassEnergy",             "'E=m*c^2'",
    "PerfectGas",                       "'P*V=n*R*T'"

};
//   clang-format on


static void invalid_equation_error()
// ----------------------------------------------------------------------------
//    Return the error message for invalid equations
// ----------------------------------------------------------------------------
{
    rt.invalid_equation_error();
}


const equation::config equation::equations =
// ----------------------------------------------------------------------------
//  Define the configuration for the equations
// ----------------------------------------------------------------------------
{
    .prefix         = L'Ⓔ',
    .type           = ID_equation,
    .first_menu     = ID_EquationsMenu00,
    .last_menu      = ID_EquationsMenu99,
    .name           = ID_EquationName,
    .value          = ID_EquationValue,
    .file           = "config/equations.csv",
    .builtins       = basic_equations,
    .nbuiltins      = sizeof(basic_equations) / sizeof(*basic_equations),
    .error          = invalid_equation_error
};



// ============================================================================
//
//   Menu implementation
//
// ============================================================================

PARSE_BODY(equation)
// ----------------------------------------------------------------------------
//    Skip, the actual parsing is done in the symbol parser
// ----------------------------------------------------------------------------
{
    return do_parsing(equations, p);
}


EVAL_BODY(equation)
// ----------------------------------------------------------------------------
//   Equations always evaluate to their value
// ----------------------------------------------------------------------------
{
    algebraic_g value = o->value();
    return rt.push(+value) ? OK : ERROR;
}


RENDER_BODY(equation)
// ----------------------------------------------------------------------------
//   Render the equation into the given buffer
// ----------------------------------------------------------------------------
{
    equation_g eq = o;
    do_rendering(equations, o, r);
    if (!r.editing())
    {
        if (object_g obj = eq->value())
        {
            r.put(':');
            obj->render(r);
        }
    }
    return r.size();
}


HELP_BODY(equation)
// ----------------------------------------------------------------------------
//   Help topic for equations
// ----------------------------------------------------------------------------
{
    return utf8("Equations Library");
}


MENU_BODY(equation_menu)
// ----------------------------------------------------------------------------
//   Build a equations menu
// ----------------------------------------------------------------------------
{
    return o->do_submenu(equation::equations, mi);
}


MENU_BODY(EquationsMenu)
// ----------------------------------------------------------------------------
//   The equations menu is dynamically populated
// ----------------------------------------------------------------------------
{
    return equation::do_collection_menu(equation::equations, mi);
}


utf8 equation_menu::name(id type, size_t &len)
// ----------------------------------------------------------------------------
//   Return the name for a menu entry
// ----------------------------------------------------------------------------
{
    return do_name(equation::equations, type, len);
}


COMMAND_BODY(EquationName)
// ----------------------------------------------------------------------------
//   Put the name of a equation on the stack
// ----------------------------------------------------------------------------
{
    int key = ui.evaluating;
    if (constant_p cst = equation::do_key(equation::equations, key))
        if (equation_p eq = cst->as<equation>())
            if (rt.push(eq))
                return OK;
    if (!rt.error())
        rt.type_error();
    return ERROR;
}


INSERT_BODY(EquationName)
// ----------------------------------------------------------------------------
//   Put the name of a equation in the editor
// ----------------------------------------------------------------------------
{
    int key = ui.evaluating;
    return ui.insert_softkey(key, " Ⓔ", " ", false);
}


COMMAND_BODY(EquationValue)
// ----------------------------------------------------------------------------
//   Put the value of a equation on the stack
// ----------------------------------------------------------------------------
{
    int key = ui.evaluating;
    if (constant_p cst = equation::do_key(equation::equations, key))
        if (equation_p eq = cst->as<equation>())
            if (object_p value = eq->value())
                if (rt.push(value))
                    return OK;
    if (!rt.error())
        rt.type_error();
    return ERROR;
}


INSERT_BODY(EquationValue)
// ----------------------------------------------------------------------------
//   Insert the value of a equation
// ----------------------------------------------------------------------------
{
    int key = ui.evaluating;
    if (object_p cstobj = equation::do_key(equation::equations, key))
        if (equation_p eq = cstobj->as<equation>())
            if (object_p value = eq->value())
                return ui.insert_object(value, " ", " ");
    return ERROR;
}
