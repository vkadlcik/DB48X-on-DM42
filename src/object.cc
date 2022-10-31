// ****************************************************************************
//  object.cc                                                     DB48X project
// ****************************************************************************
//
//   File Description:
//
//     Runtime support for objects
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

#include "algebraic.h"
#include "arithmetic.h"
#include "decimal-32.h"
#include "decimal-64.h"
#include "decimal128.h"
#include "font.h"
#include "functions.h"
#include "integer.h"
#include "list.h"
#include "parser.h"
#include "renderer.h"
#include "runtime.h"
#include "stack-cmds.h"
#include "symbol.h"
#include "text.h"
#include "variables.h"

#include <stdio.h>


runtime &object::RT = runtime::RT;

RECORDER(object,         16, "Operations on objects");
RECORDER(parse,          16, "Parsing objects");
RECORDER(parse_attempts,256, "Attempts parsing an object");
RECORDER(render,         16, "Rendering objects");
RECORDER(eval,           16, "Evaluating objects");
RECORDER(run,            16, "Running commands on objects");
RECORDER(object_errors,  16, "Runtime errors on objects");


// Define dummy opcode classes
#define OPCODE(n)       struct n : object {};
#define ID(n)
#include "ids.tbl"


const object::handler_fn object::handler[NUM_IDS] =
// ----------------------------------------------------------------------------
//   The list of all possible handler
// ----------------------------------------------------------------------------
{
#define ID(id)  [ID_##id] = (handler_fn) id::object_handler,
#include "ids.tbl"
};


const cstring object::id_name[NUM_IDS] =
// ----------------------------------------------------------------------------
//   The long name of all objects or commands
// ----------------------------------------------------------------------------
{
#define ID(id)                  #id,
#include "ids.tbl"
};


const cstring object::fancy_name[NUM_IDS] =
// ----------------------------------------------------------------------------
//   The long name of all objects or commands
// ----------------------------------------------------------------------------
{
#define ID(id)                  #id,
#define CMD(id)                 #id,
#define NAMED(id, name)         name,
#include "ids.tbl"
};


void object::error(utf8 message, utf8 source, runtime &rt)
// ----------------------------------------------------------------------------
//    Send the error to the runtime
// ----------------------------------------------------------------------------
//    This function is not inline to avoid including runtime.h in object.h
{
    rt.error(message, source);
}


object_p object::parse(utf8 source, size_t &size, runtime &rt)
// ----------------------------------------------------------------------------
//  Try parsing the object as a top-level temporary
// ----------------------------------------------------------------------------
{
    record(parse, ">Parsing [%s] %u IDs to try", source, NUM_IDS);

    // Skip spaces and newlines
    size_t skipped = 0;
    while (*source == ' ' || *source == '\n')
    {
        source++;
        skipped++;
    }

    parser p(source, size);
    result r   = SKIP;
    utf8   err = nullptr;
    utf8   src = nullptr;

    // Try parsing with the various handlers
    for (uint i = 0; r == SKIP && i < NUM_IDS; i++)
    {
        // Parse ID_symbol last, we need to check commands first
        uint candidate = (i + ID_symbol + 1) % NUM_IDS;
        p.candidate = id(candidate);
        record(parse_attempts, "Trying [%s] against %+s", src, name(id(i)));
        r = (result) handler[candidate](rt, PARSE, &p, nullptr, nullptr);
        if (r != SKIP)
            record(parse_attempts, "Result for ID %+s was %+s (%d) for [%s]",
                   name(p.candidate), name(r), r, source);
        if (r == WARN)
        {
            err = rt.error();
            src = rt.source();
            rt.error(nullptr);
            r = SKIP;
        }
    }

    record(parse, "<Done parsing [%s], end is at %d", source, p.end);
    size = p.end + skipped;

    if (r == SKIP)
    {
        if (err)
            error(err, src);
        else
            error("Syntax error", source);
    }

    return r == OK ? p.out : nullptr;
}


size_t object::render(char *output, size_t length, runtime &rt) const
// ----------------------------------------------------------------------------
//   Render the object in a text buffer
// ----------------------------------------------------------------------------
{
    record(render, "Rendering %+s %p into %p", name(), this, output);
    renderer r(this, output, length);
    return run(RENDER, rt, &r);
}


cstring object::render(bool editing, runtime &rt) const
// ----------------------------------------------------------------------------
//   Render the object into the scratchpad
// ----------------------------------------------------------------------------
{
    record(render, "Rendering %+s %p into scratchpad", name(), this);
    size_t available = rt.available();
    gcmstring buffer = (char *) rt.scratchpad();
    renderer r(this, buffer, available, editing);
    size_t actual = run(RENDER, rt, &r);
    record(render, "Rendered %+s as size %u [%s]",
           name(), actual, (char *) buffer);
    if (actual + 1 > available)
        return nullptr;

    // Allocate in the scratchpad, and null-terminate
    char *allocated = (char *) rt.allocate(actual + 1);
    allocated[actual] = 0;
    return allocated;
}


cstring object::edit(runtime &rt) const
// ----------------------------------------------------------------------------
//   Render an object into the scratchpad, then move it into editor
// ----------------------------------------------------------------------------
//   Note that it is still null-terminated, but will no longer be as soon as
//   it is being edited
{
    cstring result = render(true, rt);
    if (result)
        rt.edit();
    return result;
}


OBJECT_HANDLER_BODY(object)
// ----------------------------------------------------------------------------
//   Default handler for object
// ----------------------------------------------------------------------------
{
    switch(op)
    {
    case EVAL:
        rt.error("Invalid object");
        return ERROR;
    case SIZE:
        return payload - obj;
    case PARSE:
        // Default is to not be parseable
        return SKIP;
    case RENDER:
        return obj->object_renderer(OBJECT_RENDERER_ARG(), rt);
    case HELP:
        return (intptr_t) "Unknown";
    default:
        return SKIP;
    }
}


OBJECT_PARSER_BODY(object)
// ----------------------------------------------------------------------------
//   Parser for the object type
// ----------------------------------------------------------------------------
//   This would only be called if a derived class forgets to implement a parser
{
    p.out = nullptr;
    p.end = 0;
    rt.error("Default object parser called", p.source);
    return ERROR;
}


OBJECT_RENDERER_BODY(object)
// ----------------------------------------------------------------------------
//   Render the object to buffer starting at begin
// ----------------------------------------------------------------------------
//   Returns number of bytes needed - If larger than end - begin, retry
{
    rt.error("Rendering unimplemented object");
    return snprintf(r.target, r.length, "<Unknown %p>", this);
}


symbol_p object::as_name() const
// ----------------------------------------------------------------------------
//   Check if something is a valid name
// ----------------------------------------------------------------------------
{
    if (type() == ID_symbol)
        return (symbol_p) this;
    if (equation_p eq = as<equation>())
        return eq->symbol();
    return nullptr;
}
