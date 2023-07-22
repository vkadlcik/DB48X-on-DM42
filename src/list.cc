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

#include "algebraic.h"
#include "array.h"
#include "equation.h"
#include "parser.h"
#include "precedence.h"
#include "program.h"
#include "renderer.h"
#include "runtime.h"
#include "utf8.h"

#include <stdio.h>


RECORDER(list, 16, "Lists");
RECORDER(list_parse, 16, "List parsing");
RECORDER(list_error, 16, "Errors processing lists");

object::result list::list_parse(id type,
                                parser  &p,
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
    gcutf8   s          = p.source;
    size_t   max        = p.length;
    object_g infix      = nullptr;
    object_g prefix     = nullptr;
    bool     negate     = false;
    int      precedence = p.precedence;
    int      lowest     = precedence;

    record(list, "Parse %+s %lc%lc precedence %d length %u [%s]",
           p.child ? "top-level" : "child", open, close, precedence, max,
           utf8(s));

    // Check if we have the opening marker
    unicode cp  = 0;
    if (open)
    {
        cp = utf8_codepoint(s);
        if (cp != open)
            return SKIP;
        s = utf8_next(s);
    }

    scribble scr;
    while (utf8_more(p.source, s, max))
    {
        cp = utf8_codepoint(s);
        if (cp == close)
        {
            s = utf8_next(s);
            break;
        }
        if (precedence && (cp == '\'' || cp == ')'))
            break;
        if (utf8_whitespace(cp))
        {
            s = utf8_next(s);
            continue;
        }

        // Parse an object
        size_t   done    = (byte *) s - (byte *) p.source;
        size_t   length  = max > done ? max - done : 0;
        object_g obj     = nullptr;
        bool     postfix = false;

        // For algebraic objects, check if we have or need parentheses
        if (precedence)
        {
            if (precedence > 0)
            {
                // Check to see if we have a sign
                if (cp == '-' || cp == '+')
                {
                    if (cp == '-')
                        negate = !negate;
                    s = utf8_next(s);
                    continue;
                }

                // Check if we see parentheses, or if we have `sin sin X`
                bool parenthese = cp == '(';
                if (parenthese  || infix || prefix)
                {
                    int childp = parenthese ? LOWEST
                        : infix ? infix->precedence() + 1
                        : SYMBOL;
                    parser child(p, s, childp);
                    unicode iopen = parenthese ? '(' : 0;
                    unicode iclose = parenthese ? ')' : 0;

                    record(list_parse, "%+s starting at offset %u '%s'",
                           parenthese ? "Parenthese" : "Child",
                           utf8(s) - utf8(p.source),
                           utf8(s));
                    auto result = list_parse(type, child, iopen, iclose);
                    if (result != OK)
                        return result;
                    obj = child.out;
                    if (!obj)
                        return ERROR;
                    length = child.end;
                    record(list_parse,
                           "Child parsed as %t length %u", object_p(obj), length);
                }
            }
            else // precedence < 0)
            {
                // Check special postfix notations
                id cmd = id(0);
                switch(cp)
                {
                case L'²':
                    cmd = ID_sq;
                    break;
                case L'³':
                    cmd = ID_cubed;
                    break;
                case '!':
                    cmd = ID_fact;
                    break;
                case L'⁻':
                    if (utf8_codepoint(utf8_next(s)) == L'¹')
                        cmd = ID_inv;
                    break;
                default:
                    break;
                }
                if (cmd)
                {
                    utf8 cur = utf8(s);
                    obj = command::static_object(cmd);
                    length = cmd == ID_inv
                        ? utf8_next(utf8_next(cur)) - cur
                        : utf8_next(cur) - cur;
                    postfix = true;
                    precedence = -precedence; // Stay in postfix mode
                }
            }
        }

        if (!obj)
        {
            obj = object::parse(s, length, precedence);
            record(list_parse,
                   "Item parsed as %t length %u", object_p(obj), length);
        }
        if (!obj)
            return ERROR;

        if (precedence && !postfix && obj->type() != ID_symbol)
        {
            // We are parsing an equation
            if (precedence > 0)
            {
                // We just parsed an algebraic, e.g. 'sin', etc
                // stash it and require parentheses for arguments
                id type = obj->type();
                if (!is_algebraic(type) && !is_symbolic(type))
                {
                    rt.prefix_expected_error();
                    return ERROR;
                }

                // TODO: A symbol could be a function, need to deal with that
                if (is_algebraic(type))
                {
                    prefix = obj;
                    obj = nullptr;
                    precedence = -SYMBOL;
                }
            }
            else
            {
                // We just parsed an infix, e.g. +, -, etc
                // stash it, or exit loop if it has lower precedence
                int objprec = obj->precedence();
                if (objprec < lowest)
                    break;
                if (!objprec)
                {
                    rt.infix_expected_error();
                    return ERROR;
                }
                if (objprec < SYMBOL)
                {
                    infix = obj;
                    precedence = -objprec;
                    obj = nullptr;
                }
            }
        }

        if (obj)
        {
            // Copy the parsed object to the scratch pad (may GC)
            do
            {
                record(list_parse, "Copying %t to scratchpad", object_p(obj));

                size_t objsize = obj->size();

                // For equations, copy only the payload
                if (precedence && obj->type() == ID_equation)
                    obj = (object_p) equation_p(object_p(obj))->value(&objsize);

                byte *objcopy = rt.allocate(objsize);
                if (!objcopy)
                    return ERROR;
                memmove(objcopy, (byte *) obj, objsize);
                if (prefix)
                {
                    obj = prefix;
                    prefix = nullptr;
                }
                else if (negate)
                {
                    obj = command::static_object(ID_neg);
                    negate = false;
                }
                else
                {
                    obj = infix;
                    infix = nullptr;
                }
            } while (obj);
        }

        // Jump past what we parsed
        s = utf8(s) + length;

        // For equations switch between infix and prefix
        precedence = -precedence;
    }

    record(list, "Exiting parser at %s infix=%t prefix=%p",
           utf8(s), object_p(infix), object_p(prefix));


    // If we still have a pending opcode here, syntax error (e.g. '1+`)
    if (infix || prefix)
    {
        if (infix)
            rt.command(infix->fancy());
        else if (prefix)
            rt.command(prefix->fancy());
        rt.argument_expected_error();
        return ERROR;
    }

    // Check that we have a matching closing character
    if (close && cp != close && !p.child)
    {
        record(list_error, "Missing terminator, got %u (%c) not %u (%c) at %s",
               cp, cp, close, close, utf8(s));
        rt.unterminated_error().source(p.source);
        return ERROR;
    }

    // Create the object
    gcbytes scratch = scr.scratch();
    size_t  alloc   = scr.growth();
    size_t  parsed  = utf8(s) - utf8(p.source);
    p.end           = parsed;
    p.out           = rt.make<list>(type, scratch, alloc);

    record(list_parse, "Parsed as %t length %u", object_p(p.out), parsed);

    // Return success
    return OK;
}


intptr_t list::list_render(renderer &r, unicode open, unicode close) const
// ----------------------------------------------------------------------------
//   Render the list into the given buffer
// ----------------------------------------------------------------------------
{
    // Write the header, e.g. "{ "
    if (open)
        r.put(open);

    // Loop on all objects inside the list
    for (object_p obj : *this)
    {
        // Add space separator (except on first object when no separator)
        if (open)
            r.put(' ');
        open = 1;

        // Render the object in what remains (may GC)
        obj->render(r);
    }

    // Add final space and closing separator
    if (close)
    {
        r.put(' ');
        r.put(close);
    }

    return r.size();
}


PARSE_BODY(list)
// ----------------------------------------------------------------------------
//    Try to parse this as an list
// ----------------------------------------------------------------------------
{
    return list_parse(ID_list, p, '{', '}');
}


RENDER_BODY(list)
// ----------------------------------------------------------------------------
//   Render the list into the given list buffer
// ----------------------------------------------------------------------------
{
    return o->list_render(r, '{', '}');
}



// ============================================================================
//
//   Command implementation
//
// ============================================================================

COMMAND_BODY(ToList)
// ----------------------------------------------------------------------------
//   Convert elements to a list
// ----------------------------------------------------------------------------
{
    uint32_t depth = 0;
    if (stack(&depth))
    {
        if (rt.depth() < depth + 1)
        {
            rt.missing_argument_error();
            return ERROR;
        }

        if (rt.pop())
        {
            scribble scr;
            for (uint i = 0; i < depth; i++)
            {
                if (object_g obj = rt.stack(depth - 1 - i))
                {
                    size_t objsz = obj->size();
                    byte_p objp = byte_p(obj);
                    if (!rt.append(objsz, objp))
                        return ERROR;
                }
            }
            object_g list = list::make(scr.scratch(), scr.growth());
            if (!rt.drop(depth))
                return ERROR;
            if (rt.push(list))
                return OK;
        }
    }
    return ERROR;
}


COMMAND_BODY(Get)
// ----------------------------------------------------------------------------
//   Get an element in a list
// ----------------------------------------------------------------------------
{
    uint32_t index = 0;
    if (stack(&index))
    {
        if (object_p obj = rt.stack(1))
        {
            id ty = obj->type();
            if (ty == ID_list || ty == ID_array)
            {
                list_p list = list_p(obj);
                if (object_p obj = (*list)[index-1])
                {
                    if (rt.drop())
                        if (rt.top(obj))
                            return OK;
                }
                else
                {
                    rt.value_error();
                }
            }
            else if (ty == ID_text)
            {
                text_p text = text_p(obj);
                size_t sz = 0;
                utf8 chars = text->value(&sz);
                uint i;
                for (i = 0; i < sz && i + 1 < index; i++)
                    chars = utf8_next(chars);
                if (index && i < sz)
                {
                    unicode cp = utf8_codepoint(chars);
                    size_t cpsz = utf8_size(cp);
                    text_g extracted = text::make(chars, cpsz);
                    if (extracted)
                        if (rt.drop())
                            if (rt.top(extracted.Safe()))
                                return OK;
                }
                else
                {
                    rt.value_error();
                }
            }
            else
            {
                rt.type_error();
            }
        }
    }
    return ERROR;
}
