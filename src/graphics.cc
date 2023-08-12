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
#include "variables.h"

typedef const based_integer *based_integer_p;
typedef const based_bignum  *based_bignum_p;
using std::max;
using std::min;



// ============================================================================
//
//   Plot parameters
//
// ============================================================================

PlotParameters::PlotParameters()
// ----------------------------------------------------------------------------
//   Default values
// ----------------------------------------------------------------------------
    : type(command::ID_Function),
      xmin(integer::make(-10)),
      ymin(integer::make(-6)),
      xmax(integer::make(10)),
      ymax(integer::make(6)),
      independent(symbol::make("x")),
      dependent(symbol::make("y")),
      resolution(integer::make(0)),
      xorigin(integer::make(0)),
      yorigin(integer::make(0)),
      xticks(integer::make(1)),
      yticks(integer::make(1)),
      xlabel(text::make("x")),
      ylabel(text::make("y"))
{
    parse();
}


bool PlotParameters::parse(list_g parms)
// ----------------------------------------------------------------------------
//   Parse a PPAR / PlotParameters list
// ----------------------------------------------------------------------------
{
    if (!parms)
        return false;

    uint index = 0;
    for (object_p obj: *parms)
    {
        bool valid = false;
        switch(index)
        {
        case 0:                 // xmin,ymin
        case 1:                 // xmax,ymax
            if (algebraic_g xa = obj->algebraic_child(0))
            {
                if (algebraic_g ya = obj->algebraic_child(1))
                {
                    (index ? xmax : xmin) = xa;
                    (index ? ymax : ymin) = ya;
                    valid = true;
                }
            }
            break;

        case 2:                 // Independent variable
        case 6:                 // Dependent variable
            if (symbol_g sym = obj->as<symbol>())
            {
                (index == 2 ? independent : dependent) = sym;
                valid = true;
            }
            break;

        case 3:
            valid = obj->is_real() || obj->is_based();
            if (valid)
                resolution = algebraic_p(obj);
            break;
        case 4:
            if (list_g origin = obj->as<list>())
            {
                obj = origin->at(0);
                if (object_p ticks = origin->at(1))
                {
                    if (ticks->is_real() || ticks->is_based())
                    {
                        xticks = yticks = algebraic_p(ticks);
                        valid = true;
                    }
                    else if (list_p tickxy = ticks->as<list>())
                    {
                        if (algebraic_g xa = tickxy->algebraic_child(0))
                        {
                            if (algebraic_g ya = tickxy->algebraic_child(0))
                            {
                                xticks = xa;
                                yticks = ya;
                                valid = true;
                            }
                        }
                    }

                }
                if (valid)
                {
                    if (object_p xl = origin->at(2))
                    {
                        valid = false;
                        if (object_p yl = origin->at(3))
                        {
                            if (text_p xt = xl->as<text>())
                            {
                                if (text_p yt = yl->as<text>())
                                {
                                    xlabel = xt;
                                    ylabel = yt;
                                    valid = true;
                                }
                            }
                        }
                    }
                }
                if (!valid)
                {
                    rt.invalid_ppar_error();
                    return false;
                }
            }
            if (obj->is_complex())
            {
                if (algebraic_g xa = obj->algebraic_child(0))
                {
                    if (algebraic_g ya = obj->algebraic_child(1))
                    {
                        xorigin = xa;
                        yorigin = ya;
                        valid = true;
                    }
                }
            }
            break;
        case 5:
            valid = obj->is_plot();
            if (valid)
                type = obj->type();
            break;

        default:
            break;
        }
        if (!valid)
        {
            rt.invalid_ppar_error();
            return false;
        }
        index++;
    }
    return true;
}


bool PlotParameters::parse(symbol_g name)
// ----------------------------------------------------------------------------
//   Parse plot parameters from a variable name
// ----------------------------------------------------------------------------
{
    if (object_p obj = directory::recall_all(name))
        if (list_p parms = obj->as<list>())
            return parse(parms);
    return false;
}


bool PlotParameters::parse(cstring name)
// ----------------------------------------------------------------------------
//   Parse plot parameters from C variable name
// ----------------------------------------------------------------------------
{
    symbol_p sym = symbol::make(name);
    return parse(sym);
}


bool PlotParameters::parse()
// ----------------------------------------------------------------------------
//   Check if we have PlotParameters or PPAR
// ----------------------------------------------------------------------------
{
    return parse("PlotParameters") || parse("PPAR");
}



// ============================================================================
//
//   Coordinate conversions
//
// ============================================================================

coord PlotParameters::pixel_adjust(object_r    obj,
                                   algebraic_r min,
                                   algebraic_r max,
                                   uint        scale,
                                   bool        isSize)
// ----------------------------------------------------------------------------
//  Convert an object to a coordinate
// ----------------------------------------------------------------------------
{
    if (!obj.Safe())
        return 0;

    coord       result = 0;
    object::id  ty     = obj->type();

    switch(ty)
    {
    case object::ID_integer:
    case object::ID_neg_integer:
    case object::ID_bignum:
    case object::ID_neg_bignum:
    case object::ID_fraction:
    case object::ID_neg_fraction:
    case object::ID_big_fraction:
    case object::ID_neg_big_fraction:
    case object::ID_decimal32:
    case object::ID_decimal64:
    case object::ID_decimal128:
    {
        algebraic_g range  = max - min;
        algebraic_g pos    = algebraic_p(obj.Safe());
        algebraic_g sa     = integer::make(scale);

        // Avoid divide by zero for bogus input
        if (!range || range->is_zero())
            range = integer::make(1);

        if (!isSize)
            pos = pos - min;
        pos = pos / range * sa;
        if (pos)
            result = pos->as_int32(0, false);
        return result;
    }

#if CONFIG_FIXED_BASED_OBJECTS
    case object::ID_hex_integer:
    case object::ID_dec_integer:
    case object::ID_oct_integer:
    case object::ID_bin_integer:
#endif // CONFIG_FIXED_BASED_OBJECTS
    case object::ID_based_integer:
        result = based_integer_p(obj.Safe())->value<coord>();
        break;

#if CONFIG_FIXED_BASED_OBJECTS
    case object::ID_hex_bignum:
    case object::ID_dec_bignum:
    case object::ID_oct_bignum:
    case object::ID_bin_bignum:
#endif // CONFIG_FIXED_BASED_OBJECTS
    case object::ID_based_bignum:
        result = based_bignum_p(obj.Safe())->value<coord>();
        break;

    default:
        rt.type_error();
        break;
    }

    return result;
}


coord PlotParameters::pixel_x(object_r pos) const
// ----------------------------------------------------------------------------
//   Given a position (can be a complex, a list or a vector), return x
// ----------------------------------------------------------------------------
{
    if (object_g x = pos->algebraic_child(0))
        return pixel_adjust(x, xmin, xmax, Screen.area().width());
    return 0;
}


coord PlotParameters::pixel_y(object_r pos) const
// ----------------------------------------------------------------------------
//   Given a position (can be a complex, a list or a vector), return y
// ----------------------------------------------------------------------------
{
    if (object_g y = pos->algebraic_child(1))
        return pixel_adjust(y, ymin, ymax, Screen.area().height());
    return 0;
}


COMMAND_BODY(Disp)
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
            PlotParameters ppar;
            coord          x      = 0;
            coord          y      = 0;
            font_p         font   = settings::font(settings::STACK);
            bool           erase  = true;
            bool           invert = false;
            id             ty     = pos->type();

            if (ty == ID_rectangular || ty == ID_polar ||
                ty == ID_list || ty == ID_array)
            {
                x = ppar.pixel_x(pos);
                y = ppar.pixel_y(pos);

                if (ty == ID_list || ty == ID_array)
                {
                    list_g args = list_p(pos.Safe());
                    if (object_p fontid = args->at(2))
                    {
                        uint32_t i = fontid->as_uint32(settings::STACK, false);
                        font = settings::font(settings::font_id(i));
                    }
                    if (object_p eflag = args->at(3))
                        erase = eflag->as_truth(true);
                    if (object_p iflag = args->at(4))
                        invert = iflag->as_truth(true);
                }
            }
            else if (pos->is_algebraic())
            {
                algebraic_g ya = algebraic_p(pos.Safe());
                ya = ya * integer::make(LCD_H/8);
                y = ya->as_uint32(0, false) - (LCD_H/8);
            }

            utf8          txt = nullptr;
            size_t        len = 0;
            blitter::size h   = font->height();

            if (text_p t = todisp->as<text>())
                txt = t->value(&len);
            else if (text_p tr = todisp->as_text(true, false))
                txt = tr->value(&len);

            pattern bg   = invert ? Settings.foreground : Settings.background;
            pattern fg   = invert ? Settings.background : Settings.foreground;
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


COMMAND_BODY(DispXY)
// ----------------------------------------------------------------------------
//   To be implemented
// ----------------------------------------------------------------------------
{
    rt.unimplemented_error();
    return ERROR;
}


COMMAND_BODY(Line)
// ----------------------------------------------------------------------------
//   Draw a line between the coordinates
// ----------------------------------------------------------------------------
{
    object_g p1 = rt.stack(1);
    object_g p2 = rt.stack(0);
    if (p1 && p2)
    {
        PlotParameters ppar;
        coord x1 = ppar.pixel_x(p1);
        coord y1 = ppar.pixel_y(p1);
        coord x2 = ppar.pixel_x(p2);
        coord y2 = ppar.pixel_y(p2);
        if (!rt.error())
        {
            rt.drop(2);
            Screen.line(x1, y1, x2, y2,
                        Settings.line_width, Settings.foreground);
            ui.draw_dirty(min(x1,x2), min(y1,y2), max(x1,x2), max(y1,y2));
            refresh_dirty();
            return OK;
        }
    }
    return ERROR;
}


COMMAND_BODY(Ellipse)
// ----------------------------------------------------------------------------
//   Draw an ellipse between the given coordinates
// ----------------------------------------------------------------------------
{
    object_g p1 = rt.stack(1);
    object_g p2 = rt.stack(0);
    if (p1 && p2)
    {
        PlotParameters ppar;
        coord x1 = ppar.pixel_x(p1);
        coord y1 = ppar.pixel_y(p1);
        coord x2 = ppar.pixel_x(p2);
        coord y2 = ppar.pixel_y(p2);
        if (!rt.error())
        {
            rt.drop(2);
            Screen.ellipse(x1, y1, x2, y2,
                           Settings.line_width, Settings.foreground);
            ui.draw_dirty(min(x1,x2), min(y1,y2), max(x1,x2), max(y1,y2));
            refresh_dirty();
            return OK;
        }
    }
    return ERROR;
}


static inline uint RadiusAdjust()
// ----------------------------------------------------------------------------
//   Adjustment for the radius of a circle or rounded rectangle
// ----------------------------------------------------------------------------
{
    return Screen.area().width() * 2;
}


COMMAND_BODY(Circle)
// ----------------------------------------------------------------------------
//   Draw a circle between the given coordinates
// ----------------------------------------------------------------------------
{
    object_g co = rt.stack(1);
    object_g ro = rt.stack(0);
    if (co && ro)
    {
        PlotParameters ppar;
        coord x = ppar.pixel_x(co);
        coord y = ppar.pixel_y(co);
        coord r = ppar.size_adjust(ro, ppar.xmin, ppar.xmax, RadiusAdjust());
        if (r < 0)
            r = -r;
        if (!rt.error())
        {
            rt.drop(2);
            Screen.circle(x, y, r, Settings.line_width, Settings.foreground);
            ui.draw_dirty(x-r, y-r, x+r, y+r);
            refresh_dirty();
            return OK;
        }
    }
    return ERROR;
}


COMMAND_BODY(Rect)
// ----------------------------------------------------------------------------
//   Draw a rectangle between the given coordinates
// ----------------------------------------------------------------------------
{
    object_g p1 = rt.stack(1);
    object_g p2 = rt.stack(0);
    if (p1 && p2)
    {
        PlotParameters ppar;
        coord x1 = ppar.pixel_x(p1);
        coord y1 = ppar.pixel_y(p1);
        coord x2 = ppar.pixel_x(p2);
        coord y2 = ppar.pixel_y(p2);
        if (!rt.error())
        {
            rt.drop(2);
            Screen.rectangle(x1, y1, x2, y2,
                             Settings.line_width, Settings.foreground);
            ui.draw_dirty(min(x1,x2), min(y1,y2), max(x1,x2), max(y1,y2));
            refresh_dirty();
            return OK;
        }
    }
    return ERROR;
}


COMMAND_BODY(RRect)
// ----------------------------------------------------------------------------
//   Draw a rounded rectangle between the given coordinates
// ----------------------------------------------------------------------------
{
    object_g p1 = rt.stack(2);
    object_g p2 = rt.stack(1);
    object_g ro = rt.stack(0);
    if (p1 && p2 && ro)
    {
        PlotParameters ppar;
        coord x1 = ppar.pixel_x(p1);
        coord y1 = ppar.pixel_y(p1);
        coord x2 = ppar.pixel_x(p2);
        coord y2 = ppar.pixel_y(p2);
        coord r  = ppar.size_adjust(ro, ppar.xmin, ppar.xmax, RadiusAdjust());
        if (!rt.error())
        {
            rt.drop(3);
            Screen.rounded_rectangle(x1, y1, x2, y2, r,
                                     Settings.line_width, Settings.foreground);
            ui.draw_dirty(min(x1,x2), min(y1,y2), max(x1,x2), max(y1,y2));
            refresh_dirty();
            return OK;
        }
    }
    return ERROR;
}


COMMAND_BODY(ClLCD)
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
