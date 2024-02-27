#ifndef GRAPHICS_H
#define GRAPHICS_H
// ****************************************************************************
//  graphics.h                                                    DB48X project
// ****************************************************************************
//
//   File Description:
//
//    Low level graphic routines
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

#include "algebraic.h"
#include "command.h"
#include "list.h"
#include "stats.h"
#include "symbol.h"
#include "target.h"


struct PlotParameters : command
// ----------------------------------------------------------------------------
//   A replication of the PlotParameters / PPAR vaariable
// ----------------------------------------------------------------------------
{
    PlotParameters(id type = ID_PlotParameters) : command(type) {}

};


struct PlotParametersAccess
// ----------------------------------------------------------------------------
//  Access to the plot parameters
// ----------------------------------------------------------------------------
{
    PlotParametersAccess();

    object::id      type;
    algebraic_g     xmin;
    algebraic_g     ymin;
    algebraic_g     xmax;
    algebraic_g     ymax;
    symbol_g        independent;
    algebraic_g     imin;
    algebraic_g     imax;
    symbol_g        dependent;
    algebraic_g     resolution;
    algebraic_g     xorigin;
    algebraic_g     yorigin;
    algebraic_g     xticks;
    algebraic_g     yticks;
    text_g          xlabel;
    text_g          ylabel;

    static object_p name();

    bool            parse(list_p list);
    bool            parse(object_p n = name());

    bool            write(object_p n = name()) const;

    bool            check_validity() const;

    static coord    pixel_adjust(object_r    p,
                                 algebraic_r min,
                                 algebraic_r max,
                                 uint        scale,
                                 bool        isSize = false);
    static coord    size_adjust(object_r    p,
                                algebraic_r min,
                                algebraic_r max,
                                uint        scale);
    coord           pair_pixel_x(object_r pos) const;
    coord           pair_pixel_y(object_r pos) const;

    coord           pixel_x(algebraic_r pos) const;
    coord           pixel_y(algebraic_r pos) const;
};


inline uint ScreenWidth()
// ----------------------------------------------------------------------------
//   Adjustment for the radius of a circle or rounded rectangle
// ----------------------------------------------------------------------------
{
    return Screen.area().width();
}


inline uint ScreenHeight()
// ----------------------------------------------------------------------------
//   Adjustment for the radius of a circle or rounded rectangle
// ----------------------------------------------------------------------------
{
    return Screen.area().height();
}


COMMAND_DECLARE(Disp);
COMMAND_DECLARE(DispXY);
COMMAND_DECLARE(Show);
COMMAND_DECLARE(Line);
COMMAND_DECLARE(Ellipse);
COMMAND_DECLARE(Circle);
COMMAND_DECLARE(Rect);
COMMAND_DECLARE(RRect);
COMMAND_DECLARE(ClLCD);
COMMAND_DECLARE(Clip);
COMMAND_DECLARE(CurrentClip);
COMMAND_DECLARE(GXor);
COMMAND_DECLARE(GOr);
COMMAND_DECLARE(GAnd);
COMMAND_DECLARE(Pict);
COMMAND_DECLARE(Gray);
COMMAND_DECLARE(RGB);

COMMAND_DECLARE(PlotMin);
COMMAND_DECLARE(PlotMax);
COMMAND_DECLARE(XRange);
COMMAND_DECLARE(YRange);
COMMAND_DECLARE(Scale);
COMMAND_DECLARE(XScale);
COMMAND_DECLARE(YScale);
COMMAND_DECLARE(Center);

#endif // GRAPHICS_H
