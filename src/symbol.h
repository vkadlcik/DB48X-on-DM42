#ifndef SYMBOL_H
#define SYMBOL_H
// ****************************************************************************
//  symbol.h                                                     DB48X project
// ****************************************************************************
//
//   File Description:
//
//      RPL names / symbols
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

#include "object.h"
#include "runtime.h"

struct symbol : object
// ----------------------------------------------------------------------------
//    Represent symbol objects
// ----------------------------------------------------------------------------
{
    symbol(gcutf8 source, size_t len, id type = ID_symbol): object(type)
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

    OBJECT_HANDLER(symbol);
    OBJECT_PARSER(symbol);
    OBJECT_RENDERER(symbol);
};

typedef const symbol *symbol_p;

#endif // SYMBOL_H
