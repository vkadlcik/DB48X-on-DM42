#ifndef SIM_DMCP
#define SIM_DMCP
// ****************************************************************************
//  sim-dmcp.h                                                    DB48X project
// ****************************************************************************
//
//   File Description:
//
//     Interface between the simulator and the user interface
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

#include <stddef.h>
#include <stdint.h>


// ============================================================================
//
//   Quick and dirty interface with the RPL thread
//
// ============================================================================

enum simulated_target
// ----------------------------------------------------------------------------
//   Constants for a given target
// ----------------------------------------------------------------------------
{
    SIM_BITS_PER_PIXEL = 1,
    SIM_LCD_W          = 400,
    SIM_LCD_H          = 240,
    SIM_LCD_SCANLINE   = 416,
};

typedef uint8_t  byte;
typedef unsigned int uint;

extern volatile uint keysync_sent;
extern volatile uint keysync_done;

extern volatile int  lcd_needsupdate;
extern int           lcd_buf_cleared;
extern uint8_t       lcd_buffer[SIM_LCD_SCANLINE * SIM_LCD_H / 8];
extern bool          shiftHeld;
extern bool          altHeld;


// ============================================================================
//
//   Quick and dirty interface with iOS
//
// ============================================================================

typedef int (*file_sel_fn)(const char *fpath, const char *fname, void *data);

void      ui_screenshot();
void      ui_push_key(int k);
void      ui_ms_sleep(uint delay);
int       ui_file_selector(const char *title,
                           const char *base_dir,
                           const char *ext,
                           file_sel_fn callback,
                           void       *data,
                           int         disp_new,
                           int         overwrite_check);
void      ui_save_setting(const char *name, const char *value);
size_t    ui_read_setting(const char *name, char *value, size_t maxlen);

#endif // SIM_DMCP
