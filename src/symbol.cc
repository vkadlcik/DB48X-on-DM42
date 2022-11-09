// ****************************************************************************
//  symbol.cc                                                    DB48X project
// ****************************************************************************
//
//   File Description:
//
//     Implementation of RPL symbols
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

#include "symbol.h"

#include "algebraic.h"
#include "command.h"
#include "parser.h"
#include "renderer.h"
#include "runtime.h"
#include "variables.h"

#include <ctype.h>
#include <stdio.h>


OBJECT_HANDLER_BODY(symbol)
// ----------------------------------------------------------------------------
//    Handle commands for symbols
// ----------------------------------------------------------------------------
{
    switch(op)
    {
    case EXEC:
        if (directory_p cat = rt.variables(0))
            if (object_p found = cat->recall(obj))
                return found->execute();
        rt.push(obj);
        return OK;
    case EVAL:
        if (directory_p cat = rt.variables(0))
            if (object_p found = cat->recall(obj))
                return found->evaluate();
        rt.push(obj);
        return OK;
    case SIZE:
        return size(obj, payload);
    case PARSE:
    {
        // Make sure we check commands first
        result r = (result) DELEGATE(command);
        if (r == OK)
            return r;
        return object_parser(OBJECT_PARSER_ARG(), rt);
    }
    case RENDER:
        return obj->object_renderer(OBJECT_RENDERER_ARG(), rt);
    case ARITY:
        return 0;
    case PRECEDENCE:
        return algebraic::SYMBOL;

    case HELP:
        return (intptr_t) "symbols";

    default:
        // Check if anyone else knows how to deal with it
        return DELEGATE(object);
    }

}


OBJECT_PARSER_BODY(symbol)
// ----------------------------------------------------------------------------
//    Try to parse this as a symbol
// ----------------------------------------------------------------------------
{
    utf8 source = p.source;
    utf8 s      = source;

    // First character must be alphabetic (TODO: Special HP alpha chars, e.g Σ
    if (!isalpha(*s++))
        return SKIP;

    // Other characters must be alphabetic
    while (*s && isalnum(*s))
        s++;

    size_t parsed = s - source;
    gcutf8 text   = source;
    p.end         = parsed;
    p.out         = rt.make<symbol>(ID_symbol, text, parsed);

    return OK;
}


OBJECT_RENDERER_BODY(symbol)
// ----------------------------------------------------------------------------
//   Render the symbol into the given symbol buffer
// ----------------------------------------------------------------------------
{
    size_t len = 0;
    utf8   txt = value(&len);
    return snprintf(r.target, r.length, "%.*s", (int) len, txt);
}


symbol_g operator+(symbol_g x, symbol_g y)
// ----------------------------------------------------------------------------
//   Concatenate two texts
// ----------------------------------------------------------------------------
{
    if (!x)
        return y;
    if (!y)
        return x;
    runtime &rt = runtime::RT;
    size_t sx = 0, sy = 0;
    utf8 tx = x->value(&sx);
    utf8 ty = y->value(&sy);
    symbol_g concat = rt.make<symbol>(symbol::ID_symbol, tx, sx + sy);
    if (concat)
    {
        utf8 tc = concat->value();
        memcpy((byte *) tc + sx, (byte *) ty, sy);
    }
    return concat;
}
