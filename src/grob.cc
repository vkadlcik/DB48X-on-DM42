// ****************************************************************************
//  grob.cc                                                       DB48X project
// ****************************************************************************
//
//   File Description:
//
//
//
//
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

#include "grob.h"

#include "graphics.h"
#include "parser.h"
#include "renderer.h"
#include "sysmenu.h"
#include "user_interface.h"

#include <cstdlib>
#include <cstring>

SIZE_BODY(grob)
// ----------------------------------------------------------------------------
//   Compute the size of a graphic object
// ----------------------------------------------------------------------------
{
    byte_p  p = o->payload();
    pixsize w = leb128<pixsize>(p);
    pixsize h = leb128<pixsize>(p);
    p += (w + 7) / 8 * h;
    return ptrdiff(p, o);
}


static inline byte hex(byte c)
// ----------------------------------------------------------------------------
//   A quick and dirty hex-to-number conversion
// ----------------------------------------------------------------------------
{
    return c >= '0' && c <= '9' ? c - '0'
        : c >= 'A' && c <= 'F' ? c - 'A' + 10
        : c >= 'a' && c <= 'f' ? c - 'a' + 10
        : 0xFF;
}


static void bitflip(byte *start, uint width, uint height, bool forward)
// ----------------------------------------------------------------------------
//    Invert the bits from left to right for HP48 compatibility
// ----------------------------------------------------------------------------
{
#ifdef REVERSE_GROBS
    uint scan = (width + 7) / 8;
    for (uint y = 0; y < height; y++)
    {
        byte *row = start + scan * y;
        if (forward)
            for (uint r = 0; r < scan; r++)
                row[r] = ~((row[r] >> 4) | (row[r] << 4));
        for (uint x = 0; x < width / 2; x++)
        {
            uint r = width - 1 - x;
            uint lo = x / 8;
            uint ro = r / 8;
            uint lb = x % 8;
            uint rb = r % 8;
            bool ls = (row[lo] & (1 << lb));
            bool rs = (row[ro] & (1 << rb));
            row[lo] = (row[lo] & ~(1 << lb)) | (rs << lb);
            row[ro] = (row[ro] & ~(1 << rb)) | (ls << rb);
        }
        if (!forward)
            for (uint r = 0; r < scan; r++)
                row[r] = ~((row[r] >> 4) | (row[r] << 4));
    }
#endif
}


PARSE_BODY(grob)
// ----------------------------------------------------------------------------
//  Parse a graphic object
// ----------------------------------------------------------------------------
{
    utf8    src = p.source;
    cstring s   = cstring(src);
    cstring e   = s + p.length;
    if (strncasecmp(s, "grob ", 5))
        return SKIP;
    s += 5;

    pixsize w = strtoul(s, (char **) &s, 10);
    pixsize h = strtoul(s, (char **) &s, 10);

    while (s < e && isspace(*s))
        s++;

    grob_g g = grob::make(w, h, nullptr);
    if (!g)
        return ERROR;

    size_t len = (w + 7) / 8 * h;
    byte   b   = 0;
    byte * d = (byte *) g->pixels(nullptr, nullptr, nullptr);
    byte * d0 = d;
    bool write = true;

    while (len && s < e)
    {
        byte c = hex(*s++);
        if (c == 0xFF)
            break;
        b = (b << 4) | c;
        write = !write;
        if (write)
        {
            *d++ = b;
            b = 0;
            len--;
        }
    }

    // Flip the bits from right to left for HP48 compatibility
    bitflip(d0, w, h, true);

    p.end = s - cstring(src);
    p.out = g.Safe();

    return OK;
}


RENDER_BODY(grob)
// ----------------------------------------------------------------------------
//  Render the graphic object
// ----------------------------------------------------------------------------
{
    pixsize w = 0;
    pixsize h = 0;
    byte_p data = o->pixels(&w, &h);
    if (r.stack())
    {
        r.printf("Graphic %u x %u", w, h);
    }
    else
    {
        r.put(Settings.command_fmt, utf8("grob"));
        r.printf(" %u %u ", w, h);

        // Flip the bits from right to left for HP48 compatibility
        byte * data0 = (byte *) data;
        bitflip(data0, w, h, false);

        size_t len = (w + 7) / 8 * h;
        while(len--)
            r.printf("%02X", *data++);

        // Flip the bits from right to left again for HP48 compatibility
        bitflip(data0, w, h, true);
    }
    return r.size();
}


object::result grob::command(grob::blitop op)
// ----------------------------------------------------------------------------
//   The shared code for GXor, GOr and GAnd
// ----------------------------------------------------------------------------
{
    if (!rt.args(3))
        return ERROR;
    if (object_p coords = rt.stack(1))
    {
        PlotParameters ppar;
        coord x = ppar.pair_pixel_x(coords);
        coord y = ppar.pair_pixel_y(coords);
        object_p src = rt.stack(0);
        object_p dst = rt.stack(2);

        if (!rt.error())
        {
            if (grob_p sg = src->as<grob>())
            {
                ui.draw_graphics();
                surface srcs = sg->pixels();
                grob_p dg = dst->as<grob>();
                if (dg || dst->type() == ID_Pict)
                {
                    surface dsts = dg ? dg->pixels() : Screen;
                    point p(0,0);
                    rect drect = srcs.area();
                    drect.offset(x,y);

                    rt.drop(2 + (dg == nullptr));
                    blitter::blit<blitter::CLIP_ALL>(dsts, srcs,
                                                     drect, p,
                                                     op, pattern::black);
                    ui.draw_dirty(drect);
                    refresh_dirty();
                    return OK;
                }
            }
            rt.type_error();
        }
    }
    return ERROR;
}
