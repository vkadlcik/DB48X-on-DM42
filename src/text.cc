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


SIZE_BODY(text)
// ----------------------------------------------------------------------------
//   Compute the size of a text (and all objects with a size at beginning)
// ----------------------------------------------------------------------------
{
    byte_p p = o->payload();
    size_t sz = leb128<size_t>(p);
    p += sz;
    return ptrdiff(p, o);
}


PARSE_BODY(text)
// ----------------------------------------------------------------------------
//    Try to parse this as a text
// ----------------------------------------------------------------------------
//    For simplicity, this deals with all kinds of texts
{
    utf8 source = p.source;
    utf8 s      = source;
    if (*s++ != '"')
        return SKIP;

    utf8   end    = source + p.length;
    size_t quotes = 0;
    bool   ok     = false;
    while (s < end)
    {
        if (*s++ == '"')
        {
            if (s >= end || *s != '"')
            {
                ok = true;
                break;
            }
            s++;
            quotes++;
        }
    }

    if (!ok)
    {
        rt.unterminated_error().source(p.source);
        return ERROR;
    }

    size_t parsed = s - source;
    size_t slen   = parsed - 2;
    gcutf8 txt    = source + 1;
    p.end         = parsed;
    p.out         = rt.make<text>(ID_text, txt, slen, quotes);

    return p.out ? OK : ERROR;
}


RENDER_BODY(text)
// ----------------------------------------------------------------------------
//   Render the text into the given text buffer
// ----------------------------------------------------------------------------
{
    size_t len = 0;
    gcutf8 txt = o->value(&len);
    size_t off = 0;
    r.put('"');
    while (off < len)
    {
        unicode c = utf8_codepoint(txt + off);
        if (c == '"')
            r.put('"');
        r.put(c);
        off += utf8_size(c);
    }
    r.put('"');
    return r.size();
}


text_g operator+(text_r x, text_r y)
// ----------------------------------------------------------------------------
//   Concatenate two texts or lists
// ----------------------------------------------------------------------------
{
    if (!x.Safe())
        return y;
    if (!y.Safe())
        return x;
    object::id type = x->type();
    size_t sx = 0, sy = 0;
    utf8 tx = x->value(&sx);
    utf8 ty = y->value(&sy);
    text_g concat = rt.make<text>(type, tx, sx + sy);
    if (concat)
    {
        utf8 tc = concat->value();
        memcpy((byte *) tc + sx, (byte *) ty, sy);
    }
    return concat;
}


text_g operator*(text_r xr, uint y)
// ----------------------------------------------------------------------------
//    Repeat the text a given number of times
// ----------------------------------------------------------------------------
{
    text_g result = rt.make<text>(xr->type(), xr->value(), 0);
    text_g x = xr;
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


static cstring conversions[] =
// ----------------------------------------------------------------------------
//   Conversion from standard ASCII to HP-48 characters
// ----------------------------------------------------------------------------
{
    "<<", "«",
    ">>", "»",
    "->", "→"
};


text_p text::import() const
// ----------------------------------------------------------------------------
//    Convert text containing sequences such as -> or <<
// ----------------------------------------------------------------------------
{
    text_p   result = this;
    size_t   sz     = 0;
    gcutf8   txt    = value(&sz);
    size_t   rcount = sizeof(conversions) / sizeof(conversions[0]);
    gcmbytes replace;
    scribble scr;

    for (size_t o = 0; o < sz; o++)
    {
        bool replaced = false;
        for (uint r = 0; r < rcount && !replaced; r += 2)
        {
            size_t olen = strlen(conversions[r]);
            if (!strncmp(conversions[r], cstring(txt.Safe()) + o, olen))
            {
                size_t rlen = strlen(conversions[r+1]);
                if (!replace)
                {
                    replace = rt.allocate(o);
                    if (!replace)
                        return result;
                    memmove((byte *) replace.Safe(), txt.Safe(), o);
                }
                byte *cp = rt.allocate(rlen);
                if (!cp)
                    return result;
                memcpy(cp, conversions[r+1], rlen);
                replaced = true;
                o += olen-1;
            }
        }

        if (!replaced && replace)
        {
            byte *cp = rt.allocate(1);
            if (!cp)
                return result;
            *cp = utf8(txt)[o];
        }
    }

    if (replace)
        if (text_p ok = make(replace.Safe(), scr.growth()))
            result = ok;

    return result;
}
