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

#include "decimal-32.h"
#include "decimal-64.h"
#include "decimal128.h"
#include "input.h"
#include "integer.h"
#include "parser.h"
#include "renderer.h"
#include "runtime.h"
#include "settings.h"
#include "symbol.h"
#include "utf8.h"
#include "version.h"

#include <ctype.h>
#include <stdio.h>

RECORDER(command,       16, "RPL Commands");
RECORDER(command_error, 16, "Errors processing a command");



OBJECT_HANDLER_BODY(command)
// ----------------------------------------------------------------------------
//    RPL handler for commands
// ----------------------------------------------------------------------------
{
    record(command, "Command %+s on %p", object::name(op), obj);
    switch(op)
    {
    case EXEC:
    case EVAL:
        record(command_error, "Invoked default command handler");
        rt.unimplemented_error();
        return ERROR;
    case SIZE:
        // Commands have no payload
        return ptrdiff(payload, obj);
    case PARSE:
        return object_parser(OBJECT_PARSER_ARG(), rt);
    case RENDER:
        return obj->object_renderer(OBJECT_RENDERER_ARG(), rt);
    case INSERT:
        return ((input *) arg)->edit(obj->fancy(), input::PROGRAM);
    case HELP:
        return (intptr_t) obj->fancy();

    default:
        // Check if anyone else knows how to deal with it
        return DELEGATE(object);
    }
}


OBJECT_PARSER_BODY(command)
// ----------------------------------------------------------------------------
//    Try to parse this as a command, using either short or long name
// ----------------------------------------------------------------------------
{
    id      i      = p.candidate;
    bool    eq     = p.precedence;

    // If we are parsing an equation, only accept algebraic command
    if (eq && (i < FIRST_ALGEBRAIC || i > LAST_ALGEBRAIC))
        return SKIP;

    id      found  = id(0);
    cstring ref    = cstring(utf8(p.source));
    size_t  maxlen = p.length;
    size_t  len    = maxlen;

    bool disambiguate = eq && (i == ID_sq || i == ID_cubed || i == ID_inv);

    cstring names[3] = { nullptr };
    names[0] = disambiguate ? nullptr : cstring(object::fancy(i));
    names[1] = cstring(object::name(i));
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
           ref, i, object::name(i), object::fancy(i), found, len);

    if (!found)
        return SKIP;

    // Record output - Dynamically generate ID for use in programs
    p.end = len;
    p.out = rt.make<command>(found);

    return OK;
}


OBJECT_RENDERER_BODY(command)
// ----------------------------------------------------------------------------
//   Render the command into the given string buffer
// ----------------------------------------------------------------------------
{
    id ty = type();
    if (ty < NUM_IDS)
    {
        auto format = Settings.command_fmt;

        // Ensure that we display + as `+` irrespective of mode
        utf8 fancy = utf8(fancy_name[ty]);
        if (!is_valid_as_name_initial(utf8_codepoint(fancy)))
            if (utf8_length(fancy) == 1)
                format = settings::commands::LONG_FORM;

        utf8 text = utf8(format == settings::commands::LONG_FORM
                         ? fancy_name[ty]
                         : id_name[ty]);
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


bool command::stack(uint32_t *result, uint level)
// ----------------------------------------------------------------------------
//   Get an unsigned value from the stack
// ----------------------------------------------------------------------------
{
    if (object_p d = RT.stack(level))
    {
        id type = d->type();
        switch(type)
        {
        case ID_integer:
            *result = integer_p(d)->value<uint32_t>();
            return true;
        case ID_neg_integer:
            *result = 0;
            return true;
        case ID_decimal128:
        {
            bid128 v = decimal128_p(d)->value();
            bid128_to_uint32_int((uint *) result, &v.value);
            return true;
        }
        case ID_decimal64:
        {
            bid64 v = decimal64_p(d)->value();
            bid64_to_uint32_int((uint *) result, &v.value);
            return true;
        }
        case ID_decimal32:
        {
            bid32 v = decimal32_p(d)->value();
            bid32_to_uint32_int((uint *) result, &v.value);
            return true;
        }
        default:
            RT.type_error();
        }
    }
    return false;
}


bool command::stack(int32_t *result, uint level)
// ----------------------------------------------------------------------------
//   Get a signed value from the stack
// ----------------------------------------------------------------------------
{
    if (object_p d = RT.stack(level))
    {
        id type = d->type();
        switch(type)
        {
        case ID_integer:
            *result = integer_p(d)->value<uint32_t>();
            return true;
        case ID_neg_integer:
            *result = -integer_p(d)->value<uint32_t>();
            return true;
        case ID_decimal128:
        {
            bid128 v = decimal128_p(d)->value();
            bid128_to_int32_int((int *) result, &v.value);
            return true;
        }
        case ID_decimal64:
        {
            bid64 v = decimal64_p(d)->value();
            bid64_to_int32_int((int *) result, &v.value);
            return true;
        }
        case ID_decimal32:
        {
            bid32 v = decimal32_p(d)->value();
            bid32_to_int32_int((int *) result, &v.value);
            return true;
        }
        default:
            RT.type_error();
        }
    }
    return false;
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
    if (object_p x = RT.pop())
        return x->execute();
    return ERROR;
}

COMMAND_BODY(True)
// ----------------------------------------------------------------------------
//   Evaluate as self
// ----------------------------------------------------------------------------
{
    if (RT.push(command::static_object(ID_True)))
        return OK;
    return ERROR;
}

COMMAND_BODY(False)
// ----------------------------------------------------------------------------
//   Evaluate as self
// ----------------------------------------------------------------------------
{
    if (RT.push(command::static_object(ID_False)))
        return OK;
    return ERROR;
}


COMMAND_BODY(ToText)
// ----------------------------------------------------------------------------
//   Convert an object to text
// ----------------------------------------------------------------------------
{
    if (gcobj obj = RT.top())
        if (gcobj txt = obj->as_text(false, false))
            if (RT.top(txt))
                return OK;
    return ERROR;
}
COMMAND_BODY(SelfInsert)
// ----------------------------------------------------------------------------
//   Find the label associated to the menu and enter it in the editor
// ----------------------------------------------------------------------------
{
    int key = Input.evaluating;
    if (key >= KEY_F1 && key <= KEY_F6)
    {
        uint plane = Input.shift_plane();
        uint menu_idx = key - KEY_F1 + plane * input::NUM_SOFTKEYS;
        if (cstring lbl = Input.labelText(menu_idx))
            for (utf8 p = utf8(lbl); *p; p = utf8_next(p))
                Input.edit(utf8_codepoint(p), input::PROGRAM);
    }
    return OK;
}


COMMAND_BODY(Ticks)
// ----------------------------------------------------------------------------
//   Return number of ticks
// ----------------------------------------------------------------------------
{
    uint ticks = sys_current_ms();
    if (integer_p ti = RT.make<integer>(ID_integer, ticks))
        if (RT.push(ti))
            return OK;
    return ERROR;
}



COMMAND_BODY(Off)
// ----------------------------------------------------------------------------
//   Switch the calculator off
// ----------------------------------------------------------------------------
{
    extern void power_off();
    power_off();
    return OK;
}


COMMAND_BODY(SystemSetup)
// ----------------------------------------------------------------------------
//   Select the system menu
// ----------------------------------------------------------------------------
{
    extern void system_setup();
    system_setup();
    return OK;
}


COMMAND_BODY(HomeDirectory)
// ----------------------------------------------------------------------------
//   Return the home directory
// ----------------------------------------------------------------------------
{
    if (gcobj dir = (object *) RT.variables(0))
        if (RT.push(dir))
            return OK;
    return ERROR;
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
    if (text_g version = text::make(version_text))
        if (RT.push(object_p(version)))
            return OK;
    return ERROR;
}
