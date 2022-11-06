#ifndef LOOPS_H
#define LOOPS_H
// ****************************************************************************
//  loops.h                                                       DB48X project
// ****************************************************************************
//
//   File Description:
//
//    Implement basic loops
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
//
// Payload format:
//
//   Loops have the same format:
//   - ID for the type
//   - Total length
//   - Condition object, typically a program
//   - Body object, typically a program, which is executed repeatedly


#include "command.h"

struct loop : command
// ----------------------------------------------------------------------------
//    Loop structures
// ----------------------------------------------------------------------------
{
    loop(gcobj body, id type);
    result condition(bool &value) const;

    static size_t required_memory(id i, gcobj body)
    {
        return leb128size(i) + body->size();
    }

    static intptr_t size(object_p obj, object_p payload)
    {
        payload = payload->skip();
        return ptrdiff(payload, obj);
    }

    static bool interrupted()   { return program::interrupted(); }

protected:
    // Shared code for parsing and rendering, taking delimiters as input
    intptr_t object_renderer(renderer &r, runtime &rt,
                             uint nsep, cstring seps[]) const;
    static result object_parser(id type, parser UNUSED &p, runtime &rt,
                                uint nsep, cstring seps[]);
    intptr_t object_renderer(renderer &r, runtime &rt,
                             cstring beg, cstring end) const;
    static result object_parser(id type, parser UNUSED &p, runtime &rt,
                                cstring beg, cstring end);
    static result counted(gcobj body, runtime &rt, bool stepping);
};


struct conditional_loop : loop
// ----------------------------------------------------------------------------
//    Loop structures
// ----------------------------------------------------------------------------
{
    conditional_loop(gcobj condition, gcobj body, id type);
    result condition(bool &value) const;

    static size_t required_memory(id i, gcobj condition, gcobj body)
    {
        return leb128size(i) + condition->size() + body->size();
    }

    static intptr_t size(object_p obj, object_p payload)
    {
        payload = payload->skip()->skip();
        return ptrdiff(payload, obj);
    }

protected:
    // Shared code for parsing and rendering, taking delimiters as input
    intptr_t object_renderer(renderer &r, runtime &rt,
                             cstring beg, cstring mid, cstring end) const;
    static result object_parser(id type, parser UNUSED &p, runtime &rt,
                                cstring beg, cstring mid, cstring end);
    static result counted(gcobj body, runtime &rt, bool stepping);
};


struct DoUntil : conditional_loop
// ----------------------------------------------------------------------------
//   do...until...end loop
// ----------------------------------------------------------------------------
{
    DoUntil(gcobj condition, gcobj body, id type)
        : conditional_loop(condition, body, type) {}

    result execute(runtime &rt) const;
    OBJECT_HANDLER(DoUntil);
    OBJECT_PARSER(DoUntil);
    OBJECT_RENDERER(DoUntil);
};


struct WhileRepeat : conditional_loop
// ----------------------------------------------------------------------------
//   while...repeat...end loop
// ----------------------------------------------------------------------------
{
    WhileRepeat(gcobj condition, gcobj body, id type)
        : conditional_loop(condition, body, type) {}

    result execute(runtime &rt) const;
    OBJECT_HANDLER(WhileRepeat);
    OBJECT_PARSER(WhileRepeat);
    OBJECT_RENDERER(WhileRepeat);
};


struct StartNext : loop
// ----------------------------------------------------------------------------
//   start..next loop
// ----------------------------------------------------------------------------
{
    StartNext(gcobj body, id type): loop(body, type) {}

    result execute(runtime &rt) const;
    OBJECT_HANDLER(StartNext);
    OBJECT_PARSER(StartNext);
    OBJECT_RENDERER(StartNext);
};


struct StartStep : StartNext
// ----------------------------------------------------------------------------
//   start..step loop
// ----------------------------------------------------------------------------
{
    StartStep(gcobj body, id type): StartNext(body, type) {}

    result execute(runtime &rt) const;
    OBJECT_HANDLER(StartStep);
    OBJECT_PARSER(StartStep);
    OBJECT_RENDERER(StartStep);
};


struct ForNext : StartNext
// ----------------------------------------------------------------------------
//   for..next loop
// ----------------------------------------------------------------------------
{
    ForNext(gcobj body, id type): StartNext(body, type) {}

    result execute(runtime &rt) const;
    OBJECT_HANDLER(ForNext);
    OBJECT_PARSER(ForNext);
    OBJECT_RENDERER(ForNext);
};


struct ForStep : ForNext
// ----------------------------------------------------------------------------
//   for..step loop
// ----------------------------------------------------------------------------
{
    ForStep(gcobj body, id type): ForNext(body, type) {}

    result execute(runtime &rt) const;
    OBJECT_HANDLER(ForStep);
    OBJECT_PARSER(ForStep);
    OBJECT_RENDERER(ForStep);
};

#endif // LOOPS_H
