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
#include "renderer.h"
#include "runtime.h"
#include "utf8.h"

#include <stdio.h>


RECORDER(list, 16, "Lists");
RECORDER(list_errors, 16, "Errors processing lists");
RECORDER(program, 16, "Program evaluation");


OBJECT_HANDLER_BODY(list)
// ----------------------------------------------------------------------------
//    Handle commands for lists
// ----------------------------------------------------------------------------
{
    switch(op)
    {
    case EXEC:
    case EVAL:
        // List values evaluate as self
        return rt.push(obj) ? OK : ERROR;
    case SIZE:
        return size(obj, payload);
    case PARSE:
        return object_parser(OBJECT_PARSER_ARG(), rt);
    case RENDER:
        return obj->object_renderer(OBJECT_RENDERER_ARG(), rt);
    case HELP:
        return (intptr_t) "list";

    default:
        // Check if anyone else knows how to deal with it
        return DELEGATE(text);
    }

}


object::result list::object_parser(id type,
                                   parser  &p,
                                   runtime &rt,
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
    gcutf8  s     = p.source;
    size_t  max   = p.length;

    // Check if we have the opening marker
    unicode cp  = 0;
    if (open)
    {
        cp = utf8_codepoint(s);
        if (cp != open)
            return SKIP;
        s = utf8_next(s);
    }

    size_t  prealloc = rt.allocated();
    gcbytes scratch = rt.scratchpad() + prealloc;
    while (size_t(utf8(s) - utf8(p.source)) < max)
    {
        cp = utf8_codepoint(s);
        if (cp == close)
        {
            s = utf8_next(s);
            break;
        }
        if (cp == ' ' || cp == '\n' || cp == '\t')
        {
            s = utf8_next(s);
            continue;
        }

        // Parse an object
        size_t done = (byte *) s - (byte *) p.source;
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

    // Check that we have a matching closing character
    if (close && cp != close)
    {
        record(list_errors, "Missing terminator, got %u (%c) at %s",
               cp, cp, (byte *) s);
        rt.unterminated_error().source(p.source);
        return ERROR;
    }

    // Create the object
    size_t alloc  = rt.allocated() - prealloc;
    size_t parsed = utf8(s) - utf8(p.source);
    p.end         = parsed;
    p.out         = rt.make<list>(type, scratch, alloc);

    // Restore the scratchpad
    rt.free(alloc);
    return OK;
}


intptr_t list::object_renderer(renderer &r, runtime &rt,
                               unicode open, unicode close) const
// ----------------------------------------------------------------------------
//   Render the list into the given buffer
// ----------------------------------------------------------------------------
{
    // Source objects
    byte_p p    = payload();
    size_t size = leb128<size_t>(p);
    byte_p end  = p + size;

    // Destination buffer
    size_t idx = 0;
    size_t available = r.length;
    byte * dst = r.target;

    // Write the header, e.g. "{ "
    byte buffer[4];
    if (open)
    {
        size_t rendered = utf8_encode(open, buffer);
        for (size_t i = 0; i < rendered; i++)
        {
            if (idx < available)
            dst[idx] = buffer[i];
            idx++;
        }
    }

    // Loop on all objects inside the list
    while (p < end)
    {
        // Add space separator
        dst = r.target;
        if (idx < available)
            dst[idx] = ' ';
        idx++;

        object_p obj = (object_p) p;
        size_t   objsize = obj->size();

        // Render the object in what remains
        size_t remaining = r.length > idx ? r.length - idx : 0;
        size_t rsize = obj->render((char *) dst + idx, remaining, rt);
        idx += rsize;

        // Loop on next object
        p += objsize;
    }

    // Add final space separator
    dst = r.target;
    if (idx < available)
        dst[idx] = ' ';
    idx++;

    // Add closing separator
    if (close)
    {
        size_t rendered = utf8_encode(close, buffer);
        for (size_t i = 0; i < rendered; i++)
        {
            if (idx < available)
                dst[idx] = buffer[i];
            idx++;
        }
    }

    return idx;
}


OBJECT_PARSER_BODY(list)
// ----------------------------------------------------------------------------
//    Try to parse this as an list
// ----------------------------------------------------------------------------
{
    return object_parser(ID_list, p, rt, '{', '}');
}


OBJECT_RENDERER_BODY(list)
// ----------------------------------------------------------------------------
//   Render the list into the given list buffer
// ----------------------------------------------------------------------------
{
    return object_renderer(r, rt, '{', '}');
}



// ============================================================================
//
//    Program
//
// ============================================================================

OBJECT_HANDLER_BODY(program)
// ----------------------------------------------------------------------------
//    Handle commands for programs
// ----------------------------------------------------------------------------
{
    switch(op)
    {
    case EXEC:
        // Execute the program
        return obj->execute(rt);
    case EVAL:
        // A normal evaluation (not from ID_eval) just places program on stack
        // e.g.: from command line, or « « 1 + 2 » »
        return rt.push(obj) ? OK : ERROR;
    case SIZE:
        return size(obj, payload);
    case PARSE:
        return object_parser(OBJECT_PARSER_ARG(), rt);
    case RENDER:
        return obj->object_renderer(OBJECT_RENDERER_ARG(), rt);
    case HELP:
        return (intptr_t) "program";

    default:
        // Check if anyone else knows how to deal with it
        return DELEGATE(list);
    }
}


OBJECT_PARSER_BODY(program)
// ----------------------------------------------------------------------------
//    Try to parse this as a program
// ----------------------------------------------------------------------------
{
    return list::object_parser(ID_program, p, rt, L'«', L'»');
}


OBJECT_RENDERER_BODY(program)
// ----------------------------------------------------------------------------
//   Render the program into the given program buffer
// ----------------------------------------------------------------------------
{
    return list::object_renderer(r, rt, L'«', L'»');
}


object::result program::execute(runtime &rt) const
// ----------------------------------------------------------------------------
//   We evaluate a program by evaluating all the objects in it
// ----------------------------------------------------------------------------
//   This is called directly from the 'eval' command
{
    byte  *p       = (byte *) payload();
    size_t len     = leb128<size_t>(p);
    gcobj  first   = (object_p) p;
    result r       = OK;
    size_t objsize = 0;

    for (gcobj obj = first; len > 0; obj += objsize, len -= objsize)
    {
        objsize = obj->size();
        record(program, "Evaluating %+s at %p, size %u, %u remaining\n",
               obj->fancy(), (object_p) obj, objsize, len);
        if (interrupted() || r != OK)
            break;
        r = obj->evaluate(rt);
    }

    return r;
}


program_p program::parse(utf8 source, size_t size)
// ----------------------------------------------------------------------------
//   Parse a program without delimiters (e.g. command line)
// ----------------------------------------------------------------------------
{
    record(program, ">Parsing command line [%s]", source);
    parser p(source, size);
    result r = list::object_parser(ID_program, p, RT, 0, 0);
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

OBJECT_HANDLER_BODY(block)
// ----------------------------------------------------------------------------
//    Handle commands for blocks
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
        return SKIP;
    case RENDER:
        return obj->object_renderer(OBJECT_RENDERER_ARG(), rt);
    case HELP:
        return (intptr_t) "block";

    default:
        // Check if anyone else knows how to deal with it
        return DELEGATE(program);
    }
}


OBJECT_RENDERER_BODY(block)
// ----------------------------------------------------------------------------
//   Render the program into the given program buffer
// ----------------------------------------------------------------------------
{
    return list::object_renderer(r, rt, 0, 0);
}


// ============================================================================
//
//    Equation
//
// ============================================================================

OBJECT_HANDLER_BODY(equation)
// ----------------------------------------------------------------------------
//    Handle commands for equations
// ----------------------------------------------------------------------------
{
    switch(op)
    {
    case EXEC:
        return obj->execute(rt);
    case EVAL:
        // Equations evaluate like programs
        return rt.push(obj) ? OK : ERROR;
    case SIZE:
        return size(obj, payload);
    case PARSE:
        return object_parser(OBJECT_PARSER_ARG(), rt);
    case RENDER:
        return obj->object_renderer(OBJECT_RENDERER_ARG(), rt);
    case HELP:
        return (intptr_t) "program";

    default:
        // Check if anyone else knows how to deal with it
        return DELEGATE(program);
    }
}


OBJECT_PARSER_BODY(equation)
// ----------------------------------------------------------------------------
//    Try to parse this as an equation
// ----------------------------------------------------------------------------
{
    return list::object_parser(ID_equation, p, rt, '\'', '\'');
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


symbol_g equation::render(uint depth, int &precedence)
// ----------------------------------------------------------------------------
//   Render an object as a symbol at a given precedence
// ----------------------------------------------------------------------------
{
    runtime &rt = RT;
    while (rt.depth() > depth)
    {
        if (gcobj obj = rt.pop())
        {
            int arity = obj->arity();
            switch(arity)
            {
            case SKIP:
                // Non-algebraic in an equation, e.g. numbers
                precedence = algebraic::UNKNOWN;
                return obj->as_symbol(rt);

            case 0:
                // Symbols
                precedence = algebraic::SYMBOL;
                if (obj->type() == ID_symbol)
                    return symbol_p(object_p(obj));
                return obj->as_symbol(rt);

            case 1:
            {
                // TODO: Prefix and postfix operators
                int argp = 0;
                symbol_g fn = obj->as_symbol(rt);
                symbol_g arg = render(depth, argp);
                if (argp < algebraic::SYMBOL)
                    arg = parentheses(arg);
                precedence = algebraic::FUNCTION;
                switch(obj->type())
                {
                case ID_sq:
                    precedence = algebraic::FUNCTION_POWER;
                    return arg + symbol::make("²");
                case ID_cubed:
                    precedence = algebraic::FUNCTION_POWER;
                    return arg + symbol::make("³");
                case ID_neg:
                    precedence = algebraic::ADDITIVE;
                    return symbol::make('-') + arg;
                case ID_inv:
                    precedence = algebraic::FUNCTION_POWER;
                    return arg + symbol::make("⁻¹");
                default:
                    break;
                }
                if (argp >= algebraic::SYMBOL)
                    arg = space(arg);
                return fn + arg;
            }
            break;
            case 2:
            {
                int lprec = 0, rprec = 0;
                symbol_g op = obj->as_symbol(rt);
                symbol_g rtxt = render(depth, rprec);
                symbol_g ltxt = render(depth, lprec);
                int prec = obj->precedence();
                if (prec != algebraic::FUNCTION)
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
                    precedence = algebraic::FUNCTION;
                    return op + arg;
                }
            }
            break;
            default:
            {
                symbol_g op = obj->as_symbol(rt);
                symbol_g args = nullptr;
                for (int a = 0; a < arity; a++)
                {
                    int prec = 0;
                    symbol_g arg = render(depth, prec);
                    if (a)
                        args = arg + symbol::make(';') + args;
                    else
                        args = arg;
                }
                args = parentheses(args);
                precedence = algebraic::FUNCTION;
                return op + args;
            }
            }
        }
    }

    return nullptr;
}


OBJECT_RENDERER_BODY(equation)
// ----------------------------------------------------------------------------
//   Render the program into the given program buffer
// ----------------------------------------------------------------------------
//   1 2 3 5 * + - 2 3 * +
{
    size_t depth   = rt.depth();
    byte_p p       = payload();
    size_t size    = leb128<size_t>(p);
    gcobj  first   = object_p(p);
    bool   ok      = true;
    size_t objsize = 0;

    // First push all things so that we have the outermost operators first
    for (gcobj obj = first; ok && size; size -= objsize, obj += objsize)
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
    symbol_g result = render(depth, precedence);
    if (result)
    {
        size_t size = 0;
        utf8 txt = result->value(&size);
        if (r.equation)
            return snprintf(r.target, r.length, "%.*s", (int) size, txt);
        else
            return snprintf(r.target, r.length, "'%.*s'", (int) size, txt);
    }
    return 0;
}


symbol_p equation::symbol() const
// ----------------------------------------------------------------------------
//   If an equation contains a single symbol, return that
// ----------------------------------------------------------------------------
{
    byte  *p       = (byte *) payload();
    size_t size    = leb128<size_t>(p);
    object_p first = (object_p) p;
    if (first->type() == ID_symbol && first->size() == size)
        return (symbol_p) first;
    return nullptr;
}


inline size_t equation::size_in_equation(object_p obj)
// ----------------------------------------------------------------------------
//   Size of an object in an equation
// ----------------------------------------------------------------------------
//   Inside an equation object, equations are reduced to their payload
{
    if (obj->type() == ID_equation)
        return equation_p(obj)->length();
    return obj->size();
}


equation::equation(uint arity, const gcobj args[], id op, id type)
// ----------------------------------------------------------------------------
//   Build an equation from N arguments
// ----------------------------------------------------------------------------
    : program(nullptr, 0, type)
{
    byte *p = payload();

    // Compute the size of the program
    size_t size = 0;
    for (uint i = 0; i < arity; i++)
        size += size_in_equation(args[i]);
    size += leb128size(op);

    // Write the size of the program
    p = leb128(p, size);

    // Write the arguments
    for (uint i = 0; i < arity; i++)
    {
        object_p obj = args[i];
        size_t objsize = 0;
        byte_p objptr = nullptr;
        if (obj->type() == ID_equation)
        {
            objptr = equation_p(obj)->value(&objsize);
        }
        else
        {
            objsize = obj->size();
            objptr = byte_p(obj);
        }
        memmove(p, objptr, objsize);
        p += objsize;
    }

    // Write the last opcode
    p = leb128(p, op);
}


size_t equation::required_memory(id type, uint arity, const gcobj args[], id op)
// ----------------------------------------------------------------------------
//   Size of an equation object with N arguments
// ----------------------------------------------------------------------------
{
    size_t size = 0;
    for (uint i = 0; i < arity; i++)
        size += size_in_equation(args[i]);
    size += leb128size(op);
    size += leb128size(size);
    size += leb128size(type);
    return size;
}



// ============================================================================
//
//    Array
//
// ============================================================================

OBJECT_HANDLER_BODY(array)
// ----------------------------------------------------------------------------
//    Handle commands for arrays
// ----------------------------------------------------------------------------
{
    switch(op)
    {
    case EXEC:
    case EVAL:
        // Programs evaluate by evaluating all elements in sequence
        return rt.push(obj) ? OK : ERROR;
    case SIZE:
        return size(obj, payload);
    case PARSE:
        return object_parser(OBJECT_PARSER_ARG(), rt);
    case RENDER:
        return obj->object_renderer(OBJECT_RENDERER_ARG(), rt);
    case HELP:
        return (intptr_t) "program";

    default:
        // Check if anyone else knows how to deal with it
        return DELEGATE(program);
    }
}


OBJECT_PARSER_BODY(array)
// ----------------------------------------------------------------------------
//    Try to parse this as a program
// ----------------------------------------------------------------------------
{
    return list::object_parser(ID_array, p, rt, '[', ']');
}


OBJECT_RENDERER_BODY(array)
// ----------------------------------------------------------------------------
//   Render the program into the given program buffer
// ----------------------------------------------------------------------------
{
    return list::object_renderer(r, rt, '[', ']');
}
