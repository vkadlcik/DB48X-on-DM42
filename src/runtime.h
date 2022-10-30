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
typedef const object *object_p;

RECORDER_DECLARE(runtime);
RECORDER_DECLARE(runtime_error);
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
//        [Pointer to object in local variable N in outermost program]
//        [ ... ]
//        [Pointer to object in local variable 1 in outermost program]
//        [Number of local variables above, may be 0]
//        [Pointer to next object to evaluate in outermost program]
//        [... the same as the above for inner objects being evaluated ...]
//        [Number of local variables in currently evaluating program, may be 0]
//      Returns         Top of return stack
//        [Pointer to outermost catalog in path]
//        [ ... intermediate catalog pointers ...]
//        [Pointer to innermost catalog in path]
//      StackBottom     Bottom of stack
//        [User stack]
//      StackTop        Top of stack
//        [Free, may be temporarily written prior to being put in scratch]
//      +Scratch        Binary scratch pad (to assemble objects like lists)
//        [Scratchpad allocated area]
//      Editor          The text editor
//        [Text editor contents]
//      Temporaries     Temporaries, allocated up
//        [Previously allocated temporary objects, can be garbage collected]
//      Globals         Global named RPL objects
//        [Top-level catalog of global objects]
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
          Scratch(),
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
        LowMem = (object_p) memory;
        HighMem = (object_p) (memory + size);
        Returns = (object_p*) HighMem;
        StackBottom = (object_p*) Returns;
        StackTop = (object_p*) StackBottom;
        Editing = 0;
        Temporaries = (object_p) LowMem;
        Globals = Temporaries;
        record(runtime, "Memory %p-%p size %u (%uK)",
               LowMem, HighMem, size, size>>10);
    }

    // Amount of space we want to keep between stack top and temporaries
    const uint redzone = 2*sizeof(object_p);;



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
        size_t aboveTemps = Editing + Scratch + redzone;
        return (byte *) StackTop - (byte *) Temporaries - aboveTemps;
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
        record(runtime,
               "Initializing object %p type %d size %u",
               Temporaries, type, size);

        // Check if we have room (may cause garbage collection)
        if (available(size) < size)
            return nullptr;    // Failed to allocate
        Obj *result = (Obj *) Temporaries;
        Temporaries = (object *) ((byte *) Temporaries + size);

        // Move the editor up (available() checked we have room)
        move(Temporaries, (object_p) result, Editing + Scratch, true);

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
        return make<Obj>(type, args...);
    }


    // ========================================================================
    //
    //    Command-line editor (and buffer for renderer)
    //
    // ========================================================================

    byte *editor()
    // ------------------------------------------------------------------------
    //   Return the buffer for the editor
    // ------------------------------------------------------------------------
    //   This must be called each time a GC could have happened
    {
        byte *ed = (byte *) Temporaries;
        return ed;
    }


    size_t edit(utf8 buffer, size_t len);
    // ------------------------------------------------------------------------
    //   Open the editor with a known buffer
    // ------------------------------------------------------------------------

    size_t edit();
    // ------------------------------------------------------------------------
    //   Append the scratch pad into the editor
    // ------------------------------------------------------------------------


    utf8 close_editor();
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


    size_t insert(size_t offset, utf8 data, size_t len)
    // ------------------------------------------------------------------------
    //   Insert data in the editor, return size inserted
    // ------------------------------------------------------------------------
    {
        record(editor,
               "Insert %u bytes at offset %u starting with %c, %u available",
               len, offset, data[0], available());
        if (offset <= Editing)
        {
            if (available(len) >= len)
            {
                size_t moved = Scratch + Editing - offset;
                byte_p edr = (byte_p) editor() + offset;
                move(object_p(edr + len), object_p(edr), moved);
                memcpy(editor() + offset, data, len);
                Editing += len;
                return len;
            }
        }
        else
        {
            record(runtime_error,
                   "Invalid insert at %zu size=%zu len=%zu [%s]\n",
                   offset, Editing, len, data);
        }
        return 0;
    }


    size_t insert(size_t offset, byte c)
    // ------------------------------------------------------------------------
    //   Insert a single character in the editor
    // ------------------------------------------------------------------------
    {
        return insert(offset, &c, 1);
    }


    size_t insert(size_t offset, utf8 data)
    // ------------------------------------------------------------------------
    //   Insert a null-terminated command name
    // ------------------------------------------------------------------------
    {
        return insert(offset, data, strlen(cstring(data)));
    }


    void remove(size_t offset, size_t len)
    // ------------------------------------------------------------------------
    //   Remove characers from the editor
    // ------------------------------------------------------------------------
    {
        record(editor, "Removing %u bytes at offset %u", len, offset);
        size_t end = offset + len;
        if (end > Editing)
            end = Editing;
        if (offset > end)
            offset = end;
        len = end - offset;
        size_t moving = Scratch + Editing - end;
        byte_p edr = (byte_p) editor() + offset;
        move(object_p(edr), object_p(edr + len), moving);
        Editing -= len;
    }



    // ========================================================================
    //
    //   Scratchpad
    //
    // ========================================================================
    //   The scratchpad is a temporary area to store binary data
    //   It is used for example while building complex or composite objects

    byte *scratchpad()
    // ------------------------------------------------------------------------
    //   Return the buffer for the scratchpad
    // ------------------------------------------------------------------------
    //   This must be called each time a GC could have happened
    {
        byte *scratch = (byte *) Temporaries + Editing;
        return scratch;
    }

    size_t allocated()
    // ------------------------------------------------------------------------
    //   Return the size of the temporary scratchpad
    // ------------------------------------------------------------------------
    {
        return Scratch;
    }

    byte *allocate(size_t sz)
    // ------------------------------------------------------------------------
    //   Allocate additional bytes at end of scratchpad
    // ------------------------------------------------------------------------
    {
        if (available(sz) >= sz)
        {
            byte *scratch = editor() + Editing + Scratch;
            Scratch += sz;
            return scratch;
        }

        // Ran out of memory despite garbage collection
        return nullptr;
    }


    byte *append(size_t sz, byte *bytes)
    // ------------------------------------------------------------------------
    //   Append some bytes at end of scratch pad
    // ------------------------------------------------------------------------
    {
        byte *ptr = allocate(sz);
        if (ptr)
            memcpy(ptr, bytes, sz);
        return ptr;
    }


    template<typename T>
    byte *append(const T& t)
    // ------------------------------------------------------------------------
    //   Append an object to the scratchpad
    // ------------------------------------------------------------------------
    {
        return append(sizeof(t), &t);
    }


    template<typename T, typename ...Args>
    byte *append(const T& t, Args... args)
    // ------------------------------------------------------------------------
    //   Append multiple objects to the scratchpad
    // ------------------------------------------------------------------------
    {
        byte *first = append(t);
        if (first && append(args...))
            return first;
        return nullptr;         // One of the allocations failed
    }


    template <typename Int>
    byte *encode(Int value)
    // ------------------------------------------------------------------------
    //   Add an LEB128-encoded value to the scratchpad
    // ------------------------------------------------------------------------
    {
        size_t sz = leb128size(value);
        byte *ptr = allocate(sz);
        if (ptr)
            leb128(ptr, value);
        return ptr;
    }


    template <typename Int, typename ...Args>
    byte *encode(Int value, Args... args)
    // ------------------------------------------------------------------------
    //   Add an LEB128-encoded value to the scratchpad
    // ------------------------------------------------------------------------
    {
        size_t sz = leb128size(value);
        byte *ptr = allocate(sz);
        if (ptr)
        {
            leb128(ptr, value);
            if (!encode(args...))
                return nullptr;
        }
        return ptr;
    }


    void free(size_t size)
    // ------------------------------------------------------------------------
    //   Free the whole scratchpad
    // ------------------------------------------------------------------------
    {
        if (Scratch >= size)
            Scratch -= size;
        else
            Scratch = 0;
    }



    // ========================================================================
    //
    //   Object management
    //
    // ========================================================================

    size_t gc();
    // ------------------------------------------------------------------------
    //   Garbage collector (purge unused objects from memory to make space)
    // ------------------------------------------------------------------------


    void move(object_p to, object_p from, size_t sz, bool scratch=false);
    // ------------------------------------------------------------------------
    //    Like memmove, but update pointers to objects
    // ------------------------------------------------------------------------


    void move_globals(object_p to, object_p from);
    // ------------------------------------------------------------------------
    //    Move data in the globals area (move everything up to end of scratch)
    // ------------------------------------------------------------------------


    size_t size(object_p obj);
    // ------------------------------------------------------------------------
    //   Query the size of an RPL object
    // ------------------------------------------------------------------------


    object_p skip(object_p obj)
    // ------------------------------------------------------------------------
    //   Skip an RPL object
    // ------------------------------------------------------------------------
    {
        return (object_p ) ((byte *) obj + size(obj));
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
        gcptr(const gcptr &o): safe(o.safe), next(RT.GCSafe)
        {
            RT.GCSafe = this;
        }
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

        operator byte  *() const                { return safe; }
        operator byte *&()                      { return safe; }
        operator bool()                         { return safe != nullptr; }
        gcptr &operator =(const gcptr &o)       { safe = o.safe; return *this; }
        gcptr &operator+=(size_t sz)            { safe += sz; return *this; }

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
        operator Obj *() const          { return (Obj *) safe; }
        operator Obj *&()               { return (Obj *&) safe; }
        Obj operator *() const          { return *((Obj *) safe); }
        Obj &operator *()               { return *((Obj *) safe); }
        Obj *operator ->() const        { return (Obj *) safe; }
#pragma GCC diagnostic pop
    };



    // ========================================================================
    //
    //   Stack
    //
    // ========================================================================

    void push(gcp<const object> obj)
    // ------------------------------------------------------------------------
    //   Push an object on top of RPL stack
    // ------------------------------------------------------------------------
    {
        // This may cause garbage collection, hence the need to adjust
        if (available(sizeof(obj)) < sizeof(obj))
            return;
        *(--StackTop) = obj;
    }

    object_p top()
    // ------------------------------------------------------------------------
    //   Return the top of the runtime stack
    // ------------------------------------------------------------------------
    {
        if (StackTop >= StackBottom)
        {
            error("Too few arguments");
            return nullptr;
        }
        return *StackTop;
    }

    void top(object_p obj)
    // ------------------------------------------------------------------------
    //   Set the top of the runtime stack
    // ------------------------------------------------------------------------
    {
        if (StackTop >= StackBottom)
            error("Too few arguments");
        else
            *StackTop = obj;
    }

    object_p pop()
    // ------------------------------------------------------------------------
    //   Pop the top-level object from the stack, or return NULL
    // ------------------------------------------------------------------------
    {
        if (StackTop >= StackBottom)
        {
            error("Too few arguments");
            return nullptr;
        }
        return *StackTop++;
    }

    object_p stack(uint idx)
    // ------------------------------------------------------------------------
    //    Get the object at a given position in the stack
    // ------------------------------------------------------------------------
    {
        if (idx >= depth())
        {
            error("Too few arguments");
            return nullptr;
        }
        return StackTop[idx];
    }

    void stack(uint idx, object_p obj)
    // ------------------------------------------------------------------------
    //    Get the object at a given position in the stack
    // ------------------------------------------------------------------------
    {
        if (idx >= depth())
            error("Too few arguments");
        else
            StackTop[idx] = obj;
    }

    void drop(uint count = 1)
    // ------------------------------------------------------------------------
    //   Pop the top-level object from the stack, or return NULL
    // ------------------------------------------------------------------------
    {
        if (count > depth())
            error("Too few arguments");
        else
            StackTop += count;
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

    void call(gcp<const object> callee)
    // ------------------------------------------------------------------------
    //   Push the current object on the RPL stack
    // ------------------------------------------------------------------------
    {
        if (available(sizeof(callee)) < sizeof(callee))
        {
            error("Too many calls");
            return;
        }
        StackTop--;
        StackBottom--;
        for (object_p *s = StackBottom; s < StackTop; s++)
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
            error("Return without a caller");
            return;
        }
        Code = *Returns++;
        StackTop++;
        StackBottom++;
        for (object_p *s = StackTop; s > StackTop; s--)
            s[0] = s[-1];
    }


    // ========================================================================
    //
    //   Error handling
    //
    // ========================================================================

    runtime &error(utf8 message, utf8 source)
    // ------------------------------------------------------------------------
    //   Set the error message
    // ------------------------------------------------------------------------
    {
        if (message)
            record(errors, "Error [%s] at source [%s]", message, source);
        else
            record(runtime, "Clearing error");
        Error = message;
        ErrorSource = source;
        return *this;
    }

    runtime &error(cstring message, cstring source = nullptr)
    // ------------------------------------------------------------------------
    //   Set the error message
    // ------------------------------------------------------------------------
    {
        return error(utf8(message), utf8(source));
    }

    runtime &error(cstring message, utf8 source)
    // ------------------------------------------------------------------------
    //   Set the error message
    // ------------------------------------------------------------------------
    {
        return error(utf8(message), source);
    }

    utf8 error()
    // ------------------------------------------------------------------------
    //   Get the error message
    // ------------------------------------------------------------------------
    {
        return Error;
    }

    utf8 source()
    // ------------------------------------------------------------------------
    //   Get the pointer to the problem
    // ------------------------------------------------------------------------
    {
        return ErrorSource;
    }

    runtime &command(utf8 cmd)
    // ------------------------------------------------------------------------
    //   Set the faulting command
    // ------------------------------------------------------------------------
    {
        ErrorCommand = cmd;
        return *this;
    }

    runtime &command(cstring cmd)
    // ------------------------------------------------------------------------
    //   Set the faulting command
    // ------------------------------------------------------------------------
    {
        return command(utf8(cmd));
    }

    utf8 command()
    // ------------------------------------------------------------------------
    //   Get the faulting command if there is one
    // ------------------------------------------------------------------------
    {
        return ErrorCommand;
    }


protected:
    utf8      Error;        // Error message if any
    utf8      ErrorSource;  // Source of the error if known
    utf8      ErrorCommand; // Source of the error if known
    object_p  Code;         // Currently executing code
    object_p  LowMem;       // Bottom of available memory
    object_p  Globals;      // Global objects
    object_p  Temporaries;  // Temporaries (must be valid objects)
    size_t    Editing;      // Text editor (utf8 encoded)
    size_t    Scratch;      // Scratch pad (may be invalid objects)
    object_p *StackTop;     // Top of user stack
    object_p *StackBottom;  // Bottom of user stack
    object_p *Returns;      // Return stack
    object_p  HighMem;      // Top of available memory

  // Pointers that are GC-adjusted
  gcptr    *GCSafe;

public:
    // The one and only runtime
    static runtime RT;
};


template<typename T>
using gcp = runtime::gcp<T>;

using gcstring  = gcp<const char>;
using gcmstring = gcp<char>;
using gcbytes   = gcp<const byte>;
using gcmbytes  = gcp<byte>;
using gcutf8    = gcp<const byte>;
using gcmutf8   = gcp<byte>;
using gcobj     = gcp<const object>;



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
