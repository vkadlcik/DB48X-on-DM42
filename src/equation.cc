// ****************************************************************************
//  equation.cc                                                   DB48X project
// ****************************************************************************
//
//   File Description:
//
//     Implementation of algebraic equations
//
//     Equations are simply programs that are rendered and parsed specially
//
//
//
//
//
//
// ****************************************************************************
//   (C) 2023 Christophe de Dinechin <christophe@dinechin.org>
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

#include "equation.h"

#include "parser.h"
#include "renderer.h"


RECORDER(equation, 16, "Processing of equations and algebraic objects");
RECORDER(equation_error, 16, "Errors with equations");

// ============================================================================
//
//    Equation
//
// ============================================================================

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
                case ID_fact:
                    precedence = precedence::SYMBOL;
                    return arg + symbol::make("!");
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
    bool   ok      = true;

    // First push all things so that we have the outermost operators first
    for (object_p obj : *o)
    {
        ok = rt.push(obj);
        if (!ok)
            break;
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
        record(equation_error, "Malformed equation, %u removed", remove);
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


object_p equation::quoted(id ty) const
// ----------------------------------------------------------------------------
//   If an equation contains a single symbol, return that
// ----------------------------------------------------------------------------
{
    byte  *p = (byte *) payload();
    size_t size = leb128<size_t>(p);
    object_p first = (object_p) p;
    if (first->type() == ty && first->size() == size)
        return first;
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


equation::equation(algebraic_r arg, id type)
// ----------------------------------------------------------------------------
//   Build an equation object from an object
// ----------------------------------------------------------------------------
    : program(nullptr, 0, type)
{
    byte *p = (byte *) payload();

    // Compute the size of the program
    size_t size =  size_in_equation(arg);

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
}


size_t equation::required_memory(id type, algebraic_r arg)
// ----------------------------------------------------------------------------
//   Size of an equation object from an object
// ----------------------------------------------------------------------------
{
    size_t size = size_in_equation(arg);
    size += leb128size(size);
    size += leb128size(type);
    return size;
}


equation::equation(id op, algebraic_r arg, id type)
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


size_t equation::required_memory(id type, id op, algebraic_r arg)
// ----------------------------------------------------------------------------
//   Size of an equation object with one argument
// ----------------------------------------------------------------------------
{
    size_t size = leb128size(op) + size_in_equation(arg);
    size += leb128size(size);
    size += leb128size(type);
    return size;
}


equation::equation(id op, algebraic_r x, algebraic_r y, id type)
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


size_t equation::required_memory(id type, id op, algebraic_r x, algebraic_r y)
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
    case ID_sq:
    case ID_cubed:
    case ID_pow:        return 6;

    case ID_fact:
    case ID_inv:        return 999;

    default:            return 0;
    }
}
