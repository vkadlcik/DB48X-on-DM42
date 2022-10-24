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
#include "parser.h"
#include "renderer.h"
#include "runtime.h"
#include "settings.h"

#include <stdio.h>
#include <ctype.h>

RECORDER(command,       16, "RPL Commands");
RECORDER(command_error, 16, "Errors processing a command");


const cstring command::command_name[NUM_IDS] =
// ----------------------------------------------------------------------------
//   The long name of all objects or commands
// ----------------------------------------------------------------------------
{
#define ID(id)                  nullptr,
#define OP(id, name)            name,
#define ALIASED_OP(id, n1, n2)  n2,
#define CMD(id)                 #id,
#include <ids.tbl>
};


const cstring command::long_name[NUM_IDS] =
// ----------------------------------------------------------------------------
//   The long name of all objects or commands
// ----------------------------------------------------------------------------
{
#define ID(id)                  nullptr,
#define CMD(id)                 #id,
#define OP(id, name)            name,
#define ALIASED_OP(id, n1, n2)  n1,
#define NAMED(id, longname)     #longname,
#include <ids.tbl>
};


OBJECT_HANDLER_BODY(command)
// ----------------------------------------------------------------------------
//    RPL handler for commands
// ----------------------------------------------------------------------------
{
    record(command, "Command %+s on %p", object::name(op), obj);
    switch(op)
    {
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
    record(command, "Parsing [%s]", p.source);

    id     found = id(0);
    utf8   name  = p.source;
    size_t len   = p.length;

    // Loop on all IDs, skipping object
    for (int i = 1; i < NUM_IDS && !found; i++)
    {
        if (cstring cmd = command_name[i])
        {
            if (strncasecmp(cstring(name), cmd, len) == 0)
            {
                found = id(i);
                len = strlen(cmd);
            }
        }
        if (cstring cmd = long_name[i])
        {
            if (strncasecmp(cstring(name), cmd, len) == 0)
            {
                found = id(i);
                len = strlen(cmd);
            }
        }
    }

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
                          ? long_name[ty]
                          : command_name[ty]);
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
