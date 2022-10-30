// ****************************************************************************
//  variables.cc                                                  DB48X project
// ****************************************************************************
//
//   File Description:
//
//      Implementation of variables
//
//      Global variables are stored in mutable catalog objects that occupy
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
#include "parser.h"
#include "renderer.h"

RECORDER(catalog,       16, "Catalogs");
RECORDER(catalog_error, 16, "Errors from catalogs");

OBJECT_HANDLER_BODY(catalog)
// ----------------------------------------------------------------------------
//    Handle commands for catalogs
// ----------------------------------------------------------------------------
{
    switch(op)
    {
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
        return (intptr_t) "catalog";

    default:
        // Check if anyone else knows how to deal with it
        return DELEGATE(list);
    }
}


OBJECT_PARSER_BODY(catalog)
// ----------------------------------------------------------------------------
//    Try to parse this as a catalog
// ----------------------------------------------------------------------------
//    Catalog should never be parsed, but do something sensible if it happens
{
    return SKIP;
}


OBJECT_RENDERER_BODY(catalog)
// ----------------------------------------------------------------------------
//   Render the catalog into the given catalog buffer
// ----------------------------------------------------------------------------
{
    return snprintf(r.target, r.length, "Catalog (internal)");
}


bool catalog::store(gcobj name, gcobj value)
// ----------------------------------------------------------------------------
//    Store an object in the catalog
// ----------------------------------------------------------------------------
//    Note that the catalog itself should never move because of GC
//    That's because it normally should reside in the globals area
{
    runtime &rt     = runtime::RT;
    object_p header = (object_p) payload();
    object_p body   = header;
    size_t   old    = leb128<size_t>(body);     // Old size of catalog
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

        // Move memory above storage if necessary
        if (vs != es)
            rt.move_globals((object_p) evalue + vs, (object_p) evalue + es);

        // Copy new value into storage location
        memmove((byte *) evalue, (byte *) value, vs);

        // Compute new size of the catalog
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

        // Move memory above end of catalog
        object_p end = body + old;
        rt.move_globals(end + requested, end);

        // Copy name and value at end of catalog
        memmove((byte *) end, (byte *) name, ns);
        memmove((byte *) end + ns, (byte *) value, vs);

        // Compute new size of the catalog
        now += requested;
    }

    // Adjust catalog size
    size_t nowh = leb128size(now);
    size_t oldh = leb128size(old);
    if (nowh != oldh)
        // Header size changed, move the catalog contents and rest of globals
        rt.move_globals(header + nowh, header + oldh);
    leb128(header, nowh);

    return true;
}


object_p catalog::lookup(object_p ref) const
// ----------------------------------------------------------------------------
//   Find if the name exists in the catalog, if so return pointer to it
// ----------------------------------------------------------------------------
{
    byte_p p = payload();
    size_t size = leb128<size_t>(p);
    size_t rsize = ref->size();

    while (size)
    {
        object_p name = (object_p) p;
        size_t ns = name->size();
        if (name == ref)          // Optimization when name is from catalog
            return name;
        if (ns == rsize && memcmp(name, ref, rsize) == 0)
            return name;

        p += ns;
        object_p value = (object_p) p;
        size_t vs = value->size();
        p += vs;

        // Defensive coding against malformed catalogs
        if (ns + vs > size)
        {
            record(catalog_error,
                   "Malformed catalog (ns=%u vs=%u size=%u)", ns, vs, size);
            return nullptr;     // Malformed catalog, quick exit
        }

        size -= (ns + vs);
    }

    return nullptr;
}


object_p catalog::recall(object_p ref) const
// ----------------------------------------------------------------------------
//   If the referenced object exists in catalog, return associated value
// ----------------------------------------------------------------------------
{
    if (object_p found = lookup(ref))
        // The value follows the name
        return found->skip();
    return nullptr;
}


size_t catalog::purge(object_p ref)
// ----------------------------------------------------------------------------
//    Purge a name (and associated value) from the catalog
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
            record(catalog_error,
                   "Purging %u bytes in %u bytes catalog", purged, old);
            purged = old;
        }

        // Update header
        object_p header = (object_p) payload();
        size_t now = old - purged;
        size_t oldh = leb128size(old);
        size_t nowh = leb128size(now);
        if (nowh > oldh)
            record(catalog_error,
                   "Purge increased catalog size from %u to %u", oldh, nowh);
        if (nowh < oldh)
            // Rare case where the catalog size itself uses less bytes
            rt.move_globals(header + nowh, header + oldh);
        leb128(header, nowh);

        return purged;
    }

    // If nothing purged, return 0
    return 0;
}
