// ****************************************************************************
//  list.cc                                                       DB48X project
// ****************************************************************************
//
//   File Description:
//
//     Implementation of RPL lists
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

#include "list.h"

#include "parser.h"
#include "renderer.h"
#include "runtime.h"
#include "utf8.h"

#include <stdio.h>


RECORDER(list, 16, "Lists");
RECORDER(list_errors, 16, "Errors processing lists");
RECORDER(program, 16, "Program evaluation");


OBJECT_HANDLER_BODY(list)
// ----------------------------------------------------------------------------
//    Handle commands for lists
// ----------------------------------------------------------------------------
{
    switch(op)
    {
    case EVAL:
        // List values evaluate as self
        rt.push(obj);
        return 0;
    case SIZE:
        return size(obj, payload);
    case PARSE:
        return object_parser(OBJECT_PARSER_ARG(), rt);
    case RENDER:
        return obj->object_renderer(OBJECT_RENDERER_ARG(), rt);
    case HELP:
        return (intptr_t) "list";

    default:
        // Check if anyone else knows how to deal with it
        return DELEGATE(string);
    }

}


object::result list::object_parser(id type,
                                   parser  &p,
                                   runtime &rt,
                                   unicode open,
                                   unicode close)
// ----------------------------------------------------------------------------
//   Generic parser for sequences (list, program, etc)
// ----------------------------------------------------------------------------
//   We have to be careful here, because parsing sub-objects may allocate new
//   temporaries, which itself may cause garbage collection. So we need to keep
//   track of our current usage of the scratchpad to allow for recursive parsing
//   of complex objects, like { { A B { C D } } }
{
    // We have to be careful that we may have to GC to make room for list
    gcutf8 s = p.source;

    // Check if we have the opening marker
    unicode cp = utf8_codepoint(s);
    if (cp != open)
        return SKIP;
    s = utf8_next(s);

    size_t  prealloc = rt.allocated();
    gcbytes scratch = rt.scratchpad() + prealloc;
    while (size_t((byte *) s  - (byte *) p.source) < p.length)
    {
        cp = utf8_codepoint(s);
        if (cp == close)
        {
            s = utf8_next(s);
            break;
        }
        if (cp == ' ' || cp == '\n')
        {
            s = utf8_next(s);
            continue;
        }

        // Parse an object
        size_t done = (byte *) s - (byte *) p.source;
        size_t length = p.length > done ? p.length - done : 0;
        gcobj obj = object::parse(s, length);
        if (!obj)
            return ERROR;

        // Copy the parsed object to the scratch pad (may GC)
        size_t objsize = obj->size();
        byte *objcopy = rt.allocate(objsize);
        memmove(objcopy, (byte *) obj, objsize);

        // Jump past what we parsed
        s = (byte_p) s + length;
    }

    // Check that we have a matching closing character
    if (cp != close)
    {
        record(list_errors, "Missing terminator, got %u (%c) at %s",
               cp, cp, (byte *) s);
        rt.error("Missing terminator", s);
        return ERROR;
    }

    // Create the object
    size_t alloc  = rt.allocated() - prealloc;
    size_t parsed = (byte_p) s - (byte_p) p.source;
    p.end         = parsed;
    p.out         = rt.make<list>(type, scratch, alloc);

    // Restore the scratchpad
    rt.free(alloc);
    return OK;
}


intptr_t list::object_renderer(renderer &r, runtime &rt,
                               unicode open, unicode close) const
// ----------------------------------------------------------------------------
//   Render the list into the given buffer
// ----------------------------------------------------------------------------
{
    // Source objects
    byte_p p    = payload();
    size_t size = leb128<size_t>(p);
    byte_p end  = p + size;

    // Destination buffer
    size_t idx = 0;
    size_t available = r.length;
    byte * dst = r.target;

    // Write the header, e.g. "{ "
    byte buffer[4];
    size_t rendered = utf8_encode(open, buffer);
    for (size_t i = 0; i < rendered; i++)
    {
        if (idx < available)
            dst[idx] = buffer[i];
        idx++;
    }

    // Loop on all objects inside the list
    while (p < end)
    {
        // Add space separator
        dst = r.target;
        if (idx < available)
            dst[idx] = ' ';
        idx++;

        object_p obj = (object_p) p;
        size_t   objsize = obj->size();

        // Render the object in what remains
        size_t remaining = r.length > idx ? r.length - idx : 0;
        size_t rsize = obj->render((char *) dst + idx, remaining, rt);
        idx += rsize;

        // Loop on next object
        p += objsize;
    }

    // Add final space separator
    dst = r.target;
    if (idx < available)
        dst[idx] = ' ';
    idx++;

    // Add closing separator
    rendered = utf8_encode(close, buffer);
    for (size_t i = 0; i < rendered; i++)
    {
        if (idx < available)
            dst[idx] = buffer[i];
        idx++;
    }

    return idx;
}


OBJECT_PARSER_BODY(list)
// ----------------------------------------------------------------------------
//    Try to parse this as an list
// ----------------------------------------------------------------------------
{
    return object_parser(ID_list, p, rt, '{', '}');
}


OBJECT_RENDERER_BODY(list)
// ----------------------------------------------------------------------------
//   Render the list into the given list buffer
// ----------------------------------------------------------------------------
{
    return object_renderer(r, rt, '{', '}');
}



// ============================================================================
//
//    Program
//
// ============================================================================

OBJECT_HANDLER_BODY(program)
// ----------------------------------------------------------------------------
//    Handle commands for programs
// ----------------------------------------------------------------------------
{
    switch(op)
    {
    case EVAL:
        // Programs evaluate by evaluating all elements in sequence
        return obj->evaluate(rt);
    case SIZE:
        return size(obj, payload);
    case PARSE:
        return object_parser(OBJECT_PARSER_ARG(), rt);
    case RENDER:
        return obj->object_renderer(OBJECT_RENDERER_ARG(), rt);
    case HELP:
        return (intptr_t) "program";

    default:
        // Check if anyone else knows how to deal with it
        return DELEGATE(list);
    }
}


OBJECT_PARSER_BODY(program)
// ----------------------------------------------------------------------------
//    Try to parse this as a program
// ----------------------------------------------------------------------------
{
    return list::object_parser(ID_program, p, rt, L'«', L'»');
}


OBJECT_RENDERER_BODY(program)
// ----------------------------------------------------------------------------
//   Render the program into the given program buffer
// ----------------------------------------------------------------------------
{
    return list::object_renderer(r, rt, L'«', L'»');
}


object::result program::evaluate(runtime &rt) const
// ----------------------------------------------------------------------------
//   We evaluate a program by evaluating all the objects in it
// ----------------------------------------------------------------------------
{
    byte  *p       = (byte *) payload();
    size_t len     = leb128<size_t>(p);
    gcobj  first   = (object_p) p;
    result r       = OK;
    size_t objsize = 0;

    for (gcobj obj = first; len > 0; obj += objsize, len -= objsize)
    {
        objsize = obj->size();
        record(program, "Evaluating %+s at %p, size %u, %u remaining\n",
               obj->fancy(), (object_p) obj, objsize, len);
        if (interrupted() || r != OK)
            break;
        r = obj->evaluate(rt);
    }

    return r;
}



// ============================================================================
//
//    Equation
//
// ============================================================================

OBJECT_HANDLER_BODY(equation)
// ----------------------------------------------------------------------------
//    Handle commands for equations
// ----------------------------------------------------------------------------
{
    switch(op)
    {
    case EVAL:
        // Equations evaluate like programs
        return obj->evaluate(rt);
    case SIZE:
        return size(obj, payload);
    case PARSE:
        return object_parser(OBJECT_PARSER_ARG(), rt);
    case RENDER:
        return obj->object_renderer(OBJECT_RENDERER_ARG(), rt);
    case HELP:
        return (intptr_t) "program";

    default:
        // Check if anyone else knows how to deal with it
        return DELEGATE(program);
    }
}


OBJECT_PARSER_BODY(equation)
// ----------------------------------------------------------------------------
//    Try to parse this as a program
// ----------------------------------------------------------------------------
{
    return list::object_parser(ID_equation, p, rt, '\'', '\'');
}


OBJECT_RENDERER_BODY(equation)
// ----------------------------------------------------------------------------
//   Render the program into the given program buffer
// ----------------------------------------------------------------------------
{
    return list::object_renderer(r, rt, '\'', '\'');
}



// ============================================================================
//
//    Array
//
// ============================================================================

OBJECT_HANDLER_BODY(array)
// ----------------------------------------------------------------------------
//    Handle commands for arrays
// ----------------------------------------------------------------------------
{
    switch(op)
    {
    case EVAL:
        // Programs evaluate by evaluating all elements in sequence
        rt.push(obj);
        return 0;
    case SIZE:
        return size(obj, payload);
    case PARSE:
        return object_parser(OBJECT_PARSER_ARG(), rt);
    case RENDER:
        return obj->object_renderer(OBJECT_RENDERER_ARG(), rt);
    case HELP:
        return (intptr_t) "program";

    default:
        // Check if anyone else knows how to deal with it
        return DELEGATE(program);
    }
}


OBJECT_PARSER_BODY(array)
// ----------------------------------------------------------------------------
//    Try to parse this as a program
// ----------------------------------------------------------------------------
{
    return list::object_parser(ID_array, p, rt, '[', ']');
}


OBJECT_RENDERER_BODY(array)
// ----------------------------------------------------------------------------
//   Render the program into the given program buffer
// ----------------------------------------------------------------------------
{
    return list::object_renderer(r, rt, '[', ']');
}
