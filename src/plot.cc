// ****************************************************************************
//  plot.cc                                                       DB48X project
// ****************************************************************************
//
//   File Description:
//
//     Function and curve plotting
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

#include "plot.h"

#include "arithmetic.h"
#include "compare.h"
#include "expression.h"
#include "functions.h"
#include "graphics.h"
#include "program.h"
#include "sysmenu.h"
#include "target.h"
#include "variables.h"


void draw_axes(const PlotParametersAccess &ppar)
// ----------------------------------------------------------------------------
//   Draw axes
// ----------------------------------------------------------------------------
{
    coord w = Screen.area().width();
    coord h = Screen.area().height();
    coord x = ppar.pixel_adjust(ppar.xorigin.Safe(), ppar.xmin, ppar.xmax, w);
    coord y = ppar.pixel_adjust(ppar.yorigin.Safe(), ppar.ymax, ppar.ymin, h);

    // Draw axes proper
    pattern pat = Settings.Foreground();
    Screen.fill(0, y, w, y, pat);
    Screen.fill(x, 0, x, h, pat);

    // Draw tick marks
    coord tx = ppar.size_adjust(ppar.xticks.Safe(), ppar.xmin, ppar.xmax, w);
    coord ty = ppar.size_adjust(ppar.yticks.Safe(), ppar.ymin, ppar.ymax, h);
    if (tx)
    {
        for (coord i = tx; x + i <= w; i += tx)
            Screen.fill(x + i, y - 2, x + i, y + 2, pat);
        for (coord i = tx; x - i >= 0; i += tx)
            Screen.fill(x - i, y - 2, x - i, y + 2, pat);
        for (coord i = ty; y + i <= h; i += ty)
            Screen.fill(x - 2, y + i, x + 2, y + i, pat);
        for (coord i = ty; y - i >= 0; i += ty)
            Screen.fill(x - 2, y - i, x + 2, y - i, pat);
    }

    // Draw arrows at end of axes
    for (uint i = 0; i < 4; i++)
    {
        Screen.fill(w - 3*(i+1), y - i, w - 3*i, y + i, pat);
        Screen.fill(x - i, 3*i, x + i, 3*(i+1), pat);
    }

    ui.draw_dirty(0, 0, w, h);
}


object::result draw_plot(object::id            kind,
                         const PlotParametersAccess &ppar,
                         object_g              eqobj = nullptr)
// ----------------------------------------------------------------------------
//  Draw an equation that takes input from the stack
// ----------------------------------------------------------------------------
{
    bool           isFn   = kind == object::ID_Function;
    algebraic_g    x      = isFn ? ppar.xmin : ppar.imin;
    object::result result = object::ERROR;
    coord          lx     = -1;
    coord          ly     = -1;
    uint           then   = sys_current_ms();
    algebraic_g    step   = ppar.resolution;
    if (step->is_zero())
        step = (isFn
                ? (ppar.xmax - ppar.xmin)
                : (ppar.imax - ppar.imin))
            / integer::make(ScreenWidth());

    if (!eqobj)
        eqobj = directory::recall_all(symbol::make("eq"));
    if (!eqobj)
    {
        rt.no_equation_error();
        return object::ERROR;
    }
    if (!eqobj->is_program())
    {
        rt.invalid_equation_error();
        return object::ERROR;
    }
    program_g eq = program_p(eqobj.Safe());

    save<symbol_g *> iref(expression::independent,
                          (symbol_g *) &ppar.independent);
    if (ui.draw_graphics())
        if (Settings.DrawPlotAxes())
            draw_axes(ppar);

    bool split_points = Settings.NoCurveFilling();
    while (!program::interrupted())
    {
        coord  rx = 0, ry = 0;
        algebraic_g y = algebraic::evaluate_function(eq, x);
        if (y)
        {
            switch(kind)
            {
            default:
                case object::ID_Function:
                rx = ppar.pixel_x(x);
                ry = ppar.pixel_y(y);
                break;
            case object::ID_Polar:
            {
                algebraic_g i = rectangular::make(integer::make(0),
                                                  integer::make(1));
                y = y * exp::run(i * x);
            }
            // Fall-through
            case object::ID_Parametric:
                if (y->is_real())
                    y = rectangular::make(y, integer::make(0));
                if (y)
                {
                    if (algebraic_g cx = y->algebraic_child(0))
                        rx = ppar.pixel_x(cx);
                    if (algebraic_g cy = y->algebraic_child(1))
                        ry = ppar.pixel_y(cy);
                }
                break;
            }
        }

        if (y)
        {
            if (lx < 0 || split_points)
            {
                lx = rx;
                ly = ry;
            }
            Screen.line(lx,ly,rx,ry, Settings.LineWidth(), Settings.Foreground());
            ui.draw_dirty(lx, ly, rx, ry);
            uint now = sys_current_ms();
            if (now - then > 500)
            {
                then = now;
                refresh_dirty();
                ui.draw_clean();
            }
            lx = rx;
            ly = ry;
        }
        else
        {
            if (!rt.error())
                rt.invalid_function_error();
            Screen.text(0, 0, rt.error(), ErrorFont,
                        pattern::white, pattern::black);
            ui.draw_dirty(0, 0, LCD_W, ErrorFont->height());
            refresh_dirty();
            ui.draw_clean();
            lx = ly = -1;
            rt.clear_error();
        }
        x = x + step;
        algebraic_g cmp = x > (isFn ? ppar.xmax : ppar.imax);
        if (!cmp)
            goto err;
        if (cmp->as_truth(false))
            break;
    }
    result = object::OK;

err:
    refresh_dirty();
    return result;
}


COMMAND_BODY(Function)
// ----------------------------------------------------------------------------
//   Draw plot from function on the stack taking stack arguments
// ----------------------------------------------------------------------------
{
    if (!rt.args(1))
        return ERROR;
    if (object_g eq = rt.pop())
    {
        PlotParametersAccess ppar;
        return draw_plot(ID_Function, ppar, eq);
    }
    return ERROR;
}


COMMAND_BODY(Parametric)
// ----------------------------------------------------------------------------
//   Draw plot from function on the stack taking stack arguments
// ----------------------------------------------------------------------------
{
    if (!rt.args(1))
        return ERROR;
    if (object_g eq = rt.pop())
    {
        PlotParametersAccess ppar;
        return draw_plot(ID_Parametric, ppar, eq);
    }
    return ERROR;
}


COMMAND_BODY(Polar)
// ----------------------------------------------------------------------------
//   Set polar plot type
// ----------------------------------------------------------------------------
{
    if (!rt.args(1))
        return ERROR;
    if (object_g eq = rt.pop())
    {
        PlotParametersAccess ppar;
        return draw_plot(ID_Polar, ppar, eq);
    }
    return ERROR;
}


COMMAND_BODY(Draw)
// ----------------------------------------------------------------------------
//   Draw plot in EQ according to PPAR
// ----------------------------------------------------------------------------
{
    if (!rt.args(0))
        return ERROR;
    PlotParametersAccess ppar;
    switch(ppar.type)
    {
    default:
    case ID_Function:
    case ID_Parametric:
    case ID_Polar:
        return draw_plot(ppar.type, ppar);
    }
    rt.invalid_plot_type_error();
    return ERROR;
}


COMMAND_BODY(Drax)
// ----------------------------------------------------------------------------
//   Draw plot axes
// ----------------------------------------------------------------------------
{
    if (!rt.args(0))
        return ERROR;
    ui.draw_graphics();

    PlotParametersAccess ppar;
    draw_axes(ppar);
    refresh_dirty();

    return OK;
}
