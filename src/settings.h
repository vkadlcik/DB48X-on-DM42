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

#include "command.h"

struct settings
// ----------------------------------------------------------------------------
//    Internal representation of settings
// ----------------------------------------------------------------------------
{
    settings()
        : precision(32),
          displayed(BID128_MAXDIGITS),
          max_nonsci(9),
          display_mode(NORMAL),
          decimal_dot('.'),
          exponent_char(L'⁳'),
          angle_mode(DEGREES),
          base(16),
          wordsize(128),
          command_fmt(LONG_FORM),
          show_decimal(true)
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

    enum display
    // ------------------------------------------------------------------------
    //   The display mode for numbers
    // ------------------------------------------------------------------------
    {
        NORMAL, FIX, SCI, ENG
    };

    enum commands
    // ------------------------------------------------------------------------
    //   Display of commands
    // ------------------------------------------------------------------------
    {
        LOWERCASE,              // Display the short name in lowercase
        UPPERCASE,              // Display the short name in uppercase
        CAPITALIZED,            // Display the short name capitalized
        LONG_FORM,              // Display the long form
    };

    display nextDisplayMode()
    {
        switch(display_mode)
        {
        case NORMAL:    display_mode = FIX; break;
        case FIX:       display_mode = SCI; break;
        case SCI:       display_mode = ENG; break;
        default:
        case ENG:       display_mode = NORMAL; break;
        }
        return display_mode;
    }

    angles nextAngleMode()
    {
        switch(angle_mode)
        {
        case DEGREES:   angle_mode = RADIANS; break;
        case RADIANS:   angle_mode = GRADS; break;
        default:
        case GRADS:     angle_mode = DEGREES; break;
        }
        return angle_mode;
    }

public:
    uint16_t precision;    // Internal precision for numbers
    uint16_t displayed;    // Number of displayed digits
    uint16_t max_nonsci;   // Number of zeroes to display before expoonent shows
    display  display_mode; // Display mode
    char     decimal_dot;  // Character used for decimal separator
    unicode  exponent_char;// The character used to represent exponents
    angles   angle_mode;   // Whether we compute in degrees, radians or grads
    uint8_t  base;         // The default base for #numbers
    uint16_t wordsize;     // Wordsize for binary numbers (in bits)
    commands command_fmt;  // How we prefer to display commands
    bool     show_decimal; // Show decimal dot for integral real numbers
};


extern settings Settings;

COMMAND_DECLARE(Std);
COMMAND_DECLARE(Fix);
COMMAND_DECLARE(Sci);
COMMAND_DECLARE(Eng);
COMMAND_DECLARE(Sig);
COMMAND_DECLARE(DisplayMode);
COMMAND_DECLARE(CycleDisplayMode);

COMMAND_DECLARE(Deg);
COMMAND_DECLARE(Rad);
COMMAND_DECLARE(Grad);
COMMAND_DECLARE(AngleMode);
COMMAND_DECLARE(CycleAngleMode);

COMMAND_DECLARE(LowerCase);
COMMAND_DECLARE(UpperCase);
COMMAND_DECLARE(Capitalized);
COMMAND_DECLARE(LongForm);
COMMAND_DECLARE(CommandCaseMode);

COMMAND_DECLARE(DecimalDot);
COMMAND_DECLARE(DecimalComma);
COMMAND_DECLARE(DecimalDisplayMode);

#endif // SETTINGS_H
