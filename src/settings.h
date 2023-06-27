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
#include "menu.h"

struct settings
// ----------------------------------------------------------------------------
//    Internal representation of settings
// ----------------------------------------------------------------------------
{
    enum { STD_DISPLAYED = 20 };

    settings()
        : precision(32),
          displayed(STD_DISPLAYED),
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


// Macro to defined a simple command handler for derived classes
#define SETTINGS_COMMAND_DECLARE(derived)                       \
struct derived : command                                        \
{                                                               \
    derived(id i = ID_##derived) : command(i) { }               \
                                                                \
    OBJECT_HANDLER(derived)                                     \
    {                                                           \
        switch(op)                                              \
        {                                                       \
        case EVAL:                                              \
        case EXEC:                                              \
            RT.command(fancy(ID_##derived));                    \
            return ((derived *) obj)->evaluate();               \
        case MENU_MARKER:                                       \
            return ((derived *) obj)->marker();                 \
        default:                                                \
            return DELEGATE(command);                           \
        }                                                       \
    }                                                           \
    static result evaluate();                                   \
    static unicode marker();                                    \
    static cstring menu_label(menu::info &mi);                  \
}

#define SETTINGS_COMMAND_BODY(derived, mkr)                     \
    unicode derived::marker() { return mkr; }                   \
    object::result derived::evaluate()

#define SETTINGS_COMMAND_NOLABEL(derived, mkr)                  \
    unicode derived::marker() { return mkr; }                   \
    cstring derived::menu_label(menu::info UNUSED &mi)          \
    {                                                           \
        return #derived;                                        \
    }                                                           \
    object::result derived::evaluate()

#define SETTINGS_COMMAND_LABEL(derived)                         \
    cstring derived::menu_label(menu::info UNUSED &mi)


SETTINGS_COMMAND_DECLARE(Std);
SETTINGS_COMMAND_DECLARE(Fix);
SETTINGS_COMMAND_DECLARE(Sci);
SETTINGS_COMMAND_DECLARE(Eng);
SETTINGS_COMMAND_DECLARE(Sig);
COMMAND_DECLARE(DisplayMode);
COMMAND_DECLARE(CycleDisplayMode);

SETTINGS_COMMAND_DECLARE(Deg);
SETTINGS_COMMAND_DECLARE(Rad);
SETTINGS_COMMAND_DECLARE(Grad);
COMMAND_DECLARE(AngleMode);
COMMAND_DECLARE(CycleAngleMode);

SETTINGS_COMMAND_DECLARE(LowerCase);
SETTINGS_COMMAND_DECLARE(UpperCase);
SETTINGS_COMMAND_DECLARE(Capitalized);
SETTINGS_COMMAND_DECLARE(LongForm);
COMMAND_DECLARE(CommandCaseMode);

SETTINGS_COMMAND_DECLARE(DecimalDot);
SETTINGS_COMMAND_DECLARE(DecimalComma);
COMMAND_DECLARE(DecimalDisplayMode);

#endif // SETTINGS_H
