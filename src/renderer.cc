// ****************************************************************************
//  renderer.cc                                                   DB48X project
// ****************************************************************************
//
//   File Description:
//
//     Structure used to render objects
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

#include "renderer.h"
#include "runtime.h"

#include <stdio.h>
#include <stdarg.h>


renderer::~renderer()
// ----------------------------------------------------------------------------
//   When we used the scratchpad, free memory used
// ----------------------------------------------------------------------------
{
    runtime &rt = runtime::RT;
    if (!target)
        rt.free(written);
}


bool renderer::put(char c)
// ----------------------------------------------------------------------------
//   Write a single character
// ----------------------------------------------------------------------------
{
    if (written >= length)
        return false;

    // Render flat for stack display
    if (c == '\n' && flat)
        c = ' ';

    if (target)
    {
        target[written++] = c;
    }
    else
    {
        runtime &rt = runtime::RT;
        byte *p = rt.allocate(1);
        if (!p)
            return false;
        *p = c;
        written++;
    }
    if (c == '\n')
    {
        for (uint i = 0; i < tabs; i++)
            if (!put('\t'))
                return false;
    }
    return true;
}


size_t renderer::printf(const char *format, ...)
// ----------------------------------------------------------------------------
//   Write a formatted string
// ----------------------------------------------------------------------------
{
    if (target)
    {
        // Fixed target: write directly there
        if (written >= length)
            return 0;

        va_list va;
        va_start(va, format);
        size_t remaining = length - written;
        size_t size = vsnprintf(target + written, remaining, format, va);
        va_end(va);
        written += size;
        return size;
    }
    else
    {
        // Write in the scratchpad
        runtime &rt = runtime::RT;
        char buf[32];
        va_list va;
        va_start(va, format);
        size_t size = vsnprintf(buf, sizeof(buf), format, va);
        va_end(va);

        // Check if we can allocate enough for the output
        byte *p = rt.allocate(size);
        if (!p)
            return 0;

        if (size < sizeof(buf))
        {
            // Common case: it fits in 32-bytes buffer, allocate directly
            memcpy(p, buf, size);
        }
        else
        {
            // Uncommon case: re-run vsnprintf in the allocated buffer
            va_list va;
            va_start(va, format);
            size = vsnprintf((char *) p, size, format, va);
            va_end(va);
        }
        written += size;
        return size;
    }
}


utf8 renderer::text() const
// ----------------------------------------------------------------------------
//   Return the buffer of what written in the renderer
// ----------------------------------------------------------------------------
{
    if (target)
        return (utf8) target;
    runtime &rt = runtime::RT;
#ifdef SIMULATOR
    *rt.scratchpad() = 0;
#endif // SIMULATOR
    return (utf8) rt.scratchpad() - written;
}
