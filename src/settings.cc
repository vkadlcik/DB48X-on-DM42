// ****************************************************************************
//  settings.cc                                                   DB48X project
// ****************************************************************************
//
//   File Description:
//
//     Representation of settings
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

#include "settings.h"

#include "command.h"
#include "integer.h"

#include <cstdarg>

settings Settings;


// ============================================================================
//
//    Commands to manipulate the settings
//
// ============================================================================

COMMAND_BODY(Std)
// ----------------------------------------------------------------------------
//   Switch to standard display mode
// ----------------------------------------------------------------------------
{
    Settings.displayed = BID128_MAXDIGITS;
    Settings.display_mode = settings::NORMAL;
    return OK;
}


static object::result set_display_mode(settings::display mode)
// ----------------------------------------------------------------------------
//   Set a mode with a given number of digits
// ----------------------------------------------------------------------------
{
    if (object_p size = runtime::RT.top())
    {
        if (integer_p digits = size->as<integer>())
        {
            Settings.displayed = digits->value<uint>();
            Settings.display_mode = mode;
            runtime::RT.pop();
            return object::OK;
        }
        else
        {
            runtime::RT.type_error();
        }
    }
    return object::ERROR;
}


COMMAND_BODY(Fix)
// ----------------------------------------------------------------------------
//   Switch to fixed display mode
// ----------------------------------------------------------------------------
{
    return set_display_mode(settings::FIX);
}


COMMAND_BODY(Sci)
// ----------------------------------------------------------------------------
//   Switch to scientific display mode
// ----------------------------------------------------------------------------
{
    return set_display_mode(settings::SCI);
}


COMMAND_BODY(Eng)
// ----------------------------------------------------------------------------
//   Switch to engineering display mode
// ----------------------------------------------------------------------------
{
    return set_display_mode(settings::ENG);
}


COMMAND_BODY(Sig)
// ----------------------------------------------------------------------------
//   Switch to significant display mode
// ----------------------------------------------------------------------------
{
    return set_display_mode(settings::NORMAL);
}


COMMAND_BODY(CycleDisplayMode)
// ----------------------------------------------------------------------------
//   Cycle among the possible display modes
// ----------------------------------------------------------------------------
{
    Settings.nextDisplayMode();
    return OK;
}


static object::result settings_command(cstring format, ...)
// ----------------------------------------------------------------------------
//    Push a command restoring a given state
// ----------------------------------------------------------------------------
{
    char buffer[80];
    va_list va;
    va_start(va, format);
    size_t size = vsnprintf(buffer, sizeof(buffer), format, va);
    va_end(va);

    if (gcobj obj = object::parse(utf8(buffer), size))
        if (runtime::RT.push(obj))
            return object::OK;
    return object::ERROR;
}


COMMAND_BODY(DisplayMode)
// ----------------------------------------------------------------------------
//   Return a program that restores the current display mode
// ----------------------------------------------------------------------------
{
    uint disp = Settings. displayed;
    switch(Settings.display_mode)
    {
    default:
    case settings::NORMAL:
        if (disp == 34)
                                return settings_command("«STD»", disp);
        else
                                return settings_command("«%u SIG»", disp);
    case settings::FIX:         return settings_command("«%u FIX»", disp);
    case settings::SCI:         return settings_command("«%u SCI»", disp);
    case settings::ENG:         return settings_command("«%u ENG»", disp);
    }
    return ERROR;
}


COMMAND_BODY(Deg)
// ----------------------------------------------------------------------------
//   Switch to degrees
// ----------------------------------------------------------------------------
{
    Settings.angle_mode = settings::DEGREES;
    return OK;
}


COMMAND_BODY(Rad)
// ----------------------------------------------------------------------------
//   Switch to radians
// ----------------------------------------------------------------------------
{
    Settings.angle_mode = settings::RADIANS;
    return OK;
}


COMMAND_BODY(Grad)
// ----------------------------------------------------------------------------
//   Switch to grads
// ----------------------------------------------------------------------------
{
    Settings.angle_mode = settings::GRADS;
    return OK;
}


COMMAND_BODY(AngleMode)
// ----------------------------------------------------------------------------
//   Return a program setting the current angle mode
// ----------------------------------------------------------------------------
{
    switch(Settings.angle_mode)
    {
    default:
    case settings::DEGREES:     return settings_command("«DEG»");
    case settings::RADIANS:     return settings_command("«RAD»");
    case settings::GRADS:       return settings_command("GRAD»");
    }
    return ERROR;
}


COMMAND_BODY(CycleAngleMode)
// ----------------------------------------------------------------------------
//   Cycle across possible angle modes
// ----------------------------------------------------------------------------
{
    Settings.nextAngleMode();
    return OK;
}


COMMAND_BODY(LowerCase)
// ----------------------------------------------------------------------------
//   Switch to lowercase command display
// ----------------------------------------------------------------------------
{
    Settings.command_fmt = settings::LOWERCASE;
    return OK;
}


COMMAND_BODY(UpperCase)
// ----------------------------------------------------------------------------
//  Switch to uppercase command display
// ----------------------------------------------------------------------------
{
    Settings.command_fmt = settings::UPPERCASE;
    return OK;
}


COMMAND_BODY(Capitalized)
// ----------------------------------------------------------------------------
//  Switch to capitalized command display
// ----------------------------------------------------------------------------
{
    Settings.command_fmt = settings::CAPITALIZED;
    return OK;
}


COMMAND_BODY(LongForm)
// ----------------------------------------------------------------------------
//   Switch to long-form command display
// ----------------------------------------------------------------------------
{
    Settings.command_fmt = settings::LONG_FORM;
    return OK;
}


COMMAND_BODY(CommandCaseMode)
// ----------------------------------------------------------------------------
//   Return a program giving the current case mode
// ----------------------------------------------------------------------------
{
    switch(Settings.command_fmt)
    {
    default:
    case settings::LOWERCASE:   return settings_command("«LowerCase»");
    case settings::UPPERCASE:   return settings_command("«UpperCase»");
    case settings::CAPITALIZED: return settings_command("«Capitalized»");
    case settings::LONG_FORM:   return settings_command("LongForm»");
    }
    return ERROR;
}


COMMAND_BODY(DecimalDot)
// ----------------------------------------------------------------------------
//  Switch to decimal dot
// ----------------------------------------------------------------------------
{
    Settings.decimal_dot = '.';
    return OK;
}


COMMAND_BODY(DecimalComma)
// ----------------------------------------------------------------------------
//  Switch to decimal comma
// ----------------------------------------------------------------------------
{
    Settings.decimal_dot = ',';
    return OK;
}


COMMAND_BODY(DecimalDisplayMode)
// ----------------------------------------------------------------------------
//   Return current decimal separator mode
// ----------------------------------------------------------------------------
{
    switch(Settings.decimal_dot)
    {
    default:
    case '.':   return settings_command("«DecimalDot»");
    case ',':   return settings_command("«DecimalComma»");
    }
    return ERROR;
}
