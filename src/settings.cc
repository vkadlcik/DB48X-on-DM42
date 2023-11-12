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

#include "arithmetic.h"
#include "bignum.h"
#include "command.h"
#include "decimal128.h"
#include "font.h"
#include "functions.h"
#include "integer.h"
#include "menu.h"
#include "renderer.h"
#include "symbol.h"
#include "user_interface.h"

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

template<typename Obj, typename T>
static void format(settings UNUSED &s, renderer &out,
                   cstring command, T value)
// ----------------------------------------------------------------------------
//   By default, we print values as integers
// ----------------------------------------------------------------------------
{
    out.printf("%d %s\n", int(value), command);
}


template<>
void format<Foreground,ularge>(settings UNUSED &s, renderer &out,
                               cstring command, ularge value)
// ----------------------------------------------------------------------------
//   Format foreground output
// ----------------------------------------------------------------------------
{
    out.printf("16#%llX %s\n", value, command);
}


template<>
void format<Background,ularge>(settings UNUSED &s, renderer &out,
                               cstring command, ularge value)
// ----------------------------------------------------------------------------
//   Format background output
// ----------------------------------------------------------------------------
{
    out.printf("16#%llX %s\n", value, command);
}


template<>
void format<DisplayMode,object::id>(settings &s, renderer &out,
                                    cstring command, object::id value)
// ----------------------------------------------------------------------------
//   Format background output
// ----------------------------------------------------------------------------
{
    if (value == object::ID_Std)
        out.printf("%s\n", value, command);
    else
        out.printf("%d %s\n", s.DisplayDigits(), command);
}


void settings::save(renderer &out, bool show_defaults)
// ----------------------------------------------------------------------------
//   Save the current settings to the given renderer
// ----------------------------------------------------------------------------
{
    settings Defaults;

#define ID(id)

#define FLAG(Enable,Disable)                    \
    if (Enable())                               \
        out.put(#Enable "\n");                  \
    else if (show_defaults)                     \
        out.put(#Disable "\n");

#define SETTING(Name, Low, High, Init)                                  \
    if (Name() != Defaults.Name() || show_defaults)                     \
        format<struct Name, typeof(Name())>(*this, out, #Name, Name());

#include "ids.tbl"

    // Save the current menu
    if (menu_p menu = ui.menu())
    {
        menu->render(out);
        out.put('\n');
    }
}


COMMAND_BODY(Modes)
// ----------------------------------------------------------------------------
//   Return a program that restores the current modes
// ----------------------------------------------------------------------------
{
    if (rt.args(0))
    {
        renderer modes;
        modes.put("«");
        Settings.save(modes);
        modes.put("»");

        size_t size = modes.size();
        gcutf8 code = modes.text();
        if (object_g program = object::parse(code, size))
            if (rt.push(program))
                return OK;
    }
    return ERROR;
}


COMMAND_BODY(ResetModes)
// ----------------------------------------------------------------------------
//   Reset the default modes
// ----------------------------------------------------------------------------
{
    if (!rt.args(0))
        return ERROR;
    Settings = settings();
    return OK;
}


font_p settings::font(font_id size)
// ----------------------------------------------------------------------------
//   Return a font based on a font size
// ----------------------------------------------------------------------------
{
    switch (size)
    {
    case EDITOR:        return ::EditorFont;
    default:
    case STACK:         return ::StackFont;
    case HELP:          return ::HelpFont;

    case LIB17:         return ::LibMonoFont10x17;
    case LIB18:         return ::LibMonoFont11x18;
    case LIB20:         return ::LibMonoFont12x20;
    case LIB22:         return ::LibMonoFont14x22;
    case LIB25:         return ::LibMonoFont17x25;
    case LIB28:         return ::LibMonoFont17x28;

    case SKR18:         return ::SkrMono13x18;
    case SKR24:         return ::SkrMono18x24;

    case FREE42:        return ::Free42Font;
    }
}


font_p settings::cursor_font(font_id size)
// ----------------------------------------------------------------------------
//   Return a cursor font based on a font size
// ----------------------------------------------------------------------------
{
    switch (size)
    {
    case EDITOR:        return ::StackFont;
    default:
    case STACK:         return ::LibMonoFont14x22;
    case HELP:          return ::HelpFont;

    case LIB17:         return ::LibMonoFont10x17;
    case LIB18:         return ::LibMonoFont10x17;
    case LIB20:         return ::LibMonoFont11x18;
    case LIB22:         return ::LibMonoFont12x20;
    case LIB25:         return ::LibMonoFont14x22;
    case LIB28:         return ::LibMonoFont17x25;

    case SKR18:
    case SKR24:         return ::SkrMono13x18;

    case FREE42:        return ::Free42Font;
    }
}



unicode settings::digit_separator(uint index)
// ----------------------------------------------------------------------------
//   Find the digit separator from
// ----------------------------------------------------------------------------
{
    static unicode sep[] = { SPACE_DEFAULT, ',', L'’', '_' };
    if (sep[index] == ',' && Settings.DecimalComma())
        return '.';
    return sep[index];
}


template<>
ularge setting_value<ularge>(object_p obj, ularge init)
// ----------------------------------------------------------------------------
//   Specialization for the ularge type
// ----------------------------------------------------------------------------
{
    if (integer_p ival = obj->as<integer>())
        return ival->value<ularge>();
    if (bignum_p ival = obj->as<bignum>())
        return ival->value<ularge>();
    rt.type_error();
    return init;
}


EVAL_BODY(value_setting)
// ----------------------------------------------------------------------------
//   Evaluate a value setting by invoking the base command
// ----------------------------------------------------------------------------
{
    id ty   = o->type();

    if (ty >= ID_Fix && ty <= ID_Sig)
    {
        uint digits = Settings.DisplayDigits();
        if (!validate(ty, digits, 0U, DB48X_MAXDIGITS))
            return ERROR;
        Settings.DisplayDigits(digits);

    }
    else if (ty == ID_Std)
    {
        Settings.DisplayDigits(settings().DisplayDigits());
    }

    switch(ty)
    {
#define ID(i)
#define SETTING_VALUE(Name, Alias, Base, Value)                 \
        case ID_##Name:         Settings.Base(Value); break;
#include "ids.tbl"

    default:
        rt.invalid_setting_error();
        return ERROR;
    }
    update(ty);
    return OK;
}


template <typename Value>
static object_p object_from_value(Value value)
// ----------------------------------------------------------------------------
//   Convert
// ----------------------------------------------------------------------------
{
    if (value < 0)
        return neg_integer::make(-value);
    return integer::make(value);
}


template <>
object_p object_from_value<object::id>(object::id value)
// ----------------------------------------------------------------------------
//   Return a static object for enum settings
// ----------------------------------------------------------------------------
{
    return command::static_object(value);
}



EVAL_BODY(recall_setting)
// ----------------------------------------------------------------------------
//   Recall the value of a setting
// ----------------------------------------------------------------------------
{
    id       ty  = o->type();
    id       rty = ID_object;
    object_p obj = nullptr;

    switch (ty)
    {
#define ID(i)
#define FLAG(Enable, Disable)                           \
    case ID_Recall##Enable:                             \
        rty = Settings.Enable() ? ID_True : ID_False;   \
        break;                                          \
    case ID_Recall##Disable:                            \
        rty = Settings.Disable() ? ID_True : ID_False;

#define SETTING(Name, Low, High, Init)                          \
        case ID_Recall##Name:                                   \
            obj = object_from_value(Settings.Name());           \
            break;
#include "ids.tbl"

    default:
        rt.invalid_setting_error();
        return ERROR;
    }

    if (rty)
        obj = command::static_object(rty);
    if (!obj || rt.push(obj))
        return ERROR;

    return OK;
}


cstring setting::printf(cstring format, ...)
// ----------------------------------------------------------------------------
//   Render a setting using some specific format
// ----------------------------------------------------------------------------
{
    va_list va;
    va_start(va, format);
    char   buf[80];
    size_t size = vsnprintf(buf, sizeof(buf), format, va);
    va_end(va);
    symbol_p sym = symbol::make(utf8(buf), size);
    return cstring(sym);
}


cstring setting::label(object::id ty)
// ----------------------------------------------------------------------------
//   Render the label for the given type
// ----------------------------------------------------------------------------
{
    settings &s = Settings;
    switch(ty)
    {
    case ID_Sig:
        if (s.DisplayMode() == ID_Std)
            return printf("%s %u", name(ty), s.DisplayDigits());
    case ID_Fix:
    case ID_Sci:
    case ID_Eng:
        if (ty == s.DisplayMode())
            return printf("%s %u", name(ty), s.DisplayDigits());
        return cstring(name(ty));

    case ID_Base:
        return printf("Base %u", s.Base());
    case ID_WordSize:
        return printf("WSize %u", s.WordSize());
    case ID_FractionIterations:
        return printf("→QIter %u", s.FractionIterations());
    case ID_FractionDigits:
        return printf("→QPrec %u", s.FractionDigits());
    case ID_Precision:
        return printf("Prec %u", s.Precision());
    case ID_MantissaSpacing:
        return printf("Mant %u", s.MantissaSpacing());
    case ID_FractionSpacing:
        return printf("Frac %u", s.FractionSpacing());
    case ID_BasedSpacing:
        return printf("Based %u", s.BasedSpacing());
    case ID_StandardExponent:
        return printf("Exp %u", s.StandardExponent());
    case ID_MinimumSignificantDigits:
        return printf("Dig %d", s.MinimumSignificantDigits());
    case ID_ResultFont:
        return printf("Result %u", s.ResultFont());
    case ID_StackFont:
        return printf("Stack %u", s.StackFont());
    case ID_EditorFont:
        return printf("Edit %u", s.EditorFont());
    case ID_MultilineEditorFont:
        return printf("MLEd %u", s.MultilineEditorFont());
    case ID_CursorBlinkRate:
        return printf("Blink %u", s.CursorBlinkRate());
    case ID_MaxBigNumBits:
        return printf("Bits %u", s.MaxBigNumBits());
    case ID_MaxRewrites:
        return printf("Rwr %u", s.MaxRewrites());


    default:
        break;
    }
    return cstring(object::fancy(ty));
}
