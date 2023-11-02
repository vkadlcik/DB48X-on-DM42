// ****************************************************************************
//  command.cc                                                    DB48X project
// ****************************************************************************
//
//   File Description:
//
//     Shared code for all RPL commands
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

#include "arithmetic.h"
#include "bignum.h"
#include "decimal-32.h"
#include "decimal-64.h"
#include "decimal128.h"
#include "dmcp.h"
#include "fraction.h"
#include "integer.h"
#include "parser.h"
#include "renderer.h"
#include "runtime.h"
#include "settings.h"
#include "symbol.h"
#include "sysmenu.h"
#include "unit.h"
#include "user_interface.h"
#include "utf8.h"
#include "version.h"

#include <ctype.h>
#include <stdio.h>

RECORDER(command,       16, "RPL Commands");
RECORDER(command_error, 16, "Errors processing a command");


PARSE_BODY(command)
// ----------------------------------------------------------------------------
//    Try to parse this as a command, using either short or long name
// ----------------------------------------------------------------------------
{
    id      i      = p.candidate;
    bool    eq     = p.precedence;

    // If we are parsing an equation, only accept algebraic command
    if (eq && !is_algebraic(i))
        return SKIP;

    id      found  = id(0);
    cstring ref    = cstring(utf8(p.source));
    size_t  maxlen = p.length;
    size_t  len    = maxlen;

    bool disambiguate = eq && (i == ID_sq || i == ID_cubed || i == ID_inv);

    cstring names[3] = { nullptr };
    names[0] = disambiguate ? nullptr : cstring(object::fancy(i));
    names[1] = cstring(name(i));
    switch(i)
    {
#define ALIAS(base, name)                                               \
        case ID_##base: names[2] = name; break;
#define ID(i)
#include "ids.tbl"
        default:
            break;
    }

    for (uint attempt = 0; !found && attempt < 3; attempt++)
    {
        if (cstring cmd = names[attempt])
        {
            len = strlen(cmd);
            if (len <= maxlen
                && strncasecmp(ref, cmd, len) == 0
                && (len >= maxlen
                    || (eq && !is_valid_as_name_initial(utf8(cmd)))
                    || is_separator(utf8(ref + len))))
                found = id(i);
        }
    }

    record(command,
           "Parsing [%s] with id %u %+s (%+s), found %u len %u",
           ref, i, name(i), fancy(i), found, len);

    if (!found)
        return SKIP;

    // Record output - Dynamically generate ID for use in programs
    p.end = len;
    p.out = rt.make<command>(found);

    return OK;
}


RENDER_BODY(command)
// ----------------------------------------------------------------------------
//   Render the command into the given string buffer
// ----------------------------------------------------------------------------
{
    id ty = o->type();
    if (ty < NUM_IDS)
    {
        auto format = Settings.command_fmt;

        // Ensure that we display + as `+` irrespective of mode
        utf8 fname = fancy(ty);
        if (!is_valid_as_name_initial(utf8_codepoint(fname)))
        {
            if (utf8_length(fname) == 1)
            {
                format = settings::commands::LONG_FORM;
                while (unit::mode)
                {
                    if (ty == ID_div)
                        r.put('/');
                    else if (ty == ID_mul)
                        r.put(unicode(L'·'));
                    else
                        break;
                    return r.size();
                }
            }
        }


        utf8 text = utf8(format == settings::commands::LONG_FORM
                         ? fname : name(ty));
        r.put(format, text);
    }

    record(command, "Render %u as [%s]", ty, (cstring) r.text());
    return r.size();
}


object_p command::static_object(id i)
// ----------------------------------------------------------------------------
//   Return a pointer to a static object representing the command
// ----------------------------------------------------------------------------
{
    static byte cmds[] =
    {
#define ID(id)                                                \
    object::ID_##id < 0x80 ? (object::ID_##id & 0x7F) | 0x00  \
                           : (object::ID_##id & 0x7F) | 0x80, \
    object::ID_##id < 0x80 ? 0 : ((object::ID_##id) >> 7),
#include "ids.tbl"
    };

    if (i >= NUM_IDS)
        i = ID_object;

    return (object_p) (cmds + 2 * i);
}


bool command::is_separator(utf8 str)
// ----------------------------------------------------------------------------
//   Check if the code point at given string is a separator
// ----------------------------------------------------------------------------
{
    unicode code = utf8_codepoint(str);
    return is_separator(code);
}


bool command::is_separator(unicode code)
// ----------------------------------------------------------------------------
//   Check if the code point at given string is a separator
// ----------------------------------------------------------------------------
{
    static utf8 separators = utf8(" ;,.'\"<=>≤≠≥[](){}«»\n\t");
    for (utf8 p = separators; *p; p = utf8_next(p))
        if (code == utf8_codepoint(p))
            return true;
    return false;
}


bool command::is_separator_or_digit(utf8 str)
// ----------------------------------------------------------------------------
//   Check if the code point at given string is a separator
// ----------------------------------------------------------------------------
{
    unicode code = utf8_codepoint(str);
    return is_separator(code);
}


bool command::is_separator_or_digit(unicode code)
// ----------------------------------------------------------------------------
//   Check if the code point at given string is a separator
// ----------------------------------------------------------------------------
{
    static utf8 separators = utf8(" ;,.'\"<=>≤≠≥[](){}«»0123456789⁳");
    for (utf8 p = separators; *p; p = utf8_next(p))
        if (code == utf8_codepoint(p))
            return true;
    return false;
}


uint32_t command::uint32_arg(uint level)
// ----------------------------------------------------------------------------
//   Get an unsigned value from the stack
// ----------------------------------------------------------------------------
{
    if (object_p d = rt.stack(level))
        return d->as_uint32(0, true);
    return 0;
}


int32_t command::int32_arg(uint level)
// ----------------------------------------------------------------------------
//   Get a signed value from the stack
// ----------------------------------------------------------------------------
{
    if (object_p d = rt.stack(level))
        return d->as_int32(0, true);
    return 0;
}



// ============================================================================
//
//   Command implementations
//
// ============================================================================

COMMAND_BODY(Eval)
// ----------------------------------------------------------------------------
//   Evaluate an object
// ----------------------------------------------------------------------------
{
    if (rt.args(1))
        if (object_p x = rt.pop())
            return x->execute();
    return ERROR;
}

COMMAND_BODY(ToText)
// ----------------------------------------------------------------------------
//   Convert an object to text
// ----------------------------------------------------------------------------
{
    if (rt.args(1))
        if (object_g obj = rt.top())
            if (object_g txt = obj->as_text(false, false))
                if (rt.top(txt))
                    return OK;
    return ERROR;
}

COMMAND_BODY(SelfInsert)
// ----------------------------------------------------------------------------
//   Find the label associated to the menu and enter it in the editor
// ----------------------------------------------------------------------------
{
    int key = ui.evaluating;
    if (key >= KEY_F1 && key <= KEY_F6)
    {
        uint plane = ui.shift_plane();
        uint menu_idx = key - KEY_F1 + plane * ui.NUM_SOFTKEYS;
        uint count = 0;
        if (cstring lbl = ui.label_text(menu_idx))
        {
            uint cpos = ui.cursorPosition();
            for (utf8 p = utf8(lbl); *p; p = utf8_next(p))
            {
                ui.edit(utf8_codepoint(p), ui.PROGRAM, false);
                count++;
                if (count == 1)
                    cpos = ui.cursorPosition();
            }
            if (count == 2)
                ui.cursorPosition(cpos);

        }
    }
    return OK;
}


EXEC_BODY(Unimplemented)
// ----------------------------------------------------------------------------
//   Display an unimplemented error
// ----------------------------------------------------------------------------
{
    int key = ui.evaluating;
    rt.command("Future");
    if (key >= KEY_F1 && key <= KEY_F6)
    {
        uint plane = ui.shift_plane();
        uint menu_idx = key - KEY_F1 + plane * ui.NUM_SOFTKEYS;
        if (cstring lbl = ui.label_text(menu_idx))
            rt.command(lbl);
    }
    rt.unimplemented_error();
    return ERROR;
}


MARKER_BODY(Unimplemented)
// ----------------------------------------------------------------------------
//   We mark unimplemented features with a little gray mark
// ----------------------------------------------------------------------------
{
    return L'░';
}


COMMAND_BODY(Ticks)
// ----------------------------------------------------------------------------
//   Return number of ticks
// ----------------------------------------------------------------------------
{
    if (rt.args(0))
    {
        uint ticks = sys_current_ms();
        if (integer_p ti = rt.make<integer>(ID_integer, ticks))
            if (rt.push(ti))
                return OK;
    }
    return ERROR;
}


COMMAND_BODY(Wait)
// ----------------------------------------------------------------------------
//   Wait the specified amount of seconds
// ----------------------------------------------------------------------------
{
    if (!rt.args(1))
        return ERROR;
    if (object_p obj = rt.top())
    {
        if (algebraic_g wtime = obj->as_algebraic())
        {
            rt.drop();
            algebraic_g scale = integer::make(1000);
            wtime = wtime * scale;
            if (wtime)
            {
                bool     negative = wtime->is_negative();
                uint32_t msec     = negative ? 0 : wtime->as_uint32(1000, true);
                uint32_t end      = sys_current_ms() + msec;
                bool     infinite = msec == 0 || negative;
                int      key      = 0;

                if (negative)
                    ui.draw_menus();
                while (!key)
                {
                    int remains = infinite ? 100 : int(end - sys_current_ms());
                    if (remains <= 0)
                        break;
                    if (remains > 50)
                        remains = 50;
                    sys_delay(remains);
                    if (!key_empty())
#if SIMULATOR
                        if (key_tail() != KEY_EXIT)
#endif
                            key = key_pop();
                }
                if (infinite)
                {
                    if (integer_p ikey = integer::make(key))
                        if (rt.push(ikey))
                            return OK;
                    return ERROR;
                }
                return OK;
            }
        }
        else
        {
            rt.type_error();
        }
    }
    return ERROR;
}



COMMAND_BODY(Bytes)
// ----------------------------------------------------------------------------
//   Return the bytes and a binary represenetation of the object
// ----------------------------------------------------------------------------
{
    if (rt.args(1))
    {
        if (object_p top = rt.top())
        {
            size_t size = top->size();
            size_t maxsize = (Settings.wordsize + 7) / 8;
            size_t hashsize = size > maxsize ? maxsize : size;
            gcbytes bytes = byte_p(top);
#if CONFIG_FIXED_BASED_OBJECTS
            // Force base 16 if we have that option
            const id type = ID_hex_bignum;
#else // !CONFIG_FIXED_BASED_OBJECTS
            const id type = ID_based_bignum;
#endif // CONFIG_FIXED_BASED_OBJECTS
            if (bignum_p bin = rt.make<bignum>(type, bytes, hashsize))
                if (rt.top(bin))
                    if (rt.push(integer::make(size)))
                        return OK;

        }
    }
    return ERROR;
}



COMMAND_BODY(Type)
// ----------------------------------------------------------------------------
//   Return the type of the top of stack as a numerical value
// ----------------------------------------------------------------------------
{
    if (rt.args(1))
        if (object_p top = rt.top())
            if (integer_p type = integer::make(uint(top->type())))
                if (rt.top(type))
                    return OK;
    return ERROR;
}


COMMAND_BODY(TypeName)
// ----------------------------------------------------------------------------
//   Return the type of the top of stack as text
// ----------------------------------------------------------------------------
{
    if (rt.args(1))
        if (object_p top = rt.top())
            if (text_p type = text::make(top->fancy()))
                if (rt.top(type))
                    return OK;
    return ERROR;
}


COMMAND_BODY(Off)
// ----------------------------------------------------------------------------
//   Switch the calculator off
// ----------------------------------------------------------------------------
{
    if (!rt.args(0))
        return ERROR;
    power_off();
    return OK;
}


COMMAND_BODY(SaveState)
// ----------------------------------------------------------------------------
//   Save the system state to disk
// ----------------------------------------------------------------------------
{
    if (!rt.args(0))
        return ERROR;
    save_system_state();
    return OK;
}


COMMAND_BODY(SystemSetup)
// ----------------------------------------------------------------------------
//   Select the system menu
// ----------------------------------------------------------------------------
{
    if (!rt.args(0))
        return ERROR;
    system_setup();
    return OK;
}


COMMAND_BODY(Version)
// ----------------------------------------------------------------------------
//   Return a version string
// ----------------------------------------------------------------------------
{
    const utf8 version_text = (utf8)
        "DB48X " DB48X_VERSION "\n"
        "A modern implementation of\n"
        "Reverse Polish Lisp (RPL)\n"
        "and a tribute to\n"
        "Bill Hewlett and Dave Packard\n"
        "© 2022-2023 Christophe de Dinechin";
    if (rt.args(0))
        if (text_g version = text::make(version_text))
            if (rt.push(object_p(version)))
                return OK;
    return ERROR;
}


COMMAND_BODY(Help)
// ----------------------------------------------------------------------------
//   Bring contextual help
// ----------------------------------------------------------------------------
{
    utf8   topic  = utf8("Overview");
    size_t length = 0;

    if (rt.depth())
    {
        if (!rt.args(1))
            return ERROR;
        if (object_p top = rt.top())
        {
            if (text_p index = top->as<text>())
            {
                utf8 what = index->value(&length);
                if (length)
                    topic = what;
            }
            else if (symbol_p index = top->as<symbol>())
            {
                topic = index->value(&length);
            }
            else
            {
                topic = top->help();
            }
        }
    }
    else
    {
        if (!rt.args(0))
            return ERROR;
    }

    ui.load_help(topic, length);
    return OK;
}


COMMAND_BODY(ToolsMenu)
// ----------------------------------------------------------------------------
//   Contextual tool menu
// ----------------------------------------------------------------------------
{
    id menu = ID_MainMenu;

    if (rt.editing())
    {
        switch(ui.editing_mode())
        {
        case ui.DIRECT:                 menu = ID_MathMenu; break;
        case ui.TEXT:                   menu = ID_TextMenu; break;
        case ui.PROGRAM:                menu = ID_ProgramMenu; break;
        case ui.ALGEBRAIC:              menu = ID_EquationsMenu; break;
        case ui.MATRIX:                 menu = ID_MatrixMenu; break;
        case ui.BASED:                  menu = ID_BasesMenu; break;
        default:
        case ui.STACK:                  break;
        }
    }
    else if (rt.depth())
    {
        if (!rt.args(1))
            return ERROR;
        if (object_p top = rt.top())
        {
            switch(top->type())
            {
            case ID_integer:
            case ID_neg_integer:
            case ID_bignum:
            case ID_neg_bignum:
            case ID_decimal128:
            case ID_decimal64:
            case ID_decimal32:          menu = ID_RealMenu; break;
            case ID_fraction:
            case ID_neg_fraction:
            case ID_big_fraction:
            case ID_neg_big_fraction:   menu = ID_FractionsMenu; break;
            case ID_polar:
            case ID_rectangular:        menu = ID_ComplexMenu; break;
#if CONFIG_FIXED_BASED_OBJECTS
            case ID_hex_integer:
            case ID_dec_integer:
            case ID_oct_integer:
            case ID_bin_integer:
            case ID_hex_bignum:
            case ID_dec_bignum:
            case ID_oct_bignum:
            case ID_bin_bignum:
#endif // CONFIG_FIXED_BASED_OBJECTS
            case ID_based_integer:
            case ID_based_bignum:       menu = ID_BasesMenu; break;
            case ID_expression:           menu = ID_SymbolicMenu; break;
            case ID_list:               menu = ID_ListMenu; break;
            case ID_array:              menu = ID_MatrixMenu; break;
            case ID_tag:                menu = ID_ObjectMenu; break;
            case ID_unit:               menu = ID_UnitsConversionsMenu; break;
            default:                    break;
            }
        }
    }
    else if (!rt.args(0))
    {
        return ERROR;
    }

    object_p obj = command::static_object(menu);
    return obj->execute();
}


COMMAND_BODY(Cycle)
// ----------------------------------------------------------------------------
//  Cycle object across multiple representations
// ----------------------------------------------------------------------------
{
    if (!rt.args(1))
        return ERROR;
    if (object_p top = rt.top())
    {
        id     cmd   = ID_object;
        id     type  = ID_object;
        id     ttype = top->type();
        switch(ttype)
        {
        case ID_decimal128:
        case ID_decimal64:
        case ID_decimal32:              cmd = ID_ToFraction;    break;
        case ID_integer:
        case ID_bignum:
        case ID_neg_integer:
        case ID_neg_bignum:
        case ID_fraction:
        case ID_neg_fraction:
        case ID_big_fraction:
        case ID_neg_big_fraction:       cmd = ID_ToDecimal;     break;
        case ID_polar:                  cmd = ID_ToRectangular; break;
        case ID_rectangular:            cmd = ID_ToPolar;       break;
#if CONFIG_FIXED_BASED_OBJECTS
        case ID_based_integer:          type = ID_hex_integer;  break;
        case ID_hex_integer:            type = ID_dec_integer;  break;
        case ID_dec_integer:            type = ID_oct_integer;  break;
        case ID_oct_integer:            type = ID_bin_integer;  break;
        case ID_bin_integer:            type = ID_based_integer;break;

        case ID_based_bignum:           type = ID_hex_bignum;   break;
        case ID_hex_bignum:             type = ID_dec_bignum;   break;
        case ID_dec_bignum:             type = ID_oct_bignum;   break;
        case ID_oct_bignum:             type = ID_bin_bignum;   break;
        case ID_bin_bignum:             type = ID_based_bignum; break;
#else // ! CONFIG_FIXED_BASED_OBJECTS
        case ID_based_integer:
        case ID_based_bignum:
            switch(Settings.base)
            {
            default:
            case 2:                     Settings.base = 16;     return OK;
            case 8:                     Settings.base = 2;      return OK;
            case 10:                    Settings.base = 8;      return OK;
            case 16:                    Settings.base = 10;     return OK;
            }
            break;
#endif // CONFIG_FIXED_BASED_OBJECTS
        case ID_expression:
            Settings.graph_stack = !Settings.graph_stack;
            break;
        case ID_list:                   type = ID_array;        break;
        case ID_array:                  type = ID_program;      break;
        case ID_program:                type = ID_list;         break;
        case ID_symbol:                 type = ID_text;         break;
        case ID_text:                   type = ID_symbol;       break;
        case ID_tag:                    cmd  = ID_dtag;         break;
        case ID_unit:
        {
            // Cycle prefix
            unit_p uobj = unit_p(top);
            uobj = uobj->cycle();
            if (uobj && rt.top(uobj))
                return OK;
            return ERROR;
        }

        default:
            rt.type_error();
            return ERROR;
        }

        // In-place retyping
        if (type != ID_object)
        {
            ASSERT(leb128size(type) == leb128size(top->type()));
            if (object_p clone = rt.clone(top))
            {
                byte *p = (byte *) clone;
                leb128(p, type);
                if (rt.top(clone))
                    return OK;
            }
            return ERROR;
        }

        // Evaluation of a command
        if (cmd != ID_object)
            return command::static_object(cmd)->evaluate();

        return OK;
    }
    return ERROR;
}


COMMAND_BODY(BinaryToReal)
// ----------------------------------------------------------------------------
//    Convert binary values to real (really integer)
// ----------------------------------------------------------------------------
{
    if (!rt.args(1))
        return ERROR;
    if (object_p top = rt.top())
    {
        id type = top->type();
        id to = ID_object;

        switch (type)
        {
#if CONFIG_FIXED_BASED_OBJECTS
        case ID_hex_integer:
        case ID_dec_integer:
        case ID_oct_integer:
        case ID_bin_integer:
            to = ID_integer;
            break;
        case ID_hex_bignum:
        case ID_dec_bignum:
        case ID_oct_bignum:
        case ID_bin_bignum:
            to = ID_bignum;
            break;
#endif // CONFIG_FIXED_BASED_OBJECTS
        case ID_based_integer:
            to = ID_integer;
            break;
        case ID_based_bignum:
            to = ID_bignum;
            break;
        default:
            rt.type_error();
            return ERROR;
        }

        ASSERT(leb128size(type) == leb128size(to));
        if (object_p clone = rt.clone(top))
        {
            byte *p = (byte *) clone;
            leb128(p, to);
            if (rt.top(clone))
                return OK;
        }
        return ERROR;

    }
    return ERROR;
}


COMMAND_BODY(RealToBinary)
// ----------------------------------------------------------------------------
//    Convert real and integer values to binary
// ----------------------------------------------------------------------------
{
    if (!rt.args(1))
        return ERROR;
    if (object_p top = rt.top())
    {
        id type = top->type();
        id to = ID_object;

        switch (type)
        {
        case ID_neg_integer:
        case ID_neg_bignum:
        case ID_neg_fraction:
        case ID_neg_big_fraction:
            rt.domain_error();
            return ERROR;
        case ID_integer:
            to = ID_based_integer;
            break;
        case ID_bignum:
            to = ID_based_bignum;
            break;
        case ID_fraction:
        case ID_big_fraction:
        case ID_decimal128:
        case ID_decimal64:
        case ID_decimal32:
            rt.unimplemented_error();
            return ERROR;
        default:
            rt.type_error();
            return ERROR;
        }

        ASSERT(leb128size(type) == leb128size(to));
        if (object_p clone = rt.clone(top))
        {
            byte *p = (byte *) clone;
            leb128(p, to);
            if (rt.top(clone))
                return OK;
        }
        return ERROR;

    }
    return ERROR;
}


COMMAND_BODY(LastMenu)
// ----------------------------------------------------------------------------
//   Go back one entry in the menu history
// ----------------------------------------------------------------------------
{
    if (!rt.args(0))
        return ERROR;
    ui.menu_pop();
    return OK;
}


COMMAND_BODY(LastArg)
// ----------------------------------------------------------------------------
//   Return the last arguments
// ----------------------------------------------------------------------------
{
    return rt.last() ? OK : ERROR;
}


COMMAND_BODY(LastX)
// ----------------------------------------------------------------------------
//   Return the last first argument
// ----------------------------------------------------------------------------
{
    return rt.last(0) ? OK : ERROR;
}


COMMAND_BODY(Undo)
// ----------------------------------------------------------------------------
//   Return the undo stack
// ----------------------------------------------------------------------------
{
    return rt.undo() ? OK : ERROR;
}


COMMAND_BODY(EditorSelect)
// ----------------------------------------------------------------------------
//   Select current cursor position
// ----------------------------------------------------------------------------
{
    return ui.editor_select() ? OK : ERROR;
}


COMMAND_BODY(EditorWordLeft)
// ----------------------------------------------------------------------------
//   Move cursor one word to the left
// ----------------------------------------------------------------------------
{
    return ui.editor_word_left() ? OK : ERROR;
}


COMMAND_BODY(EditorWordRight)
// ----------------------------------------------------------------------------
//   Move cursor one word to the right
// ----------------------------------------------------------------------------
{
    return ui.editor_word_right() ? OK : ERROR;
}


COMMAND_BODY(EditorBegin)
// ----------------------------------------------------------------------------
//   Move cursor to beginning of editor
// ----------------------------------------------------------------------------
{
    return ui.editor_begin() ? OK : ERROR;
}


COMMAND_BODY(EditorEnd)
// ----------------------------------------------------------------------------
//   Move cursor to end of editor
// ----------------------------------------------------------------------------
{
    return ui.editor_end() ? OK : ERROR;
}


COMMAND_BODY(EditorCut)
// ----------------------------------------------------------------------------
//   Cut selection
// ----------------------------------------------------------------------------
{
    return ui.editor_cut() ? OK : ERROR;
}


COMMAND_BODY(EditorCopy)
// ----------------------------------------------------------------------------
//   Copy selection
// ----------------------------------------------------------------------------
{
    return ui.editor_copy() ? OK : ERROR;
}


COMMAND_BODY(EditorPaste)
// ----------------------------------------------------------------------------
//   Paste selection
// ----------------------------------------------------------------------------
{
    return ui.editor_paste() ? OK : ERROR;
}


COMMAND_BODY(EditorSearch)
// ----------------------------------------------------------------------------
//   Search selection
// ----------------------------------------------------------------------------
{
    return ui.editor_search() ? OK : ERROR;
}


COMMAND_BODY(EditorReplace)
// ----------------------------------------------------------------------------
//   Replace selection
// ----------------------------------------------------------------------------
{
    return ui.editor_replace() ? OK : ERROR;
}


COMMAND_BODY(EditorClear)
// ----------------------------------------------------------------------------
//   Clear selection
// ----------------------------------------------------------------------------
{
    return ui.editor_clear() ? OK : ERROR;
}


COMMAND_BODY(EditorFlip)
// ----------------------------------------------------------------------------
//   Flip selection point and cursor
// ----------------------------------------------------------------------------
{
    return ui.editor_selection_flip() ? OK : ERROR;
}
