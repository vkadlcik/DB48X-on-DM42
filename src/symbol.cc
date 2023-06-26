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
#include "utf8.h"
#include "variables.h"

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
        return rt.push(obj) ? OK : ERROR;
    case EVAL:
        if (directory_p cat = rt.variables(0))
            if (object_p found = cat->recall(obj))
                return found->evaluate();
        return rt.push(obj) ? OK : ERROR;
    case SIZE:
        return size(obj, payload);
    case PARSE:
    {
        // Make sure we check commands first
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


static inline bool is_valid_as_name_initial(unicode cp)
// ----------------------------------------------------------------------------
//   Check if character is valid as initial of a name
// ----------------------------------------------------------------------------
{
    return (cp >= 'A' && cp <= 'Z')
        || (cp >= 'a' && cp <= 'z')
        || (cp >= 0x100 &&
            (cp != L'÷' &&      // Exclude symbols you can't have in a name
             cp != L'×' &&
             cp != L'∂'));
}


static inline bool is_valid_in_name(unicode cp)
// ----------------------------------------------------------------------------
//   Check if character is valid in a name
// ----------------------------------------------------------------------------
{
    return is_valid_as_name_initial(cp)
        || (cp >= '0' && cp <= '9');
}


static inline bool is_valid_in_name(utf8 s)
// ----------------------------------------------------------------------------
//   Check if first character in a string is valid in a name
// ----------------------------------------------------------------------------
{
    return is_valid_in_name(utf8_codepoint(s));
}


OBJECT_PARSER_BODY(symbol)
// ----------------------------------------------------------------------------
//    Try to parse this as a symbol
// ----------------------------------------------------------------------------
{
    utf8 source = p.source;
    utf8 s      = source;

    // First character must be alphabetic
    unicode cp = utf8_codepoint(s);
    if (!is_valid_as_name_initial(cp))
        return SKIP;
    s = utf8_next(s);

    // Other characters must be alphabetic
    while (is_valid_in_name(s))
        s = utf8_next(s);


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
    r.put(txt, len);
    return r.size();
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
