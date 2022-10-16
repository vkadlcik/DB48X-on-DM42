#ifndef RUNTIME_H
#define RUNTIME_H
// ****************************************************************************
//  runtime.h                                                     DB48X project
// ****************************************************************************
//
//   File Description:
//
//     The basic RPL runtime
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

#include "recorder.h"
#include "types.h"

#include <cstdio>
#include <cstring>


struct object;                  // RPL object
struct global;                  // RPL global variable

RECORDER_DECLARE(runtime);
RECORDER_DECLARE(errors);
RECORDER_DECLARE(gc);
RECORDER_DECLARE(editor);

struct runtime
// ----------------------------------------------------------------------------
//   The RPL runtime information
// ----------------------------------------------------------------------------
//   Layout in memory is as follows
//
//      HighMem         End of usable memory
//      Returns         Top of return stack
//      StackBottom     Bottom of stack
//      StackTop        Top of stack
//        ...
//      Editor          The text editor
//      Temporaries     Temporaries, allocated down
//      Globals         Global named RPL objects
//      LowMem          Bottom of memory
//
//   When allocating a temporary, we move 'Temporaries' up
//   When allocating stuff on the stack, we move StackTop down
//   Everything above StackTop is word-aligned
//   Everything below Temporaries is byte-aligned
//   Stack elements point to temporaries, globals or robjects (read-only)
{
    runtime(byte *mem = nullptr, size_t size = 0)
        : Error(nullptr),
          ErrorSource(nullptr),
          ErrorCommand(nullptr),
          Code(nullptr),
          LowMem(),
          Globals(),
          Temporaries(),
          Editing(),
          StackTop(),
          StackBottom(),
          Returns(),
          HighMem(),
          GCSafe(nullptr)
    {
        memory(mem, size);
    }
    ~runtime() {}

    void memory(byte *memory, size_t size)
    {
        LowMem = (object *) memory;
        HighMem = (object *) (memory + size);
        Returns = (object **) HighMem;
        StackBottom = (object **) Returns;
        StackTop = (object **) StackBottom;
        Editing = 0;
        Temporaries = (object *) LowMem;
        Globals = (global *) Temporaries;
        record(runtime, "Memory %p-%p size %u (%uK)",
               LowMem, HighMem, size, size>>10);
    }

    // Amount of space we want to keep between stack top and temporaries
    const uint redzone = 8;



    // ========================================================================
    //
    //    Temporaries
    //
    // ========================================================================

    size_t available()
    // ------------------------------------------------------------------------
    //   Return the size available for temporaries
    // ------------------------------------------------------------------------
    {
        return (byte *) StackTop - (byte *) Temporaries - Editing - redzone;
    }

    size_t available(size_t size)
    // ------------------------------------------------------------------------
    //   Check if we have enough for the given size
    // ------------------------------------------------------------------------
    {
        if (available() < size)
        {
            gc();
            size_t avail = available();
            if (avail < size)
                error("Out of memory");
            return avail;
        }
        return size;
    }

    template <typename Obj, typename ... Args>
    Obj *make(typename Obj::id type, const Args &... args)
    // ------------------------------------------------------------------------
    //   Make a new temporary of the given size
    // ------------------------------------------------------------------------
    {
        // Find required memory for this object
        size_t size = Obj::required_memory(type, args...);

        // Check if we have room
        if (available(size) < size)
            return nullptr;    // Failed to allocate
        Obj *result = (Obj *) Temporaries;
        Temporaries = (object *) ((byte *) Temporaries + size);

        // Initialize the object in place
        new(result) Obj(args..., type);

        // Return initialized object
        return result;
    }

    template <typename Obj, typename ... Args>
    Obj *make(const Args &... args)
    // ------------------------------------------------------------------------
    //   Make a new temporary of the given size
    // ------------------------------------------------------------------------
    {
        // Find the required type for this object
        typename Obj::id type = Obj::static_type();
        return make(type, args...);
    }

    void dispose(object *object)
    // ------------------------------------------------------------------------
    //   Dispose of a temporary (must not be referenced elsewhere)
    // ------------------------------------------------------------------------
    {
        if (skip(object) == Temporaries)
            Temporaries = object;
        else
            unused(object);
    }


    // ========================================================================
    //
    //    Command-line editor (and buffer for renderer)x
    //
    // ========================================================================

    char *editor()
    // ------------------------------------------------------------------------
    //   Return the buffer for the editor
    // ------------------------------------------------------------------------
    //   This must be called each time a GC could have happened
    {
        char *ed = (char *) Temporaries;
        return ed;
    }


    void edit(cstring buffer, size_t len)
    // ------------------------------------------------------------------------
    //   Open the editor with a known buffer
    // ------------------------------------------------------------------------
    {
        if (available(len) < len)
        {
            record(editor, "Insufficent memory for %u bytes", len);
            error("Out of memory", "Editor");
            return;
        }

        memcpy((byte *) Temporaries, buffer, len);
        Editing = len;
    }


    char *close_editor();
    // ------------------------------------------------------------------------
    //   Close the editor and encapsulate its content in a temporary string
    // ------------------------------------------------------------------------


    size_t editing()
    // ------------------------------------------------------------------------
    //   Current size of the editing buffer
    // ------------------------------------------------------------------------
    {
        return Editing;
    }


    void clear()
    // ------------------------------------------------------------------------
    //   Clear the editor
    // ------------------------------------------------------------------------
    {
        Editing = 0;
    }


    void insert(size_t offset, cstring data, size_t len)
    // ------------------------------------------------------------------------
    //   Insert data in the editor
    // ------------------------------------------------------------------------
    {
        if (offset <= Editing)
        {
            if (available(len) >= len)
            {
                size_t moved = Editing - offset;
                memmove(editor() + offset + len, editor() + offset, moved);
                memcpy(editor() + offset, data, len);
                Editing += len;
            }
        }
        else
        {
            fprintf(stderr, "Invalid insert at %zu size=%zu len=%zu [%.*s]\n",
                    offset, Editing, len, (int) len, data);
        }
    }


    void insert(size_t offset, char c)
    // ------------------------------------------------------------------------
    //   Insert a single character in the editor
    // ------------------------------------------------------------------------
    {
        insert(offset, &c, 1);
    }


    void remove(size_t offset, size_t len)
    // ------------------------------------------------------------------------
    //   Remove characers from the editor
    // ------------------------------------------------------------------------
    {
        size_t end = offset + len;
        if (end > Editing)
            end = Editing;
        if (offset > end)
            offset = end;
        len = end - offset;
        memmove(editor() + offset, editor() + offset + len, Editing - end);
        Editing -= len;
    }



    // ========================================================================
    //
    //   Stack
    //
    // ========================================================================

    void push(object *obj)
    // ------------------------------------------------------------------------
    //   Push an object on top of RPL stack
    // ------------------------------------------------------------------------
    {
        if (available(sizeof(obj)) < sizeof(obj))
            return;
        *(--StackTop) = obj;
    }

    object *top()
    // ------------------------------------------------------------------------
    //   Return the top of the runtime stack
    // ------------------------------------------------------------------------
    {
        return StackTop < StackBottom ? *StackTop : nullptr;
    }

    void top(object *obj)
    // ------------------------------------------------------------------------
    //   Set the top of the runtime stack
    // ------------------------------------------------------------------------
    {
        if (StackTop >= StackBottom)
            error("Cannot replace empty stack");
        else
            *StackTop = obj;
    }

    object *pop()
    // ------------------------------------------------------------------------
    //   Pop the top-level object from the stack, or return NULL
    // ------------------------------------------------------------------------
    {
        if (StackTop >= StackBottom)
            return error("Not enough arguments"), nullptr;
        return *StackTop++;
    }

    object *stack(uint idx)
    // ------------------------------------------------------------------------
    //    Get the object at a given position in the stack
    // ------------------------------------------------------------------------
    {
        if (idx >= depth())
            return error("Insufficient stack depth"), nullptr;
        return StackTop[idx];
    }

    void stack(uint idx, object *obj)
    // ------------------------------------------------------------------------
    //    Get the object at a given position in the stack
    // ------------------------------------------------------------------------
    {
        if (idx >= depth())
            error("Insufficient stack depth");
        else
            StackTop[idx] = obj;
    }

    uint depth()
    // ------------------------------------------------------------------------
    //   Return the stack depth
    // ------------------------------------------------------------------------
    {
        return StackBottom - StackTop;
    }



    // ========================================================================
    //
    //   Return stack
    //
    // ========================================================================

    void call(object *callee)
    // ------------------------------------------------------------------------
    //   Push the current object on the RPL stack
    // ------------------------------------------------------------------------
    {
        if (available(sizeof(callee)) < sizeof(callee))
        {
            error("Too many recursive calls");
            return;
        }
        StackTop--;
        StackBottom--;
        for (object **s = StackBottom; s < StackTop; s++)
            s[0] = s[1];
        *(--Returns) = Code;
        Code = callee;
    }

    void ret()
    // ------------------------------------------------------------------------
    //   Return from an RPL call
    // ------------------------------------------------------------------------
    {
        if ((byte *) Returns >= (byte *) HighMem)
        {
            error("Cannot return without a caller");
            return;
        }
        Code = *Returns++;
        StackTop++;
        StackBottom++;
        for (object **s = StackTop; s > StackTop; s--)
            s[0] = s[-1];
    }


    // ========================================================================
    //
    //   Object management
    //
    // ========================================================================

    size_t  gc();
    void    unused(object *obj, object *next);
    void    unused(object *obj)
    // ------------------------------------------------------------------------
    //   Mark an object unused knowing its end
    // ------------------------------------------------------------------------
    {
        unused(obj, skip(obj));
    }

    size_t  size(object *obj);
    object *skip(object *obj)
    // ------------------------------------------------------------------------
    //   Skip an RPL object
    // ------------------------------------------------------------------------
    {
        return (object *) ((byte *) obj + size(obj));
    }


    struct gcptr
    // ------------------------------------------------------------------------
    //   Protect a pointer against garbage collection
    // ------------------------------------------------------------------------
    {
        gcptr(byte *ptr) : safe(ptr), next(RT.GCSafe)
        {
            RT.GCSafe = this;
        }
        gcptr(const gcptr &o) = delete;
        ~gcptr()
        {
            gcptr *last = nullptr;
            for (gcptr *gc = RT.GCSafe; gc; gc = gc->next)
            {
                if (gc == this)
                {
                    if (last)
                        last->next = gc->next;
                    else
                        RT.GCSafe = gc->next;
                    break;
                }
                last = gc;
            }
        }

        operator byte  *() const   { return safe; }
        operator byte *&()         { return safe; }

    private:
        byte  *safe;
        gcptr *next;

        friend struct runtime;
    };


    template<typename Obj>
    struct gcp : gcptr
    // ------------------------------------------------------------------------
    //   Protect a pointer against garbage collection
    // ------------------------------------------------------------------------
    {
        gcp(Obj *obj): gcptr((byte *) obj) {}
        ~gcp() {}

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wstrict-aliasing"
        operator Obj *() const  { return (Obj *) safe; }
        operator Obj *&()       { return (Obj *&) safe; }
#pragma GCC diagnostic pop
    };



    // ========================================================================
    //
    //   Error handling
    //
    // ========================================================================

    runtime &error(cstring message, cstring source = nullptr)
    // ------------------------------------------------------------------------
    //   Set the error message
    // ------------------------------------------------------------------------
    {
        record(errors, "Error [%s] at source [%s]", message, source);
        Error = message;
        ErrorSource = source;
        return *this;
    }

    runtime &error(cstring message, byte_p source)
    // ------------------------------------------------------------------------
    //   Set the error message
    // ------------------------------------------------------------------------
    {
        return error(message, (cstring) source);
    }

    cstring error()
    // ------------------------------------------------------------------------
    //   Get the error message
    // ------------------------------------------------------------------------
    {
        return Error;
    }

    cstring source()
    // ------------------------------------------------------------------------
    //   Get the pointer to the problem
    // ------------------------------------------------------------------------
    {
        return ErrorSource;
    }

    runtime &command(cstring cmd)
    // ------------------------------------------------------------------------
    //   Set the faulting command
    // ------------------------------------------------------------------------
    {
        ErrorCommand = cmd;
        return *this;
    }

    cstring command()
    // ------------------------------------------------------------------------
    //   Get the faulting command if there is one
    // ------------------------------------------------------------------------
    {
        return ErrorCommand;
    }


protected:
    cstring  Error;             // Error message if any
    cstring  ErrorSource;       // Source of the error if known
    cstring  ErrorCommand;      // Source of the error if known
    object  *Code;              // Currently executing code
    object  *LowMem;
    global  *Globals;
    object  *Temporaries;
    size_t   Editing;
    object **StackTop;
    object **StackBottom;
    object **Returns;
    object  *HighMem;

    // Pointers that are GC-adjusted
    gcptr   *GCSafe;

public:
    // The one and only runtime
    static runtime RT;
};


template<typename T>
using gcp = runtime::gcp<T>;

using gcstring = gcp<const char>;
using gcmstring = gcp<char>;



// ============================================================================
//
//    Allocate objects
//
// ============================================================================

template <typename Obj, typename ...Args>
inline Obj *make(Args &... args)
// ----------------------------------------------------------------------------
//    Create an object in the runtime
// ----------------------------------------------------------------------------
{
    return runtime::RT.make<Obj>(args...);
}


template <typename Obj>
inline void *operator new(size_t UNUSED size, Obj *where)
// ----------------------------------------------------------------------------
//    Placement new for objects
// ----------------------------------------------------------------------------
{
    return where;
}

#endif // RUNTIME_H
