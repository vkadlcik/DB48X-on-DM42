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
#include "precedence.h"
#include "text.h"
#include "utf8.h"


GCP(symbol);

struct symbol : text
// ----------------------------------------------------------------------------
//    Represent symbol objects
// ----------------------------------------------------------------------------
{
    symbol(id type, gcutf8 source, size_t len): text(type, source, len)
    { }

    static symbol_g make(char c)
    {
        return rt.make<symbol>(ID_symbol, utf8(&c), 1);
    }

    static symbol_g make(cstring s)
    {
        return rt.make<symbol>(ID_symbol, utf8(s), strlen(s));
    }

    object_p recall(bool noerror = true) const;
    bool     store(object_g obj) const;

public:
    OBJECT_DECL(symbol);
    PARSE_DECL(symbol);
    EVAL_DECL(symbol);
    EXEC_DECL(symbol);
    RENDER_DECL(symbol);
    PREC_DECL(SYMBOL);
};

symbol_g operator+(symbol_r x, symbol_r y);


inline bool is_valid_as_name_initial(unicode cp)
// ----------------------------------------------------------------------------
//   Check if character is valid as initial of a name
// ----------------------------------------------------------------------------
{
    return (cp >= 'A' && cp <= 'Z')
        || (cp >= 'a' && cp <= 'z')
        || (cp >= 0x100 &&
            (cp != L'÷' &&      // Exclude symbols you can't have in a name
             cp != L'×' &&
             cp != L'↑' &&
             cp != L'∂' &&
             cp != L'⁻' &&
             cp != L'¹' &&
             cp != L'²' &&
             cp != L'³' &&
             cp != L'ⅈ' &&
             cp != L'∡'));
}


inline bool is_valid_as_name_initial(utf8 s)
// ----------------------------------------------------------------------------
//   Check if first character in a string is valid in a name
// ----------------------------------------------------------------------------
{
    return is_valid_as_name_initial(utf8_codepoint(s));
}


inline bool is_valid_in_name(unicode cp)
// ----------------------------------------------------------------------------
//   Check if character is valid in a name
// ----------------------------------------------------------------------------
{
    return is_valid_as_name_initial(cp)
        || (cp >= '0' && cp <= '9');
}


inline bool is_valid_in_name(utf8 s)
// ----------------------------------------------------------------------------
//   Check if first character in a string is valid in a name
// ----------------------------------------------------------------------------
{
    return is_valid_in_name(utf8_codepoint(s));
}

#endif // SYMBOL_H
