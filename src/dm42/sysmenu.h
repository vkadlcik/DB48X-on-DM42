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
    MI_DB48_SETTINGS = 1,       // Application settings
    MI_DB48_ABOUT,              // Display the "About" dialog

    MI_48STATE,                 // Menu for 48 state load and save
    MI_48STATE_CLEAN,           // Restart with a clean state
    MI_48STATE_LOAD,            // Load a state from disk
    MI_48STATE_MERGE,           // Merge a state from disk
    MI_48STATE_SAVE,            // Save state to disk
};


extern const smenu_t  application_menu;
extern const smenu_t  settings_menu;
extern const smenu_t  state_menu;
extern const smenu_t  program_menu;

// Callbacks installed in the SDB to run the menu system
int                   menu_item_run(uint8_t mid);
cstring               menu_item_description(uint8_t mid, char *, const int);
bool                  load_state_file(cstring path);
bool                  save_state_file(cstring path);
bool                  load_system_state();
bool                  save_system_state();
void                  power_off();
void                  system_setup();

#endif // SYSMENU_H
