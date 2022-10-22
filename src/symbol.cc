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

#include "parser.h"
#include "renderer.h"
#include "runtime.h"
#include "command.h"

#include <stdio.h>
#include <ctype.h>


OBJECT_HANDLER_BODY(symbol)
// ----------------------------------------------------------------------------
//    Handle commands for symbols
// ----------------------------------------------------------------------------
{
    switch(op)
    {
    case EVAL:
        // TODO: Symbols are looked up, and if not found, evaluate as self
        rt.push(obj);
        return 0;
    case SIZE:
    {
        byte *p = (byte *) payload;
        size_t len = leb128<size_t>(p);
        p += len;
        return ptrdiff(p, obj);
    }
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
    cstring source = p.source;
    cstring s      = source;

    // First character must be alphabetic (TODO: Special HP alpha chars, e.g Σ
    if (!isalpha(*s++))
        return SKIP;

    // Other characters must be
    while (*s && isalnum(*s))
        s++;

    // This must end with a space or end of command
    if (*s && !isspace(*s))
    {
        rt.error("Name syntax error", s);
        return ERROR;
    }

    size_t    parsed = s - source;
    gcstring  text   = source;
    p.end            = parsed;
    p.out            = rt.make<symbol>(ID_symbol, text, parsed);

    return OK;
}


OBJECT_RENDERER_BODY(symbol)
// ----------------------------------------------------------------------------
//   Render the symbol into the given symbol buffer
// ----------------------------------------------------------------------------
{
    size_t  len = 0;
    cstring txt = text(&len);
    return snprintf(r.target, r.length, "%.*s", (int) len, txt);
}
