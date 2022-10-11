// ****************************************************************************
//  runtime.cc                                                    DB48X project
// ****************************************************************************
//
//   File Description:
//
//     Implementation of the RPL runtime
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

#include <runtime.h>
#include <object.h>

// The one and only runtime
runtime runtime::RT;


size_t runtime::gc()
// ----------------------------------------------------------------------------
//   Recycle unused temporaries
// ----------------------------------------------------------------------------
//   Temporaries can only be referenced from the stack
//   Objects in the global area are copied there, so they need no recycling
{
    size_t  recycled = 0;
    object *next;
    for (object *obj = Temporaries; obj < Globals; obj = next)
    {
        bool found = false;
        next = skip(obj);
        for (object **s = StackBottom; s < StackTop && !found; s++)
            found = *s == obj;
        if (!found)
        {
            recycled += next - obj;
            unused(obj, next);
        }
    }
    return recycled;
}

void runtime::unused(object *obj, object *next)
// ----------------------------------------------------------------------------
//   An object is unused, need to move temporaries below it and adjust stack
// ----------------------------------------------------------------------------
{
    size_t sz = next - obj;
    object *first = Temporaries;

    // Adjust the stack pointers
    for (object **s = StackBottom; s < StackTop; s++)
        if (*s >= first && *s < next)
            *s += sz;

    // Adjust the protected pointers
    for (object *p = GCSafe; p; p = p->next)
        if (p->safe >= first && p->safe < next)
            p->safe += sz;

    // Adjust the temporaries pointer
    Temporaries += sz;
}


size_t runtime::size(object *obj)
// ----------------------------------------------------------------------------
//   Delegate the size to the object
// ----------------------------------------------------------------------------
{
    return obj->size();
}
