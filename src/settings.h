#ifndef SETTINGS_H
#define SETTINGS_H
// ****************************************************************************
//  settings.h                                                    DB48X project
// ****************************************************************************
//
//   File Description:
//
//     List of system-wide settings
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

#include <types.h>

struct settings
// ----------------------------------------------------------------------------
//    Internal representation of settings
// ----------------------------------------------------------------------------
{
    settings()
        : precision(32),
          displayed(0),
          decimalDot('.'),
          exponentChar(0x98),   // The special mini-'E'
          angle_mode(DEGREES),
          base(10)
    {}

    enum angles
    // ------------------------------------------------------------------------
    //   The base used for angles
    // ------------------------------------------------------------------------
    {
        DEGREES,
        RADIANS,
        GRADS,
        NUM_ANGLES,
    };

    angles nextAngleMode()
    {
        switch(angle_mode)
        {
        case DEGREES:   angle_mode = RADIANS; break;
        case RADIANS:   angle_mode = GRADS; break;
        case GRADS:     angle_mode = DEGREES; break;
        default:        angle_mode = DEGREES; break;
        }
        return angle_mode;
    }

public:
    uint   precision;    // Internal precision for numbers
    uint   displayed;    // Number of displayed digits
    char   decimalDot;   // Character used for decimal separator
    char   exponentChar; // The character used to represent exponents
    angles angle_mode;   // Whether we compute in degrees, radians or grads
    uint   base;         // The default base for #numbers
};


extern settings Settings;

#endif // SETTINGS_H
