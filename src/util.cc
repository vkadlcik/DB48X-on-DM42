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
#include <dmcp.h>

void beep(int frequency, int duration)
// ----------------------------------------------------------------------------
//   Emit a short beep
// ----------------------------------------------------------------------------
{
    start_buzzer_freq(frequency * 1000);
    sys_delay(duration);
    stop_buzzer();
}


void click(int frequency)
// ----------------------------------------------------------------------------
//   A very short beep
// ----------------------------------------------------------------------------
{
    beep(frequency, 10);
}


void screenshot()
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
    }

    // End click
    click(8000);
}
