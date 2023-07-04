#ifndef TEXT_H
#define TEXT_H
// ****************************************************************************
//  text.h                                                  DB48X project
// ****************************************************************************
//
//   File Description:
//
//      The RPL text object type
//
//      Operations on text values
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
//   The text object is a sequence of bytes containing:
//   - The type ID (one byte)
//   - The LEB128-encoded length of the text (one byte in most cases)
//   - The characters of the text, not null-terminated
//
//   On most texts, this format uses 3 bytes less than on the HP48.

#include "object.h"
#include "runtime.h"
#include "text.h"


struct text : object
// ----------------------------------------------------------------------------
//    Represent text objects
// ----------------------------------------------------------------------------
{
    text(gcutf8 source, size_t len, id type = ID_text): object(type)
    {
        utf8 s = (utf8) source;
        byte *p = (byte *) payload(this);
        p = leb128(p, len);
        while (len--)
            *p++ = *s++;
    }

    static size_t required_memory(id i, gcutf8 UNUSED str, size_t len)
    {
        return leb128size(i) + leb128size(len) + len;
    }

    static text *make(utf8 str, size_t len)
    {
        gcutf8 gcstr = str;
        return rt.make<text>(gcstr, len);
    }

    static text *make(utf8 str)
    {
        return make(str, strlen(cstring(str)));
    }

    static text *make(cstring str, size_t len)
    {
        return make(utf8(str), len);
    }

    size_t length() const
    {
        byte_p p = payload(this);
        return leb128<size_t>(p);
    }

    static intptr_t size(object_p obj, object_p payload)
    {
        byte *p = (byte *) payload;
        size_t len = leb128<size_t>(p);
        p += len;
        return ptrdiff(p, obj);
    }

    utf8 value(size_t *size = nullptr) const
    {
        byte_p p   = payload(this);
        size_t len = leb128<size_t>(p);
        if (size)
            *size = len;
        return (utf8) p;
    }

    text_p import() const;      // Import text containing << or >> or ->

public:
    OBJECT_DECL(text);
    PARSE_DECL(text);
    SIZE_DECL(text);
    RENDER_DECL(text);
};

typedef const text     *text_p;
typedef gcp<const text> text_g;

// Some operators on texts
text_g operator+(text_g x, text_g y);
text_g operator*(text_g x, uint y);

#endif // TEXT_H
