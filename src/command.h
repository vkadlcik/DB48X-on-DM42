#ifndef COMMAND_H
#define COMMAND_H
// ****************************************************************************
//  command.h                                                     DB48X project
// ****************************************************************************
//
//   File Description:
//
//     Description of an RPL command
//
//     All RPL commands take input on the stack and emit results on the stack
//     There are facilities for type checking the stack inputs
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
//
//     Unlike traditional RPL, commands are case-insensitive, i.e. you can
//     use either "DUP" or "dup". There is a setting to display them as upper
//     or lowercase. The reason is that on the DM42, lowercases look good.
//
//     Additionally, many commands also have a long form. There is also an
//     option to prefer displaying as long form. This does not impact encoding,
//     and when typing programs, you can always use the short form

#include "object.h"
#include "runtime.h"
#include "list.h"

struct command : object
// ----------------------------------------------------------------------------
//   Shared logic for all commands
// ----------------------------------------------------------------------------
{
    command(id i): object(i) {}

    template<typename Obj>
    const Obj *arg(uint level = 0, Obj *def = nullptr)
    // ------------------------------------------------------------------------
    //   Return the arg at a given level on the stack, or default value
    // ------------------------------------------------------------------------
    {
        const Obj *obj = RT.stack(level);
        if (obj && obj->type() == Obj::static_type())
            return (Obj *) obj;
        return def;
    }

    // Overload command to adjust based on settings
    cstring name();

    // Get a static command pointer for a given command
    static object_p static_object(id i);

    // Check if a code point or text is a separator
    static bool is_separator(unicode code);
    static bool is_separator(utf8 str);
    static bool is_separator_or_digit(unicode code);
    static bool is_separator_or_digit(utf8 str);

    // Standard object interface
    OBJECT_PARSER(command);
    OBJECT_RENDERER(command);
    OBJECT_HANDLER_NO_ID(command);

};


// Macro to defined a simple command handler for derived classes
#define COMMAND_DECLARE(derived)                                \
    struct derived : command                                    \
    {                                                           \
        derived(id i = ID_##derived) : command(i) { }           \
                                                                \
        OBJECT_HANDLER(derived)                                 \
        {                                                       \
            if (op == EVAL || op == EXEC)                       \
            {                                                   \
                RT.command(fancy(ID_##derived));                \
                return ((derived *) obj)->evaluate();           \
            }                                                   \
            return DELEGATE(command);                           \
        }                                                       \
        static result evaluate();                               \
    }

#define COMMAND_BODY(derived)                   \
    object::result derived::evaluate()

#define COMMAND(derived)                        \
    COMMAND_DECLARE(derived);                   \
    inline COMMAND_BODY(derived)



// ============================================================================
//
//    Some basic commands
//
// ============================================================================

struct Unimplemented : command
// ----------------------------------------------------------------------------
//   Used for unimplemented commands, e.g. in menus
// ----------------------------------------------------------------------------
{
    Unimplemented(id i = ID_Unimplemented) : command(i) { }
    OBJECT_HANDLER(Unimplemented)
    {
        if (op == EVAL || op == EXEC)
        {
            RT.error("Not yet implemented");
            return ERROR;
        }
        if (op == MENU_MARKER)
            return L'░';

        return DELEGATE(command);
    }
};


COMMAND(Eval)
// ----------------------------------------------------------------------------
//   Evaluate an object
// ----------------------------------------------------------------------------
{
    object_p x = RT.pop();
    return x->execute();
    return ERROR;
}

COMMAND_DECLARE(SelfInsert);
// ----------------------------------------------------------------------------
//   Insert the label associated to a menu
// ----------------------------------------------------------------------------

COMMAND_DECLARE(Ticks);
// ----------------------------------------------------------------------------
//   Measure number of milliseconds
// ----------------------------------------------------------------------------


#endif // COMMAND_H
