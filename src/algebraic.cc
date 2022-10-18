// ****************************************************************************
//  algebraic.cc                                                DB48X project
// ****************************************************************************
//
//   File Description:
//
//     Shared code for all algebraic commands
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

#include "integer.h"
#include "parser.h"
#include "renderer.h"
#include "runtime.h"
#include "settings.h"

#include <ctype.h>
#include <stdio.h>


RECORDER(algebraic,       16, "RPL Algebraics");
RECORDER(algebraic_error, 16, "Errors processing a algebraic");


OBJECT_HANDLER_BODY(algebraic)
// ----------------------------------------------------------------------------
//    RPL handler for algebraics
// ----------------------------------------------------------------------------
{
    record(algebraic, "Algebraic %+s on %p", object::name(op), obj);
    switch(op)
    {
    case EVAL:
        record(algebraic_error, "Invoked default algebraic handler");
        rt.error("Algebraic is not implemented");
        return ERROR;

    default:
        // Check if anyone else knows how to deal with it
        return DELEGATE(command);
    }
}


// ============================================================================
//
//   Simple operators
//
// ============================================================================

ALGEBRAIC_BODY(inv)
// ----------------------------------------------------------------------------
//   Invert is implemented as 1/x
// ----------------------------------------------------------------------------
{
    RT.push(RT.make<integer>(ID_integer, 1));
    run(ID_swap);
    run(ID_div);
    return OK;
}


ALGEBRAIC_BODY(neg)
// ----------------------------------------------------------------------------
//   Negate is implemented as 0-x
// ----------------------------------------------------------------------------
{
    RT.push(RT.make<integer>(ID_integer, 0));
    run(ID_swap);
    run(ID_sub);
    return OK;
}
