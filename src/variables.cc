// ****************************************************************************
//  variables.cc                                                  DB48X project
// ****************************************************************************
//
//   File Description:
//
//      Implementation of variables
//
//      Global variables are stored in mutable directory objects that occupy
//      a reserved area of the runtime, and can grow/shrinnk as you store
//      or purge global variables
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

#include "variables.h"

#include "integer.h"
#include "list.h"
#include "parser.h"
#include "renderer.h"

RECORDER(directory,       16, "Catalogs");
RECORDER(directory_error, 16, "Errors from directorys");

OBJECT_HANDLER_BODY(directory)
// ----------------------------------------------------------------------------
//    Handle commands for directorys
// ----------------------------------------------------------------------------
{
    switch(op)
    {
    case EXEC:
    case EVAL:
        // Catalogs evaluate as self
        rt.push(obj);
        return OK;
    case SIZE:
        return size(obj, payload);
    case PARSE:
        return object_parser(OBJECT_PARSER_ARG(), rt);
    case RENDER:
        return obj->object_renderer(OBJECT_RENDERER_ARG(), rt);
    case HELP:
        return (intptr_t) "directory";

    default:
        // Check if anyone else knows how to deal with it
        return DELEGATE(list);
    }
}


OBJECT_PARSER_BODY(directory)
// ----------------------------------------------------------------------------
//    Try to parse this as a directory
// ----------------------------------------------------------------------------
//    Catalog should never be parsed, but do something sensible if it happens
{
    return SKIP;
}


OBJECT_RENDERER_BODY(directory)
// ----------------------------------------------------------------------------
//   Render the directory into the given directory buffer
// ----------------------------------------------------------------------------
{
    return snprintf(r.target, r.length, "Catalog (internal)");
}


bool directory::store(gcobj name, gcobj value)
// ----------------------------------------------------------------------------
//    Store an object in the directory
// ----------------------------------------------------------------------------
//    Note that the directory itself should never move because of GC
//    That's because it normally should reside in the globals area
{
    runtime &rt     = runtime::RT;
    object_p header = (object_p) payload();
    object_p body   = header;
    size_t   old    = leb128<size_t>(body);     // Old size of directory
    size_t   now    = old;                      // Updated size
    size_t   vs     = value->size();            // Size of value

    if (gcobj existing = lookup(name))
    {
        // Replace an existing entry
        gcobj evalue = existing->skip();
        size_t es = evalue->size();
        if (vs > es)
        {
            size_t requested = vs - es;
            if (rt.available(requested) < requested)
                return false;           // Out of memory
        }

        // Clone any value in the stack that points to the existing value
        rt.clone_global(evalue);

        // Move memory above storage if necessary
        if (vs != es)
            rt.move_globals((object_p) evalue + vs, (object_p) evalue + es);

        // Copy new value into storage location
        memmove((byte *) evalue, (byte *) value, vs);

        // Compute new size of the directory
        now += vs - es;
    }
    else
    {
        // New entry, need to make room for name and value
        size_t ns = name->size();
        size_t vs = value->size();
        size_t requested = vs + ns;
        if (rt.available(requested) < requested)
            return false;               // Out of memory

        // Move memory above end of directory
        object_p end = body + old;
        rt.move_globals(end + requested, end);

        // Copy name and value at end of directory
        memmove((byte *) end, (byte *) name, ns);
        memmove((byte *) end + ns, (byte *) value, vs);

        // Compute new size of the directory
        now += requested;
    }

    // Adjust directory size
    size_t nowh = leb128size(now);
    size_t oldh = leb128size(old);
    if (nowh != oldh)
        // Header size changed, move the directory contents and rest of globals
        rt.move_globals(header + nowh, header + oldh);
    leb128(header, now);

    return true;
}


object_p directory::lookup(object_p ref) const
// ----------------------------------------------------------------------------
//   Find if the name exists in the directory, if so return pointer to it
// ----------------------------------------------------------------------------
{
    byte_p p = payload();
    size_t size = leb128<size_t>(p);
    size_t rsize = ref->size();

    while (size)
    {
        object_p name = (object_p) p;
        size_t ns = name->size();
        if (name == ref)          // Optimization when name is from directory
            return name;
        if (ns == rsize && memcmp(name, ref, rsize) == 0)
            return name;

        p += ns;
        object_p value = (object_p) p;
        size_t vs = value->size();
        p += vs;

        // Defensive coding against malformed directorys
        if (ns + vs > size)
        {
            record(directory_error,
                   "Lookup malformed directory (ns=%u vs=%u size=%u)",
                   ns, vs, size);
            return nullptr;     // Malformed directory, quick exit
        }

        size -= (ns + vs);
    }

    return nullptr;
}


object_p directory::recall(object_p ref) const
// ----------------------------------------------------------------------------
//   If the referenced object exists in directory, return associated value
// ----------------------------------------------------------------------------
{
    if (object_p found = lookup(ref))
        // The value follows the name
        return found->skip();
    return nullptr;
}


size_t directory::purge(object_p ref)
// ----------------------------------------------------------------------------
//    Purge a name (and associated value) from the directory
// ----------------------------------------------------------------------------
{
    if (object_p name = lookup(ref))
    {
        size_t ns = name->size();
        object_p value = name + ns;
        size_t vs = value->size();
        size_t purged = ns + vs;

        runtime &rt = runtime::RT;
        rt.move_globals(name, name + purged);

        size_t old = object::size();
        if (old < purged)
        {
            record(directory_error,
                   "Purging %u bytes in %u bytes directory", purged, old);
            purged = old;
        }

        // Update header
        object_p header = (object_p) payload();
        size_t now = old - purged;
        size_t oldh = leb128size(old);
        size_t nowh = leb128size(now);
        if (nowh > oldh)
            record(directory_error,
                   "Purge increased directory size from %u to %u", oldh, nowh);
        if (nowh < oldh)
            // Rare case where the directory size itself uses less bytes
            rt.move_globals(header + nowh, header + oldh);
        leb128(header, now);

        return purged;
    }

    // If nothing purged, return 0
    return 0;
}


size_t directory::enumerate(enumeration_fn callback, void *arg)
// ----------------------------------------------------------------------------
//   Process all the variables in turn, return number of true values
// ----------------------------------------------------------------------------
{
    byte_p p     = payload();
    size_t size  = leb128<size_t>(p);
    size_t count = 0;

    while (size)
    {
        symbol_p name = (symbol_p) p;
        size_t   ns   = name->object::size();
        p += ns;
        object_p value = (object_p) p;
        size_t   vs    = value->size();
        p += vs;

        // Defensive coding against malformed directorys
        if (ns + vs > size)
        {
            record(directory_error,
                   "Malformed directory during enumeration (ns=%u vs=%u size=%u)",
                   ns, vs, size);
            return 0;     // Malformed directory, quick exit
        }

        if (!callback || callback(name, value, arg))
            count++;

        size -= (ns + vs);
    }

    return count;
}



// ============================================================================
//
//    Variable-related commands
//
// ============================================================================

COMMAND_BODY(Sto)
// ----------------------------------------------------------------------------
//   Store a global variable into current directory
// ----------------------------------------------------------------------------
{
    directory *cat = RT.variables(0);
    if (!cat)
    {
        RT.error("No current directory");
        return ERROR;
    }

    // Check that we have two objects in the stack
    object_p x = RT.stack(0);
    object_p y = RT.stack(1);
    if (x && y)
    {
        symbol_p name = x->as_name();
        if (!name)
        {
            RT.error("Invalid name");
            return ERROR;
        }

        if (cat->store(name, y))
        {
            RT.drop();
            RT.drop();
            return OK;
        }
    }

    // Otherwise, return an error
    return ERROR;
}


COMMAND_BODY(Rcl)
// ----------------------------------------------------------------------------
//   Recall a global variable from current directory
// ----------------------------------------------------------------------------
{
    object_p x = RT.stack(0);
    if (!x)
        return ERROR;
    symbol_p name = x->as_name();
    if (!name)
    {
        RT.error("Invalid name");
        return ERROR;
    }

    // Lookup all directorys, starting with innermost one
    directory *cat = nullptr;
    for (uint depth = 0; (cat = RT.variables(depth)); depth++)
    {
        if (object_p value = cat->recall(name))
        {
            RT.top(value);
            return OK;
        }
    }

    // Otherwise, return an error
    RT.error("Undefined name");
    return ERROR;
}


COMMAND_BODY(Purge)
// ----------------------------------------------------------------------------
//   Purge a global variable from current directory
// ----------------------------------------------------------------------------
{
    object_p x = RT.stack(0);
    if (!x)
        return ERROR;
    symbol_p name = x->as_name();
    if (!name)
    {
        RT.error("Invalid name");
        return ERROR;
    }
    RT.pop();

    // Lookup all directorys, starting with innermost one
    directory *cat = RT.variables(0);
    if (!cat)
    {
        RT.error("No current directory");
        return ERROR;
    }

    // Purge the object (HP48 doesn't error out if name does not exist)
    cat->purge(name);
    return OK;
}


COMMAND_BODY(PurgeAll)
// ----------------------------------------------------------------------------
//   Purge a global variable from current directory and enclosing directories
// ----------------------------------------------------------------------------
{
    object_p x = RT.stack(0);
    if (!x)
        return ERROR;
    symbol_p name = x->as_name();
    if (!name)
    {
        RT.error("Invalid name");
        return ERROR;
    }
    RT.pop();

    // Lookup all directorys, starting with innermost one, and purge there
    directory *cat = nullptr;
    for (uint depth = 0; (cat = RT.variables(depth)); depth++)
        cat->purge(name);

    return OK;
}


COMMAND_BODY(Mem)
// ----------------------------------------------------------------------------
//    Return amount of available memory
// ----------------------------------------------------------------------------
//    The HP48 manual specifies that mem performs garbage collection
{
    RT.gc();
    run<FreeMemory>();
    return OK;
}


COMMAND_BODY(GarbageCollect)
// ----------------------------------------------------------------------------
//   Run the garbage collector
// ----------------------------------------------------------------------------
{
    size_t saved = RT.gc();
    integer_p result = RT.make<integer>(ID_integer, saved);
    RT.push(result);
    return OK;
}


COMMAND_BODY(FreeMemory)
// ----------------------------------------------------------------------------
//   Return amount of free memory (available without garbage collection)
// ----------------------------------------------------------------------------
{
    size_t available = RT.available();
    integer_p result = RT.make<integer>(ID_integer, available);
    RT.push(result);
    return OK;
}



// ============================================================================
//
//    Variables menu
//
// ============================================================================

OBJECT_HANDLER_BODY(VariablesMenu)
// ----------------------------------------------------------------------------
//   Process the MENU command for VariablesMenu
// ----------------------------------------------------------------------------
{
    switch(op)
    {
    case MENU:
    {
        info &mi     = *((info *) arg);
        uint  nitems = count_variables();
        items_init(mi, nitems, 1);
        list_variables(mi);
        return OK;
    }
    default:
        return DELEGATE(menu);

    }
}


uint VariablesMenu::count_variables()
// ----------------------------------------------------------------------------
//    Count the variables in the current directory
// ----------------------------------------------------------------------------
{
    directory *cat = RT.variables(0);
    if (!cat)
    {
        RT.error("No current directory");
        return 0;
    }
    return cat->count();
}


static bool evaluate_variable(symbol_p name, object_p value, void *arg)
// ----------------------------------------------------------------------------
//   Add a variable to evaluate in the menu
// ----------------------------------------------------------------------------
{
    menu::info &mi = *((menu::info *) arg);
    menu::items(mi, name, value);
    return true;
}


static bool recall_variable(symbol_p name, object_p UNUSED value, void *arg)
// ----------------------------------------------------------------------------
//   Add a variable to evaluate in the menu
// ----------------------------------------------------------------------------
//   For a name X, we create an object « 'Name' RCL »
{
    menu::info &mi = *((menu::info *) arg);
    menu::items(mi, name, menu::ID_VariablesMenuRecall);
    return true;
}


static bool store_variable(symbol_p name, object_p UNUSED value, void *arg)
// ----------------------------------------------------------------------------
//   Add a variable to evaluate in the menu
// ----------------------------------------------------------------------------
{
    menu::info &mi = *((menu::info *) arg);
    menu::items(mi, name, menu::ID_VariablesMenuStore);
    return true;
}



void VariablesMenu::list_variables(info &mi)
// ----------------------------------------------------------------------------
//   Fill the menu with variable names
// ----------------------------------------------------------------------------
{
    directory *cat = RT.variables(0);
    if (!cat)
    {
        RT.error("No current directory");
        return;
    }

    uint skip = mi.skip;
    mi.plane  = 0;
    mi.planes = 1;
    cat->enumerate(evaluate_variable, &mi);
    mi.plane  = 1;
    mi.planes = 2;
    mi.skip   = skip;
    mi.index  = mi.plane * input::NUM_SOFTKEYS;
    cat->enumerate(recall_variable, &mi);
    mi.plane  = 2;
    mi.planes = 3;
    mi.index  = mi.plane * input::NUM_SOFTKEYS;
    mi.skip   = skip;
    cat->enumerate(store_variable, &mi);

    for (uint k = 0; k < input::NUM_SOFTKEYS - (mi.pages > 1); k++)
    {
        Input.marker(k + 1 * input::NUM_SOFTKEYS, L'▶', true);
        Input.marker(k + 2 * input::NUM_SOFTKEYS, L'▶', false);
    }
}


COMMAND_BODY(VariablesMenuRecall)
// ----------------------------------------------------------------------------
//   Recall a variable from the VariablesMenu
// ----------------------------------------------------------------------------
{
    int key = Input.evaluating;
    if (key >= KEY_F1 && key <= KEY_F6)
        if (symbol_p name = Input.label(key - KEY_F1))
            if (directory *cat = RT.variables(0))
                if (object_p value = cat->recall(name))
                    if (RT.push(value))
                        return OK;

    return ERROR;
}


COMMAND_BODY(VariablesMenuStore)
// ----------------------------------------------------------------------------
//   Store a variable from the VariablesMenu
// ----------------------------------------------------------------------------
{
    int key = Input.evaluating;
    if (key >= KEY_F1 && key <= KEY_F6)
        if (symbol_p name = Input.label(key - KEY_F1))
            if (directory *cat = RT.variables(0))
                if (object_p value = RT.pop())
                    if (cat->store(name, value))
                        return OK;

    return ERROR;
}
