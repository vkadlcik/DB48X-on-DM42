#ifndef RENDERER_H
#define RENDERER_H
// ****************************************************************************
//  renderer.h                                                    DB48X project
// ****************************************************************************
//
//   File Description:
//
//
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
#include "runtime.h"

struct renderer
// ----------------------------------------------------------------------------
//  Arguments to the RENDER command
// ----------------------------------------------------------------------------
{
    renderer(object_p what, char *target, size_t length,
             bool editing = false, bool equation = false)
        : what(what), target(target), length(length),
          indent(), editing(editing), equation(equation) {}

    gcobj       what;           // Object being rendered
    gcmstring   target;         // Buffer where we render the object
    size_t      length;         // Available space
    uint        indent;         // Amount of indent
    bool        editing  : 1;   // For editing
    bool        equation : 1;   // As equation
};


#endif // RENDERER_H
