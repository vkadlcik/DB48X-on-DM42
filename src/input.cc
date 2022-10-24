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

#include "command.h"
#include "graphics.h"
#include "menu.h"
#include "runtime.h"
#include "settings.h"
#include "target.h"
#include "util.h"

#include <dmcp.h>
#include <stdio.h>

// The primary input of the calculator
input    Input;

RECORDER(input, 16, "Input processing");

runtime &input::RT = runtime::RT;

input::input()
// ----------------------------------------------------------------------------
//   Initialize the input
// ----------------------------------------------------------------------------
    : cursor(0),
      xoffset(0),
      mode(STACK),
      last(0),
      stack(LCD_H),
      cx(0),
      cy(0),
      cchar(' '),
      shift(false),
      xshift(false),
      alpha(false),
      lowercase(false),
      hideMenu(false),
      down(false),
      up(false),
      repeat(false),
      longpress(false),
      blink(false)
{
}


bool input::end_edit()
// ----------------------------------------------------------------------------
//   Clear the editor
// ----------------------------------------------------------------------------
{
    size_t edlen = RT.editing();
    if (edlen)
    {
        gcutf8 editor = RT.close_editor();
        if (editor)
        {
            gcobj obj = object::parse(editor);
            if (obj)
            {
                // We successfully parsed the line
                clear_editor();
                obj->evaluate();
            }
            else
            {
                // Move cursor to error if there is one
                utf8 pos = RT.source();
                utf8 ed = editor;
                if (pos >= editor && pos <= ed + edlen)
                    cursor = pos - ed;
                if (!RT.edit(ed, edlen))
                    cursor = 0;
                beep(3300, 100);
                return false;
            }
        }
    }
    alpha = shift = xshift = false;
    return true;
}


void input::clear_editor()
// ----------------------------------------------------------------------------
//   Clear the editor either after edit, or when pressing EXIT
// ----------------------------------------------------------------------------
{
    RT.clear();
    cursor    = 0;
    xoffset   = 0;
    alpha     = false;
    shift     = false;
    xshift    = false;
    lowercase = false;
    longpress = false;
    repeat    = false;
}


bool input::key(int key)
// ----------------------------------------------------------------------------
//   Process an input key
// ----------------------------------------------------------------------------
{
    longpress = key && sys_timer_timeout(TIMER0);
    record(input, "Key %d shifts %d longpress", key, shift_plane(), longpress);
    sys_timer_disable(TIMER0);
    repeat = false;

    if (RT.error())
    {
        if (key == KEY_EXIT || key == KEY_ENTER || key == KEY_BSP)
            RT.error(nullptr);
        else if (key)
            beep(2200, 75);
        return true;
    }

    bool result =
        handle_shifts(key)    ||
        handle_editing(key)   ||
        handle_alpha(key)     ||
        handle_digits(key)    ||
        handle_functions(key) ||
        false;


    if (!key && last != KEY_SHIFT)
    {
        shift = false;
        xshift = false;
    }

    if (key && result)
    {
        // Initiate key repeat
        if (repeat)
            sys_timer_start(TIMER0, longpress ? 80 : 500);
        blink = true; // Show cursor if things changed
        sys_timer_disable(TIMER1);
        sys_timer_start(TIMER1, 300);
    }

    return result;
}


void input::assign(int key, uint plane, object *code)
// ----------------------------------------------------------------------------
//   Assign an object to a given key
// ----------------------------------------------------------------------------
{
    if (key >= 1 && key <= NUM_KEYS && plane <= NUM_PLANES)
        function[plane][key - 1] = code;
}


object *input::assigned(int key, uint plane)
// ----------------------------------------------------------------------------
//   Assign an object to a given key
// ----------------------------------------------------------------------------
{
    if (key >= 1 && key <= NUM_KEYS && plane <= NUM_PLANES)
        return function[plane][key - 1];
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
        int softkey_id       = menu_id % NUM_SOFTKEYS;
        int key              = KEY_F1 + softkey_id;
        int plane            = menu_id / NUM_SOFTKEYS;
        function[plane][key] = fn;
        strncpy(menu_label[plane][softkey_id], label, NUM_LABEL_CHARS);
    }
}


void input::draw_menus()
// ----------------------------------------------------------------------------
//   Draw the softkey menus
// ----------------------------------------------------------------------------
{
    int     plane = shift_plane();
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
        utf8 label = utf8(lowercase ? "abc" : "ABC");
        Screen.text(360, 1, label, HeaderFont, pattern::white);
    }

    const uint  ann_width  = 15;
    const uint  ann_height = 12;
    const byte *source     = nullptr;
    if (xshift)
    {
        static const byte ann_right[] =
        {
            0xfe, 0x3f, 0xff, 0x7f, 0x9f, 0x7f,
            0xcf, 0x7f, 0xe7, 0x7f, 0x03, 0x78,
            0x03, 0x70, 0xe7, 0x73, 0xcf, 0x73,
            0x9f, 0x73, 0xff, 0x73, 0xfe, 0x33
        };
        source = ann_right;
    }
    if (shift)
    {
        static const byte ann_left[] =
        {
            0xfe, 0x3f, 0xff, 0x7f, 0xff, 0x7c,
            0xff, 0x79, 0xff, 0x73, 0x0f, 0x60,
            0x07, 0x60, 0xe7, 0x73, 0xe7, 0x79,
            0xe7, 0x7c, 0xe7, 0x7f, 0xe6, 0x3f
        };
        source = ann_left;
    }
    if (source)
    {
        pixword *sw = (pixword *) source;
        surface s(sw, ann_width, ann_height, 16);
        Screen.copy(s, 340, (HeaderFont->height() - ann_height) / 2);
    }

    // Temporary - Display some internal information
    char            buffer[64];
    static unsigned counter = 0;
    snprintf(buffer, sizeof(buffer), "%c %u", longpress ? 'L' : ' ',
             counter++);
    Screen.text(120, 0, utf8(buffer), HeaderFont, pattern::white);
}


void input::draw_editor()
// ----------------------------------------------------------------------------
//   Draw the editor
// ----------------------------------------------------------------------------
{
    // Get the editor area
    utf8   ed   = RT.editor();
    size_t len  = RT.editing();
    utf8   last = ed + len;
    font_p font = EditorFont;

    if (!len)
    {
        // Editor is not open, compute stack bottom
        stack = LCD_H - (hideMenu ? 0 : LCD_MENU_LINES);
        return;
    }

    // Count rows and colums
    int     rows   = 1; // Number of rows in editor
    int     column = 0; // Current column
    int     cwidth = 0; // Column width
    int     edrow  = 0; // Row number of line being edited
    int     edcol  = 0; // Column of line being edited
    int     cursx  = 0; // Cursor X position

    for (utf8 p = ed; p <= last; p = utf8_next(p))
    {
        if (p - ed == (int) cursor)
        {
            edrow = rows - 1;
            edcol = column;
            cursx = cwidth;
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
            utf8code cp = utf8_codepoint(p);
            cwidth += font->width(cp);
        }
    }

    // Check if we want to move the cursor up or down
    if (up || down)
    {
        int  r    = 0;
        int  c    = 0;
        int  tgt  = edrow - up + down;
        bool done = false;
        for (utf8 p = ed; p < last && !done; p = utf8_next(p))
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
            if ((r == tgt && c > edcol) || r > tgt)
            {
                cursor = p - ed;
                edrow  = r;
                done   = true;
            }
        }
        up   = false;
        down = false;
    }

    // Draw the area that fits on the screen
    int   lineHeight      = font->height();
    int   top             = HeaderFont->height() + 2;
    int   bottom          = LCD_H - (hideMenu ? 0 : LCD_MENU_LINES);
    int   availableHeight = bottom - top;
    int   availableRows   = availableHeight / lineHeight;
    utf8  display         = ed;

    graphics::rect clip = Screen.clip();
    Screen.clip(0, top, LCD_W, bottom);
    Screen.fill(pattern::white);
    if (rows > availableRows)
    {
        // Skip rows to show the cursor
        int skip = edrow < availableRows         ? 0
                 : edrow >= rows - availableRows ? rows - availableRows
                                                 : edrow - availableRows / 2;
        for (int r = 0; r < skip; r++)
            while (*display != '\n')
                display = utf8_next(display);
        rows = availableRows;
    }

    // Draw the editor rows
    int skip  = 64;
    size cursw = EditorFont->width('M');
    if (xoffset > cursx)
        xoffset = (cursx > skip) ? cursx - skip : 0;
    else if (xoffset + LCD_W - cursw < cursx)
        xoffset = cursx - LCD_W + cursw + skip;

    coord y = bottom - rows * lineHeight;
    coord x = -xoffset;
    stack = y;

    cchar = 0;
    int r = 0;
    while (r < rows && display <= last)
    {
        bool atCursor = display == ed + cursor;
        utf8code c = utf8_codepoint(display);
        display = utf8_next(display);
        if (atCursor)
        {
            cx    = x;
            cy    = y;
            cchar = display <= last ? c : ' ';
        }
        if (c == '\n')
        {
            y += lineHeight;
            r++;
            continue;
        }
        if (display > last)
            break;

        int cw = font->width(c);
        if (x +cw >= 0 && x < LCD_W)
            x = Screen.glyph(x, y, c, font);
        else
            x += cw;
    }
    if (!cchar)
    {
        cx    = x;
        cy    = y;
        cchar = ' ';
    }

    Screen.clip(clip);
}


int input::draw_cursor()
// ----------------------------------------------------------------------------
//   Draw the cursor at the location
// ----------------------------------------------------------------------------
//   This function returns the cursor vertical position for screen refresh
{
    // Do not draw
    if (!RT.editing())
    {
        sys_timer_disable(TIMER1);
        return -1;
    }

    size ch = EditorFont->height();
    size cw = EditorFont->width(cchar);
    Screen.fill(cx, cy, cx + cw - 1, cy + ch - 1, pattern::gray75);

    // Write the character under the cursor
    Screen.glyph(cx, cy, cchar, EditorFont);

    if (blink)
    {
        utf8code cursorChar = mode == DIRECT    ? 'd'
                            : mode == TEXT      ? (lowercase ? 'l' : 'c')
                            : mode == PROGRAM   ? 'p'
                            : mode == ALGEBRAIC ? 'a'
                            : mode == MATRIX    ? 'm'
                                                : 'x';
        size     csrh       = CursorFont->height();
        size csrw = CursorFont->width(cursorChar);
        coord csrx = cx;
        coord csry = cy + (ch - csrh)/2;
        Screen.fill(csrx, cy, csrx+1, cy + ch - 1, pattern::black);
        Screen.fill(csrx, csry, csrx + csrw-1, csry + csrh-1, pattern::black);
        Screen.glyph(csrx, csry, cursorChar, CursorFont, pattern::white);
    }

    if (sys_timer_timeout(TIMER1) || !sys_timer_active(TIMER1))
    {
        // Refresh the cursor after 500ms
        sys_timer_disable(TIMER1);
        sys_timer_start(TIMER1, 500);
        blink = !blink;
    }

    return cy;
}


void input::draw_error()
// ----------------------------------------------------------------------------
//   Draw the error message if there is one
// ----------------------------------------------------------------------------
{
    if (utf8 err = RT.error())
    {
        const int border = 4;
        coord top    = HeaderFont->height() + 10;
        coord height = LCD_H / 3;
        coord width  = LCD_W - 8;
        coord x      = LCD_W / 2 - width / 2;
        coord y      = top;

        graphics::rect clip = Screen.clip();
        graphics::rect rect(x, y, x + width - 1, y + height - 1);
        Screen.fill(rect, pattern::gray50);
        rect.inset(border);
        Screen.fill(rect, pattern::white);
        rect.inset(2);

        Screen.clip(rect);
        if (utf8 cmd = RT.command())
        {
            coord x = Screen.text(rect.x1, rect.y1, cmd, ErrorFont);
            Screen.text(x, rect.y1, utf8(" error:"), ErrorFont);
        }
        else
        {
            Screen.text(rect.x1, rect.y1, utf8("Error:"), ErrorFont);
        }
        rect.y1 += ErrorFont->height();
        Screen.text(rect.x1, rect.y1, err, ErrorFont);
        Screen.clip(clip);
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
        if (longpress)
        {
            xshift = true;
            shift  = false;
        }
        else if (xshift)
        {
            xshift = false;
        }
        else
        {
            xshift = false;
#define SHM(d, a, s) ((d << 2) | (a << 1) | (s << 0))
#define SHD(d, a, s) (1 << SHM(d, a, s))
            bool dshift = last == KEY_SHIFT; // Double shift toggles alpha
            int  plane  = SHM(dshift, alpha, shift);
            const unsigned nextShift =
                SHD(0, 0, 0) | SHD(0, 1, 0) | SHD(1, 0, 0);
            const unsigned nextAlpha =
                SHD(0, 0, 1) | SHD(0, 1, 0) | SHD(0, 1, 1) | SHD(1, 0, 1);
            shift  = (nextShift & (1 << plane)) != 0;
            alpha  = (nextAlpha & (1 << plane)) != 0;
            repeat = true;
        }
        consumed = true;
#undef SHM
#undef SHD
    }

    if (key)
        last = key;
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
        record(input, "Editing key %d", key);
        switch (key)
        {
        case KEY_BSP:
            repeat = true;
            if (shift && cursor < editing)
            {
                // Shift + Backspace = Delete to right of cursor
                utf8 ed = RT.editor();
                uint after = utf8_next(ed, cursor, editing);
                RT.remove(cursor, after - cursor);
            }
            else if (!shift && cursor > 0)
            {
                // Backspace = Erase on left of cursor
                utf8 ed = RT.editor();
                uint before = cursor;
                cursor = utf8_previous(ed, cursor);
                RT.remove(cursor, before - cursor);
            }
            else
            {
                // Limits of line: beep
                repeat = false;
                beep(4400, 50);
            }
            return true;
        case KEY_ENTER:
        {
            if (shift)
            {
                // TODO: Show Alpha menu
                // For now, enter Alpha mode or shift lowercase
                if (alpha)
                    lowercase = !lowercase;
                else
                    alpha = true;
            }
            else if (xshift)
            {
                // Process it as alpha (CR)
                return false;
            }
            else
            {
                // Finish editing and parse the result
                end_edit();
            }
            return true;
        }
        case KEY_EXIT:
            if (shift)
                // Power off
                SET_ST(STAT_PGM_END);
            else if (RT.error())
                // Clear error
                RT.error(nullptr);
            else
                // Clear the editor
                clear_editor();
            return true;

        case KEY_UP:
            repeat = true;
            if (shift)
            {
                up = true;
            }
            else if (xshift)
            {
                cursor = 0;
            }
            else if (cursor > 0)
            {
                utf8 ed = RT.editor();
                cursor = utf8_previous(ed, cursor);
            }
            else
            {
                repeat = false;
                beep(4000, 50);
            }
            return true;
        case KEY_DOWN:
            repeat = true;
            if (shift)
            {
                down = true;
            }
            else if (xshift)
            {
                cursor = editing;
            }
            else if (cursor < editing)
            {
                utf8 ed = RT.editor();
                cursor = utf8_next(ed, cursor, editing);
            }
            else
            {
                repeat = false;
                beep(4800, 50);
            }
            return true;
        case 0:
            return false;
        }

    }
    else
    {
        switch(key)
        {
        case KEY_ENTER:
            if (shift)
            {
                if (alpha)
                    lowercase = !lowercase;
                else
                    alpha = true;
                return true;
            }
            break;
        case KEY_EXIT:
            if (shift)
                SET_ST(STAT_PGM_END);
            alpha = false;
            return true;
        }
    }

    return consumed;
}


bool input::handle_alpha(int key)
// ----------------------------------------------------------------------------
//    Handle alphabetic input
// ----------------------------------------------------------------------------
{
    if (!alpha || !key || key == KEY_ENTER || key == KEY_BSP)
        return false;

    static const char upper[] =
        "ABCDEF"
        "GHIJKL"
        "_MNO_"
        "_PQRS"
        "_TUVW"
        "_XYZ_"
        "_:, ;"
        "......";
    static const char lower[] =
        "abcdef"
        "ghijkl"
        "_mno_"
        "_pqrs"
        "_tuvw"
        "_xyz_"
        "_:, ;"
        "......";

#define DIV        "\x80"  // ÷
#define MUL        "\x81"  // ×
#define SQRT       "\x82"  // √
#define INTEG      "\x83"  // ∫
#define FILL       "\x84"  // ▒
#define SIGMA      "\x85"  // Σ
#define STO        "\x86"  // ▶
#define PI         "\x87"  // π
#define INVQ       "\x88"  // ¿
#define LE         "\x89"  // ≤
#define LF         "\x8A"  // ␊
#define GE         "\x8B"  // ≥
#define NE         "\x8C"  // ≠
#define CR         "\x8D"  // ↲
#define DOWN       "\x8E"  // ↓
#define RIGHT      "\x8F"  // →
#define LEFT       "\x90"  // ←
#define MU         "\x91"  // μ
#define POUND      "\x92"  // £
#define DEG        "\x93"  // °
#define ANGST      "\x94"  // Å
#define NTILD      "\x95"  // Ñ
#define ABAR       "\x96"  // Ä
#define ANGLE      "\x97"  // ∡
#define EXP        "\x98"  // ᴇ
#define AE         "\x99"  // Æ
#define ETC        "\x9A"  // …
#define ESC        "\x9B"  // ␛
#define OUML       "\x9C"  // Ö
#define UUML       "\x9D"  // Ü
#define FILL2      "\x9E"  // ▒
#define SQ         "\x9F"  // ■
#define DOWNTRI    "\xA0"  // Down triangle
#define UPTRI      "\xA1"  // Up triangle
#define FREE       "@"

    static const utf8code shifted[] =
    {
        L'Σ', '^', L'√', '(', '[', '{',
        L'▶', '%', L'π', '<', '=', '>',
        '_',  '"', '\'', L'⁳', '_',
        '_', '7', '8', '9', L'÷',
        '_', '4', '5', '6', L'×',
        '_', '1', '2', '3', '-',
        '_', '0', '.',  L'«', '+',
        '.', '.', '.', '.', '.', '.'
    };

    static const  utf8code xshifted[] =
    {
        L'∫', L'↑', L'∜', L'μ', L'∡', L'°',
        L'←', L'→', L'↓', L'≤', L'≠', L'≥',
        '\n', L'⇄', L'…', L'£', '_',
        '_',  '~', '\\', L'∏',  '/',
        '_',  '$',  L'∞', '|' , '*',
        '_',  '&',   '@', '#',  '_',
        '_',  ';',  L'·', '?',  '!',
        '.', '.', '.', '.', '.', '.'
    };

    key--;
    utf8code c =
        xshift    ? xshifted[key] :
        shift     ? shifted[key]  :
        lowercase ? lower[key]    :
        upper[key];
    byte utf8buf[4];
    size_t len = utf8_encode(c, utf8buf);
    record(input, "Codepoint %u reads as %u, length %u: %02X %02X %02X %02X",
           c, utf8_codepoint(utf8buf), len,
           utf8buf[0], utf8buf[1], utf8buf[2], utf8buf[3]);
    cursor += RT.insert(cursor, utf8buf, len);

    // Test delimiters
    utf8code closing = 0;
    switch(c)
    {
    case '(':  closing = ')'; break;
    case '[':  closing = ']'; break;
    case '{':  closing = '}'; break;
    case ':':  closing = ':'; break;
    case '"':  closing = '"'; break;
    case '\'': closing = '\''; break;
    case L'«': closing = L'»'; break;
    }
    if (closing)
    {
        len = utf8_encode(closing, utf8buf);
        RT.insert(cursor, utf8buf, len);
    }
    repeat = true;
    return true;
}


bool input::handle_digits(int key)
// ----------------------------------------------------------------------------
//    Handle alphabetic input
// ----------------------------------------------------------------------------
{
    if (alpha || !key)
        return false;

    static const char numbers[] =
        "______"
        "______"
        "__-__"
        "_789_"
        "_456_"
        "_123_"
        "_0.__"
        "_____";

    // Hard-code system menu
    if (!alpha && shift && key == KEY_0)
    {
        SET_ST(STAT_MENU);
        handle_menu(&application_menu, MENU_RESET, 0);
        CLR_ST(STAT_MENU);
        wait_for_key_release(-1);
        return true;
    }

    if (key == KEY_CHS && RT.editing())
    {
        // Special case for change of sign
        byte *ed = RT.editor();
        utf8 p  = ed + cursor;
        while (p > ed)
        {
            p = utf8_previous(p);
            utf8code c = utf8_codepoint(p);
            if ((c < '0' || c > '9') && c != Settings.decimalDot)
                break;
        }

        utf8code c = utf8_codepoint(p);
        if (c == 'e' || c == 'E' || c == Settings.exponentChar)
        {
            p = utf8_next(p);
            c = utf8_codepoint(p);
        }

        if (c == '-' || c == '+')
            *((byte *) p) = '+' + '-' - c;
        else
            cursor += RT.insert(p - ed, '-');
        return true;
    }
    else if (key == KEY_E && RT.editing())
    {
        byte buf[4];
        size_t sz = utf8_encode(Settings.exponentChar, buf);
        cursor += RT.insert(cursor, buf, sz);
        return true;
    }
    else if (key > KEY_CHS)
    {
        key--;
        char c = numbers[key];
        if (c == '_')
            return false;
        if (c == '.')
            c = Settings.decimalDot;
        if (c == '4' && shift)
            c = '#';
        cursor += RT.insert(cursor, c);
        repeat = true;
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
    OP2BYTES(KEY_INV, command::ID_inv),
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
    OP2BYTES(KEY_ENTER, command::ID_dup),
    OP2BYTES(KEY_SWAP, command::ID_swap),
    OP2BYTES(KEY_CHS, command::ID_neg),
    OP2BYTES(KEY_E, 0),
    OP2BYTES(KEY_BSP, command::ID_drop),
    OP2BYTES(KEY_UP, 0),
    OP2BYTES(KEY_7, 0),
    OP2BYTES(KEY_8, 0),
    OP2BYTES(KEY_9, 0),
    OP2BYTES(KEY_DIV, command::ID_div),
    OP2BYTES(KEY_DOWN, 0),
    OP2BYTES(KEY_4, 0),
    OP2BYTES(KEY_5, 0),
    OP2BYTES(KEY_6, 0),
    OP2BYTES(KEY_MUL, command::ID_mul),
    OP2BYTES(KEY_SHIFT, 0),
    OP2BYTES(KEY_1, 0),
    OP2BYTES(KEY_2, 0),
    OP2BYTES(KEY_3, 0),
    OP2BYTES(KEY_SUB, command::ID_sub),
    OP2BYTES(KEY_EXIT, 0),
    OP2BYTES(KEY_0, 0),
    OP2BYTES(KEY_DOT, 0),
    OP2BYTES(KEY_RUN, 0),
    OP2BYTES(KEY_ADD, command::ID_add),

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

    record(input, "Handle function for key %d (plane %d) ", key, shift_plane());
    if (key == KEY_STO)
        RT.gc();

    int     plane = shift_plane();
    object *obj   = function[plane][key - 1];
    if (obj)
    {
        obj->evaluate();
        return true;
    }
    const byte *ptr = defaultCommand[plane] + 2 * (key - 1);
    if (*ptr)
    {
        // If we have the editor open, need to close it
        if (end_edit())
        {
            obj = (object *) ptr; // Uh oh! Evaluate bytecode in ROM
            obj->evaluate();
        }
        return true;
    }


    return false;
}
