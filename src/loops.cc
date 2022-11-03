// ****************************************************************************
//  loops.cc                                                      DB48X project
// ****************************************************************************
//
//   File Description:
//
//     Implementation of basic loops
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

#include "loops.h"

#include "command.h"
#include "decimal-32.h"
#include "decimal-64.h"
#include "decimal128.h"
#include "integer.h"
#include "parser.h"
#include "renderer.h"
#include "runtime.h"
#include "utf8.h"

#include <stdio.h>
#include <string.h>


RECORDER(loop, 16, "Loops");
RECORDER(loop_errors, 16, "Errors processing loops");


object::result loop::condition(bool &value) const
// ----------------------------------------------------------------------------
//   Check if the stack is a true condition
// ----------------------------------------------------------------------------
{
    if (object_p cond = RT.pop())
    {
        switch(cond->type())
        {
        case ID_hex_integer:
        case ID_oct_integer:
        case ID_bin_integer:
        case ID_dec_integer:
        case ID_integer:
        case ID_neg_integer:
            value = integer_p(cond)->value<ularge>() != 0;
            return OK;
        case ID_decimal128:
            value = !decimal128_p(cond)->is_zero();
            return OK;
        case ID_decimal64:
            value = !decimal64_p(cond)->is_zero();
            return OK;
        case ID_decimal32:
            value = !decimal32_p(cond)->is_zero();
            return OK;
        default:
            RT.error("Bad argument type");
        }
    }
    return ERROR;
}


object::result loop::object_parser(id       type,
                                   parser  &p,
                                   runtime &rt,
                                   cstring  open,
                                   cstring  close,
                                   cstring  middle)
// ----------------------------------------------------------------------------
//   Generic parser for loops
// ----------------------------------------------------------------------------
//   Like for programs, we have to be careful here, because parsing sub-objects
//   may allocate new temporaries, which itself may cause garbage collection.
{
    // We have to be careful that we may have to GC to make room for loop
    gcutf8  s             = p.source;
    cstring separators[3] = { open, middle, close };
    uint    steps         = middle ? 3 : 2;
    size_t  max           = p.length;
    size_t  len           = max;
    size_t  prealloc      = rt.allocated();
    gcbytes scratch       = rt.scratchpad() + prealloc;
    size_t  condsize      = 0;
    size_t  bodysize      = 0;
    size_t  size          = 0;
    gcobj   condition     = nullptr;
    gcobj   body          = nullptr;

    if (!middle)
        separators[1] = separators[2];

    // Loop over the two or three separators
    for (uint step = 0;
         step < steps && size_t(utf8(s) - utf8(p.source)) < max;
         step++)
    {
        cstring sep   = cstring(separators[step]);
        bool    found = false;

        // Scan the body of the loop
        while (!found && size_t(utf8(s) - utf8(p.source)) < max)
        {
            // Skip spaces
            unicode cp = utf8_codepoint(s);
            if (cp == ' ' || cp == '\n')
            {
                s = utf8_next(s);
                continue;
            }

            // Check if we have the separator
            len = strlen(sep);
            if (len <= max
                && strncasecmp(cstring(utf8(s)), sep, len) == 0
                && (len >= max || command::is_separator(utf8(s) + len)))
            {
                s += len;
                found = true;
                continue;
            }

            // If we get there and are at step 0, this is a failure
            if (step == 0)
                return SKIP;

            // Parse an object
            size_t done = utf8(s) - utf8(p.source);
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

        if (!found)
        {
            // If we did not find the terminator, we reached end of text
            RT.error("Unterminated loop", utf8(p.source));
            return ERROR;
        }

        // Create the program object for condition or body
        size_t alloc  = rt.allocated() - prealloc;
        object_p prog = rt.make<program>(type, scratch, alloc);
        rt.free(alloc);
        condsize = bodysize;
        bodysize = prog->size();
        if (step == steps - 1)
            body = prog;
        else
            condition = prog;
        size += bodysize;
    }

    // Allocate the loop
    byte *buf = rt.allocate(size);
    byte *ptr = buf;
    if (condition)
    {
        memmove(ptr, object_p(condition), condsize);
        ptr += condsize;
    }
    memmove(ptr, object_p(body), bodysize);

    size_t parsed = utf8(s) - utf8(p.source);
    p.end         = parsed;
    p.out         = rt.make<loop>(type, scratch, size);
    rt.free(size);

    return OK;
}


static inline void put(byte *dst, size_t &idx, size_t available, char c)
// ----------------------------------------------------------------------------
//   Safely put a char in the buffer
// ----------------------------------------------------------------------------
{
    if (idx < available)
        dst[idx] = c;
    idx++;
}


static inline void put(byte *dst, size_t &idx, size_t available, cstring src)
// ----------------------------------------------------------------------------
//   Safely put info into the buffer
// ----------------------------------------------------------------------------
{
    for (cstring p = src; *p; p++)
        put(dst, idx, available, *p);
}


static inline void indent(byte *dst, size_t &idx, size_t available, uint indent)
// ----------------------------------------------------------------------------
//   Safely put a char in the buffer
// ----------------------------------------------------------------------------
{
    put(dst, idx, available, '\n');
    for(uint i = 0; i < indent; i++)
        put(dst, idx, available, '\t');
}


intptr_t loop::object_renderer(renderer &r,
                               runtime  &rt,
                               cstring   open,
                               cstring   close,
                               cstring   middle) const
// ----------------------------------------------------------------------------
//   Render the loop into the given buffer
// ----------------------------------------------------------------------------
{
    // Source objects
    byte_p p    = payload();
    size_t size = leb128<size_t>(p);

    // Isolate condition and body
    object_p condition = middle ? object_p(p) : nullptr;
    object_p body = condition ? condition->skip() : object_p(p);

    // Destination buffer
    size_t idx = 0;
    size_t available = r.length;
    byte * dst = r.target;

    // Write the header, e.g. "DO", and indent condition
    indent(dst, idx, available, r.indent);
    put(dst, idx, available, open);
    indent(dst, idx, available, ++r.indent);

    // Emit the condition
    size_t remaining = r.length > idx ? r.length - idx : 0;
    if (condition)
    {
        idx += condition->render((char *) dst + idx, remaining, rt);

        // Emit separator after condition
        indent(dst, idx, available, --r.indent);
        put(dst, idx, available, middle);
        indent(dst, idx, available, ++r.indent);
    }

    // Emit body
    remaining = r.length > idx ? r.length - idx : 0;
    idx += body->render((char *) dst + idx, remaining, rt);

    // Emit closing separator
    indent(dst, idx, available, --r.indent);
    put(dst, idx, available, close);
    indent(dst, idx, available, r.indent);

    size = size + 0;
    return idx;
}



// ============================================================================
//
//   DO...UNTIL...END loop
//
// ============================================================================

OBJECT_HANDLER_BODY(DoUntil)
// ----------------------------------------------------------------------------
//    Handle commands for do..until..end
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
        return loop::object_parser(ID_DoUntil, OBJECT_PARSER_ARG(), rt,
                                   "do", "end", "until");
    case RENDER:
        return obj->loop::object_renderer(OBJECT_RENDERER_ARG(), rt,
                                          "do", "end", "until");
    case HELP:
        return (intptr_t) "DoUntilLoop";

    default:
        // Check if anyone else knows how to deal with it
        return DELEGATE(loop);
    }
}


object::result DoUntil::execute(runtime &rt) const
// ----------------------------------------------------------------------------
//   Evaluate a do..until..end loop
// ----------------------------------------------------------------------------
//   In this loop, the body comes first
{
    byte  *p       = (byte *) payload();
    size_t len     = leb128<size_t>(p);
    gcobj  body    = object_p(p);
    gcobj  cond    = body->skip();
    result r       = OK;

    while (!interrupted() && r == OK)
    {
        r = body->evaluate(rt);
        if (r != OK)
            break;
        r = cond->evaluate(rt);
        if (r != OK)
            break;
        bool test = false;
        r = condition(test);
        if (r != OK || test)
            break;
    }
    len = len + 0;
    return r;
}


// ============================================================================
//
//   WHILE...REPEAT...END loop
//
// ============================================================================

OBJECT_HANDLER_BODY(WhileRepeat)
// ----------------------------------------------------------------------------
//    Handle commands for while..repeat..end
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
        return loop::object_parser(ID_WhileRepeat, OBJECT_PARSER_ARG(), rt,
                                   "while", "end", "repeat");
    case RENDER:
        return obj->loop::object_renderer(OBJECT_RENDERER_ARG(), rt,
                                          "while", "end", "repeat");
    case HELP:
        return (intptr_t) "WhileRepeat";

    default:
        // Check if anyone else knows how to deal with it
        return DELEGATE(loop);
    }
}


object::result WhileRepeat::execute(runtime &rt) const
// ----------------------------------------------------------------------------
//   Evaluate a while..repeat..end loop
// ----------------------------------------------------------------------------
//   In this loop, the condition comes first
{
    byte  *p       = (byte *) payload();
    size_t len     = leb128<size_t>(p);
    gcobj  cond    = object_p(p);
    gcobj  body    = cond->skip();
    result r       = OK;

    while (!interrupted() && r == OK)
    {
        r = cond->evaluate();
        if (r != OK)
            break;
        bool test = false;
        r = condition(test);
        if (r != OK || test)
            break;
        r = body->evaluate(rt);
    }
    len = len + 0;
    return r;
}



// ============================================================================
//
//   START...NEXT loop
//
// ============================================================================

OBJECT_HANDLER_BODY(StartNext)
// ----------------------------------------------------------------------------
//    Handle commands for for..next loop
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
        return loop::object_parser(ID_StartNext, OBJECT_PARSER_ARG(), rt,
                                   "start", "next");
    case RENDER:
        return obj->loop::object_renderer(OBJECT_RENDERER_ARG(), rt,
                                          "start", "next");
    case HELP:
        return (intptr_t) "StartNextLoop";

    default:
        // Check if anyone else knows how to deal with it
        return DELEGATE(loop);
    }
}


object::result loop::counted(gcobj body, runtime &rt, bool stepping)
// ----------------------------------------------------------------------------
//   Evaluate a counted loop
// ----------------------------------------------------------------------------
{
    object::result r      = object::OK;
    object_p       finish = rt.stack(0);
    object_p       start  = rt.stack(1);

    // If stack is empty, exit
    if (!start || !finish)
        return ERROR;

    // Check that we have integers
    integer_p ifinish = finish->as<integer>();
    integer_p istart = start->as<integer>();
    if (!ifinish || !istart)
    {
        RT.error("Invalid type");
        return ERROR;
    }

    // Pop them from the stack
    RT.pop();
    RT.pop();

    ularge incr = 1;
    ularge cnt  = istart->value<ularge>();
    ularge last = ifinish->value<ularge>();

    while (!interrupted() && r == OK)
    {
        r = body->evaluate();
        if (r != OK)
            break;
        if (stepping)
        {
            object_p step = RT.pop();
            if (!step)
                return ERROR;
            integer_p istep = step->as<integer>();
            if (!istep)
            {
                RT.error("Invalid type");
                return ERROR;
            }
            incr = istep->value<ularge>();
        }
        cnt += incr;
        if (cnt > last)
            break;
    }
    return r;
}


object::result StartNext::execute(runtime &rt) const
// ----------------------------------------------------------------------------
//   Evaluate a for..next loop
// ----------------------------------------------------------------------------
{
    byte    *p    = (byte *) payload();
    size_t   len  = leb128<size_t>(p);
    object_p body = object_p(p);
    len = len + 0;
    return counted(body, rt, false);
}


// ============================================================================
//
//   START...STEP loop
//
// ============================================================================

OBJECT_HANDLER_BODY(StartStep)
// ----------------------------------------------------------------------------
//    Handle commands for for..step loop
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
        return loop::object_parser(ID_StartStep, OBJECT_PARSER_ARG(), rt,
                                   "start", "step");
    case RENDER:
        return obj->loop::object_renderer(OBJECT_RENDERER_ARG(), rt,
                                          "start", "step");
    case HELP:
        return (intptr_t) "StartStepLoop";

    default:
        // Check if anyone else knows how to deal with it
        return DELEGATE(loop);
    }
}


object::result StartStep::execute(runtime &rt) const
// ----------------------------------------------------------------------------
//   Evaluate a for..step loop
// ----------------------------------------------------------------------------
{
    byte    *p    = (byte *) payload();
    size_t   len  = leb128<size_t>(p);
    object_p body = object_p(p);
    len = len + 0;
    return counted(body, rt, true);
}



// ============================================================================
//
//   FOR...NEXT loop
//
// ============================================================================

OBJECT_HANDLER_BODY(ForNext)
// ----------------------------------------------------------------------------
//    Handle commands for for..next loop
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
        return loop::object_parser(ID_ForNext, OBJECT_PARSER_ARG(), rt,
                                   "for", "next");
    case RENDER:
        return obj->loop::object_renderer(OBJECT_RENDERER_ARG(), rt,
                                          "for", "next");
    case HELP:
        return (intptr_t) "ForNextLoop";

    default:
        // Check if anyone else knows how to deal with it
        return DELEGATE(loop);
    }
}


object::result ForNext::execute(runtime &rt) const
// ----------------------------------------------------------------------------
//   Evaluate a for..next loop
// ----------------------------------------------------------------------------
{
    byte    *p    = (byte *) payload();
    size_t   len  = leb128<size_t>(p);
    object_p body = object_p(p);
    len = len + 0;
    return counted(body, rt, false);
}



// ============================================================================
//
//   FOR...STEP loop
//
// ============================================================================

OBJECT_HANDLER_BODY(ForStep)
// ----------------------------------------------------------------------------
//    Handle commands for for..step loop
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
        return loop::object_parser(ID_ForStep, OBJECT_PARSER_ARG(), rt,
                                   "for", "step");
    case RENDER:
        return obj->loop::object_renderer(OBJECT_RENDERER_ARG(), rt,
                                          "for", "step");
    case HELP:
        return (intptr_t) "ForStepLoop";

    default:
        // Check if anyone else knows how to deal with it
        return DELEGATE(loop);
    }
}


object::result ForStep::execute(runtime &rt) const
// ----------------------------------------------------------------------------
//   Evaluate a for..step loop
// ----------------------------------------------------------------------------
{
    byte    *p    = (byte *) payload();
    size_t   len  = leb128<size_t>(p);
    object_p body = object_p(p);
    len = len + 0;
    return counted(body, rt, true);
}
