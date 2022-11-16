// ****************************************************************************
//  runtime.cc                                                    DB48X project
// ****************************************************************************
//
//   File Description:
//
//     Implementation of the RPL runtime
//
//     See memory layout in header file
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

#include "runtime.h"

#include "input.h"
#include "object.h"
#include "variables.h"

#include <cstring>

// The one and only runtime
runtime runtime::RT(nullptr, 0);

RECORDER(runtime,       16, "RPL runtime");
RECORDER(runtime_error, 16, "RPL runtime error (anomalous behaviors)");
RECORDER(editor,        16, "Text editor (command line)");
RECORDER(errors,        16, "Runtime errors)");
RECORDER(gc,           256, "Garbage collection events");
RECORDER(gc_errors,     16, "Garbage collection errors");
RECORDER(gc_details,   256, "Details about garbage collection (noisy)");


// ============================================================================
//
//   Initialization
//
// ============================================================================

runtime::runtime(byte *mem, size_t size)
// ----------------------------------------------------------------------------
//   Runtime constructor
// ----------------------------------------------------------------------------
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
    if (mem)
        memory(mem, size);
}


void runtime::memory(byte *memory, size_t size)
// ----------------------------------------------------------------------------
//   Assign the given memory range to the runtime
// ----------------------------------------------------------------------------
{
    LowMem = (object_p) memory;
    HighMem = (object_p) (memory + size);

    // Stuff at top of memory
    Returns = (object_p*) HighMem - 1;                  // Locals for top level
    *Returns = (object_p) 0;                            // 0 local variables
    StackBottom = (object_p*) Returns - 1;
    StackTop = (object_p*) StackBottom;                 // Empty stack

    // Stuff at bottom of memory
    Globals = LowMem;
    directory_p home = new((void *) Globals) directory();   // Home directory
    *StackBottom = (object_p) home;                     // Current search path
    Globals = home->skip();                             // Globals after directory
    Temporaries = Globals;                              // Area for temporaries
    Editing = 0;                                        // No editor
    Scratch = 0;                                        // No scratchpad

    record(runtime, "Memory %p-%p size %u (%uK)",
           LowMem, HighMem, size, size>>10);
}



// ============================================================================
//
//    Temporaries
//
// ============================================================================

size_t runtime::available()
// ----------------------------------------------------------------------------
//   Return the size available for temporaries
// ----------------------------------------------------------------------------
{
    size_t aboveTemps = Editing + Scratch + redzone;
    return (byte *) StackTop - (byte *) Temporaries - aboveTemps;
}


size_t runtime::available(size_t size)
// ----------------------------------------------------------------------------
//   Check if we have enough for the given size
// ----------------------------------------------------------------------------
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



// ============================================================================
//
//   Garbage collection
//
// ============================================================================

#ifdef SIMULATOR
static bool integrity_test(object_p first,
                           object_p last,
                           object_p *stack,
                           object_p *stackEnd)
// ----------------------------------------------------------------------------
//   Check all the objects in a given range
// ----------------------------------------------------------------------------
{
    object_p next, obj;

    for (obj = first; obj < last; obj = next)
    {
        object::id type = obj->type();
        if (type >= object::NUM_IDS)
            return false;
        next = obj->skip();
    }
    if (obj != last)
        return false;

    for (object_p *s = stack; s < stackEnd; s++)
        if (!*s || (*s)->type() >= object::NUM_IDS)
            return false;

    return true;
}


static void dump_object_list(cstring  message,
                             object_p first,
                             object_p last,
                             object_p *stack,
                             object_p *stackEnd)
// ----------------------------------------------------------------------------
//   Dump all objects in a given range
// ----------------------------------------------------------------------------
{
    uint count = 0;
    size_t sz = 0;
    object_p next;

    record(gc, "%+s object list", message);
    for (object_p obj = first; obj < last; obj = next)
    {
        object::id i = obj->type();
        if (i >= object::NUM_IDS)
        {
            record(gc_errors, " %p: corrupt object ID type %u", obj, i);
            break;
        }

        next = obj->skip();
        record(gc, " %p+%llu: %+s (%d)",
               obj, next - obj, object::name(i), i);
        sz += next - obj;
        count++;
    }
    record(gc, "%+s stack", message);
    for (object_p *s = stack; s < stackEnd; s++)
        record(gc, " %u: %p (%+s)", s - stack, *s, object::name((*s)->type()));
    record(gc, "%+s: %u objects using %u bytes", message, count, sz);
}
#endif // SIMULATOR


runtime::gcptr::~gcptr()
// ----------------------------------------------------------------------------
//   Destructor for a garbage-collected pointer
// ----------------------------------------------------------------------------
{
    gcptr *last = nullptr;
    if (this == RT.GCSafe)
    {
        RT.GCSafe = next;
        return;
    }

    for (gcptr *gc = RT.GCSafe; gc; gc = gc->next)
    {
        if (gc == this)
        {
            last->next = gc->next;
            return;
        }
        last = gc;
    }
}


size_t runtime::gc()
// ----------------------------------------------------------------------------
//   Recycle unused temporaries
// ----------------------------------------------------------------------------
//   Temporaries can only be referenced from the stack
//   Objects in the global area are copied there, so they need no recycling
//   This algorithm is linear in number of objects and moves only live data
{
    size_t   recycled = 0;
    object_p first    = (object_p) Globals;
    object_p last     = Temporaries;
    object_p free     = first;
    object_p next;

    record(gc, "Garbage collection, available %u, range %p-%p",
           available(), first, last);
#ifdef SIMULATOR
    if (!integrity_test(first, last, StackTop, StackBottom))
    {
        record(gc_errors, "Integrity test failed pre-collection");
        recorder_dump();
        RECORDER_TRACE(gc) = 2;
    }
    if (RECORDER_TRACE(gc) > 1)
        dump_object_list("Pre-collection",
                         first, last, StackTop, StackBottom);
#endif // SIMULATOR

    for (object_p obj = first; obj < last; obj = next)
    {
        bool found = false;
        next = obj->skip();
        record(gc_details, "Scanning object %p (ends at %p)", obj, next);
        for (object_p *s = StackTop; s < StackBottom && !found; s++)
        {
            found = *s >= obj && *s < next;
            if (found)
                record(gc_details, "Found %p at stack level %u",
                       obj, s - StackTop);
        }
        if (!found)
        {
            for (gcptr *p = GCSafe; p && !found; p = p->next)
            {
                found = p->safe >= (byte *) obj && p->safe <= (byte *) next;
                if (found)
                    record(gc_details, "Found %p in GC-safe pointer %p (%p)",
                           obj, p->safe, p);
            }
        }
        if (!found)
        {
            object_p *functions = &Input.function[0][0];
            size_t count = input::NUM_PLANES * input::NUM_KEYS;
            object_p *lastf = functions + count;
            for (object_p *p = functions; p < lastf && !found; p++)
            {
                found = *p >= obj && *p <= next;
                if (found)
                    record(gc_details, "Found %p in input function table %u",
                           obj, p - functions);
            }
        }
        if (found)
        {
            // Move object to free space
            record(gc_details, "Moving %p-%p to %p", obj, next, free);
            move(free, obj, next - obj);
            free += next - obj;
        }
        else
        {
            recycled += next - obj;
            record(gc_details, "Recycling %p size %u total %u",
                   obj, next - obj, recycled);
        }
    }

    // Move the command line and scratch buffer
    if (Editing + Scratch)
    {
        object_p edit = Temporaries;
        move(edit - recycled, edit, Editing + Scratch, true);
    }

    // Adjust Temporaries
    Temporaries -= recycled;


#ifdef SIMULATOR
    if (!integrity_test(Globals, Temporaries, StackTop, StackBottom))
    {
        record(gc_errors, "Integrity test failed post-collection");
        recorder_dump();
        RECORDER_TRACE(gc) = 2;
    }
    if (RECORDER_TRACE(gc) > 1)
        dump_object_list("Post-collection",
                         (object_p) Globals, Temporaries,
                         StackTop, StackBottom);
#endif // SIMULATOR

    record(gc, "Garbage collection done, purged %u, available %u",
           recycled, available());
    return recycled;
}


void runtime::move(object_p to, object_p from, size_t size, bool scratch)
// ----------------------------------------------------------------------------
//   Move objects in memory to a new location, adjusting pointers
// ----------------------------------------------------------------------------
//   The scratch flag indicates that we move the scratch area. In that case,
//   we don't need to adjust stack or function pointers, only gc-safe pointers.
//   Furthermore, scratch pointers may (temporarily) be above the scratch area.
//   See list parser for an example.
{
    int delta = to - from;
    if (!delta)
        return;

    // Move the object in memory
    memmove((byte *) to, (byte *) from, size);

    // Adjust the protected pointers
    object_p last = scratch ? (object_p) StackTop : from + size;
    record(gc_details, "Adjustment range is %p-%p, %+s",
           from, last, scratch ? "scratch" : "no scratch");
    for (gcptr *p = GCSafe; p; p = p->next)
    {
        if (p->safe >= (byte *) from && p->safe < (byte *) last)
        {
            record(gc_details, "Adjusting GC-safe %p from %p to %p",
                   p, p->safe, p->safe + delta);
            p->safe += delta;
        }
    }

    // No need to walk the stack pointers and function pointers
    if (scratch)
        return;

    // Adjust the stack pointers
    for (object_p *s = StackTop; s < StackBottom; s++)
    {
        if (*s >= from && *s < last)
        {
            record(gc_details, "Adjusting stack level %u from %p to %p",
                   s - StackTop, *s, *s + delta);
            *s += delta;
        }
    }

    // Adjust the input function pointers
    object_p *functions = &Input.function[0][0];
    size_t count = input::NUM_PLANES * input::NUM_KEYS;
    object_p *lastf = functions + count;
    for (object_p *p = functions; p < lastf; p++)
    {
        if (*p >= from && *p < last)
        {
            record(gc_details, "Adjusting input function %u from %p to %p",
                   p - functions, *p, *p + delta);
            *p += delta;
        }
    }
}


void runtime::move_globals(object_p to, object_p from)
// ----------------------------------------------------------------------------
//    Move data in the globals area
// ----------------------------------------------------------------------------
//    In that case, we need to move everything up to the scratchpad
{
    object_p last = (object_p) scratchpad() + allocated();
    object_p first = to < from ? to : from;
    size_t moving = last - first;
    move(to, from, moving);

    // Adjust Globals and Temporaries
    int delta = to - from;
    if (Globals >= first && Globals < last)             // Probably never
        Globals += delta;
    if (Temporaries >= first && Temporaries < last)     // Probably always
        Temporaries += delta;
}



// ============================================================================
//
//    Editor
//
// ============================================================================

size_t runtime::insert(size_t offset, utf8 data, size_t len)
// ----------------------------------------------------------------------------
//   Insert data in the editor, return size inserted
// ----------------------------------------------------------------------------
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


void runtime::remove(size_t offset, size_t len)
// ----------------------------------------------------------------------------
//   Remove characers from the editor
// ----------------------------------------------------------------------------
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


utf8 runtime::close_editor()
// ----------------------------------------------------------------------------
//   Close the editor and encapsulate its content into a string
// ----------------------------------------------------------------------------
//   This will move the editor below the temporaries, encapsulated as
//   a string. After that, it is safe to allocate temporaries without
//   overwriting the editor
{
    // Compute the extra size we need for a string header
    size_t hdrsize = leb128size(object::ID_text) + leb128size(Editing + 1);
    if (available(hdrsize) < hdrsize)
        return nullptr;

    // Move the editor data above that header
    char *ed = (char *) Temporaries;
    char *str = ed + hdrsize;
    memmove(str, ed, Editing);

    // Null-terminate that string for safe use by C code
    str[Editing] = 0;
    record(editor, "Closing editor size %u at %p [%s]", Editing, ed, str);

    // Write the string header
    ed = leb128(ed, object::ID_text);
    ed = leb128(ed, Editing + 1);

    // Move Temporaries past that newly created string
    Temporaries = (object_p) str + Editing + 1;

    // We are no longer editing
    Editing = 0;

    // Return a pointer to a valid C string safely wrapped in a RPL string
    return utf8(str);
}


size_t runtime::edit(utf8 buf, size_t len)
// ----------------------------------------------------------------------------
//   Open the editor with a known buffer
// ----------------------------------------------------------------------------
{
    gcutf8 buffer = buf;        // Need to keep track of GC movements

    if (available(len) < len)
    {
        record(editor, "Insufficent memory for %u bytes", len);
        out_of_memory_error();
        Editing = 0;
        return 0;
    }

    // Copy the scratchpad up (available() ensured we have room)
    if (Scratch)
        memmove((char *) Temporaries + len, Temporaries, Scratch);

    memcpy((byte *) Temporaries, (byte *) buffer, len);
    Editing = len;
    return len;
}


size_t runtime::edit()
// ----------------------------------------------------------------------------
//   Append the scratchpad to the editor (at end of buffer)
// ----------------------------------------------------------------------------
{
    record(editor, "Editing scratch pad size %u, editor was %u",
           Scratch, Editing);
    Editing += Scratch;
    Scratch = 0;

    record(editor, "Editor size now %u", Editing);
    return Editing;
}


byte *runtime::allocate(size_t sz)
// ----------------------------------------------------------------------------
//   Allocate additional bytes at end of scratchpad
// ----------------------------------------------------------------------------
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


byte *runtime::append(size_t sz, byte *bytes)
// ----------------------------------------------------------------------------
//   Append some bytes at end of scratch pad
// ----------------------------------------------------------------------------
{
    byte *ptr = allocate(sz);
    if (ptr)
        memcpy(ptr, bytes, sz);
    return ptr;
}


object_p runtime::clone(object_p source)
// ----------------------------------------------------------------------------
//   Clone an object into the temporaries area
// ----------------------------------------------------------------------------
//   This is useful when storing into a global referenced from the stack
{
    size_t size = source->size();
    if (available(size) < size)
        return nullptr;
    object_p result = Temporaries;
    Temporaries = object_p((byte *) Temporaries + size);
    move(Temporaries, result, Editing + Scratch, true);
    memmove((void *) result, source, size);
    return result;
}


object_p runtime::clone_global(object_p global)
// ----------------------------------------------------------------------------
//   Check if any entry in the stack points to a given global, if so clone it
// ----------------------------------------------------------------------------
{
    object_p cloned = nullptr;
    for (object_p *s = StackTop; s < StackBottom; s++)
    {
        if (*s == global)
        {
            if (!cloned)
                cloned = clone(global);
            *s = cloned;
        }
    }
    return cloned;
}



// ============================================================================
//
//   RPL stack
//
// ============================================================================

bool runtime::push(gcp<const object> obj)
// ----------------------------------------------------------------------------
//   Push an object on top of RPL stack
// ----------------------------------------------------------------------------
{
    // This may cause garbage collection, hence the need to adjust
    if (available(sizeof(obj)) < sizeof(obj))
        return false;
    *(--StackTop) = obj;
    return true;
}


object_p runtime::top()
// ----------------------------------------------------------------------------
//   Return the top of the runtime stack
// ----------------------------------------------------------------------------
{
    if (StackTop >= StackBottom)
    {
        error("Too few arguments");
        return nullptr;
    }
    return *StackTop;
}


bool runtime::top(object_p obj)
// ----------------------------------------------------------------------------
//   Set the top of the runtime stack
// ----------------------------------------------------------------------------
{
    if (StackTop >= StackBottom)
    {
        error("Too few arguments");
        return false;
    }
    *StackTop = obj;
    return true;
}


object_p runtime::pop()
// ----------------------------------------------------------------------------
//   Pop the top-level object from the stack, or return NULL
// ----------------------------------------------------------------------------
{
    if (StackTop >= StackBottom)
    {
        error("Too few arguments");
        return nullptr;
    }
    return *StackTop++;
}


object_p runtime::stack(uint idx)
// ----------------------------------------------------------------------------
//    Get the object at a given position in the stack
// ----------------------------------------------------------------------------
{
    if (idx >= depth())
    {
        error("Too few arguments");
        return nullptr;
    }
    return StackTop[idx];
}


bool runtime::stack(uint idx, object_p obj)
// ----------------------------------------------------------------------------
//    Get the object at a given position in the stack
// ----------------------------------------------------------------------------
{
    if (idx >= depth())
    {
        error("Too few arguments");
        return false;
    }
    StackTop[idx] = obj;
    return true;
}


bool runtime::roll(uint idx)
// ----------------------------------------------------------------------------
//    Move the object at a given position in the stack
// ----------------------------------------------------------------------------
{
    if (idx)
    {
        idx--;
        if (idx >= depth())
        {
            error("Too few arguments");
            return false;
        }
        object_p s = StackTop[idx];
        memmove(StackTop + 1, StackTop, idx * sizeof(*StackTop));
        *StackTop = s;
    }
    return true;
}


bool runtime::rolld(uint idx)
// ----------------------------------------------------------------------------
//    Get the object at a given position in the stack
// ----------------------------------------------------------------------------
{
    if (idx)
    {
        idx--;
        if (idx >= depth())
        {
            error("Too few arguments");
            return false;
        }
        object_p s = *StackTop;
        memmove(StackTop, StackTop + 1, idx * sizeof(*StackTop));
        StackTop[idx] = s;
    }
    return true;
}


bool runtime::drop(uint count)
// ----------------------------------------------------------------------------
//   Pop the top-level object from the stack, or return NULL
// ----------------------------------------------------------------------------
{
    if (count > depth())
    {
        error("Too few arguments");
        return false;
    }
    StackTop += count;
    return true;
}



// ============================================================================
//
//   Return stack
//
// ============================================================================

void runtime::call(gcp<const object> callee)
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


void runtime::ret()
// ----------------------------------------------------------------------------
//   Return from an RPL call
// ----------------------------------------------------------------------------
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



// ============================================================================
//
//
//
// ============================================================================

#define ERROR(name, msg)                        \
runtime &runtime::name##_error()                \
{                                               \
    return error(msg);                          \
}
#include "errors.tbl"
