#ifndef RPLSTRING_H
#define RPLSTRING_H
// ****************************************************************************
//  rplstring.h                                                  DB48X project
// ****************************************************************************
//
//   File Description:
//
//      The RPL string object type
//
//      Operations on rplstring values
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
//   The string object is a sequence of bytes containing:
//   - The type ID (one byte)
//   - The LEB128-encoded length of the string (one byte in most cases)
//   - The characters of the string, not null-terminated
//
//   On most strings, this format uses 3 bytes less than on the HP48.

#include "object.h"
#include "runtime.h"

struct string : object
// ----------------------------------------------------------------------------
//    Represent string objects
// ----------------------------------------------------------------------------
{
    string(gcutf8 source, size_t len, id type = ID_string): object(type)
    {
        utf8 s = (utf8) source;
        byte *p = (byte *) payload();
        p = leb128(p, len);
        while (len--)
            *p++ = *s++;
    }

    static size_t required_memory(id i, gcutf8 UNUSED str, size_t len)
    {
        return leb128size(i) + leb128size(len) + len;
    }

    static string *make(utf8 str, size_t len)
    {
        gcutf8 gcstr = str;
        return RT.make<string>(gcstr, len);
    }

    static string *make(utf8 str)
    {
        return make(str, strlen(cstring(str)));
    }

    size_t length() const
    {
        byte *p = payload();
        return leb128<size_t>(p);
    }

    utf8 text(size_t *size = nullptr) const
    {
        byte  *p   = payload();
        size_t len = leb128<size_t>(p);
        if (size)
            *size = len;
        return (utf8) p;
    }

    OBJECT_HANDLER(string);
    OBJECT_PARSER(string);
    OBJECT_RENDERER(string);
};

typedef const string *string_p;

#endif // STRING_H
