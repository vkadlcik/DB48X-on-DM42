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

#include "dmcp.h"

#include "dmcp_fonts.c"
#include "sim-rpl.h"
#include "sim-screen.h"
#include "sim-window.h"
#include "types.h"
#include "recorder.h"

#include <stdarg.h>
#include <stdio.h>
#include <sys/time.h>
#include <target.h>

#pragma GCC diagnostic ignored "-Wunused-parameter"

RECORDER(dmcp,          64, "DMCP system calls");
RECORDER(dmcp_error,    64, "DMCP errors");
RECORDER(dmcp_warning,  64, "DMCP warnings");
RECORDER(dmcp_notyet,   64, "DMCP features that are not yet implemented");
RECORDER(keys,          64, "DMCP key handling");
RECORDER(keys_warning,  64, "Warnings related to key handling");
RECORDER(lcd,           64, "DMCP lcd/display functions");
RECORDER(lcd_width,     64, "Width of strings and chars");
RECORDER(lcd_warning,   64, "Warnings from lcd/display functions");

#undef ppgm_fp

volatile int lcd_needsupdate = 0;
int lcd_buf_cleared = 0;
uint8_t lcd_buffer[LCD_SCANLINE * LCD_H / 8];

static disp_stat_t t20_ds = { .f = &lib_mono_10x17 };
static disp_stat_t t24_ds = { .f = &lib_mono_12x20 };
static disp_stat_t fReg_ds = { .f = &lib_mono_17x25 };
static FIL ppgm_fp_file;

sys_sdb_t sdb =
{
    .ppgm_fp = &ppgm_fp_file,
    .pds_t20 = &t20_ds,
    .pds_t24 = &t24_ds,
    .pds_fReg = &fReg_ds,
};


void LCD_power_off(int UNUSED clear)
{
    record(dmcp, "LCD_power_off");
}


void LCD_power_on()
{
    record(dmcp, "LCD_power_on");
}

uint32_t read_power_voltage()
{
    return 2000 + sys_current_ms() % 1500;
}

int get_lowbat_state()
{
    return read_power_voltage() < 2300;
}

int usb_powered()
{
    return sys_current_ms() / 1000 % 3;
}

int create_screenshot(int report_error)
{
    record(dmcp_notyet,
           "create_screenshot(%d) not implemented", report_error);
    return 0;
}


void draw_power_off_image(int allow_errors)
{
    record(dmcp_notyet,
           "draw_power_off_image(%d) not implemented", allow_errors);
}
int handle_menu(const smenu_t * menu_id, int action, int cur_line)
{
    record(dmcp_notyet, "handle_menu(%p, %d, %d) not implemented",
           menu_id, action, cur_line);
    return 0;
}

int8_t  keys[4] = { 0 };
uint    keyrd   = 0;
uint    keywr   = 0;
enum {  nkeys = sizeof(keys) };

int key_empty()
{
    return keyrd == keywr;
}

int key_remaining()
{
    return nkeys - (keywr - keyrd);
}

int key_pop()
{
    if (keyrd != keywr)
    {
        record(keys, "Key %d (rd %u wr %u)", keys[keyrd], keyrd, keywr);
        return keys[keyrd++ % nkeys];
    }
    return -1;
}

int key_pop_last()
{
    if (keywr - keyrd > 1)
        keyrd = keywr - 1;
    if (keyrd != keywr)
        return keys[keyrd++ % nkeys];
    return -1;
}

void key_pop_all()
{
    keyrd = 0;
    keywr = 0;
}
int key_push(int k)
{
    record(keys, "Push key %d (wr %u rd %u)", k, keywr, keyrd);
    MainWindow::theMainWindow()->pushKey(k);
    if (keywr - keyrd < nkeys)
        keys[keywr++ % nkeys] = k;
    else
        record(keys_warning, "Dropped key %d (wr %u rd %u)", k, keywr, keyrd);
    return keywr - keyrd < nkeys;
}
int read_key(int *k1, int *k2)
{
    *k1 = keywr - keyrd > 0 ? keys[(keywr - 1) % nkeys] : 0;
    *k2 = keyrd - keywr > 1 ? keys[(keywr - 2) % nkeys] : 0;
    return keyrd != keywr;
}
void lcd_clear_buf()
{
    record(lcd, "Clearing buffer");
    for (unsigned i = 0; i < sizeof(lcd_buffer); i++)
        lcd_buffer[i] = 0xFF;
}

inline void lcd_set_pixel(int x, int y)
{
    if (x < 0 || x > LCD_W || y < 0 || y > LCD_H)
    {
        record(lcd_warning, "Clearing pixel at (%d, %d)", x, y);
        return;
    }
    unsigned bo = y * LCD_SCANLINE + (LCD_W - x);
    if (bo/8 < sizeof(lcd_buffer))
        lcd_buffer[bo / 8] |= (1 << (bo % 8));
}

inline void lcd_clear_pixel(int x, int y)
{

    if (x < 0 || x > LCD_W || y < 0 || y > LCD_H)
    {
        record(lcd_warning, "Setting pixel at (%d, %d)", x, y);
        return;
    }
    unsigned bo = y * LCD_SCANLINE + (LCD_W - x);
    if (bo/8 < sizeof(lcd_buffer))
        lcd_buffer[bo / 8] &= ~(1 << (bo % 8));
}

inline void lcd_pixel(int x, int y, int val)
{
    if (!val)
        lcd_set_pixel(x, y);
    else
        lcd_clear_pixel(x, y);
}

void lcd_draw_menu_keys(const char *keys[])
{
    int my = LCD_H - t20->f->height - 4;
    int mh = t20->f->height + 2;
    int mw = (LCD_W - 10) / 6;
    int sp = (LCD_W - 5) - 6 * mw;

    t20->inv = 1;
    t20->lnfill = 0;
    t20->bgfill = 1;
    t20->newln = 0;
    t20->y = my + 1;

    record(lcd, "Menu [%s][%s][%s][%s][%s][%s]",
           keys[0], keys[1], keys[2], keys[3], keys[4], keys[5]);
    for (int m = 0; m < 6; m++)
    {
        int x = (2 * m + 1) * mw / 2 + (m * sp) / 5 + 2;
        lcd_fill_rect(x - mw/2+2, my,   mw-4,   mh, 1);
        lcd_fill_rect(x - mw/2+1, my+1, mw-2, mh-2, 1);
        lcd_fill_rect(x - mw/2,   my+2, mw,   mh-4, 1);

        // Truncate the menu to fit
        // Note that DMCP is NOT robust to overflow here and can die
        int size = 11;
        int w = 0;
        char buffer[12];
        do
        {
            snprintf(buffer, sizeof(buffer), "%.*s", size, keys[m]);
            w = lcd_textWidth(t20, buffer);
            size--;
        } while (w > mw);

        if (size < (int) strlen(keys[m]))
            record(lcd_warning,
                       "Menu entry %d [%s] is too long "
                       "(%d chars lost, shows as [%s])",
                       m, keys[m], (int) strlen(keys[m]) - size + 1, buffer);

        t20->x = x - w / 2;
        lcd_puts(t20, buffer);
    }
    t20->lnfill = 1;
    t20->inv = 0;
}

void lcd_fill_rect(uint32_t x, uint32_t y, uint32_t w, uint32_t h, int val)
{
    if (val)
        record(lcd, "Fill  rectangle (%u,%u) + (%u, %u)", x, y, w, h);
    else
        record(lcd, "Clear rectangle (%u,%u) + (%u, %u)", x, y, w, h);

    if (x + w > LCD_W)
    {
        record(lcd_warning,
               "Rectangle X is outside screen (%u, %u) + (%u, %u)",
               x, y, w, h);
        w = LCD_W - x;
        if (w > LCD_W)
            x = w = 0;
    }
    if (y +h > LCD_H)
    {
        record(lcd_warning,
               "Rectangle Y is outside screen (%u, %u) + (%u, %u)",
               x, y, w, h);
        h = LCD_W - y;
        if (h > LCD_W)
            y = h = 0;
    }

    for (uint r = y; r < y + h; r++)
        for (uint c = x; c < x + w; c++)
            lcd_pixel(c, r, val);
}

int lcd_fontWidth(disp_stat_t * ds)
{
    return ds->f->width;
}
int lcd_for_calc(int what)
{
    record(dmcp_notyet, "lcd_for_calc %d not implemented", what);
    return 0;
}
int lcd_get_buf_cleared()
{
    record(lcd, "get_buf_cleared returns %d", lcd_buf_cleared);
    return lcd_buf_cleared;
}
int lcd_lineHeight(disp_stat_t * ds)
{
    return ds->f->height;
}
uint8_t * lcd_line_addr(int y)
{
    if (y < 0 || y > LCD_H)
    {
        record(lcd_warning, "lcd_line_addr(%d), line is out of range", y);
        y = 0;
    }
    unsigned offset = y * LCD_SCANLINE / 8;
    return lcd_buffer + offset;
}
int lcd_toggleFontT(int nr)
{
    return nr;
}
int lcd_nextFontNr(int nr)
{
    if (nr < (int) dmcp_fonts_count - 1)
        nr++;
    else
        nr = dmcp_fonts_count - 1;
    return nr;
}
int lcd_prevFontNr(int nr)
{
    if (nr > 0)
        nr--;
    else
        nr = 0;
    return nr;
}
void lcd_prevLn(disp_stat_t * ds)
{
    ds->y -= lcd_lineHeight(ds);
    ds->x = ds->xoffs;
}
void lcd_print(disp_stat_t * ds, const char* fmt, ...)
{
    static char buffer[256];
    va_list args;
    va_start(args, fmt);
    vsnprintf(buffer, sizeof(buffer), fmt, args);
    va_end(args);
    lcd_puts(ds, buffer);
}

void lcd_forced_refresh()
{
    record(lcd, "Forced refresh");
    lcd_needsupdate++;
}
void lcd_refresh()
{
    record(lcd, "Refresh");
    lcd_needsupdate++;
}
void lcd_refresh_dma()
{
    record(lcd, "Refresh DMA");
    lcd_needsupdate++;
}
void lcd_refresh_wait()
{
    record(lcd, "Refresh Wait");
    lcd_needsupdate++;
}
void lcd_refresh_lines(int ln, int cnt)
{
    record(lcd, "Refresh lines (%d-%d) count %d", ln, ln+cnt-1, cnt);
    lcd_needsupdate += (ln >= 0 && cnt > 0);
}
void lcd_setLine(disp_stat_t * ds, int ln_nr)
{
    ds->x = ds->xoffs;;
    ds->y = ln_nr * lcd_lineHeight(ds);
    record(lcd, "set line %u coord (%d, %d)", ln_nr, ds->x, ds->y);
}

void lcd_setXY(disp_stat_t * ds, int x, int y)
{
    record(lcd, "set XY (%d, %d)", x, y);
    ds->x = x;
    ds->y = y;
}
void lcd_set_buf_cleared(int val)
{
    record(lcd, "Set buffer cleared %d", val);
    lcd_buf_cleared = val;
}
void lcd_switchFont(disp_stat_t * ds, int nr)
{
    record(lcd, "Select font %d", nr);
    if (nr >= 0 && nr <= (int) dmcp_fonts_count)
        ds->f = dmcp_fonts[nr];
    else
        record(lcd_warning, "lcd_switchFont for bad font number %d", nr);
}

int lcd_charWidth(disp_stat_t * ds, int c)
{
    int                width = 0;
    const line_font_t *f     = ds->f;
    byte               first = f->first_char;
    byte               count = f->char_cnt;
    const uint16_t    *offs  = f->offs;
    const uint8_t     *data  = f->data;
    uint               xspc  = ds->xspc;

    c -= first;
    if (c >= 0 && c < count)
    {
        uint off = offs[c];
        width += data[off + 0] + data[off + 2] + xspc;
        record(lcd_width,
               "Character width of %c (%d=0x%x) is %d",
               c + first, c + first, c + first, width);
    }
    else
    {
        record(lcd_width, "Character width of nonexistent %d is %d", c, width);
    }

    return width;
}

int lcd_textWidth(disp_stat_t * ds, const char* text)
{
    int                width = 0;
    byte               c;
    const line_font_t *f     = ds->f;
    byte               first = f->first_char;
    byte               count = f->char_cnt;
    const uint16_t    *offs  = f->offs;
    const uint8_t     *data  = f->data;
    uint               xspc  = ds->xspc;
    const byte         *p    = (const byte *) text;

    while ((c = *p++))
    {
        c -= first;
        if (c >= 0 && c < count)
        {
            uint off = offs[c];
            width += data[off + 0] + data[off + 2] + xspc;
        }
        else
        {
            record(lcd_width,
                   "Nonexistent character %d at offset %d in [%s]",
                   c + first, p - (const byte *) text, text);
        }
    }
    return width;
}

void lcd_writeClr(disp_stat_t *ds)
{
    record(lcd, "Clearing display state"); // Not sure this is what it does
    ds->x      = ds->xoffs;
    ds->y      = 0;
    ds->inv    = 0;
    ds->bgfill = 1;
    ds->lnfill = 1;
    ds->xspc   = 1;
}

void lcd_writeNl(disp_stat_t *ds)
{
    ds->x = ds->xoffs;
    ds->y += lcd_lineHeight(ds);
    record(lcd, "New line, now at (%d, %d)", ds->x, ds->y);
}

inline void lcd_writeTextInternal(disp_stat_t *ds, const char *text, int write)
{
    uint               c;
    const line_font_t *f        = ds->f;
    uint               first    = f->first_char;
    uint               count    = f->char_cnt;
    uint               height   = f->height;
    const uint8_t     *data     = f->data;
    const uint16_t    *offs     = f->offs;
    int                xspc     = ds->xspc;
    int                x        = ds->x + xspc;
    int                y        = ds->y + ds->ln_offs;
    int                inv      = ds->inv != 0;
    const byte        *p        = (const byte *) text;

    if (write)
        record(lcd, "Write text [%s] at (%d, %d)", text, x, y);
    else
        record(lcd, "Skip text [%s] at (%d, %d)", text, x, y);

    if (ds->lnfill)
        lcd_fill_rect(ds->xoffs, y, LCD_W, height, inv);

    while ((c = *p++))
    {
        c -= first;
        if (c < count)
        {
            int            off  = offs[c];
            const uint8_t *dp   = data + off;
            int            cx   = *dp++;
            int            cy   = *dp++;
            int            cols = *dp++;
            int            rows = *dp++;

            if (!write)
            {
                x += cx + cols;
                continue;
            }

            for (int r = 0; r < cy; r++)
                for (int c = 0; c < cx + cols; c++)
                    lcd_pixel(x+c, y+r, inv);

            for (int r = 0; r < rows; r++)
            {
                int data = 0;
                for (int c = 0; c < cols; c += 8)
                    data |= *dp++ << c;

                for (int c = 0; c < cx; c++)
                    lcd_pixel(x+c, y+r, inv);

                for (int c = 0; c < cols; c++)
                {
                    int val = (data >> (cols - c - 1)) & 1;
                    if (val || ds->bgfill)
                        lcd_pixel(x + c + cx, y + r + cy, val != inv);
                }
            }

            for (uint r = cy + rows; r < height; r++)
                for (int c = 0; c < cx + cols; c++)
                    lcd_pixel(x+c, y+r, inv);


            x += cx + cols + xspc;
        }
        else
        {
            record(lcd_warning,
                   "Nonexistent character [%d] in [%s] at %d, max=%d",
                   c + first, text, p - (byte_p) text, count + first);
        }

    }
    ds->x = x;
    if (ds->newln)
    {
        ds->x = ds->xoffs;
        ds->y += height;
    }
}

void lcd_writeText(disp_stat_t * ds, const char* text)
{
    lcd_writeTextInternal(ds, text, 1);
}
void lcd_writeTextWidth(disp_stat_t * ds, const char* text)
{
    lcd_writeTextInternal(ds, text, 0);
}
void reset_auto_off()
{
    // No effect
}
void rtc_wakeup_delay()
{
    record(dmcp_notyet, "rtc_wakeup_delay not implemented");
}
void run_help_file(const char * help_file)
{
    record(dmcp_notyet, "run_help_file not implemented");
}
void run_help_file_style(const char * help_file, user_style_fn_t *user_style_fn)
{
    record(dmcp_notyet, "run_help_file_style not implemented");
}
void start_buzzer_freq(uint32_t freq)
{
    record(dmcp_notyet, "start_buzzer_freq not implemented");
}
void stop_buzzer()
{
    record(dmcp_notyet, "stop_buzzer not implemented");
}
void sys_delay(uint32_t ms_delay)
{
    QThread::msleep(ms_delay);
}

static struct timer
{
    uint32_t deadline;
    bool     enabled;
} timers[4];

void sys_sleep()
{
    while (key_empty())
    {
        uint32_t now = sys_current_ms();
        for (int i = 0; i < 4; i++)
            if (timers[i].enabled && int(timers[i].deadline - now) < 0)
                return;
        QThread::msleep(20);
    }
    CLR_ST(STAT_SUSPENDED | STAT_OFF | STAT_PGM_END);
}

void sys_timer_disable(int timer_ix)
{
    timers[timer_ix].enabled = false;
}

void sys_timer_start(int timer_ix, uint32_t ms_value)
{
    uint32_t now = sys_current_ms();
    uint32_t then = now + ms_value;
    timers[timer_ix].deadline = then;
    timers[timer_ix].enabled = true;
}
int sys_timer_active(int timer_ix)
{
    return timers[timer_ix].enabled;
}

int sys_timer_timeout(int timer_ix)
{
    uint32_t now = sys_current_ms();
    if (timers[timer_ix].enabled)
        return int(timers[timer_ix].deadline - now) < 0;
    return false;
}

void wait_for_key_press()
{
    record(dmcp_notyet, "wait_for_key_press not implemented");
}
void wait_for_key_release(int tout)
{
    record(dmcp_notyet, "wait_for_key_release not implemented");
}


int file_selection_screen(const char * title, const char * base_dir, const char * ext, file_sel_fn_t sel_fn,
                          int disp_new, int overwrite_check, void * data)
{
    record(dmcp, "file_selection_screen not imlemented");
    return 0;
}

int power_check_screen()
{
    record(dmcp, "file_selection_screen not imlemented");
    return 0;
}

int sys_disk_ok()
{
    return 1;
}

int sys_disk_write_enable(int val)
{
    return 0;
}

uint32_t sys_current_ms()
{
    struct timeval tv;
    gettimeofday(&tv, NULL);
    return (tv.tv_sec * 1000000 + tv.tv_usec) / 1000;
}


FRESULT f_open(FIL *fp, const TCHAR *path, BYTE mode)
{
    record(dmcp_notyet, "f_open not implemented");
    return FR_NOT_ENABLED;
}
FRESULT f_close(FIL *fp)
{
    record(dmcp_notyet, "f_close not implemented");
    return FR_NOT_ENABLED;
}

FRESULT f_read(FIL *fp, void *buff, UINT btr, UINT *br)
{
    record(dmcp_notyet, "f_read not implemented");
    return FR_NOT_ENABLED;
}

FRESULT f_write(FIL *fp, const void *buff, UINT btw, UINT *bw)
{
    record(dmcp_notyet, "f_write not implemented");
    return FR_NOT_ENABLED;
}

FRESULT f_lseek(FIL *fp, FSIZE_t ofs)
{
    record(dmcp_notyet, "f_lseek not implemented");
    return FR_NOT_ENABLED;
}

FRESULT f_rename(const TCHAR *path_old, const TCHAR *path_new)
{
    record(dmcp_notyet, "f_rename not implemented");
    return FR_NOT_ENABLED;
}

FRESULT f_unlink(const TCHAR *path)
{
    record(dmcp_notyet, "f_unlink not implemented");
    return FR_NOT_ENABLED;
}
