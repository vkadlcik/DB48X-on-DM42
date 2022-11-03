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


#include "list.h"

struct loop : program
// ----------------------------------------------------------------------------
//    Loop structures
// ----------------------------------------------------------------------------
{
    loop(gcbytes bytes, size_t len, id type): program(bytes, len, type) {}

    result condition(bool &value) const;

protected:
    // Shared code for parsing and rendering, taking delimiters as input
    intptr_t object_renderer(renderer &r, runtime &rt,
                             cstring beg, cstring end, cstring mid = 0) const;
    static result object_parser(id type, parser UNUSED &p, runtime &rt,
                                cstring beg, cstring end, cstring mid = 0);
    static result counted(gcobj body, runtime &rt, bool stepping);
};


struct DoUntil : loop
// ----------------------------------------------------------------------------
//   do...until...end loop
// ----------------------------------------------------------------------------
{
    DoUntil(gcbytes bytes, size_t len, id type): loop(bytes, len, type) {}

    result execute(runtime &rt) const;
    OBJECT_HANDLER(DoUntil);
    OBJECT_PARSER(DoUntil);
    OBJECT_RENDERER(DoUntil);
};


struct WhileRepeat : loop
// ----------------------------------------------------------------------------
//   while...repeat...end loop
// ----------------------------------------------------------------------------
{
    WhileRepeat(gcbytes bytes, size_t len, id type): loop(bytes, len, type) {}

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
    StartNext(gcbytes bytes, size_t len, id type): loop(bytes, len, type) {}

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
    StartStep(gcbytes by, size_t len, id type): StartNext(by, len, type) {}

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
    ForNext(gcbytes bytes, size_t len, id type): StartNext(bytes, len, type) {}

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
    ForStep(gcbytes bytes, size_t len, id type): ForNext(bytes, len, type) {}

    result execute(runtime &rt) const;
    OBJECT_HANDLER(ForStep);
    OBJECT_PARSER(ForStep);
    OBJECT_RENDERER(ForStep);
};

#endif // LOOPS_H
