// ****************************************************************************
//  util.cc                                                       DB48X project
// ****************************************************************************
//
//   File Description:
//
//     Basic utilities
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

#include "util.h"

#include "settings.h"
#include "target.h"

#include <dmcp.h>

void invert_screen()
// ----------------------------------------------------------------------------
//   Invert the screen and refresh it
// ----------------------------------------------------------------------------
{
    Screen.invert();
    lcd_refresh_lines(0, LCD_H);
}


void beep(int frequency, int duration)
// ----------------------------------------------------------------------------
//   Emit a short beep
// ----------------------------------------------------------------------------
{
    bool beeping = Settings.BeepOn();
    bool flash = Settings.SilentBeepOn();

    if (beeping)
        start_buzzer_freq(frequency * 1000);
    if (flash)
        invert_screen();
    sys_delay(duration);
    if (beeping)
        stop_buzzer();
    if (flash)
        invert_screen();
}


void click(int frequency)
// ----------------------------------------------------------------------------
//   A very short beep
// ----------------------------------------------------------------------------
{
    bool silent = Settings.SilentBeepOn();
    Settings.SilentBeepOn(false);
    beep(frequency, 10);
    Settings.SilentBeepOn(silent);
}


bool screenshot()
// ----------------------------------------------------------------------------
//  Take a screenshot
// ----------------------------------------------------------------------------
{
    click(4400);

    // Make screenshot - allow to report errors
    if (create_screenshot(1) == 2)
    {
        // Was error just wait for confirmation
        wait_for_key_press();
        return false;
    }

    // End click
    click(8000);

    return true;
}


void assertion_failed(const char *msg)
// ----------------------------------------------------------------------------
//   Function to make it easier to put a breakpoint somewhere
// ----------------------------------------------------------------------------
{
    record(assert_error, "Assertion failed: %s", msg);
}
