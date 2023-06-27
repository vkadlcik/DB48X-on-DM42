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
#include "input.h"
#include "integer.h"
#include "menu.h"

#include <cstdarg>
#include <cstdlib>
#include <random>
#include <stdexcept>

settings Settings;


// ============================================================================
//
//   Save the current settings to a renderer
//
// ============================================================================

void settings::save(renderer &out, bool show_defaults)
// ----------------------------------------------------------------------------
//   Save the current settings to the given renderer
// ----------------------------------------------------------------------------
{
    // Save the current menu
    if (menu_p menu = Input.menu())
    {
        menu->render(out, runtime::RT);
        out.put('\n');
    }

    // Save current computation precision
    if (precision != BID128_MAXDIGITS || show_defaults)
        out.put("%u Precision\n", precision);

    // Save current display setting
    switch(display_mode)
    {
    case NORMAL:
        if (displayed == settings::STD_DISPLAYED)
        {
            if (show_defaults)
                out.put("%STD\n");
        }
        else    out.printf("%u SIG\n", displayed); break;
    case FIX:   out.printf("%u FIX\n", displayed); break;
    case SCI:   out.printf("%u SCI\n", displayed); break;
    case ENG:   out.printf("%u ENG\n", displayed); break;
    }

    // Save Decimal separator
    if (decimal_dot == ',')
        out.put("DecimalComma\n");
    else if (show_defaults)
        out.put("DecimalDot\n");

    // Save preferred exponent display mode
    if (exponent_char != L'⁳' || !fancy_exponent)
        out.put("ClassicExponent\n");
    else if (show_defaults)
        out.put("FancyExponent\n");

    // Save preferred expenent for switching to scientfiic mode
    if (max_nonsci != 9 || show_defaults)
        out.printf("%u StandardExponent\n", max_nonsci);

    // Save current angle mode
    switch(angle_mode)
    {
    default:
    case DEGREES:       if (show_defaults)      out.put("DEG\n"); break;
    case RADIANS:                               out.put("RAD\n"); break;
    case GRADS:                                 out.put("GRAD\n"); break;
    }

    // Save default base
    if( base != 16 || show_defaults)
        out.printf("%u Base\n", base);

    // Save default word size
    if (wordsize != 64 || show_defaults)
        out.printf("%u WordSize\n", wordsize);

    // Save default command format
    switch(command_fmt)
    {
    default:
    case LONG_FORM:     if (show_defaults)      out.put("LongForm\n");    break;
    case LOWERCASE:                             out.put("lowercase\n");   break;
    case UPPERCASE:                             out.put("UPPERCASE\n");   break;
    case CAPITALIZED:                           out.put("Capitalized\n"); break;
    }

    // Check if we want to show 1.0 as 1 and not 1.
    if (!show_decimal)
        out.put("NoTrailingDecimal\n");
    else if (show_defaults)
        out.put("TrailingDecimal\n");
}


COMMAND_BODY(Modes)
// ----------------------------------------------------------------------------
//   Return a program that restores the current modes
// ----------------------------------------------------------------------------
{
    renderer modes;
    modes.put("«");
    Settings.save(modes);
    modes.put("»");

    size_t size = modes.size();
    gcutf8 code = modes.text();
    if (gcobj program = object::parse(code, size))
        if (runtime::RT.push(program))
            return OK;
    return ERROR;
}



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


SETTINGS_COMMAND_NOLABEL(Deg,
                         Settings.angle_mode == settings::DEGREES ? MARK : 0)
// ----------------------------------------------------------------------------
//   Switch to degrees
// ----------------------------------------------------------------------------
{
    Settings.angle_mode = settings::DEGREES;
    return OK;
}


SETTINGS_COMMAND_NOLABEL(Rad,
                         Settings.angle_mode == settings::RADIANS ? MARK : 0)
// ----------------------------------------------------------------------------
//   Switch to radians
// ----------------------------------------------------------------------------
{
    Settings.angle_mode = settings::RADIANS;
    return OK;
}


SETTINGS_COMMAND_NOLABEL(Grad,
                         Settings.angle_mode == settings::GRADS ? MARK : 0)
// ----------------------------------------------------------------------------
//   Switch to grads
// ----------------------------------------------------------------------------
{
    Settings.angle_mode = settings::GRADS;
    return OK;
}


SETTINGS_COMMAND_NOLABEL(LowerCase,
                         Settings.command_fmt == settings::LOWERCASE ? MARK : 0)
// ----------------------------------------------------------------------------
//   Switch to lowercase command display
// ----------------------------------------------------------------------------
{
    Settings.command_fmt = settings::LOWERCASE;
    return OK;
}


SETTINGS_COMMAND_NOLABEL(UpperCase,
                         Settings.command_fmt == settings::UPPERCASE ? MARK : 0)
// ----------------------------------------------------------------------------
//  Switch to uppercase command display
// ----------------------------------------------------------------------------
{
    Settings.command_fmt = settings::UPPERCASE;
    return OK;
}


SETTINGS_COMMAND_NOLABEL(Capitalized,
                         Settings.command_fmt==settings::CAPITALIZED ? MARK : 0)
// ----------------------------------------------------------------------------
//  Switch to capitalized command display
// ----------------------------------------------------------------------------
{
    Settings.command_fmt = settings::CAPITALIZED;
    return OK;
}


SETTINGS_COMMAND_NOLABEL(LongForm,
                         Settings.command_fmt==settings::LONG_FORM ? MARK : 0)
// ----------------------------------------------------------------------------
//   Switch to long-form command display
// ----------------------------------------------------------------------------
{
    Settings.command_fmt = settings::LONG_FORM;
    return OK;
}


SETTINGS_COMMAND_NOLABEL(DecimalDot, Settings.decimal_dot == '.' ? MARK : 0)
// ----------------------------------------------------------------------------
//  Switch to decimal dot
// ----------------------------------------------------------------------------
{
    Settings.decimal_dot = '.';
    return OK;
}


SETTINGS_COMMAND_NOLABEL(DecimalComma, Settings.decimal_dot == ',' ? MARK : 0)
// ----------------------------------------------------------------------------
//  Switch to decimal comma
// ----------------------------------------------------------------------------
{
    Settings.decimal_dot = ',';
    return OK;
}


SETTINGS_COMMAND_NOLABEL(TrailingDecimal, Settings.show_decimal ? MARK : 0)
// ----------------------------------------------------------------------------
//  Indicate that we want a trailing decimal separator
// ----------------------------------------------------------------------------
{
    Settings.show_decimal = true;
    return OK;
}


SETTINGS_COMMAND_NOLABEL(NoTrailingDecimal, !Settings.show_decimal ? MARK : 0)
// ----------------------------------------------------------------------------
//  Indicate that we don't want a traiing decimal separator
// ----------------------------------------------------------------------------
{
    Settings.show_decimal = false;
    return OK;
}


SETTINGS_COMMAND_BODY(Precision, 0)
// ----------------------------------------------------------------------------
//   Setting the precision
// ----------------------------------------------------------------------------
{
    if (object_p size = runtime::RT.top())
    {
        if (integer_p digits = size->as<integer>())
        {
            uint disp = digits->value<uint>();
            Settings.precision = std::min(disp, (uint) BID128_MAXDIGITS);
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


SETTINGS_COMMAND_LABEL(Precision)
// ----------------------------------------------------------------------------
//   Return the label for the current precision
// ----------------------------------------------------------------------------
{
    // We can share the buffer here since only one mode is active
    static char buffer[12];
    snprintf(buffer, sizeof(buffer), "Prec %u", Settings.precision);
    return buffer;
}


SETTINGS_COMMAND_BODY(StandardExponent, 0)
// ----------------------------------------------------------------------------
//   Setting the maximum exponent before switching to scientific mode
// ----------------------------------------------------------------------------
{
    if (object_p size = runtime::RT.top())
    {
        if (integer_p digits = size->as<integer>())
        {
            uint disp = digits->value<uint>();
            Settings.max_nonsci = std::min(disp, (uint) BID128_MAXDIGITS);
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


SETTINGS_COMMAND_LABEL(StandardExponent)
// ----------------------------------------------------------------------------
//   Return the label for the current standard exponent
// ----------------------------------------------------------------------------
{
    // We can share the buffer here since only one mode is active
    static char buffer[12];
    snprintf(buffer, sizeof(buffer), "Exp %u", Settings.max_nonsci);
    return buffer;
}


SETTINGS_COMMAND_NOLABEL(FancyExponent, Settings.fancy_exponent ? MARK : 0)
// ----------------------------------------------------------------------------
//   Setting the maximum exponent before switching to scientific mode
// ----------------------------------------------------------------------------
{
    Settings.fancy_exponent = true;
    Settings.exponent_char = L'⁳';
    return OK;
}


SETTINGS_COMMAND_NOLABEL(ClassicExponent, !Settings.fancy_exponent ? MARK : 0)
// ----------------------------------------------------------------------------
//   Setting the maximum exponent before switching to scientific mode
// ----------------------------------------------------------------------------
{
    Settings.fancy_exponent = false;
    Settings.exponent_char = 'E';
    return OK;
}


SETTINGS_COMMAND_BODY(Base, 0)
// ----------------------------------------------------------------------------
//   Setting the maximum exponent before switching to scientific mode
// ----------------------------------------------------------------------------
{
    if (object_p size = runtime::RT.top())
    {
        if (integer_p digits = size->as<integer>())
        {
            uint base = digits->value<uint>();
            runtime::RT.pop();
            if (base >= 2 && base <= 36)
            {
                Settings.base = base;
                return object::OK;
            }
            runtime::RT.invalid_base_error();
        }
        else
        {
            runtime::RT.type_error();
        }
    }
    return object::ERROR;
}


SETTINGS_COMMAND_LABEL(Base)
// ----------------------------------------------------------------------------
//   Return the label for the current base
// ----------------------------------------------------------------------------
{
    // We can share the buffer here since only one mode is active
    static char buffer[12];
    snprintf(buffer, sizeof(buffer), "Base %u", Settings.base);
    return buffer;
}


SETTINGS_COMMAND_NOLABEL(Bin, Settings.base == 2 ? MARK : 0)
// ----------------------------------------------------------------------------
//   Select binary mode
// ----------------------------------------------------------------------------
{
    Settings.base = 2;
    return OK;
}


SETTINGS_COMMAND_NOLABEL(Oct, Settings.base == 8 ? MARK : 0)
// ----------------------------------------------------------------------------
//   Select octal mode
// ----------------------------------------------------------------------------
{
    Settings.base = 8;
    return OK;
}


SETTINGS_COMMAND_NOLABEL(Dec, Settings.base == 10 ? MARK : 0)
// ----------------------------------------------------------------------------
//   Select decimalmode
// ----------------------------------------------------------------------------
{
    Settings.base = 10;
    return OK;
}


SETTINGS_COMMAND_NOLABEL(Hex, Settings.base == 16 ? MARK : 0)
// ----------------------------------------------------------------------------
//   Select hexadecimal mode
// ----------------------------------------------------------------------------
{
    Settings.base = 16;
    return OK;
}


SETTINGS_COMMAND_BODY(stws, 0)
// ----------------------------------------------------------------------------
//   Setting the word size for binary computations
// ----------------------------------------------------------------------------
{
    if (object_p size = runtime::RT.top())
    {
        if (integer_p digits = size->as<integer>())
        {
            uint ws = digits->value<uint>();
            runtime::RT.pop();
            Settings.wordsize = ws;
            return OK;
        }
        else
        {
            runtime::RT.type_error();
        }
    }
    return object::ERROR;
}


SETTINGS_COMMAND_LABEL(stws)
// ----------------------------------------------------------------------------
//   Return the label for the current word size
// ----------------------------------------------------------------------------
{
    static char buffer[16];
    snprintf(buffer, sizeof(buffer), "WordSz %u", Settings.wordsize);
    return buffer;
}


COMMAND_BODY(rcws)
// ----------------------------------------------------------------------------
//  Recall the current wordsize
// ----------------------------------------------------------------------------
{
    if (gcobj ws = integer::make(Settings.wordsize))
        if (runtime::RT.push(ws))
            return OK;
    return ERROR;
}
