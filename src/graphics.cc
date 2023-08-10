// ****************************************************************************
//  graphics.cc                                                   DB48X project
// ****************************************************************************
//
//   File Description:
//
//     RPL graphic routines
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

#include "graphics.h"

#include "arithmetic.h"
#include "bignum.h"
#include "blitter.h"
#include "complex.h"
#include "decimal128.h"
#include "fraction.h"
#include "integer.h"
#include "list.h"
#include "sysmenu.h"
#include "target.h"
#include "user_interface.h"

typedef const based_integer *based_integer_p;
typedef const based_bignum  *based_bignum_p;
using std::min;
using std::max;


static coord to_coord(object_g pos, int scaling)
// ----------------------------------------------------------------------------
//  Convert an object to a coordinate
// ----------------------------------------------------------------------------
{
    coord y = 0;
    object::id ty = pos->type();

    switch(ty)
    {
    case object::ID_integer:
    case object::ID_neg_integer:
        y = integer_p(pos.Safe())->value<uint16_t>();
        if (ty == object::ID_neg_integer)
            y = -y;
        y = scaling * (y - 1);
        break;
    case object::ID_bignum:
    case object::ID_neg_bignum:
        y = bignum_p(pos.Safe())->value<uint16_t>();
        if (ty == object::ID_neg_bignum)
            y = -y;
        y = scaling * (y - 1);
        break;

#if CONFIG_FIXED_BASED_OBJECTS
    case object::ID_hex_integer:
    case object::ID_dec_integer:
    case object::ID_oct_integer:
    case object::ID_bin_integer:
#endif // CONFIG_FIXED_BASED_OBJECTS
    case object::ID_based_integer:
        y = based_integer_p(pos.Safe())->value<coord>();
        break;

#if CONFIG_FIXED_BASED_OBJECTS
    case object::ID_hex_bignum:
    case object::ID_dec_bignum:
    case object::ID_oct_bignum:
    case object::ID_bin_bignum:
#endif // CONFIG_FIXED_BASED_OBJECTS
    case object::ID_based_bignum:
        y = based_bignum_p(pos.Safe())->value<coord>();
        break;

    case object::ID_neg_fraction:
    case object::ID_neg_big_fraction:
    case object::ID_fraction:
    case object::ID_big_fraction:
        y = scaling
            * fraction_p(pos.Safe())->numerator()->value<coord>()
            / fraction_p(pos.Safe())->denominator()->value<coord>()
            * (1 - 2 * (ty == object::ID_neg_fraction ||
                        ty == object::ID_neg_big_fraction))
            - scaling;
        break;

    case object::ID_decimal32:
    case object::ID_decimal64:
    case object::ID_decimal128:
    {
        algebraic_g ya = algebraic_p(pos.Safe());
        if (algebraic::real_promotion(ya, object::ID_decimal128))
        {
            algebraic_g scale = algebraic_p(integer::make(scaling));
            ya = ya * scale;
            if (ya.Safe())
                y = ya->as_uint32(0, true);
        }
        break;
    }
    default:
        rt.type_error();
        break;
    }

    return y;
}


COMMAND_BODY(disp)
// ----------------------------------------------------------------------------
//   Display text on the given line
// ----------------------------------------------------------------------------
//   For compatibility reasons, integer values of the line from 1 to 8
//   are positioned like on the HP48, each line taking 30 pixels
//   The coordinate can additionally be one of:
//   - A non-integer value, which allows more precise positioning on screen
//   - A complex number, where the real part is the horizontal position
//     and the imaginary part is the vertical position going up
//   - A list { x y } with the same meaning as for a complex
//   - A list { #x #y } to give pixel-precise coordinates
{
    if (object_g pos = rt.pop())
    {
        if (object_g todisp = rt.pop())
        {
            coord  x       = 0;
            coord  y       = 0;
            font_p font    = settings::font(settings::STACK);
            bool   erase   = true;
            bool   invert  = false;

            if (rectangular_g rpos = pos->as<rectangular>())
            {
                x = to_coord(rpos->x().Safe(), 30);
                y = to_coord(rpos->y().Safe(), 30);
            }
            else if (list_p args = pos->as<list>())
            {
                int      fontid = -1;
                uint     idx    = 0;
                object_g xo, yo;

                for (object_p obj : *args)
                {
                    idx++;
                    switch(idx)
                    {
                    case 1: xo = obj; break;
                    case 2: yo = obj; break;
                    case 3: fontid = obj->as_uint32(true); break;
                    case 4: erase = obj->as_truth(true); break;
                    case 5: invert = obj->as_truth(true); break;
                    default: xo = nullptr; break;
                    }
                }

                if (!xo || !yo)
                {
                    rt.type_error();
                    return ERROR;
                }

                if (fontid >= 0)
                {
                    font_p font = settings::font(settings::font_id(fontid));
                    x = to_coord(xo, font->height());
                    y = to_coord(yo, font->width('M'));
                }
                else
                {
                    x = to_coord(xo, 30);
                    y = to_coord(yo, 30);
                }
            }
            else
            {
                y = to_coord(pos, 30);
            }

            utf8          txt = nullptr;
            size_t        len = 0;
            blitter::size h   = font->height();

            if (text_p t = todisp->as<text>())
                txt = t->value(&len);
            else if (text_p tr = todisp->as_text(true, false))
                txt = tr->value(&len);

            pattern bg   = invert ? pattern::black : pattern::white;
            pattern fg   = invert ? pattern::white : pattern::black;
            utf8    last = txt + len;

            ui.draw_start(false);
            ui.draw_user_screen();
            while (txt < last)
            {
                unicode       cp = utf8_codepoint(txt);
                blitter::size w  = font->width(cp);

                if (x + w >= LCD_W || cp == '\n')
                {
                    x = 0;
                    y += font->height();
                    if (cp == '\n')
                        continue;
                }
                if (cp == '\t')
                    cp = ' ';

                if (erase)
                    Screen.fill(x, y, x+w-1, y+h-1, bg);
                Screen.glyph(x, y, cp, font, fg);
                ui.draw_dirty(x, y , x+w-1, y+h-1);
                txt = utf8_next(txt);
                x += w;
            }

            refresh_dirty();
            return OK;
        }
    }
    return ERROR;
}


COMMAND_BODY(dispxy)
// ----------------------------------------------------------------------------
//   To be implemented
// ----------------------------------------------------------------------------
{
    rt.unimplemented_error();
    return ERROR;
}


COMMAND_BODY(line)
// ----------------------------------------------------------------------------
//   Draw a line between the coordinates
// ----------------------------------------------------------------------------
{
    object_p x1o = rt.stack(3);
    object_p y1o = rt.stack(2);
    object_p x2o = rt.stack(1);
    object_p y2o = rt.stack(0);
    if (x1o && y1o && x2o && y2o)
    {
        coord x1 = to_coord(x1o, 1);
        coord y1 = to_coord(y1o, 1);
        coord x2 = to_coord(x2o, 1);
        coord y2 = to_coord(y2o, 1);
        if (!rt.error())
        {
            rt.drop(4);
            Screen.line(x1, y1, x2, y2,
                        Settings.line_width, Settings.foreground);
            ui.draw_dirty(min(x1,x2), min(y1,y2), max(x1,x2), max(y1,y2));
            refresh_dirty();
            return OK;
        }
    }
    return ERROR;
}


COMMAND_BODY(ellipse)
// ----------------------------------------------------------------------------
//   Draw an ellipse between the given coordinates
// ----------------------------------------------------------------------------
{
    object_p x1o = rt.stack(3);
    object_p y1o = rt.stack(2);
    object_p x2o = rt.stack(1);
    object_p y2o = rt.stack(0);
    if (x1o && y1o && x2o && y2o)
    {
        coord x1 = to_coord(x1o, 1);
        coord y1 = to_coord(y1o, 1);
        coord x2 = to_coord(x2o, 1);
        coord y2 = to_coord(y2o, 1);
        if (!rt.error())
        {
            rt.drop(4);
            Screen.ellipse(x1, y1, x2, y2,
                           Settings.line_width, Settings.foreground);
            ui.draw_dirty(min(x1,x2), min(y1,y2), max(x1,x2), max(y1,y2));
            refresh_dirty();
            return OK;
        }
    }
    return ERROR;
}


COMMAND_BODY(circle)
// ----------------------------------------------------------------------------
//   Draw a circle between the given coordinates
// ----------------------------------------------------------------------------
{
    object_p xo = rt.stack(2);
    object_p yo = rt.stack(1);
    object_p ro = rt.stack(0);
    if (xo && yo && ro)
    {
        coord x = to_coord(xo, 1);
        coord y = to_coord(yo, 1);
        coord r  = to_coord(ro, 1);
        if (r < 0)
            r = -r;
        if (!rt.error())
        {
            rt.drop(3);
            Screen.circle(x, y, r, Settings.line_width, Settings.foreground);
            ui.draw_dirty(x-r, y-r, x+r, y+r);
            refresh_dirty();
            return OK;
        }
    }
    return ERROR;
}


COMMAND_BODY(cllcd)
// ----------------------------------------------------------------------------
//   Clear the LCD screen before drawing stuff on it
// ----------------------------------------------------------------------------
{
    ui.draw_start(false);
    ui.draw_user_screen();
    Screen.fill(0, 0, LCD_W, LCD_H, pattern::white);
    ui.draw_dirty(0, 0, LCD_W-1, LCD_H-1);
    refresh_dirty();
    return OK;
}
