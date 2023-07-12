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
#include "complex.h"
#include "decimal-32.h"
#include "decimal-64.h"
#include "decimal128.h"
#include "font.h"
#include "fraction.h"
#include "functions.h"
#include "integer.h"
#include "list.h"
#include "locals.h"
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
#include "user_interface.h"
#include "variables.h"

#include <stdio.h>


RECORDER(object,         16, "Operations on objects");
RECORDER(parse,          16, "Parsing objects");
RECORDER(parse_attempts,256, "Attempts parsing an object");
RECORDER(render,         16, "Rendering objects");
RECORDER(eval,           16, "Evaluating objects");
RECORDER(run,            16, "Running commands on objects");
RECORDER(object_errors,  16, "Runtime errors on objects");
RECORDER(assert_error,   16, "Assertion failures");


const object::dispatch object::handler[NUM_IDS] =
// ----------------------------------------------------------------------------
//   Table of handlers for each object type
// ----------------------------------------------------------------------------
{
#define ID(id)          NAMED(id,#id)
#define CMD(id)         ID(id)
#define NAMED(id, label)                                        \
    [ID_##id] = {                                               \
        .name        = #id,                                     \
        .fancy       = label,                                   \
        .size        = (size_fn)        id::do_size,            \
        .parse       = (parse_fn)       id::do_parse,           \
        .help        = (help_fn)        id::do_help,            \
        .evaluate    = (evaluate_fn)    id::do_evaluate,        \
        .execute     = (execute_fn)     id::do_execute,         \
        .render      = (render_fn)      id::do_render,          \
        .insert      = (insert_fn)      id::do_insert,          \
        .menu        = (menu_fn)        id::do_menu,            \
        .menu_marker = (menu_marker_fn) id::do_menu_marker,     \
        .arity       = id::ARITY,                               \
        .precedence  = id::PRECEDENCE                           \
    },
#include "ids.tbl"
};


object_p object::parse(utf8 source, size_t &size, int precedence)
// ----------------------------------------------------------------------------
//  Try parsing the object as a top-level temporary
// ----------------------------------------------------------------------------
//  If precedence is set, then we are parsing inside an equation
//  + if precedence > 0, then we are parsing an object of that precedence
//  + if precedence < 0, then we are parsing an infix at that precedence
{
    record(parse, ">Parsing [%s] precedence %d, %u IDs to try",
           source, precedence, NUM_IDS);

    // Skip spaces and newlines
    size_t skipped = utf8_skip_whitespace(source);
    if (skipped >= size)
        return nullptr;
    size -= skipped;

    parser p(source, size, precedence);
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
        r = handler[candidate].parse(p);
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
            rt.error(err).source(src);
        else
            rt.syntax_error().source(p.source);
    }

    return r == OK ? p.out : nullptr;
}


size_t object::render(char *output, size_t length) const
// ----------------------------------------------------------------------------
//   Render the object in a text buffer
// ----------------------------------------------------------------------------
{
    record(render, "Rendering %+s %p into %p", name(), this, output);
    renderer r(output, length);
    return render(r);
}


cstring object::edit() const
// ----------------------------------------------------------------------------
//   Render an object into the scratchpad, then move it into editor
// ----------------------------------------------------------------------------
{
    utf8 tname = name();     // Object may be GC'd during render
    record(render, "Rendering %+s %p into editor", tname, this);
    renderer r;
    size_t size = render(r);
    record(render, "Rendered %+s as size %u [%s]", tname, size, r.text());
    if (size)
    {
        rt.edit();
        r.clear();
    }
    return (cstring) rt.editor();
}


text_p object::as_text(bool edit, bool equation) const
// ----------------------------------------------------------------------------
//   Render an object into a text
// ----------------------------------------------------------------------------
{
    if (type() == ID_text && !equation)
        return text_p(this);

    record(render, "Rendering %+s %p into text", name(), this);
    renderer r(equation, edit);
    size_t size = render(r);
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



// ============================================================================
//
//   Default implementations for the object protocol
//
// ============================================================================

PARSE_BODY(object)
// ----------------------------------------------------------------------------
//   By default, cannot parse an object
// ----------------------------------------------------------------------------
{
    return SKIP;
}


HELP_BODY(object)
// ----------------------------------------------------------------------------
//   Default help topic for an object is the fancy name
// ----------------------------------------------------------------------------
{
    return o->fancy();
}


EVAL_BODY(object)
// ----------------------------------------------------------------------------
//   Show an error if we attempt to evaluate an object
// ----------------------------------------------------------------------------
{
    return rt.push(o) ? OK : ERROR;
}


EXEC_BODY(object)
// ----------------------------------------------------------------------------
//   The default execution is to evaluate
// ----------------------------------------------------------------------------
{
    return o->evaluate();
}


SIZE_BODY(object)
// ----------------------------------------------------------------------------
//   The default size is just the ID
// ----------------------------------------------------------------------------
{
    return ptrdiff(o->payload(), o);
}


RENDER_BODY(object)
// ----------------------------------------------------------------------------
//  The default for rendering is to print a pointer
// ----------------------------------------------------------------------------
{
    r.printf("Internal:%s[%p]", name(o->type()), o);
    return r.size();
}


INSERT_BODY(object)
// ----------------------------------------------------------------------------
//   Default insertion is as a program object
// ----------------------------------------------------------------------------
{
    return ui.edit(o->fancy(), ui.PROGRAM);
}


MENU_BODY(object)
// ----------------------------------------------------------------------------
//   No operation on menus by default
// ----------------------------------------------------------------------------
{
    return false;
}


MARKER_BODY(object)
// ----------------------------------------------------------------------------
//   No menu marker by default
// ----------------------------------------------------------------------------
{
    return 0;
}


object_p object::as_quoted(id ty) const
// ----------------------------------------------------------------------------
//   Check if something is a quoted value of the given type
// ----------------------------------------------------------------------------
//   This is typically used to quote symbols or locals (e.g. 'A')
{
    if (type() == ty)
        return this;
    if (equation_p eq = as<equation>())
        return eq->quoted(ty);
    return nullptr;
}


int object::as_truth(bool error) const
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
    case ID_based_integer:
        return !(integer_p(this)->is_zero());
    case ID_bignum:
    case ID_neg_bignum:
    case ID_bin_bignum:
    case ID_oct_bignum:
    case ID_dec_bignum:
    case ID_hex_bignum:
        return !(bignum_p(this)->is_zero());
    case ID_decimal128:
        return !decimal128_p(this)->is_zero();
    case ID_decimal64:
        return !decimal64_p(this)->is_zero();
    case ID_decimal32:
        return !decimal32_p(this)->is_zero();
    default:
        if (error)
            rt.type_error();
    }
    return -1;
}


#if SIMULATOR
cstring object::debug() const
// ----------------------------------------------------------------------------
//   Render an object from the debugger
// ----------------------------------------------------------------------------
{
    renderer r(false, true, true);
    render(r);
    r.put(char(0));
    return cstring(r.text());
}


cstring debug(object_p object)
// ----------------------------------------------------------------------------
//    Print an object pointer, for use in the debugger
// ----------------------------------------------------------------------------
{
    return object ? object->debug() : nullptr;
}


cstring debug(object_g object)
// ----------------------------------------------------------------------------
//   Same from an object_g
// ----------------------------------------------------------------------------
{
    return object ? object->debug() : nullptr;
}


cstring debug(object *object)
// ----------------------------------------------------------------------------
//   Same from an object *
// ----------------------------------------------------------------------------
{
    return object ? object->debug() : nullptr;
}


cstring debug(uint level)
// ----------------------------------------------------------------------------
//   Read a stack level
// ----------------------------------------------------------------------------
{
    if (object_g obj = rt.stack(level))
    {
        // We call both the object_g and object * variants so linker keeps them
        if (cstring result = obj->debug())
            return result;
        else if (object *op = (object *) object_p(obj))
            return debug(op);
    }
    return nullptr;
}


cstring debug()
// ----------------------------------------------------------------------------
//   Read top of stack
// ----------------------------------------------------------------------------
{
    return debug(0U);
}
#endif // SIMULATOR
