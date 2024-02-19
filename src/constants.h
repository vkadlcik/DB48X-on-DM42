#ifndef CONSTANTS_H
#define CONSTANTS_H
// ****************************************************************************
//  constants.h                                                   DB48X project
// ****************************************************************************
//
//   File Description:
//
//     Constant values loaded from a constants file
//
//     Constants are loaded from a `config/constants.csv` file.
//     This makes it possible to define them with arbitrary precision
//
//
//
//
//
// ****************************************************************************
//   (C) 2024 Christophe de Dinechin <christophe@dinechin.org>
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

#include "command.h"
#include "menu.h"
#include "symbol.h"

GCP(constant);

struct constant : symbol
// ----------------------------------------------------------------------------
//   A constant is a symbol where the value is looked up from a file
// ----------------------------------------------------------------------------
{
    constant(id type, gcutf8 source, size_t len): symbol(type, source, len)
    { }

    static constant_g make(cstring s)
    {
        return rt.make<constant>(ID_constant, utf8(s), strlen(s));
    }

    static constant_g make(gcutf8 s, size_t len)
    {
        return rt.make<constant>(ID_constant, s, len);
    }


    utf8 name(size_t *size = nullptr) const
    {
        return text::value(size);
    }
    algebraic_p value() const;

public:
    OBJECT_DECL(constant);
    EVAL_DECL(constant);
    PARSE_DECL(constant);
    RENDER_DECL(constant);
    HELP_DECL(constant);
};


struct constant_menu : menu
// ----------------------------------------------------------------------------
//   A constant menu is like a standard menu, but with constants
// ----------------------------------------------------------------------------
{
    constant_menu(id type) : menu(type) { }
    static utf8 name(id type, size_t &len);

public:
    MENU_DECL(constant_menu);
};


#define ID(i)
#define CONSTANT_MENU(ConstantMenu)     struct ConstantMenu : constant_menu {};
#include "ids.tbl"

COMMAND_DECLARE(ConstantName);
COMMAND_DECLARE(ConstantValue);

#endif // CONSTANT_H
