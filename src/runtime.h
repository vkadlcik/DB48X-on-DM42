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

#include "types.h"


struct object;                  // RPL object
struct global;                  // RPL global variable

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
          Code(nullptr),
          LowMem(),
          Globals(),
          Temporaries(),
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
        Temporaries = (object *) LowMem;
        Globals = (global *) Temporaries;
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
        return (byte *) StackTop - (byte *) Temporaries - redzone;
    }

    size_t available(size_t size)
    // ------------------------------------------------------------------------
    //   Check if we have enough for the given size
    // ------------------------------------------------------------------------
    {
        if (available() < size)
            return gc();
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
            return error("Cannot replace empty stack");
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
            return error("Insufficient stack depth");
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
            return error("Too many recursive calls");
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
            return error("Cannot return without a caller");
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
        gcptr(object *ptr = nullptr) : safe(ptr), next(RT.GCSafe)
        {
            RT.GCSafe = this;
        }
        gcptr(const gcptr &o) = delete;
        ~gcptr()
        {
            gcptr *last = nullptr;
            for (gcptr *gc = RT.GCSafe; gc; gc = gc->next)
            {
                last = gc;
                if (gc == this)
                {
                    if (last)
                        last->next = gc->next;
                    else
                        RT.GCSafe = gc->next;
                    break;
                }
            }
        }

        operator object *() const    { return safe; }
        operator object *&()         { return safe; }

    private:
        object *safe;
        gcptr  *next;

        friend struct runtime;
    };


    template<typename Obj>
    struct gcp : gcptr
    // ------------------------------------------------------------------------
    //   Protect a pointer against garbage collection
    // ------------------------------------------------------------------------
    {
        gcp(Obj *obj): gcptr(obj) {}
        ~gcp() {}

        operator Obj *() const  { return (Obj *) safe; }
        operator Obj *&()       { return (Obj *&) safe; }
    };



    // ========================================================================
    //
    //   Error handling
    //
    // ========================================================================

    void error(cstring message, cstring source = nullptr)
    // ------------------------------------------------------------------------
    //   Set the error message
    // ------------------------------------------------------------------------
    {
        Error = message;
        ErrorSource = source;
    }


  public:
    cstring  Error;             // Error message if any
    cstring  ErrorSource;       // Source of the error if known
    object  *Code;              // Currently executing code
    object  *LowMem;
    global  *Globals;
    object  *Temporaries;
    object **StackTop;
    object **StackBottom;
    object **Returns;
    object  *HighMem;

    // Pointers that are GC-adjusted
    gcptr   *GCSafe;

    // The one and only runtime
    static runtime RT;
};


template<typename T>
using gcptr = runtime::gcp<T>;

using gcstring = gcptr<const char>;
using gcmstring = gcptr<char>;



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
