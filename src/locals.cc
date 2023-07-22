// ****************************************************************************
//  locals.cc                                                     DB48X project
// ****************************************************************************
//
//   File Description:
//
//
//
//
//
//
//
//
//
//
// ****************************************************************************
//   (C) 2023 Christophe de Dinechin <christophe@dinechin.org>
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

#include "locals.h"

#include "equation.h"
#include "parser.h"
#include "program.h"
#include "renderer.h"
#include "symbol.h"
#include "utf8.h"

#include <strings.h>


locals_stack *locals_stack::stack = nullptr;



// ============================================================================
//
//   Implementation of program with local variables
//
// ============================================================================

static inline bool is_program_separator(unicode cp)
// ----------------------------------------------------------------------------
//   Check if a given unicode character can begin a program object
// ----------------------------------------------------------------------------
{
    return cp == L'«'           // Program object
        || cp == '\''           // Equation
        || cp == '{';           // List
}


PARSE_BODY(locals)
// ----------------------------------------------------------------------------
//    Try to parse this as a block with locals
// ----------------------------------------------------------------------------
{
    gcutf8  s   = p.source;
    size_t  max = p.length;
    unicode cp  = utf8_codepoint(s);
    if (cp != L'→' && cp != L'▶')
        return SKIP;
    s = utf8_next(s);

    // Check that we have a space after that, could be →List otherwise
    cp = utf8_codepoint(s);
    if (!utf8_whitespace(cp))
        return SKIP;

    // Parse the names
    scribble scr;
    size_t   names  = 0;
    gcmbytes countp = rt.scratchpad();
    byte     encoding[4];

    while (utf8_more(p.source, s, max))
    {
        cp = utf8_codepoint(s);
        if (utf8_whitespace(cp))
        {
            s = utf8_next(s);
            continue;
        }
        if (is_program_separator(cp))
        {
            break;
        }
        if (!is_valid_as_name_initial(cp))
        {
            rt.syntax_error().source(s).command("locals");
            return ERROR;
        }

        // Allocate byte for name length
        gcmbytes lengthp = rt.scratchpad();
        size_t namelen = 0;
        while (is_valid_in_name(cp) && utf8_more(p.source, s, max))
        {
            size_t cplen = utf8_encode(cp, encoding);
            gcbytes namep = rt.allocate(cplen);
            if (!namep)
                return ERROR;
            memcpy(namep, encoding, cplen);
            namelen += cplen;
            s += cplen;
            cp = utf8_codepoint(s);
        }

        // Encode name
        size_t lsize = leb128size(namelen);
        gcbytes endp = rt.allocate(lsize);
        if (!endp)
            return ERROR;
        byte *lp = lengthp;
        memmove(lp + lsize, lp, namelen);
        leb128(lp, namelen);

        // Count names
        names++;
    }

    // If we did not get a program after the names, fail
    if (!is_program_separator(cp))
    {
        rt.syntax_error().command("locals").source(s);
        return ERROR;
    }

    // Encode number of names
    size_t csz  = leb128size(names);
    byte *end = rt.allocate(csz);
    if (!end)
        return ERROR;
    byte  *cntp = countp;
    size_t sz  = end - cntp;
    memmove(cntp + csz, cntp, sz);
    leb128(cntp, names);

    // Build the program with the context pointing to the names
    locals_stack frame((byte_p)countp);
    size_t decls = utf8(s) - utf8(p.source);
    p.source += decls;
    p.length -= decls;

    object::result result = ERROR;
    switch(cp)
    {
    case L'«':  result = program ::do_parse(p); break;
    case  '\'': result = equation::do_parse(p); break;
    case '{':   result = list    ::do_parse(p); break;
    default:                                    break;
    }
    if (result != OK)
        return result;

    // Copy the program to the scratchpad
    object_g pgm = p.out;
    if (!pgm)
        return ERROR;
    sz = pgm->size();
    end = rt.allocate(sz);
    memmove(end, byte_p(pgm), sz);

    // Compute total number of bytes in payload and build object
    gcbytes scratch = scr.scratch();
    size_t alloc = scr.growth();
    p.out = rt.make<locals>(ID_locals, scratch, alloc);

    // Adjust size of parsed text for what we parsed before program
    p.end += decls;

    return OK;
}


RENDER_BODY(locals)
// ----------------------------------------------------------------------------
//   Render the program into the given program buffer
// ----------------------------------------------------------------------------
{
    // Skip object size
    gcbytes p = o->payload();
    size_t  objsize = leb128<size_t>(p.Safe());
    (void) objsize;

    // Create a local frame for rendering local names
    locals_stack frame(p);

    // Emit header
    r.put("→ ");

    // Loop on names
    size_t names = leb128<size_t>(p.Safe());
    for (size_t n = 0; n < names; n++)
    {
        size_t len = leb128<size_t>(p.Safe());
        r.put(p.Safe(), len);
        r.put(' ');
        p += len;
    }

    // Render object (which should be a program, an equation or a list)
    object_p obj = object_p(p.Safe());
    return obj->render(r);
}


EVAL_BODY(locals)
// ----------------------------------------------------------------------------
//   Evaluate a program with locals (executes the code)
// ----------------------------------------------------------------------------
{
    object_g p   = object_p(o->payload());
    size_t   len = leb128<size_t>(p.Safe());
    (void) len;

    // Copy local values from stack
    size_t names   = leb128<size_t>(p.Safe());
    if (!rt.locals(names))
        return ERROR;

    // Skip names to get to program
    for (uint n = 0; n < names; n++)
    {
        size_t nlen = leb128<size_t>(p.Safe());
        p += nlen;
    }

    // Execute result
    result res = p->execute();

    // Remove locals
    rt.unlocals(names);

    // Return result from execution
    return res;
}



// ============================================================================
//
//  Implementation of local name
//
// ============================================================================

SIZE_BODY(local)
// ----------------------------------------------------------------------------
//  Compute size for a local object
// ----------------------------------------------------------------------------
{
    byte_p p = o->payload();
    return ptrdiff(p, o) + leb128size(p);
}


PARSE_BODY(local)
// ----------------------------------------------------------------------------
//    Check if we have local names, and check if there is a match
// ----------------------------------------------------------------------------
{
    utf8 source = p.source;
    utf8 s      = source;

    // First character must be alphabetic
    unicode cp = utf8_codepoint(s);
    if (!is_valid_as_name_initial(cp))
        return SKIP;

    // Check what is acceptable in a name
    while (is_valid_in_name(s))
        s = utf8_next(s);
    size_t len = s - source;

    // Check all the locals currently in effect
    size_t index = 0;
    for (locals_stack *f = locals_stack::current(); f; f = f->enclosing())
    {
        // Need to null-check here because we create null locals parsing 'for'
        if (gcbytes names = f->names())
        {
            // Check if name is found in local frame
            size_t count = leb128<size_t>(names.Safe());
            for (size_t n = 0; n < count; n++)
            {
                size_t nlen = leb128<size_t>(names.Safe());
                if (nlen == len &&
                    strncasecmp(cstring(names.Safe()), cstring(source), nlen) == 0)
                {
                    // Found a local name, return it
                    gcutf8 text   = source;
                    p.end         = len;
                    p.out         = rt.make<local>(ID_local, index);
                    return OK;
                }
                names += nlen;
                index++;
            }
        }
    }

    // Not found in locals, treat as a global name
    return SKIP;
}


RENDER_BODY(local)
// ----------------------------------------------------------------------------
//   Render a local name
// ----------------------------------------------------------------------------
{
    gcbytes p = o->payload();
    uint index = leb128<uint>(p.Safe());

    for (locals_stack *f = locals_stack::current(); f; f = f->enclosing())
    {
        gcbytes names = f->names();

        // Check if name is found in local frame
        size_t count = leb128<size_t>(names.Safe());
        if (index >= count)
        {
            // Name is beyond current frame, skip to next one
            index -= count;
            continue;
        }

        // Skip earlier names in index
        for (size_t n = 0; n < index; n++)
        {
            size_t len = leb128<size_t>(names.Safe());
            names += len;
        }

        // Emit name and exit
        size_t len = leb128<size_t>(names.Safe());
        r.put(names.Safe(), len);
        return r.size();
    }

    // We have not found the name, render bogus name
    r.printf("InvalidLocalName%u", index);
    return r.size();
}


EVAL_BODY(local)
// ----------------------------------------------------------------------------
//   Evaluate a local by fetching it from locals area and putting it on stack
// ----------------------------------------------------------------------------
{
    if (object_g obj = o->recall())
        if (rt.push(obj))
            return OK;
    return ERROR;
}


EXEC_BODY(local)
// ----------------------------------------------------------------------------
//   Execute a local by fetching it from locals and executing it
// ----------------------------------------------------------------------------
{
    if (object_g obj = o->recall())
        return obj->execute();
    return ERROR;
}
