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
#include "expression.h"
#include "parser.h"
#include "renderer.h"
#include "runtime.h"
#include "unit.h"
#include "utf8.h"
#include "variables.h"

#include <stdio.h>


EVAL_BODY(symbol)
// ----------------------------------------------------------------------------
//   Evaluate a symbol by looking it up
// ----------------------------------------------------------------------------
{
    if (unit::mode)
        if (unit_p u = unit::lookup(o))
            if (rt.push(u))
                return OK;
    if (object_p found = directory::recall_all(o))
        return found->execute();
    if (object_g eq = expression::make(o))
        if (rt.push(eq))
            return OK;
    return ERROR;
}


PARSE_BODY(symbol)
// ----------------------------------------------------------------------------
//    Try to parse this as a symbol
// ----------------------------------------------------------------------------
{
    utf8    source = p.source;
    size_t  max    = p.length;
    size_t  parsed = 0;

    // First character must be alphabetic
    unicode cp = utf8_codepoint(source);
    if (!is_valid_as_name_initial(cp))
        return SKIP;
    parsed = utf8_next(source, parsed, max);

    // Other characters must be alphabetic
    while (parsed < max && is_valid_in_name(source + parsed))
        parsed = utf8_next(source, parsed, max);

    gcutf8 text   = source;
    p.end         = parsed;
    p.out         = rt.make<symbol>(ID_symbol, text, parsed);

    return OK;
}


RENDER_BODY(symbol)
// ----------------------------------------------------------------------------
//   Render the symbol into the given symbol buffer
// ----------------------------------------------------------------------------
{
    size_t len = 0;
    utf8   txt = o->value(&len);
    r.put(txt, len);
    return r.size();
}


symbol_g operator+(symbol_r x, symbol_r y)
// ----------------------------------------------------------------------------
//   Concatenate two texts
// ----------------------------------------------------------------------------
{
    if (!x.Safe())
        return y;
    if (!y.Safe())
        return x;
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


object_p symbol::recall(bool noerror) const
// ----------------------------------------------------------------------------
//   Recall the value associated with the symbol
// ----------------------------------------------------------------------------
{
    directory *dir = rt.variables(0);
    if (object_p found = dir->recall(this))
        return found;
    return noerror ? this : nullptr;
}


bool symbol::store(object_g value) const
// ----------------------------------------------------------------------------
//   Store something in the value associated with the symbol
// ----------------------------------------------------------------------------
{
    directory *dir = rt.variables(0);
    object_g name = this;
    return dir->store(name, value);
}


bool symbol::is_same_as(symbol_p other) const
// ----------------------------------------------------------------------------
//   Return true of two symbols represent the same thing
// ----------------------------------------------------------------------------
{
    size_t sz, osz;
    utf8 txt = value(&sz);
    utf8 otxt = other->value(&osz);
    if (sz != osz)
        return false;
    return strncasecmp(cstring(txt), cstring(otxt), sz) == 0;
}
