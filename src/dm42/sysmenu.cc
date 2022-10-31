// ****************************************************************************
//  sysmenu.cc                                                   DB48X project
// ****************************************************************************
//
//   File Description:
//
//     Handles the DMCP application menus on the DM42
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

#include "sysmenu.h"

#include "main.h"
#include "types.h"

#include <cstdio>
#include <dmcp.h>

const uint8_t application_menu_items[] =
// ----------------------------------------------------------------------------
//    Application menu items
// ----------------------------------------------------------------------------
{
    MI_SETTINGS,                // Application setting
    MI_ABOUT_PROGRAM,           // About dialog
    MI_PGM_LOAD,                // Load program
    MI_MSC,                     //
    MI_SYSTEM_ENTER,            // Enter system
    0
}; // Terminator


const smenu_t application_menu =
// ----------------------------------------------------------------------------
//   Application menu
// ----------------------------------------------------------------------------
{
    "Setup",  application_menu_items,   NULL, NULL
};


const uint8_t settings_menu_items[] =
// ----------------------------------------------------------------------------
//    The settings menu
// ----------------------------------------------------------------------------
{
    MI_SET_TIME,                // Standard set time menu
    MI_SET_DATE,                // Standard set date menu
    MI_BEEP_MUTE,               // Mute the beep
    MI_SLOW_AUTOREP,            // Slow auto-repeat
    0
}; // Terminator


const smenu_t settings_menu =
// ----------------------------------------------------------------------------
//   The settings menu
// ----------------------------------------------------------------------------
{
    "Settings",  settings_menu_items,  NULL, NULL
};


void about_dialog()
// ----------------------------------------------------------------------------
//   Display the About dialog
// ----------------------------------------------------------------------------
{
  lcd_clear_buf();
  lcd_writeClr(t24);

  // Header based on original system about
  lcd_for_calc(DISP_ABOUT);
  lcd_putsAt(t24,4,"");
  lcd_prevLn(t24);

  // Display the main text
  int h2 = lcd_lineHeight(t20)/2; // Extra spacing
  lcd_setXY(t20, t24->x, t24->y + h2);
  lcd_puts(t20, "DB48X v" PROGRAM_VERSION " (C) C. de Dinechin");
  t20->y += h2;
  lcd_puts(t20, "DMCP platform (C) SwissMicros GmbH");
  lcd_puts(t20, "Intel Decimal Floating Point Library v2.0u1");
  lcd_puts(t20, "  (C) 2007-2018, Intel Corp.");

  t20->y = LCD_Y - lcd_lineHeight(t20);
  lcd_putsR(t20, "    Press EXIT key to continue...");

  lcd_refresh();

  wait_for_key_press();
}


int menu_item_run(uint8_t menu_id)
// ----------------------------------------------------------------------------
//   Callback to run a menu item
// ----------------------------------------------------------------------------
{
  int ret = 0;

  switch(menu_id)
  {
  case MI_ABOUT_PROGRAM: about_dialog(); break;
  case MI_SETTINGS:      ret = handle_menu(&settings_menu, MENU_ADD, 0); break;
  default:               ret = MRET_UNIMPL; break;
  }

  return ret;
}


cstring menu_item_description(uint8_t          menu_id,
                              char *UNUSED     s,
                              const int UNUSED len)
// ----------------------------------------------------------------------------
//   Return the menu item description
// ----------------------------------------------------------------------------
{
    const char *ln;

    switch (menu_id)
    {
    case MI_SETTINGS:           ln = "Settings >";      break;
    case MI_ABOUT_PROGRAM:      ln = "About >";         break;
    default:                    ln = NULL;              break;
    }

    return ln;
}
