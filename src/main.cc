// ****************************************************************************
//  main.cc                                                       DB48X project
// ****************************************************************************
//
//   File Description:
//
//      The DB48X main RPL loop
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
//
// This code is distantly derived from the SwissMicro SDKDemo calculator

#include "main.h"

#include "input.h"
#include "menu.h"
#include "num.h"
#include "rpl.h"
#include "settings.h"
#include "util.h"

#include <algorithm>
#include <cctype>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <dmcp.h>

using std::min;
using std::max;

static void redraw_lcd()
// ----------------------------------------------------------------------------
//   Redraw the whole LCD
// ----------------------------------------------------------------------------
{
    // Write the header
    lcd_clear_buf();
    lcd_writeClr(t20);
    t20->newln = 0; // No skip to next line
    lcd_putsR(t20, "DB48X");

    // Draw the various components handled by input
    Input.draw_annunciators();
    Input.draw_menus();
    Input.draw_editor();
    Input.draw_error();

    // Refres the screen
    lcd_refresh();
}


static void handle_key(int key)
// ----------------------------------------------------------------------------
//   Handle all input keys
// ----------------------------------------------------------------------------
{
    bool consumed = Input.key(key);
    if (!consumed && key != 0)
        beep(1835, 125);
}


void program_init()
// ----------------------------------------------------------------------------
//   Initialize the program
// ----------------------------------------------------------------------------
{
    // Setup application menu callbacks
    run_menu_item_app = menu_item_run;
    menu_line_str_app = menu_item_description;

#ifndef DEBUG
    // Give 64K to the runtime
    size_t size = 64 * 1024;
#else
    // Give 2K to the runtime to stress-test the GC
    size_t size = 2 * 1024;
#endif
    byte *memory = (byte *) malloc(size);
    runtime::RT.memory(memory, size);

    // The following is just to link the same set of functions as DM42
    if (memory == (byte *) program_init)
    {
        double     d = *memory;
        BID_UINT64 a;
        BID_UINT128 res;
        binary64_to_bid64(&a, &d);
        bid64_to_bid128(&res, &a);
        num_add(&res, &res, &res);
        num_sub(&res, &res, &res);
        num_mul(&res, &res, &res);
        num_div(&res, &res, &res);
        num_div(&res, &res, &res);
        num_sqrt(&res, &res);
        num_log10(&res, &res);
        num_log(&res, &res);
        num_pow(&res, &res, &res);
        num_mul(&res, &res, &res);
        num_exp10(&res, &res);
        num_exp(&res, &res);
        num_sin(&res, &res);
        num_cos(&res, &res);
        num_tan(&res, &res);
        num_asin(&res, &res);
        num_acos(&res, &res);
        num_atan(&res, &res);
    }
}


extern "C" void program_main()
// ----------------------------------------------------------------------------
//   DMCP main entry point and main loop
// ----------------------------------------------------------------------------
// Status flags:
// ST(STAT_PGM_END)   - Program should go to off state (set by auto off timer)
// ST(STAT_SUSPENDED) - Program signals it is ready for off
// ST(STAT_OFF)       - Program in off state (only [EXIT] key can wake it up)
// ST(STAT_RUNNING)   - OS doesn't sleep in this mode
{
    int key = 0;

    // Initialization
    program_init();
    redraw_lcd();

    // Main loop
    while(true)
    {
        // Already in off mode and suspended
        if ((ST(STAT_PGM_END) && ST(STAT_SUSPENDED)) ||
            // Go to sleep if no keys available
            (!ST(STAT_PGM_END) && key_empty()))
        {
            CLR_ST(STAT_RUNNING);
            sys_sleep();
        }

        // Wakeup in off state or going to sleep
        if (ST(STAT_PGM_END) || ST(STAT_SUSPENDED))
        {
            if (!ST(STAT_SUSPENDED))
            {
                // Going to off mode
                lcd_set_buf_cleared(0); // Mark no buffer change region
                draw_power_off_image(1);

                LCD_power_off(0);
                SET_ST(STAT_SUSPENDED);
                SET_ST(STAT_OFF);
            }
            // Already in OFF -> just continue to sleep above
            continue;
        }

        // Well, we are woken-up
        SET_ST(STAT_RUNNING);

        // We definitely reached active state, clear suspended flag
        CLR_ST(STAT_SUSPENDED);

        // Get up from OFF state
        if (ST(STAT_OFF))
        {
            LCD_power_on();

            // Ensure that RTC readings after power off will be OK
            rtc_wakeup_delay();

            CLR_ST(STAT_OFF);

            // Check if we need to redraw
            if (!lcd_get_buf_cleared())
                lcd_forced_refresh();
        }

        // Key is ready -> clear auto off timer
        if (!key_empty())
        {
            reset_auto_off();
            key = key_pop();
        }

        // Fetch the key (<0: no key event, >0: key pressed, 0: key released)
        if (key >= 0)
        {
            handle_key(key);

            // Redraw the LCD
            redraw_lcd();
        }
    }
}
