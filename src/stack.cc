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

#include "display.h"
#include "object.h"
#include "runtime.h"
#include "input.h"

#include <dmcp.h>


stack Stack;
runtime &stack::RT = runtime::RT;


stack::stack()
// ----------------------------------------------------------------------------
//   Constructor does nothing at the moment
// ----------------------------------------------------------------------------
{}


void stack::draw_stack()
// ----------------------------------------------------------------------------
//   Draw the stack on screen
// ----------------------------------------------------------------------------
{
    display dstk(fReg);
    dstk.font(3);

    int lineHeight = dstk.lineHeight();
    int top        = lcd_lineHeight(t20) + 2;
    int bottom     = Input.stack_screen_bottom() - 1;
    int depth      = RT.depth();
    int hdrx       = dstk.width('0') + 2;

    if (!depth)
        return;

    cstring saveError = RT.error();

    lcd_fill_rect(hdrx, top, 1, bottom - top, 1);
    if (RT.editing())
        lcd_fill_rect(0, bottom, LCD_W, 1, 1);

    char buf[80];
    for (int level = 0; level < depth; level++)
    {
        int y = bottom - (level + 1) * lineHeight;
        if (y <= top)
            break;

        dstk.xy(0, y).clearing(false).write("%d", level+1);

        object_p obj = RT.stack(level);
        size_t size = obj->render(buf, sizeof(buf));
        if (size >= sizeof(buf))
            size = sizeof(buf) - 1;
        buf[size] = 0;

        int width = dstk.width(buf);
        dstk.xy(LCD_W - width - 2, y).write(buf);
    }

    // Clear any error raised during rendering
    RT.error(saveError);
}
