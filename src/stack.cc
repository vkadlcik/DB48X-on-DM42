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

#include "graphics.h"
#include "input.h"
#include "object.h"
#include "runtime.h"
#include "target.h"

#include <dmcp.h>


stack    Stack;
runtime &stack::RT = runtime::RT;

using coord        = graphics::coord;
using size         = graphics::size;


stack::stack()
// ----------------------------------------------------------------------------
//   Constructor does nothing at the moment
// ----------------------------------------------------------------------------
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
    size  lineHeight = StackFont->height();
    coord top        = HeaderFont->height() + 2;
    coord bottom     = Input.stack_screen_bottom() - 1;
    uint  depth      = RT.depth();
    uint  digits     = countDigits(depth);
    coord hdrx       = StackFont->width('0') * digits + 2;
    size  avail      = LCD_W - hdrx - 10;

    Screen.fill(0, top, LCD_W, bottom - 1, pattern::white);
    if (!depth)
        return;

    utf8 saveError = RT.error();
    utf8 saveCmd   = RT.command();

    Screen.fill(hdrx, top, hdrx, bottom, pattern::gray50);
    if (RT.editing())
        Screen.fill(0, bottom, LCD_W, bottom, pattern::gray50);

    char buf[80];
    for (uint level = 0; level < depth; level++)
    {
        coord y = bottom - (level + 1) * lineHeight;
        if (y <= top)
            break;

        snprintf(buf, sizeof(buf), "%d", level + 1);
        size w = StackFont->width(utf8(buf));
        Screen.text(hdrx - w, y, utf8(buf), StackFont);

        gcobj  obj  = RT.stack(level);
        size_t len = obj->render(buf, sizeof(buf));
        if (len >= sizeof(buf))
            len = sizeof(buf) - 1;
        buf[len]  = 0;

        w = StackFont->width(utf8(buf));
        if (w > avail)
        {
            unicode sep   = L'…';
            coord    skip  = StackFont->width(sep) + w - avail;
            coord    sskip = skip;
            coord    x     = LCD_W - avail;
            coord    split = 200;
            utf8     p     = utf8(buf);

            while (*p && x < split)
            {
                x += StackFont->width(utf8_codepoint(p));
                p = utf8_next(p);
            }
            x += StackFont->width(sep);
            while (*p && skip >= 0)
            {
                skip -= StackFont->width(utf8_codepoint(p));
                p = utf8_next(p);
            }
            while (*p)
            {
                x += StackFont->width(utf8_codepoint(p));
                p = utf8_next(p);
            }
            x = 2 * LCD_W - avail - x;
            p = utf8(buf);
            skip = sskip;

            while (*p && x < split)
            {
                x = Screen.glyph(x, y, utf8_codepoint(p), StackFont);
                p = utf8_next(p);
            }
            x = Screen.glyph(x, y, sep, StackFont, pattern::gray50);
            while (*p && skip >= 0)
            {
                skip -= StackFont->width(utf8_codepoint(p));
                p = utf8_next(p);
            }
            while (*p)
            {
                x = Screen.glyph(x, y, utf8_codepoint(p), StackFont);
                p = utf8_next(p);
            }
        }
        else
        {
            Screen.text(LCD_W - w, y, utf8(buf), StackFont);
        }
    }

    // Clear any error raised during rendering
    RT.error(saveError, saveCmd);
}
