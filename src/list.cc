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
    case EXEC:
    case EVAL:
        // List values evaluate as self
        rt.push(obj);
        return OK;
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
        return DELEGATE(text);
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
    gcutf8  s   = p.source;
    size_t  max = p.length;

    // Check if we have the opening marker
    unicode cp  = 0;
    if (open)
    {
        cp = utf8_codepoint(s);
        if (cp != open)
            return SKIP;
        s = utf8_next(s);
    }

    size_t  prealloc = rt.allocated();
    gcbytes scratch = rt.scratchpad() + prealloc;
    while (size_t(utf8(s) - utf8(p.source)) < max)
    {
        cp = utf8_codepoint(s);
        if (cp == close)
        {
            s = utf8_next(s);
            break;
        }
        if (cp == ' ' || cp == '\n' || cp == '\t')
        {
            s = utf8_next(s);
            continue;
        }

        // Parse an object
        size_t done = (byte *) s - (byte *) p.source;
        size_t length = max > done ? max - done : 0;
        gcobj obj = object::parse(s, length);
        if (!obj)
            return ERROR;

        // Copy the parsed object to the scratch pad (may GC)
        size_t objsize = obj->size();
        byte *objcopy = rt.allocate(objsize);
        memmove(objcopy, (byte *) obj, objsize);

        // Jump past what we parsed
        s = utf8(s) + length;
    }

    // Check that we have a matching closing character
    if (close && cp != close)
    {
        record(list_errors, "Missing terminator, got %u (%c) at %s",
               cp, cp, (byte *) s);
        rt.error("Missing terminator", s);
        return ERROR;
    }

    // Create the object
    size_t alloc  = rt.allocated() - prealloc;
    size_t parsed = utf8(s) - utf8(p.source);
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
    if (open)
    {
        size_t rendered = utf8_encode(open, buffer);
        for (size_t i = 0; i < rendered; i++)
        {
            if (idx < available)
            dst[idx] = buffer[i];
            idx++;
        }
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
    if (close)
    {
        size_t rendered = utf8_encode(close, buffer);
        for (size_t i = 0; i < rendered; i++)
        {
            if (idx < available)
                dst[idx] = buffer[i];
            idx++;
        }
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
    case EXEC:
        // Execute the program
        return obj->execute(rt);
    case EVAL:
        // A normal evaluation (not from ID_eval) just places program on stack
        // e.g.: from command line, or « « 1 + 2 » »
        rt.push(obj);
        return OK;
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


object::result program::execute(runtime &rt) const
// ----------------------------------------------------------------------------
//   We evaluate a program by evaluating all the objects in it
// ----------------------------------------------------------------------------
//   This is called directly from the 'eval' command
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


program_p program::parse(utf8 source, size_t size)
// ----------------------------------------------------------------------------
//   Parse a program without delimiters (e.g. command line)
// ----------------------------------------------------------------------------
{
    record(program, ">Parsing command line [%s]", source);
    parser p(source, size);
    result r = list::object_parser(ID_program, p, RT, 0, 0);
    record(program, "<Command line [%s], end at %u, result %p",
           utf8(p.source), p.end, object_p(p.out));
    if (r != OK)
        return nullptr;
    object_p  obj  = p.out;
    if (!obj)
        return nullptr;
    program_p prog = obj->as<program>();
    return prog;
}



// ============================================================================
//
//    Block
//
// ============================================================================

OBJECT_HANDLER_BODY(block)
// ----------------------------------------------------------------------------
//    Handle commands for blocks
// ----------------------------------------------------------------------------
{
    switch(op)
    {
    case EXEC:
    case EVAL:
        return obj->execute(rt);
    case SIZE:
        return size(obj, payload);
    case PARSE:
        return SKIP;
    case RENDER:
        return obj->object_renderer(OBJECT_RENDERER_ARG(), rt);
    case HELP:
        return (intptr_t) "block";

    default:
        // Check if anyone else knows how to deal with it
        return DELEGATE(program);
    }
}


OBJECT_RENDERER_BODY(block)
// ----------------------------------------------------------------------------
//   Render the program into the given program buffer
// ----------------------------------------------------------------------------
{
    return list::object_renderer(r, rt, 0, 0);
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
    case EXEC:
        return obj->execute(rt);
    case EVAL:
        // Equations evaluate like programs
        rt.push(obj);
        return OK;
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


symbol_p equation::symbol() const
// ----------------------------------------------------------------------------
//   If an equation contains a single symbol, return that
// ----------------------------------------------------------------------------
{
    byte  *p       = (byte *) payload();
    size_t size    = leb128<size_t>(p);
    object_p first = (object_p) p;
    if (first->type() == ID_symbol && first->size() == size)
        return (symbol_p) first;
    return nullptr;
}


equation::equation(uint arity, const gcobj args[], id op, id type)
// ----------------------------------------------------------------------------
//   Build an equation from N arguments
// ----------------------------------------------------------------------------
    : program(nullptr, 0, type)
{
    byte *p = payload();

    // Compute the size of the program
    size_t size = 0;
    for (uint i = 0; i < arity; i++)
        size += args[i]->size();
    size += leb128size(op);

    // Write the size of the program
    p = leb128(p, size);

    // Write the arguments
    for (uint i = 0; i < arity; i++)
    {
        size_t objsize = args[i]->size();
        memmove(p, byte_p(args[i]), objsize);
        p += objsize;
    }

    // Write the last opcode
    p = leb128(p, op);
}


size_t equation::required_memory(id type, uint arity, const gcobj args[], id op)
// ----------------------------------------------------------------------------
//   Size of an equation object with N arguments
// ----------------------------------------------------------------------------
{
    size_t size = 0;
    for (uint i = 0; i < arity; i++)
        size += args[i]->size();
    size += leb128size(op);
    size += leb128size(size);
    size += leb128size(type);
    return size;
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
    case EXEC:
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
