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

COMMAND(dup)
// ----------------------------------------------------------------------------
//   Implement the RPL "dup" command, duplicate top of stack
// ----------------------------------------------------------------------------
{
    if (object_p top = RT.top())
    {
        RT.push(top);
        return OK;
    }
    return ERROR;
}


COMMAND(drop)
// ----------------------------------------------------------------------------
//   Implement the RPL "drop" command, remove top of stack
// ----------------------------------------------------------------------------
{
    RT.pop();
    return RT.error() ? ERROR : OK;
}

COMMAND(swap)
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


#endif // STACK_CMDS_H
