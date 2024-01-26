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
#include "compare.h"
#include "complex.h"
#include "grob.h"
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

PlotParametersAccess::PlotParametersAccess()
// ----------------------------------------------------------------------------
//   Default values
// ----------------------------------------------------------------------------
    : type(command::ID_Function),
      xmin(integer::make(-10)),
      ymin(integer::make(-6)),
      xmax(integer::make(10)),
      ymax(integer::make(6)),
      independent(symbol::make("x")),
      imin(integer::make(-10)),
      imax(integer::make(10)),
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


object_p PlotParametersAccess::name()
// ----------------------------------------------------------------------------
//   Return the name for the variable
// ----------------------------------------------------------------------------
{
    return command::static_object(object::ID_PlotParameters);

}


bool PlotParametersAccess::parse(list_p parms)
// ----------------------------------------------------------------------------
//   Parse a PPAR / PlotParametersAccess list
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
            if (list_g ilist = obj->as<list>())
            {
                int ok = 0;
                if (object_p name = ilist->at(0))
                    if (symbol_p sym = name->as<symbol>())
                        ok++, independent = sym;
                if (object_p obj = ilist->at(1))
                    if (algebraic_p val = obj->as_algebraic())
                        ok++, imin = val;
                if (object_p obj = ilist->at(2))
                    if (algebraic_p val = obj->as_algebraic())
                        ok++, imax = val;
                valid = ok == 3;
                break;
            }

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

        // Check that we have sane input
        if (valid)
            valid = check_validity();

        if (!valid)
        {
            rt.invalid_ppar_error();
            return false;
        }
        index++;
    }
    return true;
}


bool PlotParametersAccess::parse(object_p name)
// ----------------------------------------------------------------------------
//   Parse plot parameters from a variable name
// ----------------------------------------------------------------------------
{
    if (object_p obj = directory::recall_all(name, false))
        if (list_p parms = obj->as<list>())
            return parse(parms);
    return false;
}


bool PlotParametersAccess::write(object_p name) const
// ----------------------------------------------------------------------------
//   Write out the plot parameters in case they were changed
// ----------------------------------------------------------------------------
{
    if (!check_validity())
    {
        rt.invalid_ppar_error();
        return false;
    }

    if (directory *dir = rt.variables(0))
    {
        rectangular_g zmin = rectangular::make(xmin, ymin);
        rectangular_g zmax = rectangular::make(xmax, ymax);
        list_g        indep = list::make(independent, imin, imax);
        complex_g     zorig = rectangular::make(xorigin, yorigin);
        list_g        ticks = list::make(xticks, yticks);
        list_g        axes  = list::make(zorig, ticks, xlabel, ylabel);
        object_g      ptype = command::static_object(type);
        symbol_g      dep = dependent;

        list_g        par =
            list::make(zmin, zmax, indep, resolution, axes, ptype, dep);
        if (par)
            return dir->store(name, +par);
    }
    return false;

}


bool PlotParametersAccess::check_validity() const
// ----------------------------------------------------------------------------
//   Check validity of the plot parameters
// ----------------------------------------------------------------------------
{
    // All labels must be defined
    if (!xmin|| !xmax|| !ymin|| !ymax)
        return false;
    if (!independent|| !dependent|| !resolution)
        return false;
    if (!imin|| !imax)
        return false;
    if (!resolution|| !xorigin|| !yorigin)
        return false;
    if (!xticks|| !yticks|| !xlabel|| !ylabel)
        return false;

    // Check values that must be real
    if (!xmin->is_real() || !xmax->is_real())
        return false;
    if (!ymin->is_real() || !ymax->is_real())
        return false;
    if (!imin->is_real() || !imax->is_real())
        return false;
    if (!resolution->is_real())
        return false;
    if (!xorigin->is_real() || !yorigin->is_real())
        return false;
    if (!xticks->is_real() && !xticks->is_based())
        return false;
    if (!yticks->is_real() && !yticks->is_based())
        return false;
    if (xlabel->type() != object::ID_text || ylabel->type() != object::ID_text)
        return false;

    // Check that the ranges are not empty
    algebraic_g test = xmin >= xmax;
    if (test->as_truth(true))
        return false;
    test = ymin >= ymax;
    if (test->as_truth(true))
        return false;
    test = imin >= imax;
    if (test->as_truth(true))
        return false;

    return true;
}




// ============================================================================
//
//   Coordinate conversions
//
// ============================================================================

coord PlotParametersAccess::pixel_adjust(object_r    obj,
                                         algebraic_r min,
                                         algebraic_r max,
                                         uint        scale,
                                         bool        isSize)
// ----------------------------------------------------------------------------
//  Convert an object to a coordinate
// ----------------------------------------------------------------------------
{
    if (!obj)
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
    case object::ID_hwfloat:
    case object::ID_hwdouble:
    case object::ID_decimal:
    case object::ID_neg_decimal:
    {
        algebraic_g range  = max - min;
        algebraic_g pos    = algebraic_p(+obj);
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
        result = based_integer_p(+obj)->value<ularge>();
        break;

#if CONFIG_FIXED_BASED_OBJECTS
    case object::ID_hex_bignum:
    case object::ID_dec_bignum:
    case object::ID_oct_bignum:
    case object::ID_bin_bignum:
#endif // CONFIG_FIXED_BASED_OBJECTS
    case object::ID_based_bignum:
        result = based_bignum_p(+obj)->value<ularge>();
        break;

    default:
        rt.type_error();
        break;
    }

    return result;
}


coord PlotParametersAccess::size_adjust(object_r    p,
                                        algebraic_r min,
                                        algebraic_r max,
                                        uint        scale)
// ----------------------------------------------------------------------------
//   Adjust the size of the parameters
// ----------------------------------------------------------------------------
{
    return pixel_adjust(p, min, max, scale, true);
}



coord PlotParametersAccess::pair_pixel_x(object_r pos) const
// ----------------------------------------------------------------------------
//   Given a position (can be a complex, a list or a vector), return x
// ----------------------------------------------------------------------------
{
    if (object_g x = pos->child(0))
        return pixel_adjust(x, xmin, xmax, Screen.area().width());
    return 0;
}


coord PlotParametersAccess::pair_pixel_y(object_r pos) const
// ----------------------------------------------------------------------------
//   Given a position (can be a complex, a list or a vector), return y
// ----------------------------------------------------------------------------
{
    if (object_g y = pos->child(1))
        return pixel_adjust(y, ymax, ymin, Screen.area().height());
    return 0;
}


coord PlotParametersAccess::pixel_x(algebraic_r x) const
// ----------------------------------------------------------------------------
//   Adjust a position given as an algebraic value
// ----------------------------------------------------------------------------
{
    object_g xo = object_p(+x);
    return pixel_adjust(xo, xmin, xmax, Screen.area().width());
}


coord PlotParametersAccess::pixel_y(algebraic_r y) const
// ----------------------------------------------------------------------------
//   Adjust a position given as an algebraic value
// ----------------------------------------------------------------------------
{
    object_g yo = object_p(+y);
    return pixel_adjust(yo, ymax, ymin, Screen.area().height());
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
    if (!rt.args(2))
        return ERROR;

    if (object_g pos = rt.pop())
    {
        if (object_g todisp = rt.pop())
        {
            PlotParametersAccess ppar;
            coord          x      = 0;
            coord          y      = 0;
            font_p         font   = settings::font(settings::STACK);
            bool           erase  = true;
            bool           invert = false;
            id             ty     = pos->type();

            if (ty == ID_rectangular || ty == ID_polar ||
                ty == ID_list || ty == ID_array)
            {
                x = ppar.pair_pixel_x(pos);
                y = ppar.pair_pixel_y(pos);

                if (ty == ID_list || ty == ID_array)
                {
                    list_g args = list_p(+pos);
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
                algebraic_g ya = algebraic_p(+pos);
                ya = ya * integer::make(LCD_H/8);
                y = ya->as_uint32(0, false) - (LCD_H/8);
            }
            else if (pos->is_based())
            {
                algebraic_g ya = algebraic_p(+pos);
                y = ppar.pixel_y(ya);
            }
            else
            {
                rt.type_error();
                return ERROR;
            }


            utf8          txt = nullptr;
            size_t        len = 0;
            blitter::size h   = font->height();

            if (text_p t = todisp->as<text>())
                txt = t->value(&len);
            else if (text_p tr = todisp->as_text(true, false))
                txt = tr->value(&len);

            pattern bg   = Settings.Background();
            pattern fg   = Settings.Foreground();
            utf8    last = txt + len;
            coord   x0   = x;

            if (invert)
                std::swap(bg, fg);
            ui.draw_graphics();
            while (txt < last)
            {
                unicode       cp = utf8_codepoint(txt);
                blitter::size w  = font->width(cp);

                txt = utf8_next(txt);
                if (x + w >= LCD_W || cp == '\n')
                {
                    x = x0;
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


static void graphics_dirty(coord x1, coord y1, coord x2, coord y2, size lw)
// ----------------------------------------------------------------------------
//   Mark region as dirty with extra size
// ----------------------------------------------------------------------------
{
    if (x1 > x2)
        std::swap(x1, x2);
    if (y1 > y2)
        std::swap(y1, y2);
    if (lw)
        ui.draw_dirty(x1 - lw/2, y1 - lw/2, x2 + (lw-1)/2, y2 + (lw-1)/2);
    else
        ui.draw_dirty(x1, y1, x2, y2);
    refresh_dirty();
}


COMMAND_BODY(Line)
// ----------------------------------------------------------------------------
//   Draw a line between the coordinates
// ----------------------------------------------------------------------------
{
    if (!rt.args(2))
        return ERROR;
    object_g p1 = rt.stack(1);
    object_g p2 = rt.stack(0);
    if (p1 && p2)
    {
        PlotParametersAccess ppar;
        coord x1 = ppar.pair_pixel_x(p1);
        coord y1 = ppar.pair_pixel_y(p1);
        coord x2 = ppar.pair_pixel_x(p2);
        coord y2 = ppar.pair_pixel_y(p2);
        if (!rt.error())
        {
            blitter::size lw = Settings.LineWidth();
            rt.drop(2);
            ui.draw_graphics();
            Screen.line(x1, y1, x2, y2, lw, Settings.Foreground());
            graphics_dirty(x1, y1, x2, y2, lw);
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
    if (!rt.args(2))
        return ERROR;
    object_g p1 = rt.stack(1);
    object_g p2 = rt.stack(0);
    if (p1 && p2)
    {
        PlotParametersAccess ppar;
        coord x1 = ppar.pair_pixel_x(p1);
        coord y1 = ppar.pair_pixel_y(p1);
        coord x2 = ppar.pair_pixel_x(p2);
        coord y2 = ppar.pair_pixel_y(p2);
        if (!rt.error())
        {
            blitter::size lw = Settings.LineWidth();
            rt.drop(2);
            ui.draw_graphics();
            Screen.ellipse(x1, y1, x2, y2, lw, Settings.Foreground());
            graphics_dirty(x1, y1, x2, y2, lw);
            return OK;
        }
    }
    return ERROR;
}


COMMAND_BODY(Circle)
// ----------------------------------------------------------------------------
//   Draw a circle between the given coordinates
// ----------------------------------------------------------------------------
{
    if (!rt.args(2))
        return ERROR;
    object_g co = rt.stack(1);
    object_g ro = rt.stack(0);
    if (co && ro)
    {
        PlotParametersAccess ppar;
        coord x = ppar.pair_pixel_x(co);
        coord y = ppar.pair_pixel_y(co);
        coord rx = ppar.size_adjust(ro, ppar.xmin, ppar.xmax, 2*ScreenWidth());
        coord ry = ppar.size_adjust(ro, ppar.ymin, ppar.ymax, 2*ScreenHeight());
        if (rx < 0)
            rx = -rx;
        if (ry < 0)
            ry = -ry;
        if (!rt.error())
        {
            blitter::size lw = Settings.LineWidth();
            rt.drop(2);
            coord x1 = x - rx/2;
            coord x2 = x + (rx-1)/2;
            coord y1 = y - ry/2;
            coord y2 = y + (ry-1)/2;
            ui.draw_graphics();
            Screen.ellipse(x1, y1, x2, y2, lw, Settings.Foreground());
            graphics_dirty(x1, y1, x2, y2, lw);
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
    if (!rt.args(2))
        return ERROR;
    object_g p1 = rt.stack(1);
    object_g p2 = rt.stack(0);
    if (p1 && p2)
    {
        PlotParametersAccess ppar;
        coord x1 = ppar.pair_pixel_x(p1);
        coord y1 = ppar.pair_pixel_y(p1);
        coord x2 = ppar.pair_pixel_x(p2);
        coord y2 = ppar.pair_pixel_y(p2);
        if (!rt.error())
        {
            rt.drop(2);
            ui.draw_graphics();
            Screen.rectangle(x1, y1, x2, y2,
                             Settings.LineWidth(), Settings.Foreground());
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
    if (!rt.args(3))
        return ERROR;
    object_g p1 = rt.stack(2);
    object_g p2 = rt.stack(1);
    object_g ro = rt.stack(0);
    if (p1 && p2 && ro)
    {
        PlotParametersAccess ppar;
        coord x1 = ppar.pair_pixel_x(p1);
        coord y1 = ppar.pair_pixel_y(p1);
        coord x2 = ppar.pair_pixel_x(p2);
        coord y2 = ppar.pair_pixel_y(p2);
        coord r  = ppar.size_adjust(ro, ppar.xmin, ppar.xmax, 2*ScreenWidth());
        if (!rt.error())
        {
            blitter::size lw = Settings.LineWidth();
            rt.drop(3);
            ui.draw_graphics();
            Screen.rounded_rectangle(x1, y1, x2, y2,
                                     r, lw, Settings.Foreground());
            graphics_dirty(x1, y1, x2, y2, lw);
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
    if (!rt.args(0))
        return ERROR;
    ui.draw_graphics();
    Screen.fill(0, 0, LCD_W, LCD_H, pattern::white);
    graphics_dirty(0, 0, LCD_W-1, LCD_H-1, 0);
    return OK;
}


COMMAND_BODY(Clip)
// ----------------------------------------------------------------------------
//   Set the clipping rectangle
// ----------------------------------------------------------------------------
{
    if (!rt.args(1))
        return ERROR;
    if (object_p top = rt.pop())
    {
        if (list_p parms = top->as<list>())
        {
            rect clip(Screen.area());
            uint index = 0;
            for (object_p parm : *parms)
            {
                coord arg = parm->as_int32(0, true);
                if (rt.error())
                    return ERROR;
                switch(index++)
                {
                case 0: clip.x1 = arg; break;
                case 1: clip.y1 = arg; break;
                case 2: clip.x2 = arg; break;
                case 3: clip.y2 = arg; break;
                default:        rt.value_error(); return ERROR;
                }
            }
            Screen.clip(clip);
            return OK;
        }
        else
        {
            rt.type_error();
        }
    }
    return ERROR;
}


COMMAND_BODY(CurrentClip)
// ----------------------------------------------------------------------------
//   Retuyrn the current clipping rectangle
// ----------------------------------------------------------------------------
{
    if (!rt.args(0))
        return ERROR;
    rect clip(Screen.clip());
    integer_g x1 = integer::make(clip.x1);
    integer_g y1 = integer::make(clip.y1);
    integer_g x2 = integer::make(clip.x2);
    integer_g y2 = integer::make(clip.y2);
    if (x1 && y1 && x2 && y2)
    {
        list_g obj = list::make(x1, y1, x2, y2);
        if (obj && rt.push(+obj))
            return OK;
    }
    return ERROR;
}



// ============================================================================
//
//   Graphic objects (grob)
//
// ============================================================================

COMMAND_BODY(GXor)
// ----------------------------------------------------------------------------
//   Graphic xor
// ----------------------------------------------------------------------------
{
    return grob::command(blitter::blitop_xor);
}


COMMAND_BODY(GOr)
// ----------------------------------------------------------------------------
//   Graphic or
// ----------------------------------------------------------------------------
{
    return grob::command(blitter::blitop_or);
}


COMMAND_BODY(GAnd)
// ----------------------------------------------------------------------------
//   Graphic and
// ----------------------------------------------------------------------------
{
    return grob::command(blitter::blitop_and);
}


COMMAND_BODY(Pict)
// ----------------------------------------------------------------------------
//   Reference to the graphic display
// ----------------------------------------------------------------------------
{
    if (!rt.args(0))
        return ERROR;
    rt.push(static_object(ID_Pict));
    return OK;
}


static object::result set_ppar_corner(bool max)
// ----------------------------------------------------------------------------
//   Shared code for PMin and PMax
// ----------------------------------------------------------------------------
{
    if (rt.args(1))
    {
        object_p corner = rt.top();
        if (corner->is_complex())
        {
            if (rectangular_g pos = complex_p(corner)->as_rectangular())
            {
                PlotParametersAccess ppar;
                (max ? ppar.xmax : ppar.xmin) = pos->re();
                (max ? ppar.ymax : ppar.ymin) = pos->im();
                if (ppar.write())
                {
                    rt.drop();
                    return object::OK;
                }
            }
            else
            {
                rt.type_error();
            }
        }
    }
    return object::ERROR;
}


COMMAND_BODY(PlotMin)
// ----------------------------------------------------------------------------
//   Set the XMin factor in the plot parameters
// ----------------------------------------------------------------------------
{
    return set_ppar_corner(false);
}


COMMAND_BODY(PlotMax)
{
    return set_ppar_corner(true);
}


static object::result set_ppar_range(bool y)
// ----------------------------------------------------------------------------
//   Shared code for XRange and YRange
// ----------------------------------------------------------------------------
{
    if (rt.args(2))
    {
        object_p min = rt.stack(1);
        object_p max = rt.stack(0);
        if (min->is_real() && max->is_real())
        {
            PlotParametersAccess ppar;
            (y ? ppar.ymin : ppar.xmin) = algebraic_p(min);
            (y ? ppar.ymax : ppar.xmax) = algebraic_p(max);
            if (ppar.write())
            {
                rt.drop(2);
                return object::OK;
            }
        }
        else
        {
            rt.type_error();
        }
    }
    return object::ERROR;
}


COMMAND_BODY(XRange)
// ----------------------------------------------------------------------------
//   Select the horizontal range for plotting
// ----------------------------------------------------------------------------
{
    return set_ppar_range(false);
}


COMMAND_BODY(YRange)
// ----------------------------------------------------------------------------
//   Select the vertical range for plotting
// ----------------------------------------------------------------------------
{
    return set_ppar_range(true);
}


static object::result set_ppar_scale(bool y)
// ----------------------------------------------------------------------------
//   Shared code for XScale and YScale
// ----------------------------------------------------------------------------
{
    if (rt.args(1))
    {
        object_p scale = rt.top();
        if (scale->is_real())
        {
            PlotParametersAccess ppar;
            algebraic_g s = algebraic_p(scale);
            algebraic_g &min = y ? ppar.ymin : ppar.xmin;
            algebraic_g &max = y ? ppar.ymax : ppar.xmax;
            algebraic_g two = integer::make(2);
            algebraic_g center = (min + max) / two;
            algebraic_g width = (max - min) / two;
            min = center - width * s;
            max = center + width * s;
            if (ppar.write())
            {
                rt.drop();
                return object::OK;
            }
        }
        else
        {
            rt.type_error();
        }
    }
    return object::ERROR;
}


COMMAND_BODY(XScale)
// ----------------------------------------------------------------------------
//   Adjust the horizontal scale
// ----------------------------------------------------------------------------
{
    return set_ppar_scale(false);
}


COMMAND_BODY(YScale)
// ----------------------------------------------------------------------------
//   Adjust the vertical scale
// ----------------------------------------------------------------------------
{
    return set_ppar_scale(true);
}


COMMAND_BODY(Scale)
// ----------------------------------------------------------------------------
//  Adjust both horizontal and vertical scale
// ----------------------------------------------------------------------------
{
    if (rt.args(2))
    {
        if (object::result err = set_ppar_scale(true))
            return err;
        if (object::result err = set_ppar_scale(false))
            return err;
        return OK;
    }
    return ERROR;
}


COMMAND_BODY(Center)
// ----------------------------------------------------------------------------
//   Center around the given coordinate
// ----------------------------------------------------------------------------
{
    if (rt.args(1))
    {
        object_p center = rt.top();
        if (center->is_complex())
        {
            if (rectangular_g pos = complex_p(center)->as_rectangular())
            {
                PlotParametersAccess ppar;
                algebraic_g          two = integer::make(2);
                algebraic_g          w   = (ppar.xmax - ppar.xmin) / two;
                algebraic_g          h   = (ppar.ymax - ppar.ymin) / two;
                algebraic_g          cx  = pos->re();
                algebraic_g          cy  = pos->im();
                ppar.xmin = cx - w;
                ppar.xmax = cx + w;
                ppar.ymin = cy - h;
                ppar.ymax = cy + h;
                if (ppar.write())
                {
                    rt.drop();
                    return object::OK;
                }
            }
            else
            {
                rt.type_error();
            }
        }
    }
    return object::ERROR;
}


COMMAND_BODY(Gray)
// ----------------------------------------------------------------------------
//   Create a pattern from a gray level
// ----------------------------------------------------------------------------
{
    if (rt.args(1))
    {
        algebraic_g gray = algebraic_p(rt.top());
        if (gray->is_real())
        {
            gray = gray * integer::make(255);
            uint level = gray->as_uint32(0, true);
            if (level > 255)
                level = 255;
            pattern pat = pattern(level, level, level);
#if CONFIG_FIXED_BASED_OBJECTS
            integer_p bits = rt.make<hex_integer>(pat.bits);
#else
            integer_p bits = rt.make<based_integer>(pat.bits);
#endif
            if (bits && rt.top(bits))
                return OK;
        }
        else
        {
            rt.type_error();
        }
    }
    return ERROR;
}


COMMAND_BODY(RGB)
// ----------------------------------------------------------------------------
//   Create a pattern from RGB levels
// ----------------------------------------------------------------------------
{
    if (rt.args(3))
    {
        algebraic_g red   = algebraic_p(rt.stack(2));
        algebraic_g green = algebraic_p(rt.stack(1));
        algebraic_g blue  = algebraic_p(rt.stack(0));
        if (red->is_real() && green->is_real() && blue->is_real())
        {
            algebraic_g scale = integer::make(255);
            red = red * scale;
            green = green * scale;
            blue = blue * scale;
            uint rl = red->as_uint32(0, true);
            uint gl = green->as_uint32(0, true);
            uint bl = blue->as_uint32(0, true);
            if (rl > 255)
                rl = 255;
            if (gl > 255)
                gl = 255;
            if (bl > 255)
                bl = 255;
            pattern pat = pattern(rl, gl, bl);
#if CONFIG_FIXED_BASED_OBJECTS
            integer_p bits = rt.make<hex_integer>(pat.bits);
#else
            integer_p bits = rt.make<based_integer>(pat.bits);
#endif
            if (bits && rt.drop(2) && rt.top(bits))
                return OK;
        }
        else
        {
            rt.type_error();
        }
    }
    return ERROR;
}
