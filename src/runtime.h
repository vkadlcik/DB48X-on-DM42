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

#include <types.h>


struct object;                  // RPL object
struct global;                  // RPL global variable

struct runtime
// ----------------------------------------------------------------------------
//   The RPL runtime information
// ----------------------------------------------------------------------------
//   Layout in memory is as follows
//
//      HighMem         End of usable memory
//      Globals         Global named RPL objects
//      Temporaries     Temporaries, allocated down
//        ...
//      StackTop        Top of stack
//      StackBottom     Bottom of stack
//      Returns         Top of return stack
//      LowMem          Bottom of memory
//
//   When allocating a temporary, we move 'Temporaries' down
//   When allocating stuff on the stack, we move StackTop up
//   What is below StackTop is word-aligned
//   Stack elements point to temporaries, globals or robjects (read-only)
//   What is above Temporaries is byte-aligned
{
    runtime(byte *memory, size_t size)
        : Error(nullptr),
          Code(nullptr),
          LowMem((object *) memory),
          Returns((object **) LowMem),
          StackBottom((object **) Returns),
          StackTop((object **) LowMem),
          Temporaries((object *) (memory + size)),
          Globals((global *) Temporaries),
          HighMem(Globals),
          GCSafe(nullptr)
    {}
    ~runtime() {}

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
        return (byte *) Temporaries - (byte *) StackTop - redzone;
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


    object *make(size_t size, type ty)
    // ------------------------------------------------------------------------
    //   Make a new temporary
    // ------------------------------------------------------------------------
    {
        if (available(size) < size)
            return nullptr;    // Failed to allocate
        Temporaries = (object *) ((byte *) Temporaries - size);
        return Temporaries;
    }

    void dispose(object *object)
    // ------------------------------------------------------------------------
    //   Dispose of a temporary (must not be referenced elsewhere)
    // ------------------------------------------------------------------------
    {
        if (object == Temporaries)
            Temporaries = skip(object);
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
        *(++StackTop) = obj;
    }

    object *top()
    // ------------------------------------------------------------------------
    //   Return the top of the runtime stack
    // ------------------------------------------------------------------------
    {
        return StackTop > StackBottom ? *StackTop : nullptr;
    }

    void top(object *obj)
    // ------------------------------------------------------------------------
    //   Set the top of the runtime stack
    // ------------------------------------------------------------------------
    {
        if (StackTop <= StackBottom)
            return error("Cannot replace empty stack");
        *StackTop = obj;
    }

    object *pop()
    // ------------------------------------------------------------------------
    //   Pop the top-level object from the stack, or return NULL
    // ------------------------------------------------------------------------
    {
        if (StackTop <= StackBottom)
            return error("Not enough arguments"), nullptr;
        return *StackTop--;
    }

    object *stack(uint idx)
    // ------------------------------------------------------------------------
    //    Get the object at a given position in the stack
    // ------------------------------------------------------------------------
    {
        if (idx >= depth())
            return error("Insufficient stack depth"), nullptr;
        return StackTop[~idx];
    }

    void stack(uint idx, object *obj)
    // ------------------------------------------------------------------------
    //    Get the object at a given position in the stack
    // ------------------------------------------------------------------------
    {
        if (idx >= depth())
            return error("Insufficient stack depth");
        StackTop[~idx] = obj;
    }

    uint depth()
    // ------------------------------------------------------------------------
    //   Return the stack depth
    // ------------------------------------------------------------------------
    {
        return StackTop - StackBottom;
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
        StackTop++;
        StackBottom++;
        for (object **s = StackTop; s > StackBottom; s--)
            s[0] = s[-1];
        *(++Returns) = Code;
        Code = callee;
    }

    void ret()
    // ------------------------------------------------------------------------
    //   Return from an RPL call
    // ------------------------------------------------------------------------
    {
        if ((byte *) Returns <= (byte *) LowMem)
            return error("Cannot return without a caller");
        Code = *Returns--;
        StackTop--;
        StackBottom--;
        for (object **s = StackBottom; s < StackTop; s++)
            s[0] = s[1];
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

        friend class runtime;
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

    void error(cstring message)
    // ------------------------------------------------------------------------
    //   Set the error message
    // ------------------------------------------------------------------------
    {
        Error = message;
    }


  public:
    cstring  Error;   // Error message if any
    object  *Code;    // Currently executing code
    object  *LowMem;
    object **Returns;
    object **StackBottom;
    object **StackTop;
    object  *Temporaries;
    global  *Globals;
    global  *HighMem;

    // Pointers that are GC-adjusted
    gcptr   *GCSafe;

    // The one and only runtime
    static runtime RT;
};



// ============================================================================
//
//    Standard C++ memory allocation
//
// ============================================================================

inline void *operator new(size_t sz, runtime &rt, type ty)
// ----------------------------------------------------------------------------
//   Allocate an object as a temporary in the runtime
// ----------------------------------------------------------------------------
{
    return rt.make(sz, ty);
}


inline void operator delete(void *mem, runtime &rt)
// ----------------------------------------------------------------------------
//   Immediately deallocate an object
// ----------------------------------------------------------------------------
{
    rt.dispose((object *) mem);
}


#endif // RUNTIME_H
