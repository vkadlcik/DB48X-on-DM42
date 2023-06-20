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


COMMAND(DupN)
// ----------------------------------------------------------------------------
//   Implement the RPL "dupN" command, duplicate two elements at top of stack
// ----------------------------------------------------------------------------
{
    uint32_t depth = 0;
    if (stack(&depth))
    {
        if (RT.pop())
        {
            for (uint i = 0; i < depth; i++)
                if (object_p obj = RT.stack(depth-1))
                    if (!RT.push(obj))
                        return ERROR;
            return OK;
        }
    }
    return ERROR;
}


COMMAND(Drop)
// ----------------------------------------------------------------------------
//   Implement the RPL "drop" command, remove top of stack
// ----------------------------------------------------------------------------
{
    if (RT.drop())
        return OK;
    return ERROR;
}


COMMAND(Drop2)
// ----------------------------------------------------------------------------
//   Implement the Drop2 command, remove two elements from the stack
// ----------------------------------------------------------------------------
{
    if (RT.drop(2))
        return OK;
    return ERROR;
}


COMMAND(DropN)
// ----------------------------------------------------------------------------
//   Implement the DropN command, remove N elements from the stack
// ----------------------------------------------------------------------------
{
    uint32_t depth = 0;
    if (stack(&depth))
        if (RT.pop())
            if (RT.drop(depth))
                return OK;
    return ERROR;
}


COMMAND(Over)
// ----------------------------------------------------------------------------
//   Implement the Over command, getting object from level 2
// ----------------------------------------------------------------------------
{
    if (object_p o = RT.stack(1))
        if (RT.push(o))
            return OK;
    return ERROR;
}


COMMAND(Pick)
// ----------------------------------------------------------------------------
//   Implement the Pick command, getting from level N
// ----------------------------------------------------------------------------
{
    uint32_t depth = 0;
    if (stack(&depth))
        if (object_p obj = RT.stack(depth))
            if (RT.top(obj))
                return OK;
    return ERROR;
}


COMMAND(Roll)
// ----------------------------------------------------------------------------
//   Implement the Roll command, moving objects from high stack level down
// ----------------------------------------------------------------------------
{
    uint32_t depth = 0;
    if (stack(&depth))
        if (RT.pop())
            if (RT.roll(depth))
                return OK;
    return ERROR;
}


COMMAND(RollD)
// ----------------------------------------------------------------------------
//   Implement the RollD command, moving objects from first level up
// ----------------------------------------------------------------------------
{
    uint32_t depth = 0;
    if (stack(&depth))
        if (RT.pop())
            if (RT.rolld(depth))
                return OK;
    return ERROR;
}


COMMAND(Rot)
// ----------------------------------------------------------------------------
//   Implement the RollD command, moving objects from first level up
// ----------------------------------------------------------------------------
{
    if (RT.roll(3))
        return OK;
    return ERROR;
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
