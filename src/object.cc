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
#include "bignum.h"
#include "catalog.h"
#include "compare.h"
#include "decimal-32.h"
#include "decimal-64.h"
#include "decimal128.h"
#include "font.h"
#include "fraction.h"
#include "functions.h"
#include "input.h"
#include "integer.h"
#include "list.h"
#include "logical.h"
#include "loops.h"
#include "menu.h"
#include "parser.h"
#include "renderer.h"
#include "runtime.h"
#include "settings.h"
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
RECORDER(assert_error,   16, "Assertion failures");


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


object_p object::parse(utf8 source, size_t &size, runtime &rt)
// ----------------------------------------------------------------------------
//  Try parsing the object as a top-level temporary
// ----------------------------------------------------------------------------
{
    record(parse, ">Parsing [%s] %u IDs to try", source, NUM_IDS);

    // Skip spaces and newlines
    size_t skipped = 0;
    while (*source == ' ' || *source == '\n' || *source == '\t')
    {
        source++;
        skipped++;
    }

    parser p(source, size);
    result r   = SKIP;
    utf8   err = nullptr;
    utf8   src = source;

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
                   name(p.candidate), name(r), r, utf8(p.source));
        if (r == WARN)
        {
            err = rt.error();
            src = rt.source();
            rt.clear_error();
            r = SKIP;
        }
    }

    record(parse, "<Done parsing [%s], end is at %d", utf8(p.source), p.end);
    size = p.end + skipped;

    if (r == SKIP)
    {
        if (err)
            RT.error(err).source(src);
        else
            RT.syntax_error().source(p.source);
    }

    return r == OK ? p.out : nullptr;
}


size_t object::render(char *output, size_t length, runtime &rt) const
// ----------------------------------------------------------------------------
//   Render the object in a text buffer
// ----------------------------------------------------------------------------
{
    record(render, "Rendering %+s %p into %p", name(), this, output);
    renderer r(output, length);
    return run(RENDER, rt, &r);
}


size_t object::render(renderer &r, runtime &rt) const
// ----------------------------------------------------------------------------
//   Render the object in a text buffer
// ----------------------------------------------------------------------------
{
    record(render, "Rendering %+s %p into existing %p", name(), this, &r);
    size_t pre = r.size();
    size_t sz = run(RENDER, rt, &r);
    record(render, "Rendered %+s as size %u [%s]", name(), sz, r.text() + pre);
    return sz;
}


cstring object::edit(runtime &rt) const
// ----------------------------------------------------------------------------
//   Render an object into the scratchpad, then move it into editor
// ----------------------------------------------------------------------------
{
    record(render, "Rendering %+s %p into editor", name(), this);
    renderer r;
    size_t size = run(RENDER, rt, &r);
    record(render, "Rendered %+s as size %u [%s]", name(), size, r.text());
    if (size)
        rt.edit();
    return (cstring) rt.editor();
}


text_p object::as_text(bool equation, runtime &rt) const
// ----------------------------------------------------------------------------
//   Render an object into a text
// ----------------------------------------------------------------------------
{
    if (type() == ID_text && !equation)
        return text_p(this);

    record(render, "Rendering %+s %p into text", name(), this);
    renderer r(equation);
    size_t size = run(RENDER, rt, &r);
    record(render, "Rendered %+s as size %u [%s]", name(), size, r.text());
    if (!size)
        return nullptr;
    id type = equation ? ID_symbol : ID_text;
    gcutf8 txt = r.text();
    text_g result = rt.make<text>(type, txt, size);
    return result;
}


void object::object_error(id type, object_p ptr)
// ----------------------------------------------------------------------------
//    Report an error in an object
// ----------------------------------------------------------------------------
{
    uintptr_t debug[2];
    byte *d = (byte *) debug;
    byte *s = (byte *) ptr;
    for (uint i = 0; i < sizeof(debug); i++)
        d[i] = s[i];
    record(object_errors,
           "Invalid type %d for %p  Data %16llX %16llX",
           type, ptr, debug[0], debug[1]);
}


OBJECT_HANDLER_BODY(object)
// ----------------------------------------------------------------------------
//   Default handler for object
// ----------------------------------------------------------------------------
{
    switch(op)
    {
    case EXEC:
    case EVAL:
        rt.invalid_object_error();
        return ERROR;
    case SIZE:
        return payload - obj;
    case PARSE:
        // Default is to not be parseable
        return SKIP;
    case RENDER:
        return obj->object_renderer(OBJECT_RENDERER_ARG(), rt);
    case INSERT:
        return ((input *) arg)->edit(obj->fancy(), input::PROGRAM);
    case HELP:
        return (intptr_t) "Unknown";
    case MENU_MARKER:
        return 0;
    case ARITY:
        return SKIP;
    case PRECEDENCE:
        return algebraic::UNKNOWN;
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
    rt.invalid_object_error().source(p.source);
    return ERROR;
}


OBJECT_RENDERER_BODY(object)
// ----------------------------------------------------------------------------
//   Render the object to buffer starting at begin
// ----------------------------------------------------------------------------
//   Returns number of bytes needed - If larger than end - begin, retry
{
    rt.invalid_object_error();
    return r.printf("<Unknown %p>", this);
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


int object::as_truth() const
// ----------------------------------------------------------------------------
//   Get the logical value for an object, or -1 if invalid
// ----------------------------------------------------------------------------
{
    id ty = type();
    switch(ty)
    {
    case ID_True:
        return 1;
    case ID_False:
        return 0;
    case ID_integer:
    case ID_neg_integer:
    case ID_bin_integer:
    case ID_oct_integer:
    case ID_dec_integer:
    case ID_hex_integer:
        return *payload() != 0;
    case ID_bignum:
    case ID_neg_bignum:
    case ID_bin_bignum:
    case ID_oct_bignum:
    case ID_dec_bignum:
    case ID_hex_bignum:
        return payload()[1] != 0; // Check if the size is not zero
    case ID_decimal128:
        return !decimal128_p(this)->is_zero();
    case ID_decimal64:
        return !decimal64_p(this)->is_zero();
    case ID_decimal32:
        return !decimal32_p(this)->is_zero();
    default:
        RT.type_error();
    }
    return -1;
}
