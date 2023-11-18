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

#include "command.h"
#include "menu.h"
#include "renderer.h"
#include "target.h"

#include <types.h>


struct renderer;

#define DB48X_MAXDIGITS uint(BID128_MAXDIGITS)

struct settings
// ----------------------------------------------------------------------------
//    Internal representation of settings
// ----------------------------------------------------------------------------
{
public:
#define ID(id)
#define FLAG(Enable, Disable)
#define SETTING(Name, Low, High, Init)          typeof(Init) Name##_bits;
#define SETTING_BITS(Name,Bits,Low,High,Init)
#include "ids.tbl"

    // Define the packed bits settings
#define ID(id)
#define FLAG(Enable, Disable)
#define SETTING(Name, Low, High, Init)
#define SETTING_BITS(Name,Bits,Low,High,Init)   uint Name##_bits : Bits;
#include "ids.tbl"

    // Define the flags
#define ID(id)
#define FLAG(Enable, Disable)                   bool Enable##_bit : 1;
#define SETTING(Name, Low, High, Init)
#define SETTING_BITS(Name,Bits,Low,High,Init)
#include "ids.tbl"

    bool reserved : 1;

public:
    enum { STD_DISPLAYED = 20 };

    enum
    {
        // Try hard to make source code unreadable
        SPACE_3_PER_EM          = L' ',
        SPACE_4_PER_EM          = L' ',
        SPACE_6_PER_EM          = L' ',
        SPACE_THIN              = L' ',
        SPACE_MEDIUM_MATH       = L' ',

        SPACE_DEFAULT           = SPACE_MEDIUM_MATH,
        SPACE_UNIT              = SPACE_6_PER_EM,

        MARK                    = L'●', // L'■'
        COMPLEX_I               = L'𝒊',
        DEGREES_SYMBOL          = L'°',
        RADIANS_SYMBOL          = L'ℼ',
        GRAD_SYMBOL             = L'ℊ',
        PI_RADIANS_SYMBOL       = L'π',
    };

    enum font_id
    // ------------------------------------------------------------------------
    //  Selection of font size for the stack
    // ------------------------------------------------------------------------
    {
        EDITOR, STACK, HELP,
        LIB28, LIB25, LIB22, LIB20, LIB18, LIB17,
        SKR24, SKR18,
        FREE42,
        FIRST_FONT = EDITOR,
        LAST_FONT = FREE42,
        NUM_FONTS
    };

#define ID(i)   static const object::id ID_##i = object::ID_##i;
#include "ids.tbl"


    settings() :
#define ID(id)
#define FLAG(Enable, Disable)
#define SETTING(Name,Low,High,Init)             Name##_bits(Init),
#define SETTING_BITS(Name,Bits,Low,High,Init)
#include "ids.tbl"

    // Define the packed bits settings
#define ID(id)
#define FLAG(Enable, Disable)
#define SETTING(Name, Low, High, Init)
#define SETTING_BITS(Name,Bits,Low,High,Init)   Name##_bits(Init - Low),
#include "ids.tbl"

    // Define the flags
#define ID(id)
#define FLAG(Enable, Disable)                   Enable##_bit(false),
#define SETTINGS(Name, Low, High, Init)
#define SETTING_BITS(Name,Bits,Low,High,Init)
#include "ids.tbl"

        reserved(false)
    {}

    // Accessor functions
#define ID(id)
#define FLAG(Enable,Disable)                                            \
    bool Enable() const                 { return Enable##_bit; }        \
    bool Disable() const                { return !Enable##_bit; }       \
    void Enable(bool flag)              { Enable##_bit = flag; }        \
    void Disable(bool flag)             { Enable##_bit = !flag; }
#define SETTING(Name, Low, High, Init)                                  \
    typeof(Init) Name() const           { return Name##_bits;  }        \
    void Name(typeof(Init) value)       { Name##_bits = value; }
#define SETTING_BITS(Name, Bits, Low, High, Init)                       \
    typeof(Init) Name() const           { return (typeof(Init)) (Low + Name##_bits);  } \
    void Name(typeof(Init) value)       { Name##_bits = value - Low; }
#include "ids.tbl"

    static font_p font(font_id sz);
    static font_p cursor_font(font_id sz);
    font_p result_font()        { return font(ResultFont()); }
    font_p stack_font()         { return font(StackFont()); }
    font_p editor_font(bool ml) { return font(ml ? MultilineEditorFont() : EditorFont()); }
    font_p cursor_font(bool ml) { return cursor_font(ml ? MultilineEditorFont() : EditorFont()); }

    static unicode digit_separator(uint index);

    unicode NumberSeparator() const
    {
        return digit_separator(NumberSeparatorCommand() - object::ID_NumberSpaces);
    }
    unicode BasedSeparator() const
    {
        return digit_separator(BasedSeparatorCommand() - object::ID_BasedSpaces);
    }
    unicode DecimalSeparator() const
    {
        return DecimalComma() ? ',' : '.';
    }
    unicode ExponentSeparator() const
    {
        return FancyExponent() ? L'⁳' : 'E';
    }

    char DateSeparator() const
    {
        uint index = DateSeparatorCommand() - object::ID_DateSlash;
        static char sep[4] = { '/', '-', '.', '\'' };
        return sep[index];
    }

    void NextDateSeparator()
    {
        DateSeparatorCommand_bits++;
    }

    void save(renderer &out, bool show_defaults = false);

    static bool     store(object::id name, object_p value);
    static object_p recall(object::id name);
    static bool     purge(object::id name);

    static bool     flag(object::id name, bool value);
    static bool     flag(object::id name, bool *value);
};


extern settings Settings;

template<typename T>
T setting_value(object_p obj, T init)
{
    return T(obj->as_uint32(init, true));
}

template <>
ularge setting_value<ularge>(object_p obj, ularge init);

template <>
int setting_value<int>(object_p obj, int init);

template <>
object::id setting_value<object::id>(object_p obj, object::id init);


struct setting : command
// ----------------------------------------------------------------------------
//   Shared code for settings
// ----------------------------------------------------------------------------
{
    setting(id i) : command(i) {}
    static result update(id ty)
    {
        rt.command(fancy(ty));
        ui.menu_refresh();
        return OK;
    }

    template<typename T>
    static bool validate(id type, T &valref, T low, T high)
    {
        if (rt.args(1))
        {
            if (object_p obj = rt.top())
            {
                T val = setting_value(obj, T(valref));
                if (!rt.error())
                {
                    if (val >= low && val <= high)
                    {
                        valref = T(val);
                        rt.pop();
                        return true;
                    }
                    rt.domain_error();
                }
            }
        }
        rt.command(fancy(type));
        return false;
    }

    static cstring label(id ty);
    static cstring printf(cstring format, ...);
};


struct value_setting : setting
// ----------------------------------------------------------------------------
//   Use a setting value
// ----------------------------------------------------------------------------
{
    value_setting(id type): setting(type) {}
    EVAL_DECL(value_setting);
};

#define ID(i)

#define FLAG(Enable, Disable)                           \
                                                        \
struct Enable : setting                                 \
{                                                       \
    Enable(id i = ID_##Enable): setting(i) {}           \
    OBJECT_DECL(Enable);                                \
    EVAL_DECL(Enable)                                   \
    {                                                   \
        Settings.Enable(true);                          \
        return update(ID_##Enable);                     \
    }                                                   \
    MARKER_DECL(Enable)                                 \
    {                                                   \
        return Settings.Enable();                       \
    }                                                   \
};                                                      \
                                                        \
struct Disable : setting                                \
{                                                       \
    Disable(id i = ID_##Disable): setting(i) {}         \
    OBJECT_DECL(Disable);                               \
    EVAL_DECL(Disable)                                  \
    {                                                   \
        Settings.Disable(true);                         \
        return update(ID_##Disable);                    \
    }                                                   \
    MARKER_DECL(Disable)                                \
    {                                                   \
        return Settings.Disable();                      \
    }                                                   \
};


#define SETTING(Name, Low, High, Init)                                  \
                                                                        \
struct Name : setting                                                   \
{                                                                       \
    Name(id i = ID_##Name) : setting(i) {}                              \
                                                                        \
    OBJECT_DECL(Name);                                                  \
    EVAL_DECL(Name)                                                     \
    {                                                                   \
        auto value = Settings.Name();                                   \
        if (!validate(ID_##Name, value, Low, High))                     \
            return ERROR;                                               \
        Settings.Name(value);                                           \
        update(ID_##Name);                                              \
        return OK;                                                      \
    }                                                                   \
};

#define SETTING_VALUE(Name, Alias, Base, Value)                 \
    struct Name : value_setting                                 \
    {                                                           \
        Name(id ty = ID_##Name) : value_setting(ty) {  }        \
        OBJECT_DECL(Name);                                      \
        MARKER_DECL(Name)                                       \
        {                                                       \
            return Settings.Base() == Value;                    \
        }                                                       \
    };

#include "ids.tbl"


COMMAND_DECLARE(Modes);
COMMAND_DECLARE(ResetModes);
COMMAND_DECLARE(RecallWordSize);

#endif // SETTINGS_H
