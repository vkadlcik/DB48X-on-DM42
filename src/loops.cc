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
#include "compare.h"
#include "decimal-32.h"
#include "decimal-64.h"
#include "decimal128.h"
#include "input.h"
#include "integer.h"
#include "parser.h"
#include "renderer.h"
#include "runtime.h"
#include "utf8.h"

#include <stdio.h>
#include <string.h>


RECORDER(loop, 16, "Loops");
RECORDER(loop_errors, 16, "Errors processing loops");



SIZE_BODY(loop)
// ----------------------------------------------------------------------------
//   Compute size for a loop
// ----------------------------------------------------------------------------
{
    object_p p = object_p(o->payload());
    p = p->skip();
    return ptrdiff(p, o);
}


loop::loop(object_g body, id type)
// ----------------------------------------------------------------------------
//   Constructor for loops
// ----------------------------------------------------------------------------
    : command(type)
{
    byte *p = (byte *) payload();
    size_t bsize = body->size();
    memmove(p, byte_p(body), bsize);
}


SIZE_BODY(conditional_loop)
// ----------------------------------------------------------------------------
//   Compute size for a conditional loop
// ----------------------------------------------------------------------------
{
    object_p p = object_p(o->payload());
    p = p->skip()->skip();
    return ptrdiff(p, o);
}


conditional_loop::conditional_loop(object_g first, object_g second, id type)
// ----------------------------------------------------------------------------
//   Constructor for conditional loops
// ----------------------------------------------------------------------------
    : loop(first, type)
{
    size_t fsize = first->size();
    byte *p = (byte *) payload() + fsize;
    size_t ssize = second->size();
    memmove(p, byte_p(second), ssize);
}



object::result conditional_loop::condition(bool &value)
// ----------------------------------------------------------------------------
//   Check if the stack is a true condition
// ----------------------------------------------------------------------------
{
    if (object_p cond = rt.pop())
    {
        int truth = cond->as_truth(true);
        if (truth >= 0)
        {
            value = (bool) truth;
            return OK;
        }
    }
    return ERROR;
}


object::result loop::object_parser(id       type,
                                   parser  &p,
                                   cstring  open,
                                   cstring  close)
// ----------------------------------------------------------------------------
//   Call object parser with two separators
// ----------------------------------------------------------------------------
{
    cstring seps[] = { open, close };
    return loop::object_parser(type, p, 2, seps);
}


intptr_t loop::object_renderer(renderer &r,
                               cstring   open,
                               cstring   close) const
// ----------------------------------------------------------------------------
//   Call object renderer with two separators
// ----------------------------------------------------------------------------
{
    cstring seps[] = { open, close };
    return loop::object_renderer(r, 2, seps);
}


object::result conditional_loop::object_parser(id       type,
                                               parser  &p,
                                               cstring  open,
                                               cstring  middle,
                                               cstring  close)
// ----------------------------------------------------------------------------
//   Call object parser with two separators
// ----------------------------------------------------------------------------
{
    cstring seps[] = { open, middle, close };
    return loop::object_parser(type, p, 3, seps);
}


intptr_t conditional_loop::object_renderer(renderer &r,
                                           cstring   open,
                                           cstring   middle,
                                           cstring   close) const
// ----------------------------------------------------------------------------
//   Call object renderer with two separators
// ----------------------------------------------------------------------------
{
    cstring seps[] = { open, middle, close };
    return loop::object_renderer(r, 3, seps);
}



object::result loop::object_parser(id       type,
                                   parser  &p,
                                   uint     steps,
                                   cstring  separators[])
// ----------------------------------------------------------------------------
//   Generic parser for loops
// ----------------------------------------------------------------------------
//   Like for programs, we have to be careful here, because parsing sub-objects
//   may allocate new temporaries, which itself may cause garbage collection.
{
    // We have to be careful that we may have to GC to make room for loop
    gcutf8   src  = p.source;
    size_t   max  = p.length;
    object_g obj1 = nullptr;
    object_g obj2 = nullptr;

    // Loop over the two or three separators
    for (uint step = 0;
         step < steps && utf8_more(p.source, src, max);
         step++)
    {
        cstring  sep   = cstring(separators[step]);
        size_t   len   = strlen(sep);
        bool     found = false;
        scribble scr;

        // Scan the body of the loop
        while (!found && utf8_more(p.source, src, max))
        {
            // Skip spaces
            unicode cp = utf8_codepoint(src);
            if (utf8_whitespace(cp))
            {
                src = utf8_next(src);
                continue;
            }

            // Check if we have the separator
            if (len <= max
                && strncasecmp(cstring(utf8(src)), sep, len) == 0
                && (len >= max || command::is_separator(utf8(src) + len)))
            {
                src += len;
                found = true;
                continue;
            }

            // If we get there and are at step 0, this is a failure
            if (step == 0)
                return SKIP;

            // Parse an object
            size_t   done   = utf8(src) - utf8(p.source);
            size_t   length = max > done ? max - done : 0;
            object_g obj    = object::parse(src, length);
            if (!obj)
                return ERROR;

            // Copy the parsed object to the scratch pad (may GC)
            size_t objsize = obj->size();
            byte *objcopy = rt.allocate(objsize);
            if (!objcopy)
                return ERROR;
            memmove(objcopy, (byte *) obj, objsize);

            // Jump past what we parsed
            src = utf8(src) + length;
        }

        if (!found)
        {
            // If we did not find the terminator, we reached end of text
            rt.unterminated_error().source(p.source);
            return ERROR;
        }
        else if (step == 0)
        {
            // If we matched the first word ('for', 'start', etc), no object
            continue;
        }


        // Create the program object for condition or body
        gcbytes  scratch = scr.scratch();
        size_t   alloc   = scr.growth();
        object_p prog    = rt.make<program>(ID_block, scratch, alloc);
        if (step == 1)
            obj1 = prog;
        else
            obj2 = prog;
    }

    size_t parsed = utf8(src) - utf8(p.source);
    p.end         = parsed;
    p.out         = steps == 2
        ? rt.make<loop>(type, obj1)
        : rt.make<conditional_loop>(type, obj1, obj2);

    return OK;
}


intptr_t loop::object_renderer(renderer &r,
                               uint      nseps,
                               cstring   separators[]) const
// ----------------------------------------------------------------------------
//   Render the loop into the given buffer
// ----------------------------------------------------------------------------
{
    // Source objects
    byte_p   p      = payload();

    // Isolate condition and body
    object_g first  = object_p(p);
    object_g second = nseps == 3 ? first->skip() : nullptr;
    uint     sep    = 0;
    auto     format = Settings.command_fmt;

    // Write the header, e.g. "DO", and indent condition
    r.put('\n');
    r.put(format, utf8(separators[sep++]));
    r.indent();

    // Emit the first object (e.g. condition in do-until)
    first->render(r);

    // Emit the second object if there is one
    if (second)
    {
        // Emit separator after condition
        r.unindent();
        r.put(format, utf8(separators[sep++]));
        r.indent();
        second->render(r);
    }

    // Emit closing separator
    r.unindent();
    r.put(format, utf8(separators[sep++]));

    return r.size();
}



// ============================================================================
//
//   DO...UNTIL...END loop
//
// ============================================================================

PARSE_BODY(DoUntil)
// ----------------------------------------------------------------------------
//  Parser for do-unti loops
// ----------------------------------------------------------------------------
{
    return conditional_loop::object_parser(ID_DoUntil, p,
                                           "do", "until", "end");
}


RENDER_BODY(DoUntil)
// ----------------------------------------------------------------------------
//   Renderer for do-until loop
// ----------------------------------------------------------------------------
{
    return o->object_renderer(r, "do", "until", "end");
}


INSERT_BODY(DoUntil)
// ----------------------------------------------------------------------------
//   Insert a do-until loop in the editor
// ----------------------------------------------------------------------------
{
    return i.edit(utf8("do  until  end"), input::PROGRAM, 3);
}


EVAL_BODY(DoUntil)
// ----------------------------------------------------------------------------
//   Evaluate a do..until..end loop
// ----------------------------------------------------------------------------
//   In this loop, the body comes first
{
    byte    *p    = (byte *) o->payload();
    object_g body = object_p(p);
    object_g cond = body->skip();
    result   r    = OK;

    while (!interrupted() && r == OK)
    {
        r = body->evaluate();
        if (r != OK)
            break;
        r = cond->evaluate();
        if (r != OK)
            break;
        bool test = false;
        r = o->condition(test);
        if (r != OK || test)
            break;
    }
    return r;
}


// ============================================================================
//
//   WHILE...REPEAT...END loop
//
// ============================================================================

PARSE_BODY(WhileRepeat)
// ----------------------------------------------------------------------------
//  Parser for while loops
// ----------------------------------------------------------------------------
{
    return conditional_loop::object_parser(ID_WhileRepeat, p,
                                           "while", "repeat", "end");
}


RENDER_BODY(WhileRepeat)
// ----------------------------------------------------------------------------
//   Renderer for while loop
// ----------------------------------------------------------------------------
{
    return o->object_renderer(r, "while", "repeat", "end");
}


INSERT_BODY(WhileRepeat)
// ----------------------------------------------------------------------------
//   Insert a while loop in the editor
// ----------------------------------------------------------------------------
{
    return i.edit(utf8("while  repeat  end"), input::PROGRAM, 6);
}


EVAL_BODY(WhileRepeat)
// ----------------------------------------------------------------------------
//   Evaluate a while..repeat..end loop
// ----------------------------------------------------------------------------
//   In this loop, the condition comes first
{
    byte    *p    = (byte *) o->payload();
    object_g cond = object_p(p);
    object_g body = cond->skip();
    result   r    = OK;

    while (!interrupted() && r == OK)
    {
        r = cond->evaluate();
        if (r != OK)
            break;
        bool test = false;
        r = condition(test);
        if (r != OK || !test)
            break;
        r = body->evaluate();
    }
    return r;
}



// ============================================================================
//
//   START...NEXT loop
//
// ============================================================================

PARSE_BODY(StartNext)
// ----------------------------------------------------------------------------
//  Parser for start-next loops
// ----------------------------------------------------------------------------
{
    return object_parser(ID_StartNext, p, "start", "next");
}


RENDER_BODY(StartNext)
// ----------------------------------------------------------------------------
//   Renderer for start-next loop
// ----------------------------------------------------------------------------
{
    return o->object_renderer(r, "start", "next");
}


INSERT_BODY(StartNext)
// ----------------------------------------------------------------------------
//   Insert a start-next loop in the editor
// ----------------------------------------------------------------------------
{
    return i.edit(utf8("start  next"), input::PROGRAM, 6);
}


object::result loop::counted(object_g body, bool stepping)
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
        rt.type_error();
        return ERROR;
    }

    // Pop them from the stack
    rt.pop();
    rt.pop();

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
            object_p step = rt.pop();
            if (!step)
                return ERROR;
            integer_p istep = step->as<integer>();
            if (!istep)
            {
                rt.type_error();
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


EVAL_BODY(StartNext)
// ----------------------------------------------------------------------------
//   Evaluate a for..next loop
// ----------------------------------------------------------------------------
{
    byte    *p    = (byte *) o->payload();
    object_p body = object_p(p);
    return counted(body, false);
}


// ============================================================================
//
//   START...STEP loop
//
// ============================================================================

PARSE_BODY(StartStep)
// ----------------------------------------------------------------------------
//  Parser for start-step loops
// ----------------------------------------------------------------------------
{
    return object_parser(ID_StartStep, p, "start", "step");
}


RENDER_BODY(StartStep)
// ----------------------------------------------------------------------------
//   Renderer for start-step loop
// ----------------------------------------------------------------------------
{
    return o->object_renderer(r, "start", "step");
}


INSERT_BODY(StartStep)
// ----------------------------------------------------------------------------
//   Insert a start-step loop in the editor
// ----------------------------------------------------------------------------
{
    return i.edit(utf8("start  step"), input::PROGRAM, 6);
}


EVAL_BODY(StartStep)
// ----------------------------------------------------------------------------
//   Evaluate a for..step loop
// ----------------------------------------------------------------------------
{
    byte    *p    = (byte *) o->payload();
    object_p body = object_p(p);
    return counted(body, true);
}



// ============================================================================
//
//   FOR...NEXT loop
//
// ============================================================================

PARSE_BODY(ForNext)
// ----------------------------------------------------------------------------
//  Parser for for-next loops
// ----------------------------------------------------------------------------
{
    return object_parser(ID_ForNext, p, "for", "next");
}


RENDER_BODY(ForNext)
// ----------------------------------------------------------------------------
//   Renderer for for-next loop
// ----------------------------------------------------------------------------
{
    return o->object_renderer(r, "for", "next");
}


INSERT_BODY(ForNext)
// ----------------------------------------------------------------------------
//   Insert a for-next loop in the editor
// ----------------------------------------------------------------------------
{
    return i.edit(utf8("for  next"), input::PROGRAM, 4);
}


EVAL_BODY(ForNext)
// ----------------------------------------------------------------------------
//   Evaluate a for..next loop
// ----------------------------------------------------------------------------
{
    byte    *p    = (byte *) o->payload();
    object_p body = object_p(p);
    return counted(body, false);
}



// ============================================================================
//
//   FOR...STEP loop
//
// ============================================================================

PARSE_BODY(ForStep)
// ----------------------------------------------------------------------------
//  Parser for for-step loops
// ----------------------------------------------------------------------------
{
    return object_parser(ID_ForStep, p, "for", "step");
}


RENDER_BODY(ForStep)
// ----------------------------------------------------------------------------
//   Renderer for for-step loop
// ----------------------------------------------------------------------------
{
    return o->object_renderer(r, "for", "step");
}


INSERT_BODY(ForStep)
// ----------------------------------------------------------------------------
//   Insert a for-step loop in the editor
// ----------------------------------------------------------------------------
{
    return i.edit(utf8("for  step"), input::PROGRAM, 4);
}


EVAL_BODY(ForStep)
// ----------------------------------------------------------------------------
//   Evaluate a for..step loop
// ----------------------------------------------------------------------------
{
    byte    *p    = (byte *) o->payload();
    object_p body = object_p(p);
    return counted(body, true);
}
