// ****************************************************************************
//  dmcp.cpp                                                      DB48X project
// ****************************************************************************
//
//   File Description:
//
//     A fake DMCP implementation with the functions we use in the simulator
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

#include <sim-screen.h>
#include <dmcp.h>
#include <target.h>
#include <types.h>

int lcd_needsupdate = 0;
uint8_t lcd_buffer[LCD_SCANLINE * LCD_H / 8];
sys_sdb_t sdb;

void LCD_power_off(int clear)
{
    UNUSED(clear);
}


void LCD_power_on()
{
}


int create_screenshot(int report_error)
{
}


void draw_power_off_image(int allow_errors)
{
}
int handle_menu(const smenu_t * menu_id, int action, int cur_line)
{
}

int key = -1;

int key_empty()
{
    return key == -1;
}
int key_pop()
{
    int k = key;
    key = -1;
    return k;
}

int key_pop_last()
{
    return key_pop();
}
void key_pop_all()
{
    key = -1;
}
int key_push(int k1)
{
    key = k1;
}
void lcd_clear_buf()
{
    for (int i = 0; i < sizeof(lcd_buffer); i++)
        lcd_buffer[i] = 0xFF;
}

void lcd_draw_menu_keys(const char *keys[])
{

}
void lcd_fill_rect(uint32_t x, uint32_t y, uint32_t dx, uint32_t dy, int val)
{
}
int lcd_fontWidth(disp_stat_t * ds)
{
}
int lcd_for_calc(int what)
{
}
void lcd_forced_refresh()
{
}
int lcd_get_buf_cleared()
{
}
int lcd_lineHeight(disp_stat_t * ds)
{
}
uint8_t * lcd_line_addr(int y)
{
}
int lcd_nextFontNr(int nr)
{
}
int lcd_prevFontNr(int nr)
{
}
void lcd_prevLn(disp_stat_t * ds)
{
}
void lcd_print(disp_stat_t * ds, const char* fmt, ...)
{
}

void lcd_refresh()
{
}
void lcd_refresh_dma()
{
}
void lcd_refresh_wait()
{
}
void lcd_refresh_lines(int ln, int cnt)
{
}
void lcd_setLine(disp_stat_t * ds, int ln_nr)
{
}

void lcd_setXY(disp_stat_t * ds, int x, int y)
{
}
void lcd_set_buf_cleared(int val)
{
}
void lcd_switchFont(disp_stat_t * ds, int nr)
{
}
int lcd_textWidth(disp_stat_t * ds, const char* text)
{
}
void lcd_writeClr(disp_stat_t * ds)
{
}
void lcd_writeText(disp_stat_t * ds, const char* text)
{
}
void lcd_writeTextWidth(disp_stat_t * ds, const char* text)
{
}
int read_key(int *k1, int *k2)
{
}
void reset_auto_off()
{
}
void rtc_wakeup_delay()
{
}
void run_help_file(const char * help_file)
{
}
void run_help_file_style(const char * help_file, user_style_fn_t *user_style_fn)
{
}
void start_buzzer_freq(uint32_t freq)
{
}
void stop_buzzer()
{
}
void sys_delay(uint32_t ms_delay)
{
}
void sys_sleep()
{
}
void wait_for_key_press()
{
}
void wait_for_key_release(int tout)
{
}
