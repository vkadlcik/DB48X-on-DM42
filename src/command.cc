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

#include "input.h"
#include "integer.h"
#include "parser.h"
#include "renderer.h"
#include "runtime.h"
#include "settings.h"
#include "utf8.h"

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
        rt.error("Command is not implemented");
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
    id      found  = id(0);
    cstring ref    = cstring(utf8(p.source));
    size_t  maxlen = p.length;
    size_t  len    = maxlen;

    cstring names[3] = { nullptr };
    names[0] = cstring(object::fancy(i));
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
                && (len >= maxlen || is_separator(utf8(ref + len))))
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
    id     ty     = type();
    size_t result = 0;
    if (ty < NUM_IDS)
    {
        char              *target = r.target;
        size_t             len    = r.length;
        settings::commands fmt    = Settings.command_fmt;
        result = snprintf(target, len, "%s",
                          fmt == settings::commands::LONG_FORM
                          ? fancy_name[ty]
                          : id_name[ty]);
        switch(Settings.command_fmt)
        {
        case settings::commands::LOWERCASE:
            for (char *p = r.target; *p; p++)
                *p = tolower(*p);
            break;

        case settings::commands::UPPERCASE:
            for (char *p = r.target; *p; p++)
                *p = toupper(*p);
            break;

        case settings::commands::CAPITALIZED:
            *target = toupper(*target);
            break;

        default:
            break;
        }
    }
    record(command, "Render %u as [%s]", ty, (cstring) r.target);
    return result;
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
    static utf8 separators = utf8(" ;,.'\"<=>≤≠≥[](){}«»");
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
