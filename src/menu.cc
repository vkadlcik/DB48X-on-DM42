// ****************************************************************************
//  menu.cc                                                       DB48X project
// ****************************************************************************
//
//   File Description:
//
//    An RPL object describing a soft menu
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
//  Payload layout:
//    Each menu entry is a pair with a symbol and the associated object
//    The symbol represents the name for the menu entry

#include "menu.h"

#include "input.h"
#include "parser.h"
#include "renderer.h"


OBJECT_HANDLER_BODY(menu)
// ----------------------------------------------------------------------------
//    Handle commands for menus
// ----------------------------------------------------------------------------
{
    switch(op)
    {
    case EVAL:
        return obj->evaluate(rt);
    case SIZE:
        return size(obj, payload);
    case PARSE:
        return object_parser(OBJECT_PARSER_ARG(), rt);
    case RENDER:
        return obj->object_renderer(OBJECT_RENDERER_ARG(), rt);
    case HELP:
        return (intptr_t) "menu";

    default:
        // Check if anyone else knows how to deal with it
        return DELEGATE(list);
    }
}


OBJECT_PARSER_BODY(menu)
// ----------------------------------------------------------------------------
//    Try to parse this as a menu
// ----------------------------------------------------------------------------
{
    return SKIP;
}


OBJECT_RENDERER_BODY(menu)
// ----------------------------------------------------------------------------
//   Render the menu into the given menu buffer
// ----------------------------------------------------------------------------
{
    return snprintf(r.target, r.length, "Menu (internal)");
}


object::result menu::evaluate(runtime & UNUSED rt) const
// ----------------------------------------------------------------------------
//   Evaluate by showing menu entries in the soft menu keys
// ----------------------------------------------------------------------------
{
    byte_p p     = payload();
    size_t size  = leb128<size_t>(p);
    uint   index = 0;

    while (size)
    {
        symbol_p symbol = (symbol_p) p;
        size_t ssize = symbol->object::size();
        p += ssize;
        object_p value = (object_p) p;
        size_t osize = value->size();
        p += osize;
        size -= ssize + osize;

        Input.menu(index++, symbol, value);
    }

    while (index < input::NUM_MENUS)
        Input.menu(index++, cstring(nullptr), nullptr);

    return OK;
}
