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
        const Obj *obj = rt.stack(level);
        if (obj && obj->type() == Obj::static_id)
            return (Obj *) obj;
        return def;
    }

    // Get the top of the stack as an integer
    static uint32_t uint32_arg(uint level = 0);
    static int32_t  int32_arg (uint level = 0);

    // Execute a command
    static result evaluate()    { return OK; }

public:
    PARSE_DECL(command);
    RENDER_DECL(command);
};


// Macro to defined a simple command handler for derived classes
#define COMMAND_DECLARE_SPECIAL(derived, base, special) \
struct derived : base                                   \
{                                                       \
    derived(id i = ID_##derived) : base(i) { }          \
                                                        \
    OBJECT_DECL(derived);                               \
    EVAL_DECL(derived)                                  \
    {                                                   \
        rt.command(o);                                  \
        return evaluate();                              \
    }                                                   \
    special                                             \
    static result evaluate();                           \
}

#define COMMAND_DECLARE(derived)                        \
    COMMAND_DECLARE_SPECIAL(derived, command, )

#define COMMAND_DECLARE_INSERT(derived)                 \
    COMMAND_DECLARE_SPECIAL(derived, command,           \
                            INSERT_DECL(derived);)

#define COMMAND_BODY(derived)                           \
    object::result derived::evaluate()

#define COMMAND(derived)                                \
    COMMAND_DECLARE(derived);                           \
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

    OBJECT_DECL(Unimplemented);
    EVAL_DECL(Unimplemented);
    MARKER_DECL(Unimplemented);
};

// Various global commands
COMMAND_DECLARE(Eval);          // Evaluate an object
COMMAND_DECLARE(Compile);       // Compile and evalaute a text
COMMAND_DECLARE(Explode);       // Explode an object (aka Obj→)
COMMAND_DECLARE(ToText);        // Convert an object to text
COMMAND_DECLARE(SelfInsert);    // Enter menu label in the editor
COMMAND_DECLARE(ReplaceChar);   // Replace character with label in editor
COMMAND_DECLARE(Ticks);         // Return number of ticks
COMMAND_DECLARE(Wait);          // Wait a given amount of time
COMMAND_DECLARE(Bytes);         // Return the bytes representation of object
COMMAND_DECLARE(Type);          // Return the type of the object
COMMAND_DECLARE(TypeName);      // Return the type name of the object
COMMAND_DECLARE(Off);           // Switch the calculator off
COMMAND_DECLARE(SaveState);     // Save state to disk
COMMAND_DECLARE(SystemSetup);   // Select the system menu
COMMAND_DECLARE(ScreenCapture); // Snapshot screen state to a file
COMMAND_DECLARE(Beep);          // Emit a sound (if enabled)
COMMAND_DECLARE(Version);       // Return a version string
COMMAND_DECLARE(Help);          // Activate online help
COMMAND_DECLARE(LastArg);       // Return last arguments
COMMAND_DECLARE(LastX);         // Return last X argument
COMMAND_DECLARE(Undo);          // Revert to the Undo stack
COMMAND_DECLARE(Cycle);         // Cycle object across multiple representations
COMMAND_DECLARE(BinaryToReal);  // Convert binary to real
COMMAND_DECLARE(RealToBinary);  // Convert real to binary

COMMAND_DECLARE(EditorSelect);  // Select from current cursor position
COMMAND_DECLARE(EditorWordLeft); // Move cursor one word left
COMMAND_DECLARE(EditorWordRight); // Move cursor one word right
COMMAND_DECLARE(EditorBegin);   // Move cursor to beginning of buffer
COMMAND_DECLARE(EditorEnd);     // Move cursor to end of buffer
COMMAND_DECLARE(EditorCut);     // Cut current selection
COMMAND_DECLARE(EditorCopy);    // Copy current selection
COMMAND_DECLARE(EditorPaste);   // Paste to cursor position
COMMAND_DECLARE(EditorSearch);  // Begin search
COMMAND_DECLARE(EditorReplace); // Replace search with cursor
COMMAND_DECLARE(EditorClear);   // Clear editor
COMMAND_DECLARE(EditorFlip);    // Flip cursor and selection

#endif // COMMAND_H
