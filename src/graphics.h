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
#include "symbol.h"
#include "target.h"


struct PlotParameters
// ----------------------------------------------------------------------------
//   A replication of the PlotParameters / PPAR vaariable
// ----------------------------------------------------------------------------
{
    PlotParameters();

    object::id  type;
    algebraic_g xmin;
    algebraic_g ymin;
    algebraic_g xmax;
    algebraic_g ymax;
    symbol_g    independent;
    symbol_g    dependent;
    algebraic_g resolution;
    algebraic_g xorigin;
    algebraic_g yorigin;
    algebraic_g xticks;
    algebraic_g yticks;
    text_g      xlabel;
    text_g      ylabel;

    bool parse(list_g list);
    bool parse(symbol_g name);
    bool parse(cstring name);
    bool parse();

    static coord pixel_adjust(object_r p,
                              algebraic_r min,
                              algebraic_r max,
                              uint scale,
                              bool isSize = false);
    static coord size_adjust(object_r p,
                             algebraic_r min,
                             algebraic_r max,
                             uint scale)
    {
        return pixel_adjust(p, min, max, scale, true);
    }
    coord pixel_x(object_r pos) const;
    coord pixel_y(object_r pos) const;
};


COMMAND_DECLARE(Disp);
COMMAND_DECLARE(DispXY);
COMMAND_DECLARE(Line);
COMMAND_DECLARE(Ellipse);
COMMAND_DECLARE(Circle);
COMMAND_DECLARE(Rect);
COMMAND_DECLARE(RRect);
COMMAND_DECLARE(ClLCD);
COMMAND_DECLARE(Drax);

#endif // GRAPHICS_H
