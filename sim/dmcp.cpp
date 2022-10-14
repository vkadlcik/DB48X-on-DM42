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

#include <QThread>
#include <sim-screen.h>
#include <dmcp.h>
#include <target.h>
#include <types.h>
#include <stdio.h>
#include <stdarg.h>
#include <sys/time.h>
#include "dmcp_fonts.c"

#pragma GCC diagnostic ignored "-Wunused-parameter"


#undef ppgm_fp

volatile int lcd_needsupdate = 1;
volatile int timer_interrupt = 0;
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
    fprintf(stderr, "LCD_power_off\n");
}


void LCD_power_on()
{
    fprintf(stderr, "LCD_power_on\n");
}


int create_screenshot(int report_error)
{
    fprintf(stderr, "create_screenshot(%d) not implemented\n", report_error);
    return 0;
}


void draw_power_off_image(int allow_errors)
{
    fprintf(stderr, "draw_power_off_image(%d) not implemented\n", allow_errors);
}
int handle_menu(const smenu_t * menu_id, int action, int cur_line)
{
    fprintf(stderr, "handle_menu(%p, %d, %d) not implemented\n", menu_id, action, cur_line);
    return 0;
}

int8_t keys[4] = { 0 };
enum { nkeys = sizeof(keys) };
int keyrd = 0;
int keywr = 0;

int key_empty()
{
    return keyrd == keywr;
}
int key_pop()
{
    if (keyrd != keywr)
        return keys[keyrd++ % nkeys];
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
int key_push(int k1)
{
    if (keywr - keyrd < nkeys)
        keys[keywr++ % nkeys] = k1;
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
    for (unsigned i = 0; i < sizeof(lcd_buffer); i++)
        lcd_buffer[i] = 0x00;
    lcd_needsupdate = 1;
}

inline void lcd_set_pixel(int x, int y)
{
    static int first_report = 0;

    unsigned bo = y * LCD_SCANLINE + (LCD_W - x);
    if (bo/8 < sizeof(lcd_buffer))
        lcd_buffer[bo / 8] |= (1 << (bo % 8));
    else if (!first_report++)
        fprintf(stderr, "Clearing outside of lcd_buffer at %u, %u > %u (x=%d, y=%d)\n",
                bo, bo/8, (uint) sizeof(lcd_buffer), x, y);
}

inline void lcd_clear_pixel(int x, int y)
{
    static int first_report = 0;

    unsigned bo = y * LCD_SCANLINE + (LCD_W - x);
    if (bo/8 < sizeof(lcd_buffer))
        lcd_buffer[bo / 8] &= ~(1 << (bo % 8));
    else if (!first_report++)
        fprintf(stderr, "Setting outside of lcd_buffer at %u, %u > %u (x=%d, y=%d)\n",
                bo, bo/8, (uint) sizeof(lcd_buffer), x, y);
}

inline void lcd_pixel(int x, int y, int val)
{
    if (val)
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
            fprintf(stderr,
                    "WARNING: Menu entry %d [%s] is too long "
                    "(%d chars lost, shows as [%s])\n",
                    m, keys[m], (int) strlen(keys[m]) - size + 1, buffer);

        t20->x = x - w / 2;
        lcd_puts(t20, buffer);
    }
    t20->lnfill = 1;
    t20->inv = 0;
}

void lcd_fill_rect(uint32_t x, uint32_t y, uint32_t dx, uint32_t dy, int val)
{
    for (uint r = y; r < y + dy; r++)
        for (uint c = x; c < x + dx; c++)
            lcd_pixel(c, r, val);
}

int lcd_fontWidth(disp_stat_t * ds)
{
    return ds->f->width;
}
int lcd_for_calc(int what)
{
    fprintf(stderr, "lcd_for_calc %d not implemented\n", what);
    return 0;
}
void lcd_forced_refresh()
{
    lcd_needsupdate = 1;
}
int lcd_get_buf_cleared()
{
    return lcd_buf_cleared;
}
int lcd_lineHeight(disp_stat_t * ds)
{
    return ds->f->height;
}
uint8_t * lcd_line_addr(int y)
{
    unsigned offset = y * LCD_SCANLINE / 8;
    return lcd_buffer + offset;
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

void lcd_refresh()
{
    lcd_needsupdate = 1;
}
void lcd_refresh_dma()
{
    lcd_needsupdate = 1;
}
void lcd_refresh_wait()
{
    lcd_needsupdate = 1;
}
void lcd_refresh_lines(int ln, int cnt)
{
    lcd_needsupdate = ln >= 0 && cnt > 0;
}
void lcd_setLine(disp_stat_t * ds, int ln_nr)
{
    ds->x = ds->xoffs;;
    ds->y = ln_nr * lcd_lineHeight(ds);
}

void lcd_setXY(disp_stat_t * ds, int x, int y)
{
    ds->x = x;
    ds->y = y;
}
void lcd_set_buf_cleared(int val)
{
    lcd_buf_cleared = val;
}
void lcd_switchFont(disp_stat_t * ds, int nr)
{
    if (nr >= 0 && nr <= (int) dmcp_fonts_count)
        ds->f = dmcp_fonts[nr];
    else
        fprintf(stderr, "lcd_switchFont not implemented\n");
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

    while ((c = (byte) *text++))
    {
        c -= first;
        if (c >= 0 && c < count)
        {
            uint off = offs[c];
            width += data[off + 0] + data[off + 2] + xspc;
        }
    }
    return width;
}

void lcd_writeClr(disp_stat_t *ds)
{
    ds->x      = ds->xoffs;
    ds->y      = 0;
    ds->inv    = 0;
    ds->bgfill = 1;
    ds->lnfill = 1;
    ds->xspc   = 1;
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

    if (ds->lnfill)
        lcd_fill_rect(ds->xoffs, y, LCD_W, height, inv);

    while ((c = (byte) *text++))
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
            fprintf(stderr, "Character [%d] (%c) not found, max=%d\n",
                    c + first, c + first, count + first);
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
    fprintf(stderr, "rtc_wakeup_delay not implemented\n");
}
void run_help_file(const char * help_file)
{
    fprintf(stderr, "run_help_file not implemented\n");
}
void run_help_file_style(const char * help_file, user_style_fn_t *user_style_fn)
{
    fprintf(stderr, "run_help_file_style not implemented\n");
}
void start_buzzer_freq(uint32_t freq)
{
    fprintf(stderr, "start_buzzer_freq not implemented\n");
}
void stop_buzzer()
{
    fprintf(stderr, "stop_buzzer not implemented\n");
}
void sys_delay(uint32_t ms_delay)
{
    QThread::msleep(ms_delay);
}
void sys_sleep()
{
    while (key_empty() && !timer_interrupt)
        QThread::msleep(20);
}
void wait_for_key_press()
{
    fprintf(stderr, "wait_for_key_press not implemented\n");
}
void wait_for_key_release(int tout)
{
    fprintf(stderr, "wait_for_key_release not implemented\n");
}


int file_selection_screen(const char * title, const char * base_dir, const char * ext, file_sel_fn_t sel_fn,
                          int disp_new, int overwrite_check, void * data)
{
    fprintf(stderr, "file_selection_screen not imlemented\n");
    return 0;
}

int power_check_screen()
{
    fprintf(stderr, "file_selection_screen not imlemented\n");
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
    fprintf(stderr, "f_open not implemented\n");
    return FR_NOT_ENABLED;
}
FRESULT f_close(FIL *fp)
{
    fprintf(stderr, "f_close not implemented\n");
    return FR_NOT_ENABLED;
}

FRESULT f_read(FIL *fp, void *buff, UINT btr, UINT *br)
{
    fprintf(stderr, "f_read not implemented\n");
    return FR_NOT_ENABLED;
}

FRESULT f_write(FIL *fp, const void *buff, UINT btw, UINT *bw)
{
    fprintf(stderr, "f_write not implemented\n");
    return FR_NOT_ENABLED;
}

FRESULT f_lseek(FIL *fp, FSIZE_t ofs)
{
    fprintf(stderr, "f_lseek not implemented\n");
    return FR_NOT_ENABLED;
}

FRESULT f_rename(const TCHAR *path_old, const TCHAR *path_new)
{
    fprintf(stderr, "f_rename not implemented\n");
    return FR_NOT_ENABLED;
}

FRESULT f_unlink(const TCHAR *path)
{
    fprintf(stderr, "f_unlink not implemented\n");
    return FR_NOT_ENABLED;
}
