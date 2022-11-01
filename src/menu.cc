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

RECORDER(menu,          16, "RPL menu class");
RECORDER(menu_error,    16, "Errors handling menus");

OBJECT_HANDLER_BODY(menu)
// ----------------------------------------------------------------------------
//    Handle commands for menus
// ----------------------------------------------------------------------------
{
    switch(op)
    {
    case EXEC:
    case EVAL:
        Input.menu(obj);
        return OK;
    case SIZE:
        return leb128size(obj->type());
    case MENU:
        record(menu_error, "Invalid menu %u", obj->type());
        return ERROR;

    default:
        return DELEGATE(command);
    }
}


void menu::items_init(info &mi, uint nitems, uint planes)
// ----------------------------------------------------------------------------
//   Initialize the info structure
// ----------------------------------------------------------------------------
{
    uint page0 = planes * input::NUM_SOFTKEYS;
    mi.planes  = planes;
    mi.plane   = 0;
    mi.index   = 0;
    if (nitems <= page0)
    {
        mi.page = 0;
        mi.skip = 0;
        mi.pages = 1;
    }
    else
    {
        uint perpage = planes * (input::NUM_SOFTKEYS - 1);
        mi.skip = mi.page * perpage;
        mi.pages = nitems / perpage;
    }
    Input.menus(0, nullptr, nullptr);
}


void menu::items(info &mi, cstring label, object_p action)
// ----------------------------------------------------------------------------
//   Add a menu item
// ----------------------------------------------------------------------------
{
    if (mi.skip > 0)
    {
        mi.skip--;
    }
    else
    {
        uint idx = mi.index++;
        if (mi.pages > 1 && mi.plane < mi.planes)
        {
            if ((idx + 1) % input::NUM_SOFTKEYS == 0)
            {
                // Insert next and previous keys in menu
                static cstring labels[input::NUM_PLANES] = { "▶", "◀︎", "◀︎◀︎" };
                static id functions[input::NUM_PLANES] =
                {
                    ID_MenuNextPage, ID_MenuPreviousPage, ID_MenuFirstPage
                };
                uint plane = mi.plane++;
                object_p function = command::static_object(functions[plane]);
                cstring label = labels[plane];
                Input.menu(idx, label, function);
                idx = mi.index++;
            }
        }
        if (idx < input::NUM_SOFTKEYS * mi.planes)
            Input.menu(idx, label, action);
    }
}
