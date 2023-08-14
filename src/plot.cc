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
#include "graphics.h"
#include "program.h"
#include "sysmenu.h"
#include "target.h"
#include "variables.h"


COMMAND_BODY(Function)
// ----------------------------------------------------------------------------
//   Set function plot type
// ----------------------------------------------------------------------------
{
    return OK;
}


COMMAND_BODY(Parametric)
// ----------------------------------------------------------------------------
//   Set parametric plot type
// ----------------------------------------------------------------------------
{
    return OK;
}


COMMAND_BODY(Polar)
// ----------------------------------------------------------------------------
//   Set polar plot type
// ----------------------------------------------------------------------------
{
    return OK;
}


object::result DrawFunctionPlot(const PlotParameters &ppar)
// ----------------------------------------------------------------------------
//   Draw a function plot
// ----------------------------------------------------------------------------
{
    algebraic_g step = ppar.resolution;
    if (step->is_zero())
        step = (ppar.xmax - ppar.xmin) / integer::make(ScreenWidth());
    algebraic_g x  = ppar.xmin;
    object_g    eq = directory::recall_all(symbol::make("eq"));
    if (!eq)
        return object::ERROR;

    coord lx   = -1;
    coord ly   = -1;
    uint  then = sys_current_ms();
    while (!program::interrupted())
    {
        coord rx = ppar.pixel_x(x);
        if (!rt.push(x.Safe()))
            return object::ERROR;
        object::result err = eq->execute();
        if (err != object::OK)
            return err;

        algebraic_g y = algebraic_p(rt.pop());
        if (!y || !y->is_algebraic())
            return object::ERROR;
        coord ry = ppar.pixel_y(y);

        if (lx >= 0)
        {
            Screen.line(lx,ly,rx,ry, Settings.line_width, Settings.foreground);
            ui.draw_dirty(lx, ly, rx, ry);
            uint now = sys_current_ms();
            if (then - now > 50)
            {
                then = now;
                refresh_dirty();
                ui.draw_clean();
            }
        }
        lx = rx;
        ly = ry;
        x = x + step;
        if ((x > ppar.xmax)->as_truth(false))
            break;
    }

    refresh_dirty();

    return object::OK;
}


object::result DrawParametricPlot(const PlotParameters &ppar)
// ----------------------------------------------------------------------------
//   Draw a parametric plot
// ----------------------------------------------------------------------------
{
    return object::OK;
}


object::result DrawPolarPlot(const PlotParameters &ppar)
// ----------------------------------------------------------------------------
//   Draw a polar plot
// ----------------------------------------------------------------------------
{
    return object::OK;
}


COMMAND_BODY(Draw)
// ----------------------------------------------------------------------------
//   Draw plot in EQ according to PPAR
// ----------------------------------------------------------------------------
{
    PlotParameters ppar;
    switch(ppar.type)
    {
    default:
    case ID_Function:   return DrawFunctionPlot(ppar);
    case ID_Parametric: return DrawParametricPlot(ppar);
    case ID_Polar:      return DrawPolarPlot(ppar);

    }
    rt.invalid_plot_type_error();
    return ERROR;
}


COMMAND_BODY(Drax)
// ----------------------------------------------------------------------------
//   Draw plot axes
// ----------------------------------------------------------------------------
{
    PlotParameters ppar;
    blitter::size w = Screen.area().width();
    blitter::size h = Screen.area().height();
    coord x = ppar.pixel_adjust(ppar.xorigin.Safe(), ppar.xmin, ppar.xmax, w);
    coord y = ppar.pixel_adjust(ppar.yorigin.Safe(), ppar.ymin, ppar.ymax, h);

    // Draw axes proper
    pattern pat = Settings.foreground;
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

    return OK;
}
