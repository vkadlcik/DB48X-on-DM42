// ****************************************************************************
//  program.cc                                                    DB48X project
// ****************************************************************************
//
//   File Description:
//
//     Implementation of RPL programs and blocks
//
//     Programs are lists with a special way to execute
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

#include "program.h"
#include "parser.h"
#include "settings.h"

RECORDER(program, 16, "Program evaluation");


bool program::running = false;

// ============================================================================
//
//    Program
//
// ============================================================================

EVAL_BODY(program)
// ----------------------------------------------------------------------------
//   Normal evaluation from a program places it on stack
// ----------------------------------------------------------------------------
{
    if (running)
        return rt.push(o) ? OK : ERROR;
    return o->run_program();
}


PARSE_BODY(program)
// ----------------------------------------------------------------------------
//    Try to parse this as a program
// ----------------------------------------------------------------------------
{
    return list_parse(ID_program, p, L'«', L'»');
}


RENDER_BODY(program)
// ----------------------------------------------------------------------------
//   Render the program into the given program buffer
// ----------------------------------------------------------------------------
{
    return o->list_render(r, L'«', L'»');
}


program_p program::parse(utf8 source, size_t size)
// ----------------------------------------------------------------------------
//   Parse a program without delimiters (e.g. command line)
// ----------------------------------------------------------------------------
{
    record(program, ">Parsing command line [%s]", source);
    parser p(source, size);
    result r = list_parse(ID_program, p, 0, 0);
    record(program, "<Command line [%s], end at %u, result %p",
           utf8(p.source), p.end, object_p(p.out));
    if (r != OK)
        return nullptr;
    object_p  obj  = p.out;
    if (!obj)
        return nullptr;
    program_p prog = obj->as<program>();
    return prog;
}


#ifdef DM42
#  pragma GCC push_options
#  pragma GCC optimize("-O3")
#endif // DM42

object::result program::run(bool synchronous) const
// ----------------------------------------------------------------------------
//   Execute a program
// ----------------------------------------------------------------------------
//   The 'save_last_args' indicates if we save `LastArgs` at this level
{
    result   result    = OK;
    size_t   depth     = rt.call_depth();
    bool     outer     = depth == 0 && !running;
    object_p first     = objects();
    object_p end       = skip();
    bool     last_args = outer ? Settings.save_last : Settings.prog_save_last;

    record(program, "Run %p (%p-%p) %+s",
           this, first, end, outer ? "outer" : "inner");
    if (!rt.run_push(first, end))
        result = ERROR;
    if (outer || synchronous)
    {
        save<bool> save_running(running, true);
        while (object_p obj = rt.run_next(depth))
        {
#if 0
            printf("%zu: [%u] %s | ", depth, rt.depth(),
                   rt.depth() ? rt.stack(0)->debug() : "<nullptr>");
            printf("%s\n", obj->debug());
#endif
            record(program, "Evaluating %+s at %p, size %u, end=%p\n",
                   obj->fancy(), obj, obj->size(), end);
            if (interrupted())
            {
                rt.interrupted_error().command(obj->fancy());
                result = ERROR;
            }
            if (result == OK)
            {
                if (last_args)
                    rt.need_save();
                result = obj->evaluate();
            }
        }
    }

    return result;
}

#ifdef DM42
#  pragma GCC pop_options
#endif // DM42


object::result program::run(object_p obj, bool sync)
// ----------------------------------------------------------------------------
//    Run a program as top-level
// ----------------------------------------------------------------------------
{
    if (program_p prog = obj->as_program())
        return prog->run(sync);
    return obj->evaluate();
}



// ============================================================================
//
//    Block
//
// ============================================================================

PARSE_BODY(block)
// ----------------------------------------------------------------------------
//  Blocks are parsed in structures like loops, not directly
// ----------------------------------------------------------------------------
{
    return SKIP;
}


RENDER_BODY(block)
// ----------------------------------------------------------------------------
//   Render the program into the given program buffer
// ----------------------------------------------------------------------------
{
    return o->list_render(r, 0, 0);
}


EVAL_BODY(block)
// ----------------------------------------------------------------------------
//   Normal evaluation from a program places it on stack
// ----------------------------------------------------------------------------
{
    return o->run_program();
}
