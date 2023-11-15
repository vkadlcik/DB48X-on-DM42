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

#include "command.h"
#include "expression.h"
#include "files.h"
#include "integer.h"
#include "list.h"
#include "locals.h"
#include "parser.h"
#include "renderer.h"


RECORDER(directory,       16, "Directories");
RECORDER(directory_error, 16, "Errors from directories");


PARSE_BODY(directory)
// ----------------------------------------------------------------------------
//    Try to parse this as a directory
// ----------------------------------------------------------------------------
//    A directory has the following structure:
//        Directory { Name1 Value1 Name2 Value2 ... }
{
    cstring ref = cstring(utf8(p.source));
    size_t maxlen = p.length;
    cstring label = "directory";
    size_t len = strlen(label);
    if (len <= maxlen
        && strncasecmp(ref, label, len) == 0
        && command::is_separator(utf8(ref + len)))
    {
        gcutf8 body = utf8(ref + len);
        maxlen -= len;
        if (object_g obj = object::parse(body, maxlen))
        {
            if (obj->type() == ID_list)
            {
                // Check that we only have names in there
                uint    count  = 0;
                gcbytes bytes  = obj->payload();
                byte_p  ptr    = bytes;
                size_t  size   = leb128<size_t>(ptr);
                gcbytes start  = ptr;
                size_t  offset = 0;

                // Loop on all objects inside the list
                while (offset < size)
                {
                    object_p obj = object_p(byte_p(start) + offset);
                    size_t   objsize = obj->size();

                    if ((count & 1) == 0)
                    {
                        if (obj->type() != ID_symbol)
                        {
                            rt.error("Invalid name in directory").source(body);
                            return ERROR;
                        }
                    }
                    count++;

                    // Loop on next object
                    offset += objsize;
                }

                // We should have an even number of items here
                if (count & 1)
                {
                    rt.malformed_directory_error().source(body);
                    return SKIP;
                }

                // If we passed all these tests, build a directory
                p.out = rt.make<directory>(ID_directory, start, size);
                p.end = maxlen + len;
                return OK;
            }
        }
    }

    return SKIP;
}


bool directory::render_name(object_p name, object_p obj, void *arg)
// ----------------------------------------------------------------------------
//    Render an item in the directory
// ----------------------------------------------------------------------------
{
    renderer &r = *((renderer *) arg);
    name->render(r);
    r.indent();
    obj->render(r);
    r.unindent();
    return true;
}


RENDER_BODY(directory)
// ----------------------------------------------------------------------------
//   Render the directory into the given directory buffer
// ----------------------------------------------------------------------------
{
    r.put("Directory {");
    r.indent();
    o->enumerate(directory::render_name, &r);
    r.unindent();
    r.put("}");
    return r.size();
}


object::result directory::enter() const
// ----------------------------------------------------------------------------
//   Enter directory when executing a directory
// ----------------------------------------------------------------------------
{
    if (rt.enter(this))
    {
        ui.menu_refresh(ID_VariablesMenu);
        return OK;
    }
    return ERROR;
}


bool directory::store(object_g name, object_g value)
// ----------------------------------------------------------------------------
//    Store an object in the directory
// ----------------------------------------------------------------------------
//    Note that the directory itself should never move because of GC
//    That's because it normally should reside in the globals area
{
    size_t      vs      = value->size();        // Size of value
    int         delta   = 0;                    // Change in directory size
    directory_g thisdir = this;                 // Can move because of GC

    // If this is a quoted name, extract it
    if (object_p quoted = name->as_quoted(ID_object))
        name = quoted;

    // Deal with all special cases
    id nty = name->type();
    switch (nty)
    {
    case ID_local:
        // Deal with local variables
        return rt.local(local_p(name.Safe())->index(), value);

    case ID_text:
    {
        // Deal with storing to file
        files_g disk = files::make("data");
        return disk->store(text_p(name.Safe()), value);
    }

    default:
    case ID_symbol:
        break;

#define ID(n)
#define SETTING(Name, Low, High, Init)          \
    case ID_##Name:
#define FLAG(Enable, Disable)                   \
    case ID_##Enable:                           \
    case ID_##Disable:
#include "ids.tbl"
        return settings::store(nty, value);
    }

    // Normal case
    if (object_g existing = lookup(name))
    {
        // Replace an existing entry
        object_g evalue = existing->skip();
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

        // Compute change in size for directories
        delta = vs - es;
    }
    else
    {
        // New entry, need to make room for name and value
        size_t  ns        = name->size();
        size_t  vs        = value->size();
        size_t  requested = vs + ns;
        byte_p  p         = payload();
        size_t  dirsize   = leb128<size_t>(p);
        gcbytes body      = p;
        if (rt.available(requested) < requested)
            return false;               // Out of memory

        // Move memory from directory up
        object_p start = object_p(body.Safe());
        if (Settings.StoreAtEnd())
            start += dirsize;
        rt.move_globals(start + requested, start);

        // Copy name and value at end of directory
        memmove((byte *) start, (byte *) name, ns);
        memmove((byte *) start + ns, (byte *) value, vs);

        // Compute new size of the directory
        delta = requested;
    }

    // Adjust all directory sizes
    adjust_sizes(thisdir, delta);

    // Refresh the variables menu
    ui.menu_refresh(ID_VariablesMenu);

    return true;
}


void directory::adjust_sizes(directory_r thisdir, int delta)
// ----------------------------------------------------------------------------
//   Ajust the size for this directory and all enclosing ones
// ----------------------------------------------------------------------------
{
    // Resize directories up the chain
    uint depth = 0;
    bool found = false;
    while (directory_g dir = rt.variables(depth++))
    {
        // Start modifying only if we find this directory in path
        if (dir.Safe() == thisdir.Safe())
            found = true;
        if (found)
        {
            byte_p p = dir->payload();
            object_p hdr = object_p(p);
            size_t dirlen = leb128<size_t>(p);
            size_t newdirlen = dirlen + delta;
            size_t szbefore  = leb128size(dirlen);
            size_t szafter = leb128size(newdirlen);
            if (szbefore != szafter)
            {
                rt.move_globals(hdr + szafter, hdr + szbefore);
                delta += szafter - szbefore;
            }
            leb128(hdr, newdirlen);
        }
    }
}


object_p directory::lookup(object_p ref) const
// ----------------------------------------------------------------------------
//   Find if the name exists in the directory, if so return pointer to it
// ----------------------------------------------------------------------------
{
    byte_p p = payload();
    size_t size = leb128<size_t>(p);
    size_t rsize = ref->size();
    bool   issym = ref->type() == ID_symbol;

    while (size)
    {
        object_p name = (object_p) p;
        size_t ns = name->size();
        if (name == ref)          // Optimization when name is from directory
            return name;
        if (ns == rsize)
        {
            if (issym)
            {
                // Regular symbols: case insensitive comparison
                if (strncasecmp(cstring(name), cstring(ref), rsize) == 0)
                    return name;
            }
            else
            {
                // Special symbols, e.g. ΣData
                if (memcmp(cstring(name), cstring(ref), rsize) == 0)
                    return name;
            }
        }

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


object_p directory::recall_all(object_p name)
// ----------------------------------------------------------------------------
//   If the referenced object exists in directory, return associated value
// ----------------------------------------------------------------------------
{
    // Strip quote if any
    if (object_p quoted = name->as_quoted(ID_object))
        name = quoted;

    // Check independent / dependent values for plotting
    if (expression::independent && name->is_same_as(*expression::independent))
        return *expression::independent_value;
    if (expression::dependent && name->is_same_as(*expression::dependent))
        return *expression::dependent_value;

    // Deal with all special cases
    id nty = name->type();
    switch (nty)
    {
    case ID_local:
        // Deal with local variables
        return rt.local(local_p(name)->index());

    case ID_text:
    {
        // Deal with storing to file
        files_g disk = files::make("data");
        return disk->recall(text_p(name));
    }

    default:
    case ID_symbol:
        break;

#define ID(n)
#define SETTING(Name, Low, High, Init)          \
    case ID_##Name:
#define FLAG(Enable, Disable)                   \
    case ID_##Enable:                           \
    case ID_##Disable:
#include "ids.tbl"
        return settings::recall(nty);
    }

    directory *dir = nullptr;
    for (uint depth = 0; (dir = rt.variables(depth)); depth++)
        if (object_p value = dir->recall(name))
            return value;
    return nullptr;
}


size_t directory::purge(object_p name)
// ----------------------------------------------------------------------------
//    Purge a name (and associated value) from the directory
// ----------------------------------------------------------------------------
{
    directory_g thisdir = this;

    // Deal with all special cases
    id nty = name->type();
    switch (nty)
    {
    case ID_local:
        // Deal with local variables
        rt.type_error();
        return 0;

    case ID_text:
    {
        // Deal with storing to file
        files_g disk = files::make("data");
        return disk->purge(text_p(name));
    }

    default:
    case ID_symbol:
        break;

#define ID(n)
#define SETTING(Name, Low, High, Init)          \
    case ID_##Name:
#define FLAG(Enable, Disable)                   \
    case ID_##Enable:                           \
    case ID_##Disable:
#include "ids.tbl"
        return settings::purge(nty);
    }

    name = lookup(name);
    if (name)
    {
        size_t   ns     = name->size();
        object_p value  = name + ns;
        size_t   vs     = value->size();
        size_t   purged = ns + vs;
        object_p header = (object_p) payload();
        object_p body   = header;
        size_t   old    = leb128<size_t>(body); // Old size of directory

        rt.move_globals(name, name + purged);

        if (old < purged)
        {
            record(directory_error,
                   "Purging %u bytes in %u bytes directory", purged, old);
            purged = old;
        }

        adjust_sizes(thisdir, -int(purged));

        // Adjust variables menu
        ui.menu_refresh(ID_VariablesMenu);

        return purged;
    }

    // If nothing purged, return 0
    return 0;
}


size_t directory::purge_all(object_p name)
// ----------------------------------------------------------------------------
//   Purge objects from
// ----------------------------------------------------------------------------
{
    size_t     result = 0;
    directory *dir    = nullptr;
    for (uint depth = 0; (dir = rt.variables(depth)); depth++)
        result += dir->purge(name);
    return result;
}


size_t directory::enumerate(enumeration_fn callback, void *arg) const
// ----------------------------------------------------------------------------
//   Process all the variables in turn, return number of true values
// ----------------------------------------------------------------------------
{
    gcbytes base  = payload();
    byte_p  p     = base;
    size_t  size  = leb128<size_t>(p);
    size_t  count = 0;

    while (size)
    {
        object_p name = object_p(p);
        size_t   ns   = name->size();
        p += ns;
        object_p value = object_p(p);
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

        // Stash in a gcp: the callback may cause garbage collection
        base = p;
        if (!callback || callback(name, value, arg))
            count++;

        size -= (ns + vs);
        p = base;
    }

    return count;
}


bool directory::find(uint index, object_p &nref, object_p &vref) const
// ----------------------------------------------------------------------------
//   Return the name of the n-th element in directory
// ----------------------------------------------------------------------------
{
    object_p p     = object_p(payload());
    size_t   size  = leb128<size_t>(p);
    object_p name  = nullptr;
    object_p value = nullptr;

    index++;
    while (index && size)
    {
        name = p;
        size_t   ns   = name->size();
        p += ns;
        value = p;
        size_t   vs    = value->size();
        p += vs;

        // Defensive coding against malformed directorys
        if (ns + vs > size)
        {
            record(directory_error,
                   "Malformed directory searching name (ns=%u vs=%u size=%u)",
                   ns, vs, size);
            return false;     // Malformed directory, quick exit
        }

        size -= (ns + vs);
        index--;
    }
    if (index)
        name = value = nullptr;
    nref = name;
    vref = value;
    return index == 0;
}


object_p directory::name(uint index) const
// ----------------------------------------------------------------------------
//   Return name at given index
// ----------------------------------------------------------------------------
{
    object_p name, value;
    if (find(index, name, value))
        return name;
    return nullptr;
}


object_p directory::value(uint index) const
// ----------------------------------------------------------------------------
//   Return name at given index
// ----------------------------------------------------------------------------
{
    object_p name, value;
    if (find(index, name, value))
        return value;
    return nullptr;
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
    if (!rt.args(2))
        return ERROR;

    directory *dir = rt.variables(0);
    if (!dir)
    {
        rt.no_directory_error();
        return ERROR;
    }

    // Check that we have two objects in the stack
    object_p name = rt.stack(0);
    object_p value = rt.stack(1);
    if (name && value && dir->store(name, value))
    {
        rt.drop(2);
        return OK;
    }

    // Otherwise, return an error
    return ERROR;
}


COMMAND_BODY(Rcl)
// ----------------------------------------------------------------------------
//   Recall a global variable from current directory
// ----------------------------------------------------------------------------
{
    if (!rt.args(1))
        return ERROR;

    object_p name = rt.stack(0);
    if (!name)
        return ERROR;

    // Lookup all directorys, starting with innermost one
    if (object_p value = directory::recall_all(name))
        return rt.top(value) ? OK : ERROR;

    // Otherwise, return an error
    rt.undefined_name_error();
    return ERROR;
}


COMMAND_BODY(Purge)
// ----------------------------------------------------------------------------
//   Purge a global variable from current directory
// ----------------------------------------------------------------------------
{
    if (!rt.args(1))
        return ERROR;
    object_p name = rt.stack(0);
    if (!name)
        return ERROR;
    if (object_p quoted = name->as_quoted(ID_object))
        name = quoted;

    // Purge the object (HP48 doesn't error out if name does not exist)
    if (directory *dir = rt.variables(0))
        dir->purge(name);
    rt.drop();
    return OK;
}


COMMAND_BODY(PurgeAll)
// ----------------------------------------------------------------------------
//   Purge a global variable from current directory and enclosing directories
// ----------------------------------------------------------------------------
{
    if (!rt.args(1))
        return ERROR;
    object_p x = rt.stack(0);
    if (!x)
        return ERROR;
    symbol_g name = x->as_quoted<symbol>();
    if (!name)
    {
        rt.invalid_name_error();
        return ERROR;
    }
    rt.pop();

    // Lookup all directorys, starting with innermost one, and purge there
    directory *dir = nullptr;
    for (uint depth = 0; (dir = rt.variables(depth)); depth++)
        dir->purge(name);

    return OK;
}


COMMAND_BODY(Mem)
// ----------------------------------------------------------------------------
//    Return amount of available memory
// ----------------------------------------------------------------------------
//    The HP48 manual specifies that mem performs garbage collection
{
    if (!rt.args(0))
        return ERROR;
    rt.gc();
    run<FreeMemory>();
    return OK;
}


COMMAND_BODY(GarbageCollect)
// ----------------------------------------------------------------------------
//   Run the garbage collector
// ----------------------------------------------------------------------------
{
    if (rt.args(0))
    {
        size_t saved = rt.gc();
        integer_p result = rt.make<integer>(ID_integer, saved);
        if (rt.push(result))
            return OK;
    }
    return  ERROR;
}


COMMAND_BODY(FreeMemory)
// ----------------------------------------------------------------------------
//   Return amount of free memory (available without garbage collection)
// ----------------------------------------------------------------------------
{
    if (rt.args(0))
    {
        size_t available = rt.available();
        integer_p result = rt.make<integer>(ID_integer, available);
        if (rt.push(result))
            return OK;
    }
    return ERROR;
}


COMMAND_BODY(SystemMemory)
// ----------------------------------------------------------------------------
//   Return the amount of memory that is seen as free by the system
// ----------------------------------------------------------------------------
{
    if (rt.args(0))
    {
        size_t mem = sys_free_mem();
        integer_p result = rt.make<integer>(ID_integer, mem);
        if (rt.push(result))
            return OK;
    }
    return ERROR;
}


COMMAND_BODY(home)
// ----------------------------------------------------------------------------
//   Return the home directory
// ----------------------------------------------------------------------------
{
    if (!rt.args(0))
        return ERROR;
    rt.updir(~0U);
    ui.menu_refresh(ID_VariablesMenu);
    return OK;
}


COMMAND_BODY(CurrentDirectory)
// ----------------------------------------------------------------------------
//   Return the current directory as an object
// ----------------------------------------------------------------------------
{
    if (rt.args(0))
    {
        directory_p dir = rt.variables(0);
        if (rt.push(dir))
            return OK;
    }
    return ERROR;
}


static bool path_callback(object_p name, object_p obj, void *arg)
// ----------------------------------------------------------------------------
//   Find the directory in enclosing directory
// ----------------------------------------------------------------------------
{
    if (obj == object_p(arg))
    {
        rt.append(name->size(), byte_p(name));
        return true;
    }
    return false;
}


list_p directory::path(id type)
// ----------------------------------------------------------------------------
//   Return the current directory path as a list object of the given type
// ----------------------------------------------------------------------------
{
    scribble scr;

    size_t sz = leb128size(ID_home);
    byte *p = rt.allocate(sz);
    leb128(p, ID_home);

    uint depth = rt.directories();
    directory_p dir = rt.homedir();
    while (depth > 1)
    {
        depth--;
        directory_p next = rt.variables(depth-1);
        if (dir->enumerate(path_callback, (void *) next) != 1)
        {
            rt.directory_path_error();
            return nullptr;
        }
        dir = next;
    }

    list_p list = list::make(type, scr.scratch(), scr.growth());
    return list;
}


COMMAND_BODY(path)
// ----------------------------------------------------------------------------
//   Build a path with the list of paths
// ----------------------------------------------------------------------------
{
    if (rt.args(0))
        if (list_p list = directory::path())
            if (rt.push(list))
                return OK;

    return ERROR;
}


COMMAND_BODY(crdir)
// ----------------------------------------------------------------------------
//   Create a directory
// ----------------------------------------------------------------------------
{
    if (!rt.args(1))
        return ERROR;

    directory *dir = rt.variables(0);
    if (!dir)
    {
        rt.no_directory_error();
        return ERROR;
    }

    if (object_p obj = rt.pop())
    {
        symbol_p name = obj->as_quoted<symbol>();
        if (!name)
        {
            rt.invalid_name_error();
            return ERROR;
        }
        if (dir->recall(name))
        {
            rt.name_exists_error();
            return ERROR;
        }

        object_p newdir = rt.make<directory>();
        if (dir->store(name, newdir))
            return OK;
    }
    return ERROR;
}


COMMAND_BODY(updir)
// ----------------------------------------------------------------------------
//   Go up one directory
// ----------------------------------------------------------------------------
{
    if (!rt.args(0))
        return ERROR;
    rt.updir();
    ui.menu_refresh(ID_VariablesMenu);
    return OK;
}


COMMAND_BODY(pgdir)
// ----------------------------------------------------------------------------
//   Really the same as 'purge'
// ----------------------------------------------------------------------------
{
    return Purge::evaluate();
}





// ============================================================================
//
//    Variables menu
//
// ============================================================================

MENU_BODY(VariablesMenu)
// ----------------------------------------------------------------------------
//   Process the MENU command for VariablesMenu
// ----------------------------------------------------------------------------
{
    uint  nitems = count_variables();
    items_init(mi, nitems, 3, 1);
    list_variables(mi);
    return OK;
}


uint VariablesMenu::count_variables()
// ----------------------------------------------------------------------------
//    Count the variables in the current directory
// ----------------------------------------------------------------------------
{
    directory *dir = rt.variables(0);
    if (!dir)
    {
        rt.no_directory_error();
        return 0;
    }
    return dir->count();
}


static bool evaluate_variable(object_p name, object_p value, void *arg)
// ----------------------------------------------------------------------------
//   Add a variable to evaluate in the menu
// ----------------------------------------------------------------------------
{
    symbol_p disp = name->as<symbol>();
    if (!disp)
        disp = name->as_symbol(true);
    menu::info &mi = *((menu::info *) arg);
    if (value->as<directory>())
        mi.marker = L'◥';
    menu::items(mi, disp, menu::ID_VariablesMenuExecute);

    return true;
}


static bool recall_variable(object_p name, object_p UNUSED value, void *arg)
// ----------------------------------------------------------------------------
//   Add a variable to evaluate in the menu
// ----------------------------------------------------------------------------
//   For a name X, we create an object « 'Name' RCL »
{
    symbol_p disp = name->as<symbol>();
    if (!disp)
        disp = name->as_symbol(true);
    menu::info &mi = *((menu::info *) arg);
    menu::items(mi, disp, menu::ID_VariablesMenuRecall);
    return true;
}


static bool store_variable(object_p name, object_p UNUSED value, void *arg)
// ----------------------------------------------------------------------------
//   Add a variable to evaluate in the menu
// ----------------------------------------------------------------------------
{
    symbol_p disp = name->as<symbol>();
    if (!disp)
        disp = name->as_symbol(true);
    menu::info &mi = *((menu::info *) arg);
    menu::items(mi, disp, menu::ID_VariablesMenuStore);
    return true;
}



void VariablesMenu::list_variables(info &mi)
// ----------------------------------------------------------------------------
//   Fill the menu with variable names
// ----------------------------------------------------------------------------
{
    directory *dir = rt.variables(0);
    if (!dir)
    {
        rt.no_directory_error();
        return;
    }

    uint skip = mi.skip;
    mi.plane  = 0;
    mi.planes = 1;
    dir->enumerate(evaluate_variable, &mi);
    mi.plane  = 1;
    mi.planes = 2;
    mi.skip   = skip;
    mi.index  = mi.plane * ui.NUM_SOFTKEYS;
    dir->enumerate(recall_variable, &mi);
    mi.plane  = 2;
    mi.planes = 3;
    mi.index  = mi.plane * ui.NUM_SOFTKEYS;
    mi.skip   = skip;
    dir->enumerate(store_variable, &mi);

    for (uint k = 0; k < ui.NUM_SOFTKEYS - (mi.pages > 1); k++)
    {
        ui.marker(k + 1 * ui.NUM_SOFTKEYS, L'▶', false);
        ui.marker(k + 2 * ui.NUM_SOFTKEYS, L'▶', true);
    }
}


COMMAND_BODY(VariablesMenuExecute)
// ----------------------------------------------------------------------------
//   Recall a variable from the VariablesMenu
// ----------------------------------------------------------------------------
{
    int key = ui.evaluating;
    if (rt.editing())
    {
        if (ui.editing_mode() != ui.DIRECT)
            return ui.insert_softkey(key, "", " ");
        if (!ui.end_edit())
            return object::ERROR;
    }

    if (key >= KEY_F1 && key <= KEY_F6)
    {
        if (directory *dir = rt.variables(0))
        {
            uint     index = key - KEY_F1 + 5 * ui.page();
            object_p name, value;
            if (dir->find(index, name, value))
            {
                if (symbol_p sym = name->as_symbol(true))
                {
                    size_t sz = 0;
                    utf8 help = sym->value(&sz);
                    ui.draw_user_command(help, sz);
                }
                return program::run(value);
            }
        }
    }

    return ERROR;
}


COMMAND_BODY(VariablesMenuRecall)
// ----------------------------------------------------------------------------
//   Recall a variable from the VariablesMenu
// ----------------------------------------------------------------------------
{
    int key = ui.evaluating;
    if (rt.editing())
    {
        if (ui.editing_mode() != ui.DIRECT)
            return ui.insert_softkey(key, "'", "' Recall ");
        if (!ui.end_edit())
            return object::ERROR;
    }

    if (key >= KEY_F1 && key <= KEY_F6)
    {
        if (directory *dir = rt.variables(0))
        {
            uint index = key - KEY_F1 + 5 * ui.page();
            if (object_p value = dir->value(index))
                if (rt.push(value))
                    return OK;
        }
    }

    return ERROR;
}


COMMAND_BODY(VariablesMenuStore)
// ----------------------------------------------------------------------------
//   Store a variable from the VariablesMenu
// ----------------------------------------------------------------------------
{
    int key = ui.evaluating;
    if (rt.editing())
    {
        if (ui.editing_mode() != ui.DIRECT)
            return ui.insert_softkey(key, "'", "' Store ");
        if (!ui.end_edit())
            return object::ERROR;
    }

    if (key >= KEY_F1 && key <= KEY_F6)
    {
        if (directory *dir = rt.variables(0))
        {
            uint index = key - KEY_F1 + 5 * ui.page();
            if (object_p name = dir->name(index))
                if (object_p value = rt.pop())
                    if (dir->store(name, value))
                        return OK;
        }
    }
    return ERROR;
}
