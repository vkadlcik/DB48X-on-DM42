#ifndef STACK_H
#define STACK_H
// ****************************************************************************
//  stack.h                                                       DB48X project
// ****************************************************************************
//
//   File Description:
//
//     Rendering of objects on the stack
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

#include "object.h"

struct runtime;

struct stack
// ----------------------------------------------------------------------------
//   Rendering of the stack
// ----------------------------------------------------------------------------
{
    stack();

    void draw_stack();

protected:
    static runtime &RT;
};

extern stack Stack;

#endif // STACK_H
