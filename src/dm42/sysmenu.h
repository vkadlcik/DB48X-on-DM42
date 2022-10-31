#ifndef SYSMENU_H
#define SYSMENU_H
// ****************************************************************************
//  menu.h                                                        DB48X project
// ****************************************************************************
//
//   File Description:
//
//     Access to DMCP menus
//
//     According to DMCP "documentation", menu items range are:
//       1..127: Usable by the application
//     128..255: System menus, e.g. Load Program
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

#include "types.h"

#include <dmcp.h>

enum menu_item
// ----------------------------------------------------------------------------
//   The menu items in our application
// ----------------------------------------------------------------------------
{
    MI_SETTINGS = 1,            // Application settings
    MI_ABOUT_PROGRAM,           // Display the "About" dialog
};


extern const smenu_t  application_menu;
extern const smenu_t  settings_menu;

// Callbacks installed in the SDB to run the menu system
int                   menu_item_run(uint8_t mid);
cstring               menu_item_description(uint8_t mid, char *, const int);

#endif // SYSMENU_H
