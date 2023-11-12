// ****************************************************************************
//  stack.cc                                                      DB48X project
// ****************************************************************************
//
//   File Description:
//
//     Rendering of the objects on the stack
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

#include "stack.h"

#include "blitter.h"
#include "grob.h"
#include "renderer.h"
#include "runtime.h"
#include "settings.h"
#include "target.h"
#include "user_interface.h"
#include "utf8.h"

#include <dmcp.h>


stack    Stack;

using coord = blitter::coord;
using size  = blitter::size;


RECORDER(tests, 16, "Information about tests");

stack::stack()
// ----------------------------------------------------------------------------
//   Constructor does nothing at the moment
// ----------------------------------------------------------------------------
#if SIMULATOR
    : history(), writer(0), reader(0)
#endif
{
}


static inline uint countDigits(uint value)
// ----------------------------------------------------------------------------
//   Count how many digits we need to display a value
// ----------------------------------------------------------------------------
{
    uint result = 1;
    while (value /= 10)
        result++;
    return result;
}


void stack::draw_stack()
// ----------------------------------------------------------------------------
//   Draw the stack on screen
// ----------------------------------------------------------------------------
{
    font_p font       = Settings.result_font();
    font_p hdrfont    = HeaderFont;
    font_p idxfont    = HelpFont;
    size   lineHeight = font->height();
    size   idxHeight  = idxfont->height();
    coord  top        = hdrfont->height() + 2;
    coord  bottom     = ui.stack_screen_bottom();
    uint   depth      = rt.depth();
    uint   digits     = countDigits(depth);
    coord  hdrx       = idxfont->width('0') * digits + 2;
    size   avail      = LCD_W - hdrx - 5;

    Screen.fill(0, top, LCD_W, bottom, pattern::white);
    if (!depth)
        return;

    rect clip      = Screen.clip();

    Screen.fill(hdrx, top, hdrx, bottom, pattern::gray50);
    if (rt.editing())
    {
        bottom--;
        Screen.fill(0, bottom, LCD_W, bottom, pattern::gray50);
    }

    char buf[8];
    coord y = bottom;
#ifdef SIMULATOR
    extern int last_key;
    if (depth == 0)
        output(last_key, object::ID_object, nullptr, 0);
#endif
    for (uint level = 0; level < depth; level++)
    {
        if (coord(y) <= top)
            break;

        grob_g   graph = nullptr;
        object_g obj   = rt.stack(level);
        size     w = 0;
        if (Settings.GraphicStackDisplay())
        {
            auto fid = !level ? Settings.ResultFont() : Settings.StackFont();
            grapher  g(avail - 2, bottom - top, fid,
                       pattern::black, pattern::gray90, true);
            graph = obj->graph(g);
            size gh = graph->height();
            if (level == 0 && lineHeight < gh)
                lineHeight = gh;
            w = graph->width();

#ifdef SIMULATOR
            if (level == 0)
            {
                renderer r(nullptr, ~0U, true);
                size_t   len = obj->render(r);
                utf8     out = r.text();
                output(last_key, obj->type(), out, len);
                record(tests,
                       "Key %d X-reg %+s size %u %s",
                       last_key, object::name(obj->type()), len, out);
            }
#endif // SIMULATOR
        }

        y -= lineHeight;
        coord ytop = y < top ? top : y;
        coord yb   = y + lineHeight-1;
        Screen.clip(0, ytop, LCD_W, yb);

        size idxOffset = (lineHeight - idxHeight) / 2;
        snprintf(buf, sizeof(buf), "%d", level + 1);
        size hw = idxfont->width(utf8(buf));
        Screen.text(hdrx - hw, y + idxOffset, utf8(buf), idxfont);

        if (graph)
        {
            surface s = graph->pixels();
            rect r = s.area();
            r.offset(LCD_W - 2 - w, y);
            Screen.copy(s, r);
        }
        else
        {
            // Text rendering
            renderer r(nullptr, ~0U, true);
            size_t   len = obj->render(r);
            utf8     out = r.text();
#ifdef SIMULATOR
            if (level == 0)
            {
                output(last_key, obj->type(), out, len);
                record(tests,
                       "Key %d X-reg %+s size %u %s",
                       last_key, object::name(obj->type()), len, out);
            }
#endif
            w = font->width(out, len);

            if (w >= avail)
            {
                uint availRows = (y + lineHeight - 1 - top) / lineHeight;
                bool dots = level != 0 || w >= avail * availRows;

                if (!dots)
                {
                    // Try to split into lines
                    size_t rlen[16];
                    uint   rows = 0;
                    utf8   end  = out + len;
                    utf8   rs   = out;
                    size   rw   = 0;
                    size   rx   = 0;
                    for (utf8 p = out; p < end; p = utf8_next(p))
                    {
                        unicode c = utf8_codepoint(p);
                        size cw = font->width(c);
                        rw += cw;
                        if (rw >= avail)
                        {
                            if (rows >= availRows)
                            {
                                dots = true;
                                break;
                            }
                            rlen[rows++] = p - rs;
                            rs = p;
                            if (rx < rw - cw)
                                rx = rw - cw;
                            rw = cw;
                        }
                    }

                    if (!dots)
                    {
                        if (end > rs)
                            rlen[rows++] = end - rs;
                        y -= (rows - 1) * lineHeight;
                        ytop = y < top ? top : y;
                        Screen.clip(0, ytop, LCD_W, yb);
                        rs = out;
                        for (uint r = 0; r < rows; r++)
                        {
                            Screen.text(LCD_W - 2 - rx,
                                        y + r * lineHeight,
                                        rs, rlen[r], font);
                            rs += rlen[r];
                        }
                    }
                }

                if (dots)
                {
                    unicode sep   = L'…';
                    coord   x     = hdrx + 5;
                    coord   split = 200;
                    coord   skip  = font->width(sep) * 3 / 2;
                    size    offs  = lineHeight / 5;

                    Screen.clip(x, ytop, split, yb);
                    Screen.text(x, y, out, len, font);
                    Screen.clip(split, ytop, split + skip, yb);
                    Screen.glyph(split + skip/8, y - offs, sep, font, pattern::gray50);
                    Screen.clip(split+skip, y, LCD_W, yb);
                    Screen.text(LCD_W - 2 - w, y, out, len, font);
                }
            }
            else
            {
                Screen.text(LCD_W - 2 - w, y, out, len, font);
            }

            font = Settings.stack_font();
        }
        lineHeight = font->height();
    }
    Screen.clip(clip);
}
