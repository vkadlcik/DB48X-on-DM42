#ifndef STACK_CMDS_H
#define STACK_CMDS_H
// ****************************************************************************
//  stack-cmds.h                                                  DB48X project
// ****************************************************************************
//
//   File Description:
//
//     RPL Stack commands
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

#include "command.h"
#include "list.h"

#include <dmcp.h>

COMMAND(Dup)
// ----------------------------------------------------------------------------
//   Implement the RPL "dup" command, duplicate top of stack
// ----------------------------------------------------------------------------
{
    if (gcobj top = RT.top())
        if (RT.push(top))
            return OK;
    return ERROR;
}


COMMAND(Dup2)
// ----------------------------------------------------------------------------
//   Implement the RPL "dup2" command, duplicate two elements at top of stack
// ----------------------------------------------------------------------------
{
    if (gcobj y = RT.stack(1))
        if (gcobj x = RT.stack(0))
            if (RT.push(y))
                if (RT.push(x))
                    return OK;
    return ERROR;
}


COMMAND(Drop)
// ----------------------------------------------------------------------------
//   Implement the RPL "drop" command, remove top of stack
// ----------------------------------------------------------------------------
{
    RT.pop();
    return RT.error() ? ERROR : OK;
}

COMMAND(Swap)
// ----------------------------------------------------------------------------
//   Implement the RPL "swap" command, swap the two top elements
// ----------------------------------------------------------------------------
{
    object_p x = RT.stack(0);
    object_p y = RT.stack(1);
    if (x && y)
    {
        RT.stack(0, y);
        RT.stack(1, x);
        return OK;
    }
    return ERROR;
}

COMMAND(Depth)
// ----------------------------------------------------------------------------
//   Return the depth of the stack
// ----------------------------------------------------------------------------
{
    uint ticks = RT.depth();
    if (integer_p ti = RT.make<integer>(ID_integer, ticks))
        if (RT.push(ti))
            return OK;
    return ERROR;
}

#endif // STACK_CMDS_H
