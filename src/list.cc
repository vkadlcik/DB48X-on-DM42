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
#include "compare.h"
#include "expression.h"
#include "grob.h"
#include "parser.h"
#include "precedence.h"
#include "program.h"
#include "renderer.h"
#include "runtime.h"
#include "symbol.h"
#include "utf8.h"
#include "variables.h"

#include <stdio.h>
#include <stdlib.h>


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
    object_g postfix    = nullptr;
    object_g obj        = nullptr;
    bool     negate     = false;
    int      precedence = p.precedence;
    int      lowest     = precedence;
    size_t   objcount   = 0;
    size_t   non_alg    = 0;
    size_t   non_alg_len= 0;

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
        cp = 0;                 // Do not accept "'" as an empty equation
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
        size_t done        = (byte *) s - (byte *) p.source;
        size_t length      = max > done ? max - done : 0;
        id     postfix_cmd = id(0);

        // For algebraic objects, check if we have or need parentheses
        if (precedence && length)
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
                bool parenthese = cp == '(' && !infix;
                if (parenthese  || infix || prefix)
                {
                    int childp = infix ? infix->precedence() + 1
                        : parenthese ? LOWEST
                        : SYMBOL;
                    parser child(p, s, childp);
                    unicode iopen = parenthese ? '(' : 0;
                    unicode iclose = parenthese ? ')' : 0;
                    id ctype = type == ID_unit ? ID_expression : type;

                    record(list_parse, "%+s starting at offset %u '%s'",
                           parenthese ? "Parenthese" : "Child",
                           utf8(s) - utf8(p.source),
                           utf8(s));
                    auto result = list_parse(ctype, child, iopen, iclose);
                    if (result != OK)
                        return result;
                    obj = child.out;
                    if (!obj)
                        return ERROR;
                    s = s + child.end;
                    record(list_parse,
                           "Child parsed as %t length %u",
                           object_p(obj), child.end);
                    precedence = -precedence; // Stay in postfix mode
                    cp = utf8_codepoint(s);
                    length = 0;
                }
            }
            if (precedence < 0)
            {
                // Check special postfix notations
                switch(cp)
                {
                case L'²':
                    postfix_cmd = ID_sq;
                    break;
                case L'³':
                    postfix_cmd = ID_cubed;
                    break;
                case '!':
                    postfix_cmd = ID_fact;
                    break;
                case L'⁻':
                    if (utf8_codepoint(utf8_next(s)) == L'¹')
                        postfix_cmd = ID_inv;
                    break;
                default:
                    break;
                }
                if (postfix_cmd)
                {
                    utf8 cur = utf8(s);
                    (obj ? postfix : obj) = command::static_object(postfix_cmd);
                    length = postfix_cmd == ID_inv
                        ? utf8_next(utf8_next(cur)) - cur
                        : utf8_next(cur) - cur;
                    precedence = -precedence; // Stay in postfix mode
                }
            }
        }

        if (!obj && length)
        {
            obj = object::parse(s, length, precedence);
            record(list_parse,
                   "Item parsed as %t length %u", object_p(obj), length);
        }
        if (!obj)
            return ERROR;

        if (precedence && !postfix_cmd)
        {
            // We are parsing an equation
            if (precedence > 0)
            {
                // We just parsed an algebraic, e.g. 'sin', etc
                // stash it and require parentheses for arguments
                id type = obj->type();
                if (!is_algebraic(type))
                {
                    if (objcount)
                    {
                        rt.prefix_expected_error().source(s, length);
                        return ERROR;
                    }
                    non_alg = +s- +p.source;
                    non_alg_len = length;
                }

                // TODO: A symbol could be a function, need to deal with that
                if (is_algebraic_fn(type))
                {
                    prefix = obj;
                    obj = nullptr;
                    precedence = -SYMBOL;
                }
            }
            else if (int objprec = obj->precedence())
            {
                // We just parsed an infix, e.g. +, -, etc
                // stash it, or exit loop if it has lower precedence
                if (objprec < lowest)
                    break;
                if (objprec < FUNCTIONAL)
                {
                    infix = obj;
                    precedence = -objprec;
                    obj = nullptr;
                }
            }
            else
            {
                precedence = -precedence;
            }
        }

        if (obj)
        {
            // Copy the parsed object to the scratch pad (may GC)
            do
            {
                record(list_parse, "Copying %t to scratchpad", object_p(obj));
                objcount++;

                size_t objsize = obj->size();

                // For equations, copy only the payload
                if (precedence)
                    if (expression_p eq = obj->as<expression>())
                        obj = eq->objects(&objsize);

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
                else if (postfix)
                {
                    obj = postfix;
                    postfix = nullptr;
                }
                else
                {
                    obj = infix;
                    infix = nullptr;
                }
            } while (obj);
        }

        // Jump past what we parsed
        s = s + length;

        // For equations switch between infix and prefix
        precedence = -precedence;
    }

    record(list, "Exiting parser at %s infix=%t prefix=%p",
           utf8(s), object_p(infix), object_p(prefix));


    // If we still have a pending opcode here, syntax error (e.g. '1+`)
    if (infix || prefix)
    {
        if (infix)
            rt.command(infix);
        else if (prefix)
            rt.command(prefix);
        rt.argument_expected_error();
        return ERROR;
    }

    if (non_alg && objcount != 1)
    {
        rt.syntax_error().source(p.source + non_alg, non_alg_len);
        return ERROR;
    }

    // Check that we have a matching closing character
    if (close && cp != close && !p.child)
    {
        record(list_error, "Missing terminator, got %u (%c) not %u (%c) at %s",
               cp, cp, close, close, utf8(s));
        rt.unterminated_error().source(p.source, +s - +p.source);
        return ERROR;
    }

    // Create the object
    gcbytes scratch = scr.scratch();
    size_t  alloc   = scr.growth();
    size_t  parsed  = utf8(s) - utf8(p.source);

    // Check for the case of an empty equation
    if (alloc == 0 && type == ID_expression)
    {
        record(list_error, "Empty equation");
        rt.syntax_error().source(p.source);
        return ERROR;
    }

    // Create the object
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
    // Check if we need an indent in the body
    bool need_indent = false;
    for (object_p obj : *this)
    {
        switch(obj->type())
        {
        case ID_list:
        case ID_program:
        case ID_array:
        case ID_locals:
        case ID_comment:
        case ID_IfThen:
        case ID_IfThenElse:
        case ID_DoUntil:
        case ID_WhileRepeat:
        case ID_StartStep:
        case ID_ForNext:
        case ID_ForStep:
        case ID_IfErrThen:
        case ID_IfErrThenElse:
            need_indent = true;
            break;
        default:
            break;
        }
        if (need_indent)
            break;
    }

    // Write the header, e.g. "{ "
    if (open)
    {
        r.put(open);
        if (need_indent)
            r.indent();
    }

    // Loop on all objects inside the list
    for (object_p obj : *this)
    {
        // Add space separator (except on first object when no separator)
        if (open && !r.hadCR())
            r.put(' ');
        open = 1;

        // Render the object in what remains (may GC)
        obj->render(r);
    }

    // Add final space and closing separator
    if (close)
    {
        if (need_indent)
            r.unindent();
        else if (open == 1)
            r.put(' ');
        r.put(close);
    }
    r.wantCR();

    return r.size();
}


list_p list::append(list_p a) const
// ----------------------------------------------------------------------------
//   Append the second list to this one
// ----------------------------------------------------------------------------
{
    text_g x = text_p(this);
    text_g y = text_p(a);
    return list_p(+(x + y));
}


list_p list::append(object_p o) const
// ----------------------------------------------------------------------------
//   Append object to list
// ----------------------------------------------------------------------------
{
    text_g x = text_p(this);
    text_g y = text::make(byte_p(o), o->size());
    return list_p(+(x + y));
}


bool list::expand_without_size() const
// ----------------------------------------------------------------------------
//   Expand items on the stack, but do not add the size
// ----------------------------------------------------------------------------
{
    size_t depth = rt.depth();
    for (object_p obj : *this)
    {
        if (!rt.push(obj))
        {
            rt.drop(rt.depth() - depth);
            return false;
        }
    }
    return true;
}


bool list::expand() const
// ----------------------------------------------------------------------------
//   Expand items on
// ----------------------------------------------------------------------------
{
    size_t depth = rt.depth();
    if (expand_without_size())
        if (rt.push(integer::make(rt.depth() - depth)))
            return true;
    rt.drop(rt.depth() - depth);
    return false;
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


HELP_BODY(list)
// ----------------------------------------------------------------------------
//   Help topic for lists
// ----------------------------------------------------------------------------
{
    return utf8("Lists");
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
    uint32_t depth = uint32_arg();
    if (!rt.error())
    {
        if (!rt.args(depth + 1))
            return ERROR;

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


COMMAND_BODY(FromList)
// ----------------------------------------------------------------------------
//   Convert elements to a list
// ----------------------------------------------------------------------------
{
    if (rt.args(1))
    {
        object_p obj = rt.top();
        if (list_p li = obj->as<list>())
        {
            rt.drop();
            if (li->expand())
                return OK;
        }
        else
        {
            rt.type_error();
        }
    }
    return ERROR;
}


COMMAND_BODY(Size)
// ----------------------------------------------------------------------------
//   Return the size of an object
// ----------------------------------------------------------------------------
//   Behaves differently from standard RPL for integers, equations and units
//   where it returns 1 and not some weirdo internal count
{
    if (rt.args(1))
    {
        object_p obj    = rt.top();
        id       oty    = obj->type();
        size_t   size   = 1;

        switch (oty)
        {
        case ID_list:
            size = list_p(obj)->items(); break;
        case ID_array:
            if (object_p result = array_p(obj)->dimensions())
                if (rt.top(result))
                    return OK;
            break;
        case ID_text:
            size = text_p(obj)->utf8_characters(); break;
        case ID_grob:
        case ID_bitmap:
            if (grob_p gr = grob_p(obj))
            {
                grob::pixsize w = 0, h = 0;
                if (gr->pixels(&w, &h))
                {
                    integer_g wo = rt.make<based_integer>(w);
                    integer_g ho = rt.make<based_integer>(h);
                    if (wo && ho && rt.top(+wo) && rt.push(+ho))
                        return OK;
                }
            }
            return ERROR;
        default:
            break;
        }

        integer_p szo = integer::make(size);
        if (szo && rt.top(szo))
            return OK;
    }
    return ERROR;
}


static object::result get(bool increment)
// ----------------------------------------------------------------------------
//   Get element from structure, incrementing index or not
// ----------------------------------------------------------------------------
{
    if (!rt.args(2))
        return object::ERROR;

    // Check we have an object at level 2
    if (object_p items = rt.stack(1))
    {
        if (symbol_p name = items->as_quoted<symbol>())
        {
            items = directory::recall_all(name, true);
            if (!items)
                return object::ERROR;
        }

        object_p item = items->at(rt.stack(0));
        if (!item)
        {
            if (!rt.error())
                rt.index_error();
        }
        else if (increment)
        {
            rt.push(item);
            object_g index = rt.stack(1);
            bool wrap = items->next_index(&+index);
            if (index)
            {
                rt.stack(1, index);
                Settings.IndexWrapped(wrap);
                return object::OK;
            }
        }
        else if (rt.pop() && rt.top(item))
        {
            return object::OK;
        }
    }
    return object::ERROR;
}


COMMAND_BODY(Get)
// ----------------------------------------------------------------------------
//   Get an element in a list
// ----------------------------------------------------------------------------
{
    return get(false);
}


COMMAND_BODY(GetI)
// ----------------------------------------------------------------------------
//   Get an element in a list and increment the index
// ----------------------------------------------------------------------------
{
    return get(true);
}


static object::result put(bool increment)
// ----------------------------------------------------------------------------
//   Put element in structure, incrementing index or not
// ----------------------------------------------------------------------------
{
    if (!rt.args(3))
        return object::ERROR;

    // Check that we have an object at level 2
    if (object_p items = rt.stack(2))
    {
        symbol_p name = items->as_quoted<symbol>();
        if (name)
        {
            items = directory::recall_all(name, true);
            if (!items)
                return object::ERROR;
        }

        if (object_g result = items->at(rt.stack(1), rt.top()))
        {
            if (increment)
            {
                object_g index = rt.stack(1);
                bool wrap = result->next_index(&+index);
                if (index)
                {
                    rt.stack(1, +index);
                    Settings.IndexWrapped(wrap);
                }
            }
            if (name)
            {
                name = rt.stack(2)->as_quoted<symbol>();
                if (directory::update(name, result))
                {
                    rt.drop(increment ? 1 : 3);
                    return object::OK;
                }
            }
            else
            {
                if (rt.drop(increment ? 1 : 2))
                    if (rt.stack(increment ? 1 : 0, result))
                        return object::OK;
            }
        }

        if(!rt.error())
            rt.index_error();
    }
    return object::ERROR;
}


COMMAND_BODY(Put)
// ----------------------------------------------------------------------------
//   Put an element in a list
// ----------------------------------------------------------------------------
{
    return put(false);
}


COMMAND_BODY(PutI)
// ----------------------------------------------------------------------------
//   Put an element in a list, incrementing the index
// ----------------------------------------------------------------------------
{
    return put(true);
}


COMMAND_BODY(Head)
// ----------------------------------------------------------------------------
//   Return first element in a list
// ----------------------------------------------------------------------------
{
    if (rt.args(1))
    {
        object_p obj = rt.top();
        id ty = obj->type();
        if (ty == ID_list || ty == ID_array)
        {
            if (object_p hd = list_p(obj)->head())
            {
                if (rt.top(hd))
                    return OK;
            }
            else
            {
                rt.dimension_error();
            }
        }
        else
        {
            rt.type_error();
        }
    }
    return ERROR;
}


COMMAND_BODY(Tail)
// ----------------------------------------------------------------------------
//   Return all but first element in a list
// ----------------------------------------------------------------------------
{
    if (rt.args(1))
    {
        object_p obj = rt.top();
        id ty = obj->type();
        if (ty == ID_list || ty == ID_array)
        {
            if (object_p tl = list_p(obj)->tail())
            {
                if (rt.top(tl))
                    return OK;
            }
            else
            {
                rt.dimension_error();
            }
        }
        else
        {
            rt.type_error();
        }
    }
    return ERROR;
}


static object::result map_reduce_filter(object_p (list::*cmd)(object_p) const)
// ----------------------------------------------------------------------------
//   Shared code for map, reduce and filter
// ----------------------------------------------------------------------------
{
    size_t depth = rt.depth();
    if (rt.args(2))
    {
        object_p   obj = rt.stack(1);
        object_g   prg = rt.top();
        object::id ty  = obj->type();
        if (ty == object::ID_list || ty == object::ID_array)
        {
            object_p result = (list_p(obj)->*cmd)(prg);
            if (!result)
                goto error;
            if (rt.drop() && rt.top(result))
                return object::OK;
        }
        else
        {
            rt.type_error();
        }
    }
error:
    if (rt.depth() > depth)
        rt.drop(rt.depth() - depth);
    return object::ERROR;
}


COMMAND_BODY(Map)
// ----------------------------------------------------------------------------
//   Apply unary function in level 1 to all elements in level 2
// ----------------------------------------------------------------------------
{
    return map_reduce_filter(&list::map_as_object);
}


COMMAND_BODY(Reduce)
// ----------------------------------------------------------------------------
//   Apply binary function in level 1 pairwise to combine elements in level 2
// ----------------------------------------------------------------------------
{
    return map_reduce_filter(&list::reduce);
}


COMMAND_BODY(Filter)
// ----------------------------------------------------------------------------
//   Filter the function in level 1 to all elements in level 2
// ----------------------------------------------------------------------------
{
    return map_reduce_filter(&list::filter_as_object);
}


algebraic_p sum_product(symbol_g  name,
                        large     a,
                        large     b,
                        program_g expr,
                        bool      product)
// ----------------------------------------------------------------------------
//   Perform a sum or product
// ----------------------------------------------------------------------------
{
    algebraic_g result = integer::make(uint(product));
    algebraic_g value;
    save<symbol_g *> iref(expression::independent, &name);

    for (large i = a; i <= b; i++)
    {
        value = integer::make(i);
        value = algebraic::evaluate_function(expr, value);
        if (!value)
            return nullptr;
        result = product ? result * value : result + value;
    }

    return result;
}


static object::result pair_map(object::id cmd)
// ----------------------------------------------------------------------------
//   Shared code for map, reduce and filter
// ----------------------------------------------------------------------------
{
    size_t depth = rt.depth();
    if (rt.args(1))
    {
        object_p   obj = rt.stack(0);
        object::id ty  = obj->type();
        if (ty == object::ID_list || ty == object::ID_array)
        {
            object_p result = list_p(obj)->reduce(command::static_object(cmd));
            if (result && rt.top(result))
                return object::OK;
        }
        else if (rt.args(4))
        {
            symbol_g name = rt.stack(3)->as_quoted<symbol>();
            if (!name)
                goto error;
            object_g init = rt.stack(2);
            object_g last = rt.stack(1);
            object_g expr = rt.stack(0);
            if (!expr->is_program())
            {
                rt.type_error();
                goto error;
            }

            if (init->is_symbolic() || last->is_symbolic())
            {
                object_g prg = command::static_object(cmd);
                expr = rt.make<list>(object::ID_expression,
                                     name, init, last, expr, prg);
                if (expr && rt.drop(3) && rt.top(expr))
                    return object::OK;
                goto error;
            }
            else if (init->is_integer() && last->is_integer())
            {
                bool      prod = cmd == object::ID_Product;
                program_g prg  = program_p(+expr);
                large     a    = init->as_int64();
                large     b    = last->as_int64();
                expr           = sum_product(name, a, b, prg, prod);
                if (expr && rt.drop(3) && rt.top(expr))
                    return object::OK;
            }
            else
            {
                rt.type_error();
            }
        }
        else
        {
            rt.type_error();
        }
    }
error:
    if (rt.depth() > depth)
        rt.drop(rt.depth() - depth);
    return object::ERROR;
}


COMMAND_BODY(Sum)
// ----------------------------------------------------------------------------
//   Return the sum of a list or array
// ----------------------------------------------------------------------------
{
    return pair_map(ID_add);
}


COMMAND_BODY(Product)
// ----------------------------------------------------------------------------
//   Return the product of a list or array
// ----------------------------------------------------------------------------
{
    return pair_map(ID_mul);
}


object_p list::head() const
// ----------------------------------------------------------------------------
//   Return the first element in the list
// ----------------------------------------------------------------------------
{
    size_t   size  = 0;
    object_p first = objects(&size);
    if (!size)
        return nullptr;
    return first;
}


list_p list::tail() const
// ----------------------------------------------------------------------------
//   Return the tail elements of the list
// ----------------------------------------------------------------------------
{
    size_t   size  = 0;
    object_p first = objects(&size);
    if (!size)
        return nullptr;
    size_t osize = first->size();
    byte_p rest  = byte_p(first) + osize;
    return list::make(type(), rest, size - osize);
}


list_p list::map(object_p prgobj) const
// ----------------------------------------------------------------------------
//   Apply an RPL object (nominally a program) on all elements in the list
// ----------------------------------------------------------------------------
{
    id       ty    = type();
    object_g prg   = prgobj;
    size_t   depth = rt.depth();
    scribble scr;
    for (object_p obj : *this)
    {
        id oty = obj->type();
        if (oty == ID_array || oty == ID_list)
        {
            list_g sub = list_p(obj)->map(prg);
            obj = +sub;
        }
        else
        {
            if (!rt.push(obj))
                goto error;
            if (program::run(prg, true) != OK)
                goto error;
            if (rt.depth() != depth + 1)
            {
                rt.misbehaving_program_error();
                goto error;
            }
            obj = rt.pop();
        }
        if (!obj)
            goto error;

        size_t objsz = obj->size();
        byte_p objp = byte_p(obj);
        if (!rt.append(objsz, objp))
            goto error;
    }

    return list::make(ty, scr.scratch(), scr.growth());

error:
    if (rt.depth() > depth)
        rt.drop(rt.depth() - depth);
    return nullptr;

}


object_p list::reduce(object_p prgobj) const
// ----------------------------------------------------------------------------
//   Apply an RPL object (nominally a program) on pairs of list elements
// ----------------------------------------------------------------------------
{
    object_g prg    = prgobj;
    size_t   depth  = rt.depth();
    object_g result = nullptr;
    for (object_p obj : *this)
    {
        if (!rt.push(obj))
            goto error;
        if (!result)
        {
            result = obj;
        }
        else
        {
            if (program::run(prg, true)!= OK)
                goto error;
            if (rt.depth() != depth + 1)
                rt.misbehaving_program_error();
            result = rt.top();
        }
        if (rt.error())
            goto error;
    }
    if (rt.depth() > depth)
        rt.drop(rt.depth() - depth);
    return result;

error:
    if (rt.depth() > depth)
        rt.drop(rt.depth() - depth);
    return nullptr;
}


list_p list::filter(object_p prgobj) const
// ----------------------------------------------------------------------------
//   Apply an RPL object (nominally a program) to filter elements in a list
// ----------------------------------------------------------------------------
{
    id       ty    = type();
    object_g prg   = prgobj;
    size_t   depth = rt.depth();
    scribble scr;
    for (object_g obj : *this)
    {
        id   oty  = obj->type();
        bool keep = false;
        if (oty == ID_array || oty == ID_list)
        {
            object_g sub = list_p(+obj)->filter(prg);
            obj = +sub;
            keep = true;
        }
        else
        {
            if (!rt.push(obj))
                goto error;
            if (program::run(prg, true)!= OK)
                goto error;
            if (rt.depth() != depth + 1)
            {
                rt.misbehaving_program_error();
                goto error;
            }
            object_p test = rt.pop();
            keep = test->as_truth(true);
            if (rt.error())
                goto error;
        }

        if (!obj)
            goto error;
        if (keep)
        {
            size_t objsz = obj->size();
            byte_p objp = byte_p(obj);
            if (!rt.append(objsz, objp))
                goto error;
        }
    }

    return list::make(ty, scr.scratch(), scr.growth());

error:
    if (rt.depth() > depth)
        rt.drop(rt.depth() - depth);
    return nullptr;
}


list_p list::map(algebraic_fn fn) const
// ----------------------------------------------------------------------------
//   Apply an algebraic function on all elements in the list
// ----------------------------------------------------------------------------
{
    id ty = type();
    scribble scr;
    for (object_p obj : *this)
    {
        id oty = obj->type();
        if (oty == ID_array || oty == ID_list)
        {
            list_g sub = list_p(obj)->map(fn);
            obj = +sub;
        }
        else
        {
            algebraic_g a = obj->as_algebraic();
            if (!a)
            {
                rt.type_error();
                return nullptr;
            }

            a = fn(a);
            if (!a)
                return nullptr;
            obj = +a;
        }

        if (!obj)
            return nullptr;
        size_t objsz = obj->size();
        byte_p objp = byte_p(obj);
        if (!rt.append(objsz, objp))
            return nullptr;
    }

    return list::make(ty, scr.scratch(), scr.growth());
}


list_p list::map(arithmetic_fn fn, algebraic_r y) const
// ----------------------------------------------------------------------------
//   Right-apply an arithmtic function on all elements in the list
// ----------------------------------------------------------------------------
{
    id ty = type();
    scribble scr;
    for (object_p obj : *this)
    {
        id oty = obj->type();
        if (oty == ID_array || oty == ID_list)
        {
            list_g sub = list_p(obj)->map(fn, y);
            obj = +sub;
        }
        else
        {
            algebraic_g a = obj->as_algebraic();
            if (!a)
            {
                rt.type_error();
                return nullptr;
            }

            a = fn(a, y);
            if (!a)
                return nullptr;
            obj = +a;
        }

        if (!obj)
            return nullptr;
        size_t objsz = obj->size();
        byte_p objp = byte_p(obj);
        if (!rt.append(objsz, objp))
            return nullptr;
    }

    return list::make(ty, scr.scratch(), scr.growth());
}


list_p list::map(algebraic_r x, arithmetic_fn fn) const
// ----------------------------------------------------------------------------
//   Left-apply an arithmtic function on all elements in the list
// ----------------------------------------------------------------------------
{
    id ty = type();
    scribble scr;
    for (object_p obj : *this)
    {
        id oty = obj->type();
        if (oty == ID_array || oty == ID_list)
        {
            list_g sub = list_p(obj)->map(x, fn);
            obj = +sub;
            if (!obj)
                return nullptr;
        }
        else
        {
            algebraic_g a = obj->as_algebraic();
            if (!a)
            {
                rt.type_error();
                return nullptr;
            }

            a = fn(x, a);
            if (!a)
                return nullptr;
            obj = +a;
        }

        size_t objsz = obj->size();
        byte_p objp = byte_p(obj);
        if (!rt.append(objsz, objp))
            return nullptr;
    }

    return list::make(ty, scr.scratch(), scr.growth());
}



// ============================================================================
//
//   Sorting
//
// ============================================================================

static int memory_compare(object_p *xp, object_p *yp)
// ----------------------------------------------------------------------------
//   Compare using type IDs and memory content
// ----------------------------------------------------------------------------
{
    object_p x = *xp;
    object_p y = *yp;
    return x->compare_to(y);
}


static int value_compare(object_p *xp, object_p *yp)
// ----------------------------------------------------------------------------
//   Sort items according to value
// ----------------------------------------------------------------------------
{
    object_p x = *xp;
    object_p y = *yp;
    object::id xty = x->type();
    object::id yty = y->type();
    if ((object::is_algebraic(xty) && object::is_algebraic(yty)) ||
        (xty == object::ID_array   && yty == object::ID_array) ||
        (xty == object::ID_list    && yty == object::ID_list))
    {
        algebraic_g xa     = algebraic_p(x);
        algebraic_g ya     = algebraic_p(y);
        int         result = 0;
        if (comparison::compare(&result, xa, ya))
            return result;
    }
    return x->compare_to(y);
}


static int value_compare_reverse(object_p *xp, object_p *yp)
// ----------------------------------------------------------------------------
//   Sort item according in decreasing value order
// ----------------------------------------------------------------------------
{
    return -value_compare(xp, yp);
}


static int memory_compare_reverse(object_p *xp, object_p *yp)
// ----------------------------------------------------------------------------
//   Sort item according in decreasing value order
// ----------------------------------------------------------------------------
{
    return -memory_compare(xp, yp);
}


static object::result do_sort(int (*compare)(object_p *x, object_p *y))
// ----------------------------------------------------------------------------
//   RPL command for a sort
// ----------------------------------------------------------------------------
{
    typedef int (*qsort_fn)(const void *, const void*);

    if (rt.args(1))
    {
        if  (object_p obj = rt.stack(0))
        {
            object::id oty = obj->type();
            if (oty == object::ID_list || oty == object::ID_array)
            {
                size_t   depth = rt.depth();
                list_g   items = list_p(obj);
                size_t   count;
                scribble scr;
                qsort_fn cmp = qsort_fn(compare);

                for (object_p item : *items)
                    if (!rt.push(item))
                        goto err;
                count = rt.depth() - depth;
                if (cmp)
                    qsort(rt.stack_base(), count, sizeof(object_p), cmp);

                for (uint i = 0; i < count; i++)
                {
                    if (object_g obj = rt.stack(i))
                    {
                        size_t objsz = obj->size();
                        byte_p objp = byte_p(obj);
                        if (!rt.append(objsz, objp))
                            goto err;
                    }
                }
                rt.drop(count);
                items = list::make(oty, scr.scratch(), scr.growth());
                if (items && rt.top(+items))
                    return object::OK;

            err:
                rt.drop(rt.depth() - depth);
                return object::ERROR;
            }
            else
            {
                rt.type_error();
            }
        }

    }
    return object::ERROR;
}


COMMAND_BODY(Sort)
// ----------------------------------------------------------------------------
//   Sort contents of a list according to value
// ----------------------------------------------------------------------------
{
    return do_sort(value_compare);
}


COMMAND_BODY(QuickSort)
// ----------------------------------------------------------------------------
//   Sort contents of a list using memory comparisons
// ----------------------------------------------------------------------------
{
    return do_sort(memory_compare);
}


COMMAND_BODY(ReverseSort)
// ----------------------------------------------------------------------------
//   Sort contents of a list according to value
// ----------------------------------------------------------------------------
{
    return do_sort(value_compare_reverse);
}


COMMAND_BODY(ReverseQuickSort)
// ----------------------------------------------------------------------------
//   Sort contents of a list using memory comparisons
// ----------------------------------------------------------------------------
{
    return do_sort(memory_compare_reverse);
}


COMMAND_BODY(ReverseList)
// ----------------------------------------------------------------------------
//   Reverse a list
// ----------------------------------------------------------------------------
{
    return do_sort(nullptr);
}
