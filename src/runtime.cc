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

#include "runtime.h"
#include "object.h"
#include <cstring>

// The one and only runtime
runtime runtime::RT(nullptr, 0);


size_t runtime::gc()
// ----------------------------------------------------------------------------
//   Recycle unused temporaries
// ----------------------------------------------------------------------------
//   Temporaries can only be referenced from the stack
//   Objects in the global area are copied there, so they need no recycling
{
    size_t  recycled = 0;
    object *last = Temporaries + Editing;
    object *next;
    for (object *obj = (object *) Globals; obj < last; obj = next)
    {
        bool found = false;
        next = skip(obj);
        for (object **s = StackTop; s < StackBottom && !found; s++)
            found = *s == obj;
        if (!found)
            for (gcptr *p = GCSafe; p && !found; p = p->next)
                found = p->safe == obj;
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
    object *last = Temporaries + Editing;

    // Adjust the stack pointers
    for (object **s = StackTop; s < StackBottom; s++)
        if (*s >= obj && *s < last)
            *s += sz;

    // Adjust the protected pointers
    for (gcptr *p = GCSafe; p; p = p->next)
        if (p->safe >= obj && p->safe < last)
            p->safe += sz;

    // Move the other temporaries down
    memmove((byte *) obj, (byte *) next, (byte *) last - (byte *) next);

    // Adjust the temporaries pointer
    Temporaries -= sz;
}


size_t runtime::size(object *obj)
// ----------------------------------------------------------------------------
//   Delegate the size to the object
// ----------------------------------------------------------------------------
{
    return obj->size(*this);
}


char *runtime::close_editor()
// ----------------------------------------------------------------------------
//   Close the editor and encapsulate its content into a string
// ----------------------------------------------------------------------------
//   This will move the editor below the temporaries, encapsulated as
//   a string. After that, it is safe to allocate temporaries without
//   overwriting the editor
{
    // Compute the extra size we need for a string header
    size_t hdrsize = leb128size(object::ID_string) + leb128size(Editing + 1);

    // Move the editor data above that header
    char *ed = (char *) Temporaries;
    char *str = ed + hdrsize;
    memmove(str, ed, Editing);

    // Null-terminate that string for safe use by C code
    str[Editing] = 0;

    // Write the string header
    ed = leb128(ed, object::ID_string);
    ed = leb128(ed, Editing + 1);

    // Move Temporaries past that newly created string
    Temporaries = (object *) str + Editing + 1;

    // We are no longer editing
    Editing = 0;

    // Return a pointer to a valid C string safely wrapped in a RPL string
    return str;
}
