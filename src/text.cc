// ****************************************************************************
//  text.cc                                                    DB48X project
// ****************************************************************************
//
//   File Description:
//
//     Implementation of basic string operations
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

#include "text.h"

#include "parser.h"
#include "renderer.h"
#include "runtime.h"

#include <stdio.h>


OBJECT_HANDLER_BODY(text)
// ----------------------------------------------------------------------------
//    Handle commands for texts
// ----------------------------------------------------------------------------
{
    switch(op)
    {
    case EXEC:
    case EVAL:
        // Text values evaluate as self
        rt.push(obj);
        return OK;
    case SIZE:
        return size(obj, payload);
    case PARSE:
        return object_parser(OBJECT_PARSER_ARG(), rt);
    case RENDER:
        return obj->object_renderer(OBJECT_RENDERER_ARG(), rt);
    case HELP:
        return (intptr_t) "text";

    default:
        // Check if anyone else knows how to deal with it
        return DELEGATE(object);
    }

}


OBJECT_PARSER_BODY(text)
// ----------------------------------------------------------------------------
//    Try to parse this as an text
// ----------------------------------------------------------------------------
//    For simplicity, this deals with all kinds of texts
{
    utf8 source = p.source;
    utf8 s      = source;
    if (*s++ != '"')
        return SKIP;

    utf8 end = source + p.length;
    while (s < end && *s != '"')
        s++;

    if (*s != '"')
    {
        rt.error("Invalid text", s);
        return ERROR;
    }
    s++;

    size_t parsed = s - source;
    size_t slen   = parsed - 2;
    gcutf8 txt    = source + 1;
    p.end         = parsed;
    p.out         = rt.make<text>(ID_text, txt, slen);

    return OK;
}


OBJECT_RENDERER_BODY(text)
// ----------------------------------------------------------------------------
//   Render the text into the given text buffer
// ----------------------------------------------------------------------------
{
    size_t  len = 0;
    utf8 txt = value(&len);
    return snprintf(r.target, r.length, "\"%.*s\"", (int) len, txt);
}


text_g operator+(text_g x, text_g y)
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
    text_g concat = rt.make<text>(text::ID_text, tx, sx + sy);
    if (concat)
    {
        utf8 tc = concat->value();
        memcpy((byte *) tc + sx, (byte *) ty, sy);
    }
    return concat;
}


text_g operator*(text_g x, uint y)
// ----------------------------------------------------------------------------
//    Repeat the text a given number of times
// ----------------------------------------------------------------------------
{
    runtime &rt = runtime::RT;
    text_g result = rt.make<text>(text::ID_text, x->value(), 0);
    while (y)
    {
        if (y & 1)
            result = result + x;
        if (!result)
            break;
        y /= 2;
        if (y)
            x = x + x;
    }
    return result;
}
