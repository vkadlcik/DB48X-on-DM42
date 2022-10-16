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

#include "integer.h"
#include "decimal128.h"
#include "decimal-64.h"
#include "decimal-32.h"
#include "rplstring.h"
#include "runtime.h"

#include <stdio.h>


runtime &object::RT = runtime::RT;

RECORDER(object,        16, "Operations on objects");
RECORDER(parse,         16, "Parsing objects");
RECORDER(render,        16, "Rendering objects");
RECORDER(eval,          16, "Evaluating objects");
RECORDER(run,           16, "Running commands on objects");
RECORDER(object_errors, 16, "Runtime errors on objects");


const object::handler_fn object::handler[NUM_IDS] =
// ----------------------------------------------------------------------------
//   The list of all possible handler
// ----------------------------------------------------------------------------
{
#define ID(id)  [ID_##id] = (handler_fn) id::object_handler,
#include <id.h>
};


const cstring object::id_name[NUM_IDS] =
// ----------------------------------------------------------------------------
//   The name of all handlers
// ----------------------------------------------------------------------------
{
#define ID(id)  #id,
#include <id.h>
};


const cstring object::cmd_name[NUM_COMMANDS] =
// ----------------------------------------------------------------------------
//   The name of all handlers
// ----------------------------------------------------------------------------
{
#define COMMAND(cmd)  #cmd,
#include <commands.h>
};


OBJECT_HANDLER_BODY(object)
// ----------------------------------------------------------------------------
//   Default handler for object
// ----------------------------------------------------------------------------
{
    switch(cmd)
    {
    case EVAL:
        rt.error("Invalid object");
        return -1;
    case SIZE:
        return payload - obj;
    case PARSE:
    {
        // Default is to identify an object by its name
        parser *p = (parser *) arg;
        if (p->candidate != ID_object)
        {
            cstring name = object::name(p->candidate);
            size_t len = strlen(name);
            if (strncasecmp(name, p->begin, len) == 0 && !isalnum(p->begin[len]))
            {
                p->end = p->begin + len;
                p->output = obj;
                return OK;
            }
        }
        return SKIP;
    }
    case RENDER:
    {
        renderer *out = (renderer *) arg;
        return snprintf(out->begin, out->end - out->begin,
                        "<Unknown object %p>", obj);
    }
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
    if (out)
        *out = nullptr;
    if (end)
        *end = begin;
    rt.error("Default object parser called", begin);
    return ERROR;
}


OBJECT_RENDERER_BODY(object)
// ----------------------------------------------------------------------------
//   Render the object to buffer starting at begin
// ----------------------------------------------------------------------------
//   Returns number of bytes needed - If larger than end - begin, retry
{
    rt.error("Rendering unimplemented object");
    return snprintf(begin, end - begin, "<Unimplemented object renderer>");
}
