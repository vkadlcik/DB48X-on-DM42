#ifndef LIBRARY_H
#define LIBRARY_H
// ****************************************************************************
//  library.h                                                    DB48X project
// ****************************************************************************
//
//   File Description:
//
//     Implementation of the library
//
//
//
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

#include "constants.h"

#include <string.h>

GCP(xlib);

struct xlib : constant
// ----------------------------------------------------------------------------
//  A library, stored in `config/libraries.csv` file
// ----------------------------------------------------------------------------
{
    xlib(id type, uint index): constant(type, index) {}

    static xlib_p make(uint index)
    {
        return rt.make<xlib>(ID_xlib, index);
    }

    static xlib_p make(id type, uint index)
    {
        return rt.make<xlib>(type, index);
    }

    static xlib_p lookup(utf8 name, size_t len, bool error)
    {
        return xlib_p(do_lookup(library, name, len, error));
    }

    static xlib_p lookup(cstring name, bool error = true)
    {
        return lookup(utf8(name), strlen(name), error);
    }

    uint        index() const
    {
        byte_p p = payload();
        return leb128<uint>(p);
    }

    utf8        name(size_t *size = nullptr) const
    {
        return do_name(library, size);
    }
    algebraic_p value() const
    {
        return do_value(library);
    }

    static const config library;
    OBJECT_DECL(xlib);
    PARSE_DECL(xlib);
    EVAL_DECL(xlib);
    RENDER_DECL(xlib);
    GRAPH_DECL(xlib);
    HELP_DECL(xlib);
};


struct library_menu : constant_menu
// ----------------------------------------------------------------------------
//   A library menu is like a constants menu but for library items (xlib)
// ----------------------------------------------------------------------------
{
    library_menu(id type) : constant_menu(type) { }
    static utf8 name(id type, size_t &len);
    MENU_DECL(library_menu);
    HELP_DECL(library_menu);
};


#define ID(i)
#define LIBRARY_MENU(LibMenu)           struct LibMenu : library_menu {};
#include "ids.tbl"

COMMAND_DECLARE_INSERT_HELP(XlibName);
COMMAND_DECLARE_INSERT_HELP(XlibValue);



#endif // LIBRARY_H
