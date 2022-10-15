// ****************************************************************************
//  input.cc                                                      DB48X project
// ****************************************************************************
//
//   File Description:
//
//     Input to the calculator
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

#include "input.h"

#include "display.h"
#include "menu.h"
#include "runtime.h"
#include "settings.h"
#include "util.h"

#include <dmcp.h>
#include <stdio.h>

// The primary input of the calculator
input Input;

runtime &input::RT = runtime::RT;

input::input()
// ----------------------------------------------------------------------------
//   Initialize the input
// ----------------------------------------------------------------------------
    : cursor(0), xoffset(0), mode(STACK), last(0),
      shift(false), xshift(false), alpha(false), lowercase(false),
      hideMenu(false), down(false), up(false)
{}


bool input::key(int key)
// ----------------------------------------------------------------------------
//   Process an input key
// ----------------------------------------------------------------------------
{
    bool result =
        handle_shifts(key)    ||
        handle_editing(key)   ||
        handle_alpha(key)     ||
        handle_functions(key) ||
        handle_digits(key)    ||
        false;

    if (key && key != KEY_SHIFT)
        shift = false;

    return result;
}


void input::assign(int key, uint plane, object *code)
// ----------------------------------------------------------------------------
//   Assign an object to a given key
// ----------------------------------------------------------------------------
{
    if (key >= 1 && key <= NUM_KEYS && plane <= NUM_PLANES)
        function[plane][key-1] = code;
}


object *input::assigned(int key, uint plane)
// ----------------------------------------------------------------------------
//   Assign an object to a given key
// ----------------------------------------------------------------------------
{
    if (key >= 1 && key <= NUM_KEYS && plane <= NUM_PLANES)
        return function[plane][key-1];
    return nullptr;
}


void input::menus(cstring labels[input::NUM_MENUS],
                  object *function[input::NUM_MENUS])
// ----------------------------------------------------------------------------
//   Assign all menus at once
// ----------------------------------------------------------------------------
{
    for (int m = 0; m < NUM_MENUS; m++)
        menu(m, labels[m], function[m]);
}


void input::menu(uint menu_id, cstring label, object *fn)
// ----------------------------------------------------------------------------
//   Assign one menu item
// ----------------------------------------------------------------------------
{
    if (menu_id < NUM_MENUS)
    {
        int softkey_id = menu_id % NUM_SOFTKEYS;
        int key = KEY_F1 + softkey_id;
        int plane = menu_id / NUM_SOFTKEYS;
        function[plane][key] = fn;
        strncpy(menu_label[plane][softkey_id], label, NUM_LABEL_CHARS);
    }
}


void input::draw_menus()
// ----------------------------------------------------------------------------
//   Draw the softkey menus
// ----------------------------------------------------------------------------
{
    int plane = shift_plane();
    cstring labels[NUM_SOFTKEYS];
    for (int k = 0; k < NUM_SOFTKEYS; k++)
        labels[k] = menu_label[plane][k];
    lcd_draw_menu_keys(labels);
}


void input::draw_annunciators()
// ----------------------------------------------------------------------------
//    Draw the annunciators for Shift, Alpha, etc
// ----------------------------------------------------------------------------
{
    // Don't clear line (we expect dark background already drawn)
    if (alpha)
    {
        cstring label = lowercase ? "abc" : "ABC";
        display dann(t20);
        int     w = dann.width(label);
        int     h = dann.lineHeight();
        dann.clearing(false)
            .x(LCD_W - w - 3)
            .y(h + 2)
            .write(label);
    }
    if (shift)
    {
        const uint ann_height = 12;
        static const byte ann_right[] =
        {
            0xfe, 0x3f, 0xff, 0x7f, 0x9f, 0x7f,
            0xcf, 0x7f, 0xe7, 0x7f, 0x03, 0x78,
            0x03, 0x70, 0xe7, 0x73, 0xcf, 0x73,
            0x9f, 0x73, 0xff, 0x73, 0xfe, 0x33
        };
        static const byte ann_left[] =
        {
            0xfe, 0x3f, 0xff, 0x7f, 0xff, 0x7c,
            0xff, 0x79, 0xff, 0x73, 0x0f, 0x60,
            0x07, 0x60, 0xe7, 0x73, 0xe7, 0x79,
            0xe7, 0x7c, 0xe7, 0x7f, 0xe6, 0x3f
        };
        const byte *source = xshift ? ann_right : ann_left;
        int top = lcd_lineHeight(t20) + 2;
        for (uint r = 0; r < ann_height; r++)
        {
            byte *dest = lcd_line_addr(r + top) + 48;
            dest[0] = ~*source++;
            dest[1] = ~*source++;
        }
    }
}


void input::draw_editor()
// ----------------------------------------------------------------------------
//   Draw the editor
// ----------------------------------------------------------------------------
{
    // Get the editor area
    char  *ed      = RT.editor();
    size_t len     = RT.editing();
    char  *last    = ed + len;
    char  *edline  = ed;

    if (!len)
        return;

    // Count rows and colums
    int     rows   = 1; // Number of rows in editor
    int     column = 0; // Current column
    int     cwidth = 0; // Column width
    int     edrow  = 0; // Row number of line being edited
    int     edcol  = 0; // Column of line being edited
    int     cursx  = 0; // Cursor X position
    display dtxt(fReg);
    display dcsr(t20);

    for (char *p = ed; p <= last; p++)
    {
        if (p - ed == (int) cursor)
        {
            edrow  = rows - 1;
            edcol  = column;
            edline = p - edcol;
            cursx  = cwidth;
        }
        if (p == last)
            break;

        if (*p == '\n')
        {
            rows++;
            column = 0;
            cwidth = 0;
        }
        else
        {
            column++;
            cwidth += dtxt.width((byte) *p);
        }
    }

    // Check if we want to move the cursor up or down
    if (up || down)
    {
        int r = 0;
        int c = 0;
        int tgt = edrow - up + down;
        bool done = false;
        for (char *p = ed; p < last && !done; p++)
        {
            if (*p == '\n')
            {
                r++;
                c = 0;
            }
            else
            {
                c++;
            }
            if ((r == tgt && c >= edcol) || r > tgt)
            {
                cursor = p - ed;
                edrow = r;
                edline = p - c;
                done = true;
            }
        }
        up   = false;
        down = false;
    }

    // Draw the area that fits on the screen
    int   lineHeight      = dtxt.lineHeight();
    int   top             = dcsr.lineHeight() + 2;
    int   bottom          = LCD_H - (hideMenu ? 0 : LCD_MENU_LINES);
    int   availableHeight = bottom - top;
    int   availableRows   = availableHeight / lineHeight;
    char *display         = ed;

    if (rows > availableRows)
    {
        // Skip rows to show the cursor
        int skip =
            edrow < availableRows ? 0 :
            edrow >= rows - availableRows ? rows - availableRows :
            edrow - availableRows / 2;
        for (int r = 0; r < skip; r++)
            while (*display != '\n')
                display++;
        rows = availableRows;
    }

    // Draw the editor rows
    int skip = 64;
    int cursw = t20->f->width;
    if (xoffset > cursx)
        xoffset = (cursx > skip) ? cursx - skip : 0;
    else if (xoffset + LCD_W - cursw < cursx)
        xoffset = cursx - LCD_W + cursw + skip;

    int y        = bottom - rows * lineHeight;
    int x        = -xoffset;
    dtxt.xy(x, y)
        .clearing(false).background(false);
    dcsr.clearing(false).background(true).inverted(true);

    for (int r = 0; r < rows && display < last; r++)
    {
        int drawCursor = display == edline;
        while (display < last && *display != '\n')
        {
            int c = *display++;
            int cw = dtxt.width(c);
            if (dtxt.x() >= 0 && dtxt.x() + cw < LCD_W)
            {
                const char buf[2] = { (char) c, 0 };
                dtxt.write(buf);
            }
            else
            {
                dtxt.x(dtxt.x() + cw);
            }
        }

        if (drawCursor)
        {
            char cursorChar =
                mode == DIRECT    ? 'd' :
                mode == TEXT      ? (lowercase ? 'l' : 'c') :
                mode == PROGRAM   ? 'p' :
                mode == ALGEBRAIC ? 'a' :
                mode == MATRIX    ? 'm' : 'x';
            char buf[2] = { cursorChar, 0 };
            lcd_fill_rect(x + cursx, y, 2, lineHeight, 1);
            dcsr.x(x + cursx)
                .y(y + (lineHeight - top) / 2 + 1)
                .write(buf);
        }

        dtxt.x(x)
            .y(dtxt.y() + lineHeight);
    }
}


void input::draw_error()
// ----------------------------------------------------------------------------
//   Draw the error message if there is one
// ----------------------------------------------------------------------------
{
    if (cstring err = RT.error())
    {
        const int border = 4;
        display   derr(fReg);
        display   dhdr(t20);

        derr.font(5);

        int top        = dhdr.lineHeight() + 10;
        int height     = LCD_H / 3;
        int width      = LCD_W - 20;
        int x          = LCD_W / 2 - width / 2;
        int y          = top;

        lcd_fill_rect(x, y, width, height, 1);
        lcd_fill_rect(x + border, y + border,
                      width - 2*border, height - 2*border, 0);

        x += 2*border+1;
        y += 2*border+1;

        derr.xy(x, y).clearing(false).background(false);
        if (cstring cmd = RT.command())
            derr.write("%s error:", cmd).newline();

        derr.write(err);
    }
}


bool input::handle_shifts(int key)
// ----------------------------------------------------------------------------
//   Handle status changes in shift keys
// ----------------------------------------------------------------------------
{
    bool consumed = false;
    if (key == KEY_SHIFT)
    {
#define SHM(d, a, s)    ((d<<2) | (a<<1) | (s<<0))
#define SHD(d, a, s)    (1 << SHM(d, a, s))
        bool dshift = last == KEY_SHIFT; // Double shift toggles alpha
        int plane = SHM(dshift, alpha, shift);
        const unsigned nextShift =
            SHD(0, 0, 0) |
            SHD(0, 1, 0) |
            SHD(1, 0, 0) |
            SHD(1, 1, 0);
        const unsigned nextAlpha =
            SHD(0, 0, 1) |
            SHD(0, 1, 0) |
            SHD(0, 1, 1) |
            SHD(1, 0, 1);
        shift = (nextShift & (1 << plane)) != 0;
        alpha = (nextAlpha & (1 << plane)) != 0;
        consumed = true;
#undef SHM
#undef SHD
    }

    if (key)
    {
        last = key;
    }
    return consumed;
}


bool input::handle_editing(int key)
// ----------------------------------------------------------------------------
//   Some keys always deal with editing
// ----------------------------------------------------------------------------
{
    bool   consumed = false;
    size_t editing  = RT.editing();

    if (editing)
    {
        switch(key)
        {
        case KEY_BSP:
            if (shift && cursor < editing)
            {
                // Shift + Backspace = Delete to right of cursor
                RT.remove(cursor, 1);
            }
            else if (!shift && cursor > 0)
            {
                // Backspace = Erase on left of cursor
                cursor--;
                RT.remove(cursor, 1);
            }
            else
            {
                // Limits of line: beep
                beep(4400, 50);
            }
            return true;
        case KEY_ENTER:
        {
            if (shift)
            {
                // TODO: Show Alpha menu
                // For now, shift lowercase
                lowercase = !lowercase;

            }
            else
            {
                // Finish editing and parse the result
                object *obj = object::parse(RT.editor());
                if (obj)
                {
                    // We successfully parsed the line
                    RT.clear();
                    shift = false;
                    alpha = false;
                    cursor = 0;
                    xoffset = 0;
                    obj->evaluate();
                }
                else
                {
                    cstring pos = RT.source();
                    cstring ed = RT.editor();
                    if (pos >= ed && pos <= ed + editing)
                        cursor = pos - ed;
                    beep(3300, 100);
                }
            }
            return true;
        }
        case KEY_EXIT:
            if (shift)
            {
                SET_ST(STAT_PGM_END);
                shift = false;
                alpha = false;
            }
            else if (RT.error())
            {
                // Clear error
                RT.error(nullptr);
                shift = false;
            }
            else
            {
                // Clear the editor
                RT.clear();
                shift = false;
                alpha = false;
            }
            return true;

        case KEY_UP:
            if (shift)
                up = true;
            else if (cursor > 0)
                cursor--;
            else
                beep(4000, 50);
            return true;
        case KEY_DOWN:
            if (shift)
                down = true;
            else if (cursor < editing)
                cursor++;
            else
                beep(4800, 50);
            return true;
        case 0:
            return false;
        }

    }
    else
    {
        switch(key)
        {
        case KEY_BSP:
            // RT.evaluate(ID_drop);
            return true;
        case KEY_ENTER:
            // RT.evaluate(ID_dup);
            return true;
        case KEY_EXIT:
            if (shift)
                SET_ST(STAT_PGM_END);
            shift = false;
            alpha = false;
            return true;
        case KEY_UP:
            // RT.evaluate(ID_stack);
            return false;
        case KEY_DOWN:
            // Bring the object to the editor
            return false;
        case 0:
            return false;
        }
    }

    return consumed;
}


bool input::handle_alpha(int key)
// ----------------------------------------------------------------------------
//    Handle alphabetic input
// ----------------------------------------------------------------------------
{
    if (!alpha || !key)
        return false;

    static const char upper[] =
        "ABCDEF"
        "GHIJKL"
        "_MNO_"
        "_PQRS"
        "_TUVW"
        "_XYZ_"
        "_:.? ";
    static const char lower[] =
        "abcdef"
        "ghijkl"
        "_mno_"
        "_pqrs"
        "_tuvw"
        "_xyz-"
        "_:.? ";
    static const char shifted[] =
        "\x85^\x82([{"
        "\x86%\x87<=>"
        "_\"'\x98_"
        "_789\x80"
        "_456\x81"
        "_123-"
        "_;,!+";

    key--;
    char c = shift ? shifted[key] : lowercase ? lower[key] : upper[key];
    RT.insert(cursor, c);
    cursor++;

    // Test delimiters
    int closing = 0;
    switch(c)
    {
    case '(':  closing = ')'; break;
    case '[':  closing = ']'; break;
    case '{':  closing = '}'; break;
    case ':':  closing = ':'; break;
    case '"':  closing = '"'; break;
    case '\'': closing = '\''; break;
    }
    if (closing)
        RT.insert(cursor, closing);
    shift = false;

    return 1;
}


bool input::handle_digits(int key)
// ----------------------------------------------------------------------------
//    Handle alphabetic input
// ----------------------------------------------------------------------------
{
    if (alpha || !key)
        return false;

    static const char numbers[] =
        "abcdef"
        "ghijkl"
        "_m-\x98_"
        "_789\x80"
        "_456\x81"
        "_123-"
        "_;, +";

    if (key == KEY_CHS)
    {
        // Special case for change of sign
        char *ed = RT.editor();
        char *p  = ed + cursor;
        while (p > ed)
        {
            p--;
            if ((*p < '0' || *p > '9') && *p != Settings.decimalDot)
                break;
        }

        if (*p == '-' || *p == '+')
        {
            *p = '+' + '-' - *p;
            return true;
        }
        else
        {
            RT.insert(p - ed, '-');
            cursor++;
        }

    }
    else if (key > KEY_CHS)
    {
        key--;
        char c = numbers[key];
        RT.insert(cursor, c);
        cursor++;
        shift = false;
        return true;
    }
    return false;
}



// ============================================================================
//
//   Tables with the default assignments
//
// ============================================================================

static const byte defaultUnshiftedCommand[2*input::NUM_KEYS] =
// ----------------------------------------------------------------------------
//   RPL code for the commands assigned by default to each key
// ----------------------------------------------------------------------------
//   All the default-assigned commands fit in one or two bytes
{
#define OP2BYTES(key, id)                       \
    (id) < 0x80 ? (id) : ((id) & 0x7F) | 0x80,  \
    (id) < 0x80 ?   0  : ((id) >> 7)

    OP2BYTES(KEY_SIGMA, 0),
    OP2BYTES(KEY_INV, 0),
    OP2BYTES(KEY_SQRT, 0),
    OP2BYTES(KEY_LOG, 0),
    OP2BYTES(KEY_LN, 0),
    OP2BYTES(KEY_XEQ, 0),
    OP2BYTES(KEY_STO, 0),
    OP2BYTES(KEY_RCL, 0),
    OP2BYTES(KEY_RDN, 0),
    OP2BYTES(KEY_SIN, 0),
    OP2BYTES(KEY_COS, 0),
    OP2BYTES(KEY_TAN, 0),
    OP2BYTES(KEY_ENTER, 0),
    OP2BYTES(KEY_SWAP, 0),
    OP2BYTES(KEY_CHS, 0),
    OP2BYTES(KEY_E, 0),
    OP2BYTES(KEY_BSP, 0),
    OP2BYTES(KEY_UP, 0),
    OP2BYTES(KEY_7, 0),
    OP2BYTES(KEY_8, 0),
    OP2BYTES(KEY_9, 0),
    OP2BYTES(KEY_DIV, 0),
    OP2BYTES(KEY_DOWN, 0),
    OP2BYTES(KEY_4, 0),
    OP2BYTES(KEY_5, 0),
    OP2BYTES(KEY_6, 0),
    OP2BYTES(KEY_MUL, 0),
    OP2BYTES(KEY_SHIFT, 0),
    OP2BYTES(KEY_1, 0),
    OP2BYTES(KEY_2, 0),
    OP2BYTES(KEY_3, 0),
    OP2BYTES(KEY_SUB, 0),
    OP2BYTES(KEY_EXIT, 0),
    OP2BYTES(KEY_0, 0),
    OP2BYTES(KEY_DOT, 0),
    OP2BYTES(KEY_RUN, 0),
    OP2BYTES(KEY_ADD, 0),

    OP2BYTES(KEY_F1, 0),
    OP2BYTES(KEY_F2, 0),
    OP2BYTES(KEY_F3, 0),
    OP2BYTES(KEY_F4, 0),
    OP2BYTES(KEY_F5, 0),
    OP2BYTES(KEY_F6, 0),

    OP2BYTES(KEY_SCREENSHOT, 0),
    OP2BYTES(KEY_SH_UP, 0),
    OP2BYTES(KEY_SH_DOWN, 0),
};


static const byte defaultShiftedCommand[2*input::NUM_KEYS] =
// ----------------------------------------------------------------------------
//   RPL code for the commands assigned by default to shifted keys
// ----------------------------------------------------------------------------
//   All the default assigned commands fit in one or two bytes
{
    OP2BYTES(KEY_SIGMA, 0),
    OP2BYTES(KEY_INV, 0),
    OP2BYTES(KEY_SQRT, 0),
    OP2BYTES(KEY_LOG, 0),
    OP2BYTES(KEY_LN, 0),
    OP2BYTES(KEY_XEQ, 0),
    OP2BYTES(KEY_STO, 0),
    OP2BYTES(KEY_RCL, 0),
    OP2BYTES(KEY_RDN, 0),
    OP2BYTES(KEY_SIN, 0),
    OP2BYTES(KEY_COS, 0),
    OP2BYTES(KEY_TAN, 0),
    OP2BYTES(KEY_ENTER, 0),
    OP2BYTES(KEY_SWAP, 0),
    OP2BYTES(KEY_CHS, 0),
    OP2BYTES(KEY_E, 0),
    OP2BYTES(KEY_BSP, 0),
    OP2BYTES(KEY_UP, 0),
    OP2BYTES(KEY_7, 0),
    OP2BYTES(KEY_8, 0),
    OP2BYTES(KEY_9, 0),
    OP2BYTES(KEY_DIV, 0),
    OP2BYTES(KEY_DOWN, 0),
    OP2BYTES(KEY_4, 0),
    OP2BYTES(KEY_5, 0),
    OP2BYTES(KEY_6, 0),
    OP2BYTES(KEY_MUL, 0),
    OP2BYTES(KEY_SHIFT, 0),
    OP2BYTES(KEY_1, 0),
    OP2BYTES(KEY_2, 0),
    OP2BYTES(KEY_3, 0),
    OP2BYTES(KEY_SUB, 0),
    OP2BYTES(KEY_EXIT, 0),
    OP2BYTES(KEY_0, 0),
    OP2BYTES(KEY_DOT, 0),
    OP2BYTES(KEY_RUN, 0),
    OP2BYTES(KEY_ADD, 0),

    OP2BYTES(KEY_F1, 0),
    OP2BYTES(KEY_F2, 0),
    OP2BYTES(KEY_F3, 0),
    OP2BYTES(KEY_F4, 0),
    OP2BYTES(KEY_F5, 0),
    OP2BYTES(KEY_F6, 0),

    OP2BYTES(KEY_SCREENSHOT, 0),
    OP2BYTES(KEY_SH_UP, 0),
    OP2BYTES(KEY_SH_DOWN, 0),
};


static const byte defaultLongShiftedCommand[2*input::NUM_KEYS] =
// ----------------------------------------------------------------------------
//   RPL code for the commands assigned by default to long-shifted keys
// ----------------------------------------------------------------------------
//   All the default assigned commands fit in one or two bytes
{
    OP2BYTES(KEY_SIGMA, 0),
    OP2BYTES(KEY_INV, 0),
    OP2BYTES(KEY_SQRT, 0),
    OP2BYTES(KEY_LOG, 0),
    OP2BYTES(KEY_LN, 0),
    OP2BYTES(KEY_XEQ, 0),
    OP2BYTES(KEY_STO, 0),
    OP2BYTES(KEY_RCL, 0),
    OP2BYTES(KEY_RDN, 0),
    OP2BYTES(KEY_SIN, 0),
    OP2BYTES(KEY_COS, 0),
    OP2BYTES(KEY_TAN, 0),
    OP2BYTES(KEY_ENTER, 0),
    OP2BYTES(KEY_SWAP, 0),
    OP2BYTES(KEY_CHS, 0),
    OP2BYTES(KEY_E, 0),
    OP2BYTES(KEY_BSP, 0),
    OP2BYTES(KEY_UP, 0),
    OP2BYTES(KEY_7, 0),
    OP2BYTES(KEY_8, 0),
    OP2BYTES(KEY_9, 0),
    OP2BYTES(KEY_DIV, 0),
    OP2BYTES(KEY_DOWN, 0),
    OP2BYTES(KEY_4, 0),
    OP2BYTES(KEY_5, 0),
    OP2BYTES(KEY_6, 0),
    OP2BYTES(KEY_MUL, 0),
    OP2BYTES(KEY_SHIFT, 0),
    OP2BYTES(KEY_1, 0),
    OP2BYTES(KEY_2, 0),
    OP2BYTES(KEY_3, 0),
    OP2BYTES(KEY_SUB, 0),
    OP2BYTES(KEY_EXIT, 0),
    OP2BYTES(KEY_0, 0),
    OP2BYTES(KEY_DOT, 0),
    OP2BYTES(KEY_RUN, 0),
    OP2BYTES(KEY_ADD, 0),

    OP2BYTES(KEY_F1, 0),
    OP2BYTES(KEY_F2, 0),
    OP2BYTES(KEY_F3, 0),
    OP2BYTES(KEY_F4, 0),
    OP2BYTES(KEY_F5, 0),
    OP2BYTES(KEY_F6, 0),

    OP2BYTES(KEY_SCREENSHOT, 0),
    OP2BYTES(KEY_SH_UP, 0),
    OP2BYTES(KEY_SH_DOWN, 0),
};


static const byte *const defaultCommand[input::NUM_PLANES] =
// ----------------------------------------------------------------------------
//   Pointers to the default commands
// ----------------------------------------------------------------------------
{
    defaultUnshiftedCommand,
    defaultShiftedCommand,
    defaultLongShiftedCommand,
};


bool input::handle_functions(int key)
// ----------------------------------------------------------------------------
//   Check if we have one of the soft menu functions
// ----------------------------------------------------------------------------
{
    if (!key)
        return false;

    // Hard-code system menu
    if (!alpha && shift && key == KEY_0)
    {
        SET_ST(STAT_MENU);
        handle_menu(&application_menu, MENU_RESET, 0);
        CLR_ST(STAT_MENU);
        wait_for_key_release(-1);
        return true;
    }


    int plane = shift_plane();
    object *obj = function[plane][key-1];
    if (obj)
    {
        obj->evaluate();
        return true;
    }
    const byte *ptr = defaultCommand[plane] + key-1;
    if (*ptr)
    {
        obj = (object *) ptr;   // Uh oh! Evaluate bytecode in ROM
        obj->evaluate();
        return true;
    }


    return false;
}
