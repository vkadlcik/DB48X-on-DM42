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
    {
        byte *p = (byte *) payload;
        size_t len = leb128<size_t>(p);
        p += len;
        return ptrdiff(p, obj);
    }
    case PARSE:
        return object_parser(OBJECT_PARSER_ARG(), rt);
    case RENDER:
        return obj->object_renderer(OBJECT_RENDERER_ARG(), rt);
    case HELP:
        return (intptr_t) "text";

    default:
        // Check if anyone else knows how to deal with it
        return DELEGATE(string);
    }

}


object::result list::object_parser(parser  &p,
                                   runtime &rt,
                                   utf8code open,
                                   utf8code close)
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
    utf8code cp = utf8_codepoint(s);
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
        size_t length = 0;
        gcobj obj = object::parse(s, length);
        if (!obj)
            return ERROR;

        // Copy the parsed object to the scratch pad (may GC)
        size_t objsize = obj->size();
        byte *objcopy = rt.allocate(objsize);
        memcpy(objcopy, (byte *) obj, objsize);

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
    p.out         = rt.make<list>(ID_list, scratch, alloc);

    // Restore the scratchpad
    rt.free(alloc);
    return OK;
}


OBJECT_PARSER_BODY(list)
// ----------------------------------------------------------------------------
//    Try to parse this as an list
// ----------------------------------------------------------------------------
//    For simplicity, this deals with all kinds of lists
{
    return object_parser(p, rt, '{', '}');
}


intptr_t list::object_renderer(renderer &r, runtime &rt,
                               utf8code open, utf8code close) const
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


OBJECT_RENDERER_BODY(list)
// ----------------------------------------------------------------------------
//   Render the list into the given list buffer
// ----------------------------------------------------------------------------
{
    return object_renderer(r, rt, '{', '}');
}
