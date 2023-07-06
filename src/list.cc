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
#include "parser.h"
#include "precedence.h"
#include "renderer.h"
#include "runtime.h"
#include "utf8.h"

#include <stdio.h>


RECORDER(list, 16, "Lists");
RECORDER(list_parse, 16, "List parsing");
RECORDER(list_errors, 16, "Errors processing lists");
RECORDER(program, 16, "Program evaluation");


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
                    int childp = parenthese ? 1
                        : infix ? equation::precedence(infix) + 1
                        : 999;
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
                    precedence = -999;
                }
            }
            else
            {
                // We just parsed an infix, e.g. +, -, etc
                // stash it, or exit loop if it has lower precedence
                int objprec = equation::precedence(obj);
                if (objprec < lowest)
                    break;
                if (!objprec)
                {
                    rt.infix_expected_error();
                    return ERROR;
                }
                if (objprec < 999)
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
        record(list_errors, "Missing terminator, got %u (%c) not %u (%c) at %s",
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
    // Source objects
    byte_p  p      = payload();
    size_t  size   = leb128<size_t>(p);
    gcbytes start  = p;
    size_t  offset = 0;

    // Write the header, e.g. "{ "
    if (open)
        r.put(open);

    // Loop on all objects inside the list
    while (offset < size)
    {
        // Add space separator (except on first object when no separator)
        if (open)
            r.put(' ');
        open = 1;

        object_p obj = object_p(byte_p(start) + offset);
        size_t   objsize = obj->size();

        // Render the object in what remains (may GC)
        obj->render(r);

        // Loop on next object
        offset += objsize;
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
//    Program
//
// ============================================================================

EVAL_BODY(program)
// ----------------------------------------------------------------------------
//   Normal evaluation from a program places it on stack
// ----------------------------------------------------------------------------
{
    return rt.push(o) ? OK : ERROR;
}


EXEC_BODY(program)
// ----------------------------------------------------------------------------
//   Execution of a program evaluates all items in turn
// ----------------------------------------------------------------------------
{
    byte  *p       = (byte *) o->payload();
    size_t len     = leb128<size_t>(p);
    result r       = OK;
    size_t objsize = 0;

    for (object_g obj = object_p(p); len > 0; obj += objsize, len -= objsize)
    {
        objsize = obj->size();
        record(program, "Evaluating %+s at %p, size %u, %u remaining\n",
               obj->fancy(), (object_p) obj, objsize, len);
        if (interrupted() || r != OK)
            break;
        r = obj->evaluate();
    }

    return r;
}


PARSE_BODY(program)
// ----------------------------------------------------------------------------
//    Try to parse this as a program
// ----------------------------------------------------------------------------
{
    return list_parse(ID_program, p, L'«', L'»');
}


RENDER_BODY(program)
// ----------------------------------------------------------------------------
//   Render the program into the given program buffer
// ----------------------------------------------------------------------------
{
    return o->list_render(r, L'«', L'»');
}


program_p program::parse(utf8 source, size_t size)
// ----------------------------------------------------------------------------
//   Parse a program without delimiters (e.g. command line)
// ----------------------------------------------------------------------------
{
    record(program, ">Parsing command line [%s]", source);
    parser p(source, size);
    result r = list_parse(ID_program, p, 0, 0);
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

EVAL_BODY(block)
// ----------------------------------------------------------------------------
//   Normal evaluation of a block executes it
// ----------------------------------------------------------------------------
{
    return do_execute(o);
}


PARSE_BODY(block)
// ----------------------------------------------------------------------------
//  Blocks are parsed in structures like loops, not directly
// ----------------------------------------------------------------------------
{
    return SKIP;
}


RENDER_BODY(block)
// ----------------------------------------------------------------------------
//   Render the program into the given program buffer
// ----------------------------------------------------------------------------
{
    return o->list_render(r, 0, 0);
}



// ============================================================================
//
//    Equation
//
// ============================================================================

EVAL_BODY(equation)
// ----------------------------------------------------------------------------
//   Normal evaluation of an equation executes it
// ----------------------------------------------------------------------------
{
    return do_execute(o);
}


PARSE_BODY(equation)
// ----------------------------------------------------------------------------
//    Try to parse this as an equation
// ----------------------------------------------------------------------------
{
    // If already parsing an equation, let upper parser deal with quote
    if (p.precedence)
        return SKIP;

    p.precedence = 1;
    auto result = list_parse(ID_equation, p, '\'', '\'');
    p.precedence = 0;

    return result;
}


symbol_g equation::parentheses(symbol_g arg)
// ----------------------------------------------------------------------------
//   Render, putting parentheses around an argument
// ----------------------------------------------------------------------------
{
    symbol_g open = symbol::make('(');
    symbol_g close = symbol::make(')');
    return open + arg + close;
}


symbol_g equation::space(symbol_g arg)
// ----------------------------------------------------------------------------
//   Render, putting parentheses around an argument
// ----------------------------------------------------------------------------
{
    return symbol::make(' ') + arg;
}


symbol_g equation::render(uint depth, int &precedence, bool editing)
// ----------------------------------------------------------------------------
//   Render an object as a symbol at a given precedence
// ----------------------------------------------------------------------------
{
    while (rt.depth() > depth)
    {
        if (object_g obj = rt.pop())
        {
            int arity = obj->arity();
            switch(arity)
            {
            case 0:
                // Symbols and other non-algebraics, e.g. numbers
                precedence = precedence::SYMBOL;
                if (obj->type() == ID_symbol)
                    return symbol_p(object_p(obj));
                return obj->as_symbol(editing);

            case 1:
            {
                int      argp = 0;
                id       oid  = obj->type();
                symbol_g fn   = obj->as_symbol(editing);
                symbol_g arg  = render(depth, argp, editing);
                int      maxp =
                    oid == ID_neg ? precedence::FUNCTION : precedence::SYMBOL;
                if (argp < maxp)
                    arg = parentheses(arg);
                precedence = precedence::FUNCTION;
                switch(oid)
                {
                case ID_sq:
                    precedence = precedence::FUNCTION_POWER;
                    return arg + symbol::make("²");
                case ID_cubed:
                    precedence = precedence::FUNCTION_POWER;
                    return arg + symbol::make("³");
                case ID_neg:
                    precedence = precedence::ADDITIVE;
                    return symbol::make('-') + arg;
                case ID_inv:
                    precedence = precedence::FUNCTION_POWER;
                    return arg + symbol::make("⁻¹");
                default:
                    break;
                }
                if (argp >= precedence::FUNCTION)
                    arg = space(arg);
                return fn + arg;
            }
            break;
            case 2:
            {
                int lprec = 0, rprec = 0;
                symbol_g op = obj->as_symbol(editing);
                symbol_g rtxt = render(depth, rprec, editing);
                symbol_g ltxt = render(depth, lprec, editing);
                int prec = obj->precedence();
                if (prec != precedence::FUNCTION)
                {
                    if (lprec < prec)
                        ltxt = parentheses(ltxt);
                    if (rprec <= prec)
                        rtxt = parentheses(rtxt);
                    precedence = prec;
                    return ltxt + op + rtxt;
                }
                else
                {
                    symbol_g arg = ltxt + symbol::make(';') + rtxt;
                    arg = parentheses(arg);
                    precedence = precedence::FUNCTION;
                    return op + arg;
                }
            }
            break;
            default:
            {
                symbol_g op = obj->as_symbol(editing);
                symbol_g args = nullptr;
                for (int a = 0; a < arity; a++)
                {
                    int prec = 0;
                    symbol_g arg = render(depth, prec, editing);
                    if (a)
                        args = arg + symbol::make(';') + args;
                    else
                        args = arg;
                }
                args = parentheses(args);
                precedence = precedence::FUNCTION;
                return op + args;
            }
            }
        }
    }

    return nullptr;
}


RENDER_BODY(equation)
// ----------------------------------------------------------------------------
//   Render the program into the given program buffer
// ----------------------------------------------------------------------------
//   1 2 3 5 * + - 2 3 * +
{
    size_t depth   = rt.depth();
    byte_p p       = o->payload();
    size_t size    = leb128<size_t>(p);
    bool   ok      = true;
    size_t objsize = 0;

    // First push all things so that we have the outermost operators first
    for (object_g obj = object_p(p); ok && size; size -= objsize, obj += objsize)
    {
        objsize = obj->size();
        ok = rt.push(obj);
    }

    if (!ok)
    {
        // We ran out of memory pushing things
        if (size_t remove = rt.depth() - depth)
            rt.drop(remove);
        return 0;
    }

    int precedence = 0;
    symbol_g result = render(depth, precedence, r.editing());
    if (size_t remove = rt.depth() - depth)
    {
        record(list_errors, "Malformed equation, %u removed", remove);
        rt.drop(remove);
    }
    if (!result)
        return 0;

    size_t len = 0;
    utf8 txt = result->value(&len);
    if (!r.equation())
        r.put('\'');
    r.put(txt, len);
    if (!r.equation())
        r.put('\'');
    return r.size();
}


symbol_p equation::symbol() const
// ----------------------------------------------------------------------------
//   If an equation contains a single symbol, return that
// ----------------------------------------------------------------------------
{
    byte  *p = (byte *) payload();
    size_t size = leb128<size_t>(p);
    object_p first = (object_p) p;
    if (first->type() == ID_symbol && first->size() == size)
        return (symbol_p) first;
    return nullptr;
}


size_t equation::size_in_equation(object_p obj)
// ----------------------------------------------------------------------------
//   Size of an object in an equation
// ----------------------------------------------------------------------------
//   Inside an equation object, equations are reduced to their payload
{
    if (obj->type() == ID_equation)
        return equation_p(obj)->length();
    return obj->size();
}


equation::equation(id op, const algebraic_g &arg, id type)
// ----------------------------------------------------------------------------
//   Build an equation from one argument
// ----------------------------------------------------------------------------
    : program(nullptr, 0, type)
{
    byte *p = (byte *) payload();

    // Compute the size of the program
    size_t size =  leb128size(op) + size_in_equation(arg);

    // Write the size of the program
    p = leb128(p, size);

    // Write the arguments
    size_t objsize = 0;
    byte_p objptr = nullptr;
    if (arg->type() == ID_equation)
    {
        objptr = equation_p(algebraic_p(arg))->value(&objsize);
    }
    else
    {
        objsize = arg->size();
        objptr = byte_p(arg);
    }
    memmove(p, objptr, objsize);
    p += objsize;

    // Write the opcode
    p = leb128(p, op);
}


size_t equation::required_memory(id type, id op, const algebraic_g &arg)
// ----------------------------------------------------------------------------
//   Size of an equation object with one argument
// ----------------------------------------------------------------------------
{
    size_t size = leb128size(op) + size_in_equation(arg);
    size += leb128size(size);
    size += leb128size(type);
    return size;
}


equation::equation(id op, const algebraic_g &x, const algebraic_g &y, id type)
// ----------------------------------------------------------------------------
//   Build an equation from two arguments
// ----------------------------------------------------------------------------
    : program(nullptr, 0, type)
{
    byte *p = (byte *) payload();

    // Compute the size of the program
    size_t size =  leb128size(op) + size_in_equation(x) + size_in_equation(y);

    // Write the size of the program
    p = leb128(p, size);

    // Write the first argument
    size_t objsize = 0;
    byte_p objptr = nullptr;
    if (x->type() == ID_equation)
    {
        objptr = equation_p(algebraic_p(x))->value(&objsize);
    }
    else
    {
        objsize = x->size();
        objptr = byte_p(x);
    }
    memmove(p, objptr, objsize);
    p += objsize;

    // Write the second argument
    if (y->type() == ID_equation)
    {
        objptr = equation_p(algebraic_p(y))->value(&objsize);
    }
    else
    {
        objsize = y->size();
        objptr = byte_p(y);
    }
    memmove(p, objptr, objsize);
    p += objsize;

    // Write the opcode
    p = leb128(p, op);
}


size_t equation::required_memory(id                 type,
                                 id                 op,
                                 const algebraic_g &x,
                                 const algebraic_g &y)
// ----------------------------------------------------------------------------
//   Size of an equation object with one argument
// ----------------------------------------------------------------------------
{
    size_t size = leb128size(op) + size_in_equation(x) + size_in_equation(y);
    size += leb128size(size);
    size += leb128size(type);
    return size;
}


int equation::precedence(id type)
// ----------------------------------------------------------------------------
//   Return the algebraic precedence associated to a given operation
// ----------------------------------------------------------------------------
{
    switch(type)
    {
    case ID_And:
    case ID_Or:
    case ID_Xor:
    case ID_NAnd:
    case ID_NOr:        return 1;

    case ID_Implies:
    case ID_Equiv:
    case ID_Excludes:   return 2;

    case ID_TestSame:
    case ID_TestLT:
    case ID_TestEQ:
    case ID_TestGT:
    case ID_TestLE:
    case ID_TestNE:
    case ID_TestGE:     return 3;

    case ID_add:
    case ID_sub:        return 4;
    case ID_mul:
    case ID_div:
    case ID_mod:
    case ID_rem:        return 5;
    case ID_pow:        return 6;

    case ID_inv:
    case ID_sq:
    case ID_cubed:      return 999;

    default:            return 0;
    }
}



// ============================================================================
//
//    Array
//
// ============================================================================

PARSE_BODY(array)
// ----------------------------------------------------------------------------
//    Try to parse this as a program
// ----------------------------------------------------------------------------
{
    return list::list_parse(ID_array, p, '[', ']');
}


RENDER_BODY(array)
// ----------------------------------------------------------------------------
//   Render the program into the given program buffer
// ----------------------------------------------------------------------------
{
    return o->list_render(r, '[', ']');
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
