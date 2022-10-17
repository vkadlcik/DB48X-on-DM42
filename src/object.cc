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

#include "decimal-32.h"
#include "decimal-64.h"
#include "decimal128.h"
#include "integer.h"
#include "parser.h"
#include "renderer.h"
#include "rplstring.h"
#include "stack-cmds.h"
#include "runtime.h"

#include <stdio.h>


runtime &object::RT = runtime::RT;

RECORDER(object,         16, "Operations on objects");
RECORDER(parse,          16, "Parsing objects");
RECORDER(parse_attempts,256, "Attempts parsing an object");
RECORDER(render,         16, "Rendering objects");
RECORDER(eval,           16, "Evaluating objects");
RECORDER(run,            16, "Running commands on objects");
RECORDER(object_errors,  16, "Runtime errors on objects");


const object::handler_fn object::handler[NUM_IDS] =
// ----------------------------------------------------------------------------
//   The list of all possible handler
// ----------------------------------------------------------------------------
{
#define ID(id)  [ID_##id] = (handler_fn) id::object_handler,
#include <ids.tbl>
};


const cstring object::id_name[NUM_IDS] =
// ----------------------------------------------------------------------------
//   The name of all objects and commands
// ----------------------------------------------------------------------------
{
#define ID(id)  #id,
#include <ids.tbl>
};


const cstring object::opcode_name[NUM_OPCODES] =
// ----------------------------------------------------------------------------
//   The name of all handlers
// ----------------------------------------------------------------------------
{
#define RPL_OPCODE(cmd)  #cmd,
#include <rpl-opcodes.tbl>
};


void object::error(cstring message, cstring source, runtime &rt)
// ----------------------------------------------------------------------------
//    Send the error to the runtime
// ----------------------------------------------------------------------------
//    This function is not inline to avoid including runtime.h in object.h
{
    rt.error(message, source);
}


object *object::parse(cstring source, size_t &size, runtime &rt)
// ----------------------------------------------------------------------------
//  Try parsing the object as a top-level temporary
// ----------------------------------------------------------------------------
{
    record(parse, ">Parsing [%s]", source);
    parser p(source, size);
    result r = SKIP;
    cstring err = nullptr;
    cstring src = nullptr;

    // Try parsing with the various handlers
    for (uint i = 0; r == SKIP && i < NUM_IDS; i++)
    {
        p.candidate = id(i);
        record(parse_attempts, "Trying [%s] against %+s", src, name(id(i)));
        r = (result) handler[i](rt, PARSE, &p, nullptr, nullptr);
        if (r != SKIP)
            record(parse_attempts, "Result was %+s (%d) for [%s]",
                   name(r), r, source);
        if (r == WARN)
        {
            err = rt.error();
            src = rt.source();
            rt.error(nullptr);
            r = SKIP;
        }
    }

    record(parse, "<Done parsing [%s], end is at %d", source, p.end);
    size = p.end;

    if (r == SKIP)
    {
        if (err)
            error(err, src);
        else
            error("Syntax error", source);
    }

    return r == OK ? p.out : nullptr;
}


size_t object::render(char *output, size_t length, runtime &rt)
// ----------------------------------------------------------------------------
//   Render the object in a text buffer
// ----------------------------------------------------------------------------
{
    record(render, "Rendering %+s %p into %p", name(), this, output);
    renderer r(this, output, length);
    return run(rt, RENDER, &r);
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
        return -1;
    case SIZE:
        return payload - obj;
    case PARSE:
    {
        // Default is to identify an object by its name
        parser &p = OBJECT_PARSER_ARG();
        if (p.candidate != ID_object)
        {
            cstring name = object::name(p.candidate);
            size_t len = strlen(name);
            if (strncasecmp(name, p.source, len) == 0)
            {
                cstring src = p.source;
                char last = src[len];
                if (!last || isspace(last))
                {
                    p.end = len;
                    p.out = obj;
                    return OK;
                }
            }
        }
        return SKIP;
    }
    case RENDER:
        return obj->object_renderer(OBJECT_RENDERER_ARG(), rt);
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
    rt.error("Default object parser called", (cstring) p.source);
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
