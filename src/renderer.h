#ifndef RENDERER_H
#define RENDERER_H
// ****************************************************************************
//  renderer.h                                                    DB48X project
// ****************************************************************************
//
//   File Description:
//
//     Structure used to record information about rendering
//
//     This works in two modes:
//     - Write to a fixed-size buffer, e.g. while rendering stack
//     - Write to the scratchpad, e.g. to edit
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
#include "utf8.h"


struct renderer
// ----------------------------------------------------------------------------
//  Arguments to the RENDER command
// ----------------------------------------------------------------------------
{
    renderer(char *buffer = nullptr, size_t length = ~0U)
        : target(buffer), length(length), written(0), tabs(0),
          eq(false) {}
    renderer(bool equation)
        : target(nullptr), length(0), written(0), tabs(0),
          eq(equation) {}
    ~renderer();

    bool put(char c);
    bool put(cstring s)
    {
        for (char c = *s++; c; c = *s++)
            if (!put(c))
                return false;
        return true;
    }
    bool put(cstring s, size_t len)
    {
        for (size_t i = 0; i < len; i++)
            if (!put(s[i]))
                return false;
        return true;
    }
    bool put(unicode code)
    {
        byte buffer[4];
        size_t rendered = utf8_encode(code, buffer);
        return put(buffer, rendered);
    }
    bool   put(utf8 s)                  { return put(cstring(s)); }
    bool   put(utf8 s, size_t len)      { return put(cstring(s), len); }



    bool   editing() const              { return target == nullptr; }
    bool   equation() const             { return eq; }
    size_t size() const                 { return written; }
    utf8   text() const;

    size_t printf(const char *format, ...);
    void   indent(int i)
    {
        tabs += i;
    }
    bool   indent()
    {
        indent(1);
        return put('\n');
    }
    bool   unindent()
    {
        indent(-1);
        return put('\n');
    }

protected:
    char        *target;        // Buffer where we render the object, or nullptr
    size_t      length;         // Available space
    size_t      written;        // Number of bytes written
    uint        tabs;           // Amount of indent
    bool        eq : 1;         // As equation
};

#endif // RENDERER_H
