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
//
// Payload format:
//
//   The symbol object is a sequence of bytes containing:
//   - The type ID (one byte)
//   - The LEB128-encoded length of the name (one byte in most cases)
//   - The characters of the name, not null-terminated
//
//   On most strings, this format uses 3 bytes less than on the HP48.
//   This representation allows arbitrary symbol names, including names with
//   weird UTF-8 symbols in them, such as ΣDATA or ∱√π²≄∞
//

#include "object.h"
#include "rplstring.h"

struct symbol : string
// ----------------------------------------------------------------------------
//    Represent symbol objects
// ----------------------------------------------------------------------------
{
    symbol(gcutf8 source, size_t len, id type = ID_symbol):
        string(source, len, type)
    { }

    OBJECT_HANDLER(symbol);
    OBJECT_PARSER(symbol);
    OBJECT_RENDERER(symbol);
};

typedef const symbol *symbol_p;

#endif // SYMBOL_H
