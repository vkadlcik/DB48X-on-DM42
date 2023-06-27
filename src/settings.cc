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
#include <cstdlib>

settings Settings;


// ============================================================================
//
//    Commands to manipulate the settings
//
// ============================================================================

static const unicode MARK = L'●'; // L'■';

static inline bool IsStd()
// ----------------------------------------------------------------------------
//   Check if the current settings is Std
// ----------------------------------------------------------------------------
{
    return Settings.display_mode == settings::NORMAL
        && Settings.displayed == settings::STD_DISPLAYED;
}


SETTINGS_COMMAND_BODY(Std, IsStd() ? MARK : 0)
// ----------------------------------------------------------------------------
//   Switch to standard display mode
// ----------------------------------------------------------------------------
{
    Settings.displayed = settings::STD_DISPLAYED;
    Settings.display_mode = settings::NORMAL;
    Input.menuNeedsRefresh();
    return OK;
}


SETTINGS_COMMAND_LABEL(Std)
// ----------------------------------------------------------------------------
//   Return the label for Std
// ----------------------------------------------------------------------------
{
    return "Std";
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
            uint disp = digits->value<uint>();
            Settings.displayed = std::min(disp, (uint) BID128_MAXDIGITS);
            Settings.display_mode = mode;
            runtime::RT.pop();
            Input.menuNeedsRefresh();
            return object::OK;
        }
        else
        {
            runtime::RT.type_error();
        }
    }
    return object::ERROR;
}


static cstring get_display_mode(settings::display mode, cstring label)
// ----------------------------------------------------------------------------
//   Compute the label for display mode
// ----------------------------------------------------------------------------
{
    if (Settings.display_mode == mode)
    {
        // We can share the buffer here since only one mode is active
        static char buffer[8];
        snprintf(buffer, sizeof(buffer), "%s %u", label, Settings.displayed);
        return buffer;
    }
    return label;
}


SETTINGS_COMMAND_BODY(Fix, Settings.display_mode == settings::FIX ? MARK : 0)
// ----------------------------------------------------------------------------
//   Switch to fixed display mode
// ----------------------------------------------------------------------------
{
    return set_display_mode(settings::FIX);
}


SETTINGS_COMMAND_LABEL(Fix)
// ----------------------------------------------------------------------------
//   Return the label for Fix mode
// ----------------------------------------------------------------------------
{
    return get_display_mode(settings::FIX, "Fix");
}


SETTINGS_COMMAND_BODY(Sci, Settings.display_mode == settings::SCI ? MARK : 0)
// ----------------------------------------------------------------------------
//   Switch to scientific display mode
// ----------------------------------------------------------------------------
{
    return set_display_mode(settings::SCI);
}


SETTINGS_COMMAND_LABEL(Sci)
// ----------------------------------------------------------------------------
//   Return the label for Fix mode
// ----------------------------------------------------------------------------
{
    return get_display_mode(settings::SCI, "Sci");
}


SETTINGS_COMMAND_BODY(Eng, Settings.display_mode == settings::ENG ? MARK : 0)
// ----------------------------------------------------------------------------
//   Switch to engineering display mode
// ----------------------------------------------------------------------------
{
    return set_display_mode(settings::ENG);
}


SETTINGS_COMMAND_LABEL(Eng)
// ----------------------------------------------------------------------------
//   Return the label for Eng mode
// ----------------------------------------------------------------------------
{
    return get_display_mode(settings::ENG, "Eng");
}


SETTINGS_COMMAND_BODY(Sig,
                      Settings.display_mode == settings::NORMAL
                      && Settings.displayed != settings::STD_DISPLAYED
                      ? MARK : 0)
// ----------------------------------------------------------------------------
//   Switch to significant display mode
// ----------------------------------------------------------------------------
{
    return set_display_mode(settings::NORMAL);
}


SETTINGS_COMMAND_LABEL(Sig)
// ----------------------------------------------------------------------------
//   Return the label for Eng mode
// ----------------------------------------------------------------------------
{
    return get_display_mode(settings::NORMAL, "Sig");
}


COMMAND_BODY(CycleDisplayMode)
// ----------------------------------------------------------------------------
//   Cycle among the possible display modes
// ----------------------------------------------------------------------------
{
    Settings.nextDisplayMode();
    Input.menuNeedsRefresh();
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


SETTINGS_COMMAND_NOLABEL(Deg,
                         Settings.angle_mode == settings::DEGREES ? MARK : 0)
// ----------------------------------------------------------------------------
//   Switch to degrees
// ----------------------------------------------------------------------------
{
    Settings.angle_mode = settings::DEGREES;
    Input.menuNeedsRefresh();
    return OK;
}


SETTINGS_COMMAND_NOLABEL(Rad,
                         Settings.angle_mode == settings::RADIANS ? MARK : 0)
// ----------------------------------------------------------------------------
//   Switch to radians
// ----------------------------------------------------------------------------
{
    Settings.angle_mode = settings::RADIANS;
    Input.menuNeedsRefresh();
    return OK;
}


SETTINGS_COMMAND_NOLABEL(Grad,
                         Settings.angle_mode == settings::GRADS ? MARK : 0)
// ----------------------------------------------------------------------------
//   Switch to grads
// ----------------------------------------------------------------------------
{
    Settings.angle_mode = settings::GRADS;
    Input.menuNeedsRefresh();
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
    Input.menuNeedsRefresh();
    return OK;
}


SETTINGS_COMMAND_NOLABEL(LowerCase,
                         Settings.command_fmt == settings::LOWERCASE ? MARK : 0)
// ----------------------------------------------------------------------------
//   Switch to lowercase command display
// ----------------------------------------------------------------------------
{
    Settings.command_fmt = settings::LOWERCASE;
    Input.menuNeedsRefresh();
    return OK;
}


SETTINGS_COMMAND_NOLABEL(UpperCase,
                         Settings.command_fmt == settings::UPPERCASE ? MARK : 0)
// ----------------------------------------------------------------------------
//  Switch to uppercase command display
// ----------------------------------------------------------------------------
{
    Settings.command_fmt = settings::UPPERCASE;
    Input.menuNeedsRefresh();
    return OK;
}


SETTINGS_COMMAND_NOLABEL(Capitalized,
                         Settings.command_fmt==settings::CAPITALIZED ? MARK : 0)
// ----------------------------------------------------------------------------
//  Switch to capitalized command display
// ----------------------------------------------------------------------------
{
    Settings.command_fmt = settings::CAPITALIZED;
    Input.menuNeedsRefresh();
    return OK;
}


SETTINGS_COMMAND_NOLABEL(LongForm,
                         Settings.command_fmt==settings::LONG_FORM ? MARK : 0)
// ----------------------------------------------------------------------------
//   Switch to long-form command display
// ----------------------------------------------------------------------------
{
    Settings.command_fmt = settings::LONG_FORM;
    Input.menuNeedsRefresh();
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


SETTINGS_COMMAND_NOLABEL(DecimalDot, Settings.decimal_dot == '.' ? MARK : 0)
// ----------------------------------------------------------------------------
//  Switch to decimal dot
// ----------------------------------------------------------------------------
{
    Settings.decimal_dot = '.';
    Input.menuNeedsRefresh();
    return OK;
}


SETTINGS_COMMAND_NOLABEL(DecimalComma, Settings.decimal_dot == ',' ? MARK : 0)
// ----------------------------------------------------------------------------
//  Switch to decimal comma
// ----------------------------------------------------------------------------
{
    Settings.decimal_dot = ',';
    Input.menuNeedsRefresh();
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
