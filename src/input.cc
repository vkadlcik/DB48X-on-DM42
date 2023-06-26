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

#include "arithmetic.h"
#include "command.h"
#include "dm42/sysmenu.h"
#include "functions.h"
#include "graphics.h"
#include "list.h"
#include "menu.h"
#include "runtime.h"
#include "settings.h"
#include "target.h"
#include "util.h"

#ifdef SIMULATOR
#include "tests.h"
#endif // SIMULATOR

#include <ctype.h>
#include <dmcp.h>
#include <stdio.h>
#include <unistd.h>

// The primary input of the calculator
input    Input;

RECORDER(input, 16, "Input processing");
RECORDER(help,  16, "On-line help");

#if SIMULATOR
#define HELPFILE_NAME   "help/db48x.md"
#else
#define HELPFILE_NAME   "/HELP/DB48X.md"
#endif // SIMULATOR

#define NUM_TOPICS      (sizeof(topics) / sizeof(topics[0]))

runtime &input::RT = runtime::RT;

input::input()
// ----------------------------------------------------------------------------
//   Initialize the input
// ----------------------------------------------------------------------------
    : command(),
      help(-1u),
      line(0),
      topic(0),
      history(0),
      topics(),
      cursor(0),
      xoffset(0),
      mode(STACK),
      last(0),
      stack(LCD_H),
      cx(0),
      cy(0),
      menuObject(),
      menuPage(),
      menuPages(),
      menuHeight(),
      shift(false),
      xshift(false),
      alpha(false),
      lowercase(false),
      down(false),
      up(false),
      repeat(false),
      longpress(false),
      blink(false),
      follow(false),
      dirtyMenu(false),
      dynamicMenu(false),
      autoComplete(false),
      helpfile()
{
    for (uint p = 0; p < NUM_PLANES; p++)
    {
        for (uint k = 0; k < NUM_KEYS; k++)
            function[p][k] = nullptr;
        for (uint k = 0; k < NUM_SOFTKEYS; k++)
        {
            menu_label[p][k] = nullptr;
            menu_marker[p][k] = 0;
            menu_marker_align[p][k] = false;
        }
    }
}


void input::edit(unicode c, modes m)
// ----------------------------------------------------------------------------
//   Begin editing with a given character
// ----------------------------------------------------------------------------
{
    // If already editing, keep current mode
    if (RT.editing())
        m = mode;

    byte utf8buf[4];
    uint savec = cursor;
    size_t len = utf8_encode(c, utf8buf);
    cursor += RT.insert(cursor, utf8buf, len);

    // Test delimiters
    unicode closing = 0;
    switch(c)
    {
    case '(':  closing = ')';  m = ALGEBRAIC; break;
    case '[':  closing = ']';  m = MATRIX;    break;
    case '{':  closing = '}';  m = PROGRAM;   break;
    case ':':  closing = ':';  m = DIRECT;    break;
    case '"':  closing = '"';  m = TEXT;      break;
    case '\'': closing = '\''; m = ALGEBRAIC; break;
    case L'«': closing = L'»'; m = PROGRAM;   break;
    }
    if (closing)
    {
        byte *ed = RT.editor();
        if (mode == PROGRAM || mode == ALGEBRAIC || mode == DIRECT)
            if (savec > 0 && ed[savec] != ' ')
                cursor += RT.insert(savec, ' ');
        len = utf8_encode(closing, utf8buf);
        RT.insert(cursor, utf8buf, len);
    }

    mode = m;
}


object::result input::edit(utf8 text, size_t len, modes m, int offset)
// ----------------------------------------------------------------------------
//   Enter the given text on the command line
// ----------------------------------------------------------------------------
{
    bool editing = RT.editing();
    byte *ed = RT.editor();

    if (!editing)
        cursor = 0;
    else if ((mode != ALGEBRAIC || m != ALGEBRAIC) && ed[cursor] != ' ')
        cursor += RT.insert(cursor, ' ');

    size_t added = RT.insert(cursor, text, len);
    cursor += added;

    if (mode != ALGEBRAIC || m != ALGEBRAIC)
        cursor += RT.insert(cursor, ' ');
    else
        cursor += RT.insert(cursor, utf8("()"), 2) - 1;

    // Offset from beginning or end of inserted text
    if (offset > 0 && cursor > len)
        cursor = cursor - len + offset;
    else if (offset < 0 && cursor > uint(-offset))
        cursor = cursor + offset;

    updateMode();
    return added == len ? object::OK : object::ERROR;
}


object::result input::edit(utf8 text, modes m, int offset)
// ----------------------------------------------------------------------------
//   Edit a null-terminated text
// ----------------------------------------------------------------------------
{
    return edit(text, strlen(cstring(text)), m, offset);
}


bool input::end_edit()
// ----------------------------------------------------------------------------
//   Clear the editor
// ----------------------------------------------------------------------------
{
    alpha   = false;
    shift   = false;
    xshift  = false;
    last    = 0;
    clear_help();
    RT.clear_error();

    size_t edlen = RT.editing();
    if (edlen)
    {
        gcutf8 editor = RT.close_editor();
        if (editor)
        {
            gcp<const program> cmds = program::parse(editor, edlen);
            if (cmds)
            {
                // We successfully parsed the line
                clear_editor();
                cmds->execute();
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
    clear_help();
}


void input::clear_help()
// ----------------------------------------------------------------------------
//   Clear help data
// ----------------------------------------------------------------------------
{
    command   = nullptr;
    help      = -1u;
    line      = 0;
    topic     = 0;
    follow    = false;
    last      = 0;
    longpress = false;
    repeat    = false;
    dirtyMenu = true;
    helpfile.close();
}


bool input::key(int key, bool repeating)
// ----------------------------------------------------------------------------
//   Process an input key
// ----------------------------------------------------------------------------
{
    int skey = key;

    longpress = key && repeating;
    record(input, "Key %d shifts %d longpress", key, shift_plane(), longpress);
    repeat = false;

#if SIMULATOR
    // Special keu to clear calculator state
    if (key == tests::CLEAR)
    {
        clear_editor();
        runtime &rt = runtime::RT;
        while (rt.depth())
            rt.pop();
        rt.clear_error();
        return true;
    }
#endif // SIMULATOR

    if (RT.error())
    {
        if (key == KEY_EXIT || key == KEY_ENTER || key == KEY_BSP ||
            key == KEY_UP || key == KEY_DOWN)
            RT.clear_error();
        else if (key)
            beep(2200, 75);
        return true;
    }

    // Hard-code OFF
    if (shift && key == KEY_EXIT)
    {
        // Power off
        SET_ST(STAT_PGM_END);
        shift = false;          // Make sure we don't have shift when waking up
        last = 0;
        clear_help();           // Otherwise shutdown images don't work
        return true;
    }

    // Hard-code system menu
    // REVISIT: Shit should probably just be a regular RPL function
    if (!alpha && shift && key == KEY_0)
    {
        SET_ST(STAT_MENU);
        handle_menu(&application_menu, MENU_RESET, 0);
        CLR_ST(STAT_MENU);
        wait_for_key_release(-1);
        shift = false;
        return true;
    }

    bool result =
        handle_shifts(key)    ||
        handle_help(key)      ||
        handle_editing(key)   ||
        handle_alpha(key)     ||
        handle_digits(key)    ||
        handle_functions(key) ||
        key == 0;

    if (RT.editing())
        updateMode();

    if (!skey && last != KEY_SHIFT)
    {
        shift = false;
        xshift = false;
    }

    if (!skey)
        command = nullptr;

    // Refresh the variables menu
    if (menuObject && dynamicMenu)
        menuObject->update(menuPage);

    return result;
}


void input::assign(int key, uint plane, object_p code)
// ----------------------------------------------------------------------------
//   Assign an object to a given key
// ----------------------------------------------------------------------------
{
    if (key >= 1 && key <= NUM_KEYS && plane <= NUM_PLANES)
        function[plane][key - 1] = code;
}


object_p input::assigned(int key, uint plane)
// ----------------------------------------------------------------------------
//   Assign an object to a given key
// ----------------------------------------------------------------------------
{
    if (key >= 1 && key <= NUM_KEYS && plane <= NUM_PLANES)
        return function[plane][key - 1];
    return nullptr;
}


void input::updateMode()
// ----------------------------------------------------------------------------
//   Scan the command line to check what the state is at the cursor
// ----------------------------------------------------------------------------
{
    utf8 ed    = RT.editor();
    utf8 last  = ed + cursor;
    uint progs = 0;
    uint lists = 0;
    uint algs  = 0;
    uint txts  = 0;
    uint vecs  = 0;
    uint hex   = 0;

    mode = DIRECT;
    for (utf8 p = ed; p < last; p = utf8_next(p))
    {
        unicode code = utf8_codepoint(p);
        if (hex && (code < '0'
                    || (code > '9' && code < 'A')
                    || (code > 'F' && code < 'a')
                    || (code > 'f')))
            hex = 0;
        switch(code)
        {
        case '\'':      algs = 1 - algs;        break;
        case '"':       txts = 1 - txts;        break;
        case '{':       lists++;                break;
        case '}':       lists--;                break;
        case '[':       vecs++;                 break;
        case ']':       vecs--;                 break;
        case L'«':      progs++;                break;
        case L'»':      progs--;                break;
        case '#':       hex++;                  break;
        }
    }

    if (txts)
        mode = TEXT;
    else if (hex)
        mode = HEXADECIMAL;
    else if (algs)
        mode = ALGEBRAIC;
    else if (vecs)
        mode = MATRIX;
    else if (lists || progs)
        mode = PROGRAM;
    else
        mode = DIRECT;
}


void input::menu(menu_p menu, uint page)
// ----------------------------------------------------------------------------
//   Set menu and page
// ----------------------------------------------------------------------------
{
    menuObject = runtime::RT.clone_if_dynamic(menu);
    menuPage = page;
    if (menu)
        menu->update(page);
    dirtyMenu = true;
}


menu_p input::menu()
// ----------------------------------------------------------------------------
//   Return the current menu
// ----------------------------------------------------------------------------
{
    return menuObject;
}


uint input::page()
// ----------------------------------------------------------------------------
//   Return the currently displayed page
// ----------------------------------------------------------------------------
{
    return menuPage;
}


void input::page(uint p)
// ----------------------------------------------------------------------------
//   Set the menu page to display
// ----------------------------------------------------------------------------
{
    menuPage = (p + menuPages) % menuPages;
    if (menuObject)
        menuObject->update(menuPage);
}


uint input::pages()
// ----------------------------------------------------------------------------
//   Return number of menu pages
// ----------------------------------------------------------------------------
{
    return menuPages;
}


void input::pages(uint p)
// ----------------------------------------------------------------------------
//   Return number of menu pages
// ----------------------------------------------------------------------------
{
    menuPages = p ? p : 1;
}


void input::menus(uint count, cstring labels[], object_p function[])
// ----------------------------------------------------------------------------
//   Assign all menus at once
// ----------------------------------------------------------------------------
{
    for (uint m = 0; m < NUM_MENUS; m++)
    {
        if (m < count)
            menu(m, labels[m], function[m]);
        else
            menu(m, cstring(nullptr), nullptr);
    }
    dynamicMenu = false;
    autoComplete = false;
}


void input::menu(uint menu_id, cstring label, object_p fn)
// ----------------------------------------------------------------------------
//   Assign one menu item
// ----------------------------------------------------------------------------
{
    if (menu_id < NUM_MENUS)
    {
        int softkey_id       = menu_id % NUM_SOFTKEYS;
        int key              = KEY_F1 + softkey_id;
        int plane            = menu_id / NUM_SOFTKEYS;
        function[plane][key-1] = fn;
        menu_label[plane][softkey_id] = label;
        menu_marker[plane][softkey_id] = 0;
        menu_marker_align[plane][softkey_id] = false;
        dirtyMenu = true;       // Redraw menu
    }
}


void input::menu(uint id, symbol_p label, object_p fn)
// ----------------------------------------------------------------------------
//   The drawing of menus recognizes symbols
// ----------------------------------------------------------------------------
{
    menu(id, (cstring) label, fn);
}


void input::marker(uint menu_id, unicode mark, bool alignLeft)
// ----------------------------------------------------------------------------
//   Record that we have a menu marker for this menu
// ----------------------------------------------------------------------------
{
    if (menu_id < NUM_MENUS)
    {
        int softkey_id       = menu_id % NUM_SOFTKEYS;
        int plane            = menu_id / NUM_SOFTKEYS;
        menu_marker[plane][softkey_id] = mark;
        menu_marker_align[plane][softkey_id] = alignLeft;
        dirtyMenu = true;
    }
}


symbol_p input::label(uint menu_id)
// ----------------------------------------------------------------------------
//   Return the label for a given menu ID
// ----------------------------------------------------------------------------
{
    cstring lbl = labelText(menu_id);
    if (*lbl == object::ID_symbol)
        return (symbol_p) lbl;
    return nullptr;
}


cstring input::labelText(uint menu_id)
// ----------------------------------------------------------------------------
//   Return the label for a given menu ID
// ----------------------------------------------------------------------------
{
    int     softkey_id = menu_id % NUM_SOFTKEYS;
    int     plane      = menu_id / NUM_SOFTKEYS;
    cstring lbl        = menu_label[plane][softkey_id];
    return lbl;
}


uint input::menuPlanes()
// ----------------------------------------------------------------------------
//   Count menu planes
// ----------------------------------------------------------------------------
{
    int planes = 3;
    if (showingHelp())
    {
        planes = 1;
    }
    else
    {
        while (planes > 0)
        {
            bool found = false;
            for (uint sk = 0; !found && sk < NUM_SOFTKEYS; sk++)
                found = menu_label[planes-1][sk] != 0;
            if (found)
                break;
            planes--;
        }
    }
    return planes;
}


int input::draw_menus(uint time, uint &period, bool force)
// ----------------------------------------------------------------------------
//   Draw the softkey menus
// ----------------------------------------------------------------------------
{
    static int  lastp   = 0;
    static uint lastt   = 0;
    int         shplane = shift_plane();
    const uint  refresh = 200;

    bool redraw = dirtyMenu || shplane != lastp || time - lastt > refresh;
    if (!force && !redraw)
        return -1;

    lastt = time;
    lastp = shplane;
    dirtyMenu = false;

    font_p font  = MenuFont;
    int    mh    = font->height() + 2;
    int    mw    = (LCD_W - 10) / 6;
    int    sp    = (LCD_W - 5) - 6 * mw;
    rect   clip  = Screen.clip();
    bool   help  = showingHelp();

    static unsigned menuShift = 0;
    menuShift++;

    int planes = menuPlanes();
    menuHeight = planes * mh;

    for (int plane = 0; plane < planes; plane++)
    {
        cstring *labels = menu_label[plane];
        if (help)
        {
            static cstring helpMenu[] =
            {
                "Home", "Page▲", "Page▼", "Link▲", "Link▼", "← Menu"
            };
            labels = helpMenu;
        }

        int my = LCD_H - (plane + 1) * mh;
        for (int m = 0; m < NUM_SOFTKEYS; m++)
        {
            int x = (2 * m + 1) * mw / 2 + (m * sp) / 5 + 2;
            rect mrect(x - mw/2-1, my, x + mw/2, my+mh-1);
            Screen.fill(mrect, pattern::white);

            mrect.inset(3,  1);
            Screen.fill(mrect, pattern::black);
            mrect.inset(-1, 1);
            Screen.fill(mrect, pattern::black);
            mrect.inset(-1, 1);
            Screen.fill(mrect, pattern::black);

            mrect.inset(2, 0);
            pattern color = pattern::white;
            if (planes > 1 && plane != shplane)
            {
                Screen.fill(mrect, pattern::white);
                color = pattern::black;
            }

            utf8 label = utf8(labels[m]);
            if (label)
            {
                unicode marker = 0;
                coord   mkw    = 0;
                coord   mkx    = 0;

                Screen.clip(mrect);
                size_t len = 0;
                if (*label == object::ID_symbol)
                {
                    // If we are given a symbol, use its length
                    label++;
                    len = leb128<size_t>(label);
                }
                else
                {
                    // Regular C string
                    len = strlen(cstring(label));
                }

                // Check if we have a marker from VariablesMenu
                if (!help)
                {
                    if (unicode mark = menu_marker[plane][m])
                    {
                        bool alignLeft = menu_marker_align[plane][m];
                        marker = mark;
                        mkw = font->width(marker);
                        mkx = alignLeft ? x - mw/2 : x + mw/2 - mkw;
                    }
                }

                size tw = font->width(label, len);
                if (tw > mw)
                {
                    dirtyMenu  = true;
                    x         -= mw/2 - 5 + menuShift % (tw - mw + 10);
                }
                else
                {
                    x = x - tw / 2;
                }
                coord ty = mrect.y1 - 3;
                Screen.text(x, ty, label, len, font, color);
                if (marker)
                    Screen.glyph(mkx, ty - 2 * (marker == L'◥'), marker, font, color);
                Screen.clip(clip);
            }
        }
    }

    if (dirtyMenu && period > refresh)
        period = refresh;

    return LCD_H - menuHeight;
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
        Screen.text(280, 1, label, HeaderFont, pattern::white);
    }

    const uint  ann_width  = 15;
    const uint  ann_height = 12;
    coord       ann_y      = (HeaderFont->height() - ann_height) / 2;
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
        surface  s(sw, ann_width, ann_height, 16);
        Screen.copy(s, 260, ann_y);
    }
}


int input::draw_battery(uint time, uint &period, bool force)
// ----------------------------------------------------------------------------
//    Draw the battery information
// ----------------------------------------------------------------------------
{
    static uint last = 0;
    if (period > 2000)
        period = 2000;

    const uint ann_height = 12;
    coord      ann_y      = (HeaderFont->height() - ann_height) / 2;

    // Print battery voltage
    static int  vdd = 3000;
    static bool low = false;
    static bool usb = false;

    if (time - last > 2000)
    {
        vdd  = (int) read_power_voltage();
        low  = get_lowbat_state();
        usb  = usb_powered();
        last = time;
        force = true;
    }
    if (!force)
        return -1;

    char buffer[64];
    snprintf(buffer, sizeof(buffer), "%d.%03dV", vdd / 1000, vdd % 1000);

    Screen.fill(310, 0, LCD_W, HeaderFont->height() + 1, pattern::black);
    Screen.text(340, 1, utf8(buffer), HeaderFont,
                low ? pattern::gray50 : pattern::white);
    Screen.fill(314, ann_y + 1, 336, ann_y + ann_height - 0, pattern::white);
    Screen.fill(310, ann_y + 3, 336, ann_y + ann_height - 3, pattern::white);

    const int batw = 334 - 315;
    int       w    = (vdd - 2000) * batw / (3090 - 2000);
    if (w > batw)
        w          = 334 - 315;
    else if (w < 1)
        w          = 1;
    Screen.fill(334 - w, ann_y + 2, 334, ann_y + ann_height - 1,
                usb ? pattern::gray50 : pattern::black);

#if 1
    // Temporary - Display some internal information
    static unsigned counter = 0;
    snprintf(buffer, sizeof(buffer), "%c %uR %uB %uB", longpress ? 'L' : ' ',
             counter++, (uint) RT.available(),
             sys_free_mem());
    Screen.fill(50, 0, 200, HeaderFont->height() + 1, pattern::black);
    Screen.text(50, 1, utf8(buffer), HeaderFont, pattern::white);
#endif

    return ann_y;
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
        stack = LCD_H - menuHeight;
        return;
    }

    // Count rows and colums
    int  rows   = 1;            // Number of rows in editor
    int  column = 0;            // Current column
    int  cwidth = 0;            // Column width
    int  edrow  = 0;            // Row number of line being edited
    int  edcol  = 0;            // Column of line being edited
    int  cursx  = 0;            // Cursor X position
    bool found  = false;

    byte *wed = (byte *) ed;
    wed[len] = 0;               // Ensure utf8_next does not go into the woods

    // Count rows to check if we need to switch to stack font
    for (utf8 p = ed; p < last; p = utf8_next(p))
        if (*p == '\n')
            rows++;
    if (rows > 2)
        font = StackFont;

    rows = 1;
    for (utf8 p = ed; p < last; p = utf8_next(p))
    {
        if (p - ed == (int) cursor)
        {
            edrow = rows - 1;
            edcol = column;
            cursx = cwidth;
            found = true;
        }

        if (*p == '\n')
        {
            rows++;
            column = 0;
            cwidth = 0;
        }
        else
        {
            column++;
            unicode cp  = utf8_codepoint(p);
            cwidth     += font->width(cp);
        }
    }
    if (!found)
    {
        edrow = rows - 1;
        edcol = column;
        cursx = cwidth;
    }

    // Check if we want to move the cursor up or down
    if (up || down)
    {
        int       r   = 0;
        int       c    = 0;
        int       tgt = edrow - (up && edrow > 0) + down;
        bool      done = false;
        for (utf8 p   = ed; p < last && !done; p = utf8_next(p))
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
        if (!done)
        {
            if (down)
            {
                cursor = len;
                edrow = rows - 1;
            }
            else if (up)
            {
                cursor = 0;
                edrow  = 0;
            }
        }
        up   = false;
        down = false;
    }

    // Draw the area that fits on the screen
    int  lineHeight      = font->height();
    int  errorHeight     = RT.error() ? LCD_H / 3 : 0;
    int  top             = HeaderFont->height() + errorHeight + 2;
    int  bottom          = LCD_H - menuHeight;
    int  availableHeight = bottom - top;
    int  availableRows   = availableHeight / lineHeight;
    int  displayRows     = (availableHeight + lineHeight - 1) / lineHeight;
    utf8 display         = ed;

    graphics::rect clip = Screen.clip();
    Screen.clip(0, top, LCD_W, bottom);
    Screen.fill(pattern::white);
    if (rows > availableRows)
    {
        // Skip rows to show the cursor
        int skip = edrow < displayRows / 2         ? 0
                 : edrow >= rows - displayRows / 2 ? rows - availableRows
                                                   : edrow - displayRows / 2;
        for (int r = 0; r < skip; r++)
        {
            do
                display = utf8_next(display);
            while (*display != '\n');
            display = utf8_next(display);
        }
        rows = availableRows;
    }

    // Draw the editor rows
    int  hskip   = 64;
    size cursw = font->width('M');
    if (xoffset > cursx)
        xoffset = (cursx > hskip) ? cursx - hskip : 0;
    else if (xoffset + LCD_W - cursw < cursx)
        xoffset = cursx - LCD_W + cursw + hskip;

    coord y = bottom - rows * lineHeight;
    coord x = -xoffset;
    int   r = 0;

    if (y < top)
        y = top;
    stack   = y;

    while (r < rows && display <= last)
    {
        bool    atCursor = display == ed + cursor;
        if (atCursor)
        {
            cx = x;
            cy = y;
        }
        if (display >= last)
            break;

        unicode c = utf8_codepoint(display);
        display = utf8_next(display);
        if (c == '\n')
        {
            y += lineHeight;
            x  = -xoffset;
            r++;
            continue;
        }
        int cw = font->width(c);
        if (x + cw >= 0 && x < LCD_W)
            x = Screen.glyph(x, y, c, font);
        else
            x += cw;
    }
    if (cursor >= len)
    {
        cx = x;
        cy = y;
    }

    Screen.clip(clip);
}


int input::draw_cursor(uint time, uint &period, bool force)
// ----------------------------------------------------------------------------
//   Draw the cursor at the location
// ----------------------------------------------------------------------------
//   This function returns the cursor vertical position for screen refresh
{
    // Do not draw if not editing or if help is being displayed
    if (!RT.editing() || showingHelp())
        return -1;

    static uint lastT = 0;
    if (period > 500)
        period = 500;
    if (!force && time - lastT < 500)
        return -1;
    lastT = time;
    if (force)
        blink = true;

    // Select editor font
    utf8   ed     = RT.editor();
    font_p edFont = EditorFont;
    size_t len    = RT.editing();
    utf8   last   = ed + len;
    uint   rows   = 1;
    for (utf8 p = ed; p < last; p = utf8_next(p))
        if (*p == '\n')
            rows++;
    if (rows > 2)
        edFont = StackFont;

    // Select cursor character
    unicode cursorChar = mode == DIRECT      ? 'D'
                       : mode == TEXT        ? (lowercase ? 'L' : 'C')
                       : mode == PROGRAM     ? 'P'
                       : mode == ALGEBRAIC   ? 'A'
                       : mode == MATRIX      ? 'M'
                       : mode == HEXADECIMAL ? 'B'
                                             : 'X';
    size    csrh       = CursorFont->height();
    size    csrw       = CursorFont->width(cursorChar);
    size    ch         = edFont->height();

    coord   x          = cx;
    utf8    p          = ed + cursor;
    rect    clip       = Screen.clip();
    coord   ytop       = HeaderFont->height() + 2;
    coord   ybot       = LCD_H - menuHeight;

    Screen.clip(0, ytop, LCD_W, ybot);
    bool spaces = false;
    while (x < cx + csrw + 1)
    {
        unicode cchar  = p < last ? utf8_codepoint(p) : ' ';
        if (cchar     == '\n')
            spaces = true;
        if (spaces)
            cchar      = ' ';
        size    cw     = edFont->width(cchar);
        Screen.fill(x, cy, x + cw - 1, cy + ch - 1,
                    x == cx ? pattern::gray75 : pattern::white);

        // Write the character under the cursor
        x = Screen.glyph(x, cy, cchar, edFont);
        if (p < last)
            p = utf8_next(p);
    }

    if (blink)
    {
        coord csrx = cx + 1;
        coord csry = cy + (ch - csrh)/2;
        Screen.fill(csrx, cy, csrx+1, cy + ch - 1, pattern::black);
        rect  r(csrx, csry - 1, csrx+csrw, csry + csrh);
        if (alpha)
        {
            Screen.fill(r, pattern::black);
            r.inset(2,2);
            Screen.fill(r, pattern::white);
            Screen.glyph(csrx, csry, cursorChar, CursorFont, pattern::black);
        }
        else
        {
            Screen.fill(r, pattern::black);
            Screen.glyph(csrx, csry, cursorChar, CursorFont, pattern::white);
        }
    }

    blink = !blink;
    Screen.clip(clip);
    return cy;
}


void input::draw_command()
// ----------------------------------------------------------------------------
//   Draw the current command
// ----------------------------------------------------------------------------
{
    if (command && !RT.error())
    {
        font_p font = HelpCodeFont;
        size   w = font->width(command);
        size   h = font->height();
        coord  x    = 25;
        coord  y    = HeaderFont->height() + 6;
        Screen.fill(x-2, y, x + w + 1, y + h, pattern::black);
        Screen.text(x, y, command, font, pattern::white);
    }
}


void input::draw_error()
// ----------------------------------------------------------------------------
//   Draw the error message if there is one
// ----------------------------------------------------------------------------
{
    if (utf8 err = RT.error())
    {
        const int border = 4;
        coord     top    = HeaderFont->height() + 10;
        coord     height = LCD_H / 3;
        coord     width  = LCD_W - 8;
        coord     x      = LCD_W / 2 - width / 2;
        coord     y      = top;

        rect clip = Screen.clip();
        rect r(x, y, x + width - 1, y + height - 1);
        Screen.fill(r, pattern::gray50);
        r.inset(border);
        Screen.fill(r, pattern::white);
        r.inset(2);

        Screen.clip(r);
        if (utf8 cmd = RT.command())
        {
            coord x = Screen.text(r.x1, r.y1, cmd, ErrorFont);
            Screen.text(x, r.y1, utf8(" error:"), ErrorFont);
        }
        else
        {
            Screen.text(r.x1, r.y1, utf8("Error:"), ErrorFont);
        }
        r.y1 += ErrorFont->height();
        Screen.text(r.x1, r.y1, err, ErrorFont);
        Screen.clip(clip);
    }
}



void input::load_help(utf8 topic)
// ----------------------------------------------------------------------------
//   Find the help message associated with the topic
// ----------------------------------------------------------------------------
{
    record(help, "Loading help topic %s", topic);

    size_t len = strlen(cstring(topic));
    command    = nullptr;
    follow     = false;

    // Need to have the help file open here
    if (!helpfile.valid())
    {
        help = -1u;
        line          = 0;
        return;
    }

    // Look for the topic in the file
    uint         matching = 0;
    uint         level    = 0;
    bool         hadcr    = true;
    helpfile.seek(0);
    for (unicode c        = helpfile.get(); c; c = helpfile.get())
    {
        if (((hadcr || matching) && c == '#') || (c == ' ' && matching == 1))
        {
            level += c == '#';
            matching = 1;
        }
        else if (matching)
        {
            // Matching is case-independent, and matches markdown hyperlinks
            if (tolower(c) == tolower(topic[matching-1]) ||
                (c         == ' ' && topic[matching-1] == '-'))
                matching++;
            else
                matching    = level = 0;
            if (matching   == len + 1)
            {
                unicode next  = helpfile.peek();
                if (next     == '\n')
                    break;
                if (next     == ' ')
                {
                    // Case of something like ## Evaluate (EVAL)
                    // We accept to match 'evaluate'
                    uint pos = helpfile.position();
                    helpfile.get();
                    if (helpfile.peek() == '(')
                    {
                        helpfile.seek(pos);
                        break;
                    }
                }
                matching = 0;
            }
        }
        hadcr = c == '\n';
    }

    // Check if we found the topic
    if (matching == len + 1)
    {
        help = helpfile.position() - (len+1) - level;
        line = 0;
        record(help, "Found topic %s at position %u level %u",
               topic, helpfile.position(), level);

        if (history >= NUM_TOPICS)
        {
            // Overflow, keep the last topics
            for (uint i         = 1; i < NUM_TOPICS; i++)
                topics[i - 1]   = topics[i];
            topics[history - 1] = help;
        }
        else
        {
            // New topic, store it
            topics[history++] = help;
        }
    }
    else
    {
        static char buffer[50];
        snprintf(buffer, sizeof(buffer), "No help for %s", topic);
        RT.error(buffer);
    }
}


struct style_description
// ----------------------------------------------------------------------------
//   A small struct recording style
// ----------------------------------------------------------------------------
{
    font_p  font;
    pattern color;
    pattern background;
    bool    bold      : 1;
    bool    italic    : 1;
    bool    underline : 1;
    bool    box       : 1;
};


enum style_name
// ----------------------------------------------------------------------------
//   Style index
// ----------------------------------------------------------------------------
{
    TITLE,
    SUBTITLE,
    NORMAL,
    BOLD,
    ITALIC,
    CODE,
    KEY,
    TOPIC,
    HIGHLIGHTED_TOPIC,
    NUM_STYLES
};


static coord draw_word(coord   x,
                       coord   y,
                       size_t  sz,
                       unicode word[],
                       font_p  font,
                       pattern color)
// ----------------------------------------------------------------------------
//   Helper to draw a particular glyph
// ----------------------------------------------------------------------------
{
    for (uint g = 0; g < sz; g++)
        x = Screen.glyph(x, y, word[g], font, color);
    return x;
}


bool input::draw_help()
// ----------------------------------------------------------------------------
//    Draw the help content
// ----------------------------------------------------------------------------
{
    if (!showingHelp())
        return false;

    using p                                    = pattern;
    const style_description styles[NUM_STYLES] =
    // -------------------------------------------------------------------------
    //  Table of styles
    // -------------------------------------------------------------------------
    {
        { HelpTitleFont,    p::black,  p::white,  false, false, false, false },
        { HelpSubTitleFont, p::black,  p::gray50,  true, false, true,  false },
        { HelpFont,         p::black,  p::white,  false, false, false, false },
        { HelpBoldFont,     p::black,  p::white,  true,  false, false, false },
        { HelpItalicFont,   p::black,  p::white,  false, true,  false, false },
        { HelpCodeFont,     p::black,  p::gray50, false, false, false, true  },
        { HelpCodeFont,     p::white,  p::black,  false, false, false, false },
        { HelpFont,         p::black,  p::gray50, false, false, true,  false },
        { HelpFont,         p::white,  p::gray10, false, false, false, false },
    };


    // Compute the size for the help display
    coord      ytop   = HeaderFont->height() + 2;
    coord      ybot   = LCD_H - (MenuFont->height() + 4);
    coord      xleft  = 0;
    coord      xright = LCD_W;
    style_name style  = NORMAL;


    // Clear help area and add some decorative elements
    rect clip = Screen.clip();
    rect r(xleft, ytop, xright, ybot);
    Screen.fill(r, pattern::gray25);
    r.inset(2);
    Screen.fill(r, pattern::black);
    r.inset(2);
    Screen.fill(r, pattern::white);

    // Clip drawing area in case some text does not really fit
    r.inset(1);
    Screen.clip(r);

    // Update drawing area
    ytop   = r.y1;
    ybot   = r.y2;
    xleft  = r.x1 + 2;
    xright = r.x2;


    // Select initial state
    font_p  font      = styles[style].font;
    coord   height = font->height();
    coord   x      = xleft;
    coord   y      = ytop + 2 - line * height;
    unicode last   = '\n';
    uint    lastTopic = 0;
    uint    shown  = 0;

    // Pun not intended
    helpfile.seek(help);

    // Display until end of help
    while (y < ybot)
    {
        unicode word[60];
        uint    widx       = 0;
        bool    emit       = false;
        bool    newline    = false;
        style_name restyle = style;

        if (last  == '\n' && !shown && y >= ytop)
            shown  = helpfile.position();

        while (!emit)
        {
            unicode ch   = helpfile.get();
            bool    skip = false;

            switch (ch)
            {
            case ' ':
                if (style <= SUBTITLE)
                {
                    skip = last == '#';
                    break;
                }
                skip = last == ' ';
                emit = style != KEY;
                break;

            case '\n':

                if (last == '\n' || last == ' ' || style <= SUBTITLE)
                {
                    emit    = true;
                    skip    = true;
                    newline = last != '\n' || helpfile.peek() != '\n';
                    while (helpfile.peek() == '\n')
                        helpfile.get();
                    restyle = NORMAL;
                }
                else
                {
                    uint    off  = helpfile.position();
                    unicode nx   = helpfile.get();
                    unicode nnx  = helpfile.get();
                    if (nx      == '#' || (nx == '*' && nnx == ' '))
                    {
                        newline = true;
                        emit = true;
                    }
                    else
                    {
                        ch   = ' ';
                        emit = true;
                    }
                    helpfile.seek(off);
                }
                break;

            case '#':
                if (last == '#' || last == '\n')
                {
                    if (restyle == TITLE)
                        restyle = SUBTITLE;
                    else
                        restyle = TITLE;
                    skip        = true;
                    emit        = true;
                    newline     = restyle == TITLE && last != '\n';
                }
                break;

            case '*':
                if (last == '\n' && helpfile.peek() == ' ')
                {
                    restyle = NORMAL;
                    ch      = L'■'; // L'•';
                    xleft   = r.x1 + 2 + font->width(utf8("■ "));
                    break;
                }
                // Fall-through
            case '_':
                if (style != CODE)
                {
                    //   **Hello** *World*
                    //   IB.....BN I.....N
                    if (last == ch)
                    {
                        if (style   == BOLD)
                            restyle  = NORMAL;
                        else
                            restyle  = BOLD;
                    }
                    else
                    {
                        style_name disp  = ch == '_' ? KEY : ITALIC;
                        if (style       == BOLD)
                            restyle = BOLD;
                        else if (style  == disp)
                            restyle      = NORMAL;
                        else
                            restyle      = disp;
                    }
                    skip = true;
                    emit = true;
                }
                break;

            case '`':
                if (last != '`' && helpfile.peek() != '`')
                {
                    if (style   == CODE)
                        restyle  = NORMAL;
                    else
                        restyle  = CODE;
                    skip         = true;
                    emit         = true;
                }
                else
                {
                    if (last == '`')
                        skip  = true;
                }
                break;

            case '[':
                if (style != CODE)
                {
                    lastTopic      = helpfile.position();
                    if (topic < shown)
                        topic      = lastTopic;
                    if (lastTopic == topic)
                        restyle    = HIGHLIGHTED_TOPIC;
                    else
                        restyle    = TOPIC;
                    skip           = true;
                    emit           = true;
                }
                break;
            case ']':
                if (style == TOPIC || style == HIGHLIGHTED_TOPIC)
                {
                    unicode n  = helpfile.get();
                    if (n     != '(')
                    {
                        ch      = n;
                        restyle = NORMAL;
                        emit = true;
                        break;
                    }

                    static char  link[60];
                    char        *p  = link;
                    while (n       != ')')
                    {
                        n      = helpfile.get();
                        if (n != '#')
                            if (p < link + sizeof(link))
                                *p++ = n;
                    }
                    if (p < link + sizeof(link))
                    {
                        p[-1]                = 0;
                        if (follow && style == HIGHLIGHTED_TOPIC)
                        {
                            if (history)
                                topics[history-1] = shown;
                            load_help(utf8(link));
                            Screen.clip(clip);
                            return draw_help();
                        }
                    }
                    restyle = NORMAL;
                    emit    = true;
                    skip    = true;
                }
                break;
            default:
                break;
            }

            if (!skip)
                word[widx++]  = ch;
            if (widx         >= sizeof(word) / sizeof(word[0]))
                emit          = true;
            last              = ch;
        }

        // Select font and color based on style
        font              = styles[style].font;
        height            = font->height();


        // Compute width of word (or words in the case of titles)
        coord width = 0;
        for (uint i  = 0; i < widx; i++)
            width += font->width(word[i]);

        if (style <= SUBTITLE)
        {
            // Center titles
            x  = (LCD_W - width) / 2;
            y += 3 * height / 4;
        }
        else
        {
            // Go to new line if this does not fit
            coord right  = x + width;
            if (right   >= xright - 1)
            {
                x = xleft;
                y += height;
            }
        }

        coord yf = y + height;
        if (yf > ytop)
        {
            pattern color     = styles[style].color;
            pattern bg        = styles[style].background;
            bool    bold      = styles[style].bold;
            bool    italic    = styles[style].italic;
            bool    underline = styles[style].underline;
            bool    box       = styles[style].box;

            // Draw a decoration
            coord xl = x;
            coord xr = x + width;
            if (box || underline)
            {
                xl -= 2;
                xr += 2;
                Screen.fill(xl, yf, xr, yf, bg);
                if (box)
                {
                    Screen.fill(xl, y, xl, yf, bg);
                    Screen.fill(xr, y, xr, yf, bg);
                    Screen.fill(xl, y, xr, y, bg);
                }
                xl += 2;
                xr -= 2;
            }
            else if (bg.bits != pattern::white.bits)
            {
                Screen.fill(xl, y, xr, yf, bg);
            }

            // Draw next word
            for (int i = 0; i < 1 + 3 * italic; i++)
            {
                x = xl;
                if (italic)
                {
                    coord yt  = y + (3-i) * height / 4;
                    coord yb  = y + (4-i) * height / 4;
                    x        += i;
                    Screen.clip(x, yt, xr + i, yb);
                }
                coord x0   = x;
                for (int b = 0; b <= bold; b++)
                    x      = draw_word(x0 + b, y, widx, word, font, color);
            }
            if (italic)
                Screen.clip(r);

            // Select style for next round
            style = restyle;
        }

        if (newline)
        {
            xleft  = r.x1 + 2;
            x = xleft;
            y += height * 5 / 4;
        }
    }

    if (helpfile.position() < topic)
        topic = lastTopic;

    Screen.clip(clip);
    follow = false;
    return true;
}


bool input::noHelpForKey(int key)
// ----------------------------------------------------------------------------
//   Return true if key requires immediate action, no help displayed
// ----------------------------------------------------------------------------
{
    bool editing  = RT.editing();

    // No help in alpha mode
    if (alpha && key < KEY_F1)
        return true;

    // No help for ENTER or BSP key while editing
    if (editing && (key == KEY_ENTER || key == KEY_BSP ||
                    key == KEY_UP || key == KEY_DOWN))
        return true;

    // No help for A-F keys in hexadecimal entry mode
    if (editing && (key >= KB_A && key <= KB_F) && mode == HEXADECIMAL)
        return true;

    // No help for digits entry
    if (!shift && !xshift)
        if (key > KEY_ENTER && key < KEY_ADD &&
            key != KEY_SUB && key != KEY_MUL && KEY != KEY_DIV)
            return true;

    // Other cases are regular functions, we can display help
    return false;
}



bool input::handle_help(int &key)
// ----------------------------------------------------------------------------
//   Handle help keys when showing help
// ----------------------------------------------------------------------------
{
    if (!showingHelp())
    {
        // Exit if we are editing or entering digits
        bool editing  = RT.editing();
        if (last == KEY_SHIFT)
            return false;

        // Check if we have a long press, if so load corresponding help
        if (key)
        {
            if (noHelpForKey(key))
                return false;

            record(help,
                   "Looking for help topic for key %d, long = %d shift=%d\n",
                   key, longpress, shift_plane());
            if (object_p obj = object_for_key(key))
            {
                record(help, "Looking for help topic for key %d\n", key);
                if (utf8 htopic = obj->help())
                {
                    record(help, "Found help topic %s\n", htopic);
                    command = htopic;
                    if (longpress)
                    {
                        helpfile.open(HELPFILE_NAME);
                        load_help(htopic);
                        if (RT.error())
                        {
                            key  = 0; // Do not execute a function if no help
                            last = 0;
                        }
                    }
                    else
                    {
                        repeat = true;
                    }
                    return true;
                }
            }
            if (!editing)
                key = 0;
        }
        else
        {
            if (!noHelpForKey(last))
                key = last;     // Time to evaluate
            last    = 0;
            command = nullptr;
        }

        // Help keyboard movements only applies when help is shown
        return false;
    }

    // Help is being shown - Special keyboard mappings
    uint count = shift ? 8 : 1;
    switch (key)
    {
    case KEY_F1:
        load_help(utf8("Overview"));
        break;
    case KEY_F2:
        count = 8;
        // Fallthrough
    case KEY_UP:
    case KEY_8:
    case KEY_SUB:
        if (line > count)
        {
            line -= count;
        }
        else
        {
            line = 0;
            count++;
            while(count--)
            {
                helpfile.seek(help);
                help = helpfile.rfind('\n');
                if (!help)
                    break;
            }
            if (help)
                help = helpfile.position();
        }
        repeat = true;
        break;

    case KEY_F3:
        count   = 8;
        // Fall through
    case KEY_DOWN:
    case KEY_2:
    case KEY_ADD:
        line   += count;
        repeat  = true;
        break;

    case KEY_F4:
    case KEY_9:
    case KEY_DIV:
        ++count;
        while(count--)
        {
            helpfile.seek(topic);
            topic = helpfile.rfind('[');
        }
        topic  = helpfile.position();
        repeat = true;
        break;
    case KEY_F5:
    case KEY_3:
    case KEY_MUL:
        helpfile.seek(topic);
        while (count--)
            helpfile.find('[');
        topic  = helpfile.position();
        repeat = true;
        break;

    case KEY_ENTER:
        follow = true;
        break;

    case KEY_F6:
    case KEY_BSP:
        if (history)
        {
            --history;
            if (history)
            {
                help = topics[history-1];
                line = 0;
                break;
            }
        }
        // Otherwise fall-through and exit

    case KEY_EXIT:
        clear_help();
        break;
    }
    return true;
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
            alpha = !alpha;
            xshift = 0;
            shift = 0;
        }
        else if (xshift)
        {
            xshift = false;
        }
        else
        {
            xshift = false;
#define SHM(d, x, s) ((d << 2) | (x << 1) | (s << 0))
#define SHD(d, x, s) (1 << SHM(d, x, s))
            // Double shift toggles xshift
            bool dshift = last == KEY_SHIFT;
            int  plane  = SHM(dshift, xshift, shift);
            const unsigned nextShift =
                SHD(0, 0, 0) | SHD(0, 1, 0) | SHD(1, 0, 0);
            const unsigned nextXShift =
                SHD(0, 0, 1) | SHD(0, 1, 0) | SHD(0, 1, 1) | SHD(1, 0, 1);
            shift  = (nextShift  & (1 << plane)) != 0;
            xshift  = (nextXShift & (1 << plane)) != 0;
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

    // Some editing keys that do not depend on data entry mode
    if (!alpha)
    {
        switch(key)
        {
        case KEY_XEQ:
            // XEQ is used to enter algebraic / equation objects
            if ((!editing  || mode != HEXADECIMAL) && !shift && !xshift)
            {
                edit(editing && mode == ALGEBRAIC ? '(' : '\'', ALGEBRAIC);
                alpha = true;
                return true;
            }
            break;
        case KEY_RUN:
            if (shift)
            {
                // Shift R/S = PRGM enters a program symbol
                edit(L'«', PROGRAM);
                last = 0;
                return true;
            }
            else if (editing)
            {
                // Stick to space role while editing, do not EVAL, repeat
                edit(' ', PROGRAM);
                repeat = true;
                return true;
            }
            break;
        }
    }

    if (editing)
    {
        record(input, "Editing key %d", key);
        switch (key)
        {
        case KEY_BSP:
            if (xshift)
                return false;
            repeat = true;
            if (shift && cursor < editing)
            {
                // Shift + Backspace = Delete to right of cursor
                utf8 ed              = RT.editor();
                uint after           = utf8_next(ed, cursor, editing);
                RT.remove(cursor, after - cursor);
            }
            else if (!shift && cursor > 0)
            {
                // Backspace = Erase on left of cursor
                utf8 ed      = RT.editor();
                uint before  = cursor;
                cursor       = utf8_previous(ed, cursor);
                RT.remove(cursor, before - cursor);
            }
            else
            {
                // Limits of line: beep
                repeat = false;
                beep(4400, 50);
            }
            // Do not stop editing if we delete last character
            if (!RT.editing())
                edit(' ', DIRECT);
            last = 0;
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
                    alpha     = true;
            }
            else if (xshift)
            {
                // Insert quotes and begin editing text
                edit('\"', TEXT);
                alpha = true;
            }
            else
            {
                // Finish editing and parse the result
                end_edit();
            }
            return true;
        }
        case KEY_EXIT:
            // Clear error if there is one, else clear editor
            if (RT.error())
                RT.clear_error();
            else
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
                cursor  = utf8_previous(ed, cursor);
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
                    alpha     = true;
                return true;
            }
            else if (xshift)
            {
                // Insert quotes and begin editing
                edit('\"', TEXT);
                alpha = true;
                return true;
            }
            break;
        case KEY_EXIT:
            if (shift)
                SET_ST(STAT_PGM_END);
            alpha = false;
            return true;
        case KEY_DOWN:
            // Key down to edit last object on stack
            if (!shift && !xshift && !alpha)
            {
                if (RT.depth())
                {
                    if (object_p obj = RT.pop())
                    {
                        obj->edit();
                        return true;
                    }
                }
            }
            break;
        }
    }


    return consumed;
}


bool input::handle_alpha(int key)
// ----------------------------------------------------------------------------
//    Handle alphabetic input
// ----------------------------------------------------------------------------
{
    bool editing = RT.editing();
    bool hex = editing && mode == HEXADECIMAL && key >= KB_A && key <= KB_F;
    if (!alpha || !key || ((key == KEY_ENTER || key == KEY_BSP) && !xshift) ||
        (key >= KEY_F1 && key <= KEY_F6))
        if (!hex)
            return false;

    static const char upper[] =
        "ABCDEF"
        "GHIJKL"
        "_MNO_"
        "_PQRS"
        "_TUVW"
        "_XYZ_"
        "_:, ;";
    static const char lower[] =
        "abcdef"
        "ghijkl"
        "_mno_"
        "_pqrs"
        "_tuvw"
        "_xyz_"
        "_:, ;";

    static const unicode shifted[] =
    {
        L'Σ', '^', L'√', L'∂', L'ρ', '(',
        L'▶', '%', L'π', '<', '=', '>',
        '_', L'⇄', L'±', L'⁳', '_',
        '_', '7', '8', '9', L'÷',
        '_', '4', '5', '6', L'×',
        '_', '1', '2', '3', '-',
        '_', '0', '.',  L'«', '+'
    };

    static const  unicode xshifted[] =
    {
        L'∏', L'∆', L'↑', L'μ', L'θ', '\'',
        L'→', L'←', L'↓', L'≤', L'≠', L'≥',
        '"',  '~', L'°', L'ε', '\n',
        '_',  '?', L'∫',   '[',  '/',
        '_',  '#',  L'∞', '|' , '*',
        '_',  '&',   '@', '$',  '_',
        '_',  ';',  L'·', '{',  '!'
    };

    // Special case: + in alpha mode shows the catalog
    if (key == KEY_ADD && !shift && !xshift)
    {
        object_p cat = command::static_object(menu::ID_Catalog);
        cat->execute();
        return true;
    }

    key--;
    unicode c =
        hex       ? upper[key]    :
        xshift    ? xshifted[key] :
        shift     ? shifted[key]  :
        lowercase ? lower[key]    :
        upper[key];
    edit(c, TEXT);
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
        "_0.__";

    if (RT.editing())
    {
        if (key == KEY_CHS)
        {
            // Special case for change of sign
            byte    *ed = RT.editor();
            utf8 p      = ed + cursor;
            unicode  c  = 0;
            while (p > ed)
            {
                p = utf8_previous(p);
                c = utf8_codepoint(p);
                if ((c < '0' || c > '9') && c != (unicode) Settings.decimal_dot)
                    break;
            }

            if (p > ed)
                p  = utf8_next(p);
            if (c == 'e' || c == 'E' || c == Settings.exponent_char)
                c  = utf8_codepoint(p);

            if (c             == '-' || c == '+')
                *((byte *) p)  = '+' + '-' - c;
            else
                cursor        += RT.insert(p - ed, '-');
            last               = 0;
            return true;
        }
        else if (key == KEY_E)
        {
            byte   buf[4];
            size_t sz  = utf8_encode(Settings.exponent_char, buf);
            cursor    += RT.insert(cursor, buf, sz);
            last       = 0;
            return true;
        }
    }
    if (!shift && !xshift && (key > KEY_CHS && key < KEY_F1))
    {
        char c  = numbers[key-1];
        if (c == '_')
            return false;
        if (c == '.')
            c = Settings.decimal_dot;
        edit(c, DIRECT);
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

    OP2BYTES(KEY_SIGMA, menu::ID_MathMenu),
    OP2BYTES(KEY_INV,   function::ID_inv),
    OP2BYTES(KEY_SQRT,  function::ID_sqrt),
    OP2BYTES(KEY_LOG,   function::ID_log10),
    OP2BYTES(KEY_LN,    function::ID_log),
    OP2BYTES(KEY_XEQ,   0),
    OP2BYTES(KEY_STO,   command::ID_Sto),
    OP2BYTES(KEY_RCL,   command::ID_VariablesMenu),
    OP2BYTES(KEY_RDN,   menu::ID_StackMenu),
    OP2BYTES(KEY_SIN,   function::ID_sin),
    OP2BYTES(KEY_COS,   function::ID_cos),
    OP2BYTES(KEY_TAN,   function::ID_tan),
    OP2BYTES(KEY_ENTER, function::ID_Dup),
    OP2BYTES(KEY_SWAP,  function::ID_Swap),
    OP2BYTES(KEY_CHS,   function::ID_neg),
    OP2BYTES(KEY_E, 0),
    OP2BYTES(KEY_BSP,   command::ID_Drop),
    OP2BYTES(KEY_UP,    0),
    OP2BYTES(KEY_7,     0),
    OP2BYTES(KEY_8,     0),
    OP2BYTES(KEY_9,     0),
    OP2BYTES(KEY_DIV,   arithmetic::ID_div),
    OP2BYTES(KEY_DOWN,  0),
    OP2BYTES(KEY_4,     0),
    OP2BYTES(KEY_5,     0),
    OP2BYTES(KEY_6,     0),
    OP2BYTES(KEY_MUL,   arithmetic::ID_mul),
    OP2BYTES(KEY_SHIFT, 0),
    OP2BYTES(KEY_1,     0),
    OP2BYTES(KEY_2,     0),
    OP2BYTES(KEY_3,     0),
    OP2BYTES(KEY_SUB,   command::ID_sub),
    OP2BYTES(KEY_EXIT,  0),
    OP2BYTES(KEY_0,     0),
    OP2BYTES(KEY_DOT,   0),
    OP2BYTES(KEY_RUN,   command::ID_Eval),
    OP2BYTES(KEY_ADD,   command::ID_add),

    OP2BYTES(KEY_F1,    0),
    OP2BYTES(KEY_F2,    0),
    OP2BYTES(KEY_F3,    0),
    OP2BYTES(KEY_F4,    0),
    OP2BYTES(KEY_F5,    0),
    OP2BYTES(KEY_F6,    0),

    OP2BYTES(KEY_SCREENSHOT, 0),
    OP2BYTES(KEY_SH_UP,  0),
    OP2BYTES(KEY_SH_DOWN, 0),
};


static const byte defaultShiftedCommand[2*input::NUM_KEYS] =
// ----------------------------------------------------------------------------
//   RPL code for the commands assigned by default to shifted keys
// ----------------------------------------------------------------------------
//   All the default assigned commands fit in one or two bytes
{
    OP2BYTES(KEY_SIGMA, menu::ID_MainMenu),
    OP2BYTES(KEY_INV,   arithmetic::ID_pow),
    OP2BYTES(KEY_SQRT,  arithmetic::ID_sq),
    OP2BYTES(KEY_LOG,   function::ID_exp10),
    OP2BYTES(KEY_LN,    function::ID_exp),
    OP2BYTES(KEY_XEQ,   menu::ID_LoopsMenu),
    OP2BYTES(KEY_STO,   menu::ID_ComplexMenu),
    OP2BYTES(KEY_RCL,   menu::ID_FractionsMenu),
    OP2BYTES(KEY_RDN,   menu::ID_ConstantsMenu),
    OP2BYTES(KEY_SIN,   function::ID_asin),
    OP2BYTES(KEY_COS,   function::ID_acos),
    OP2BYTES(KEY_TAN,   function::ID_atan),
    OP2BYTES(KEY_ENTER, 0),
    OP2BYTES(KEY_SWAP,  menu::ID_LastThingsMenu),
    OP2BYTES(KEY_CHS,   menu::ID_ModesMenu),
    OP2BYTES(KEY_E,     menu::ID_PlotMenu),
    OP2BYTES(KEY_BSP,   0),
    OP2BYTES(KEY_UP,    0),
    OP2BYTES(KEY_7,     menu::ID_SolverMenu),
    OP2BYTES(KEY_8,     menu::ID_SymbolicMenu),
    OP2BYTES(KEY_9,     menu::ID_MatrixMenu),
    OP2BYTES(KEY_DIV,   menu::ID_StatisticsMenu),
    OP2BYTES(KEY_DOWN,  0),
    OP2BYTES(KEY_4,     menu::ID_BasesMenu),
    OP2BYTES(KEY_5,     menu::ID_UnitsMenu),
    OP2BYTES(KEY_6,     menu::ID_FlagsMenu),
    OP2BYTES(KEY_MUL,   menu::ID_ProbabilitiesMenu),
    OP2BYTES(KEY_SHIFT, 0),
    OP2BYTES(KEY_1,     0),
    OP2BYTES(KEY_2,     0),
    OP2BYTES(KEY_3,     menu::ID_ProgramMenu),
    OP2BYTES(KEY_SUB,   menu::ID_IOMenu),
    OP2BYTES(KEY_EXIT,  0),
    OP2BYTES(KEY_0,     0),
    OP2BYTES(KEY_DOT,   0),
    OP2BYTES(KEY_RUN,   0),
    OP2BYTES(KEY_ADD,   menu::ID_Catalog),

    OP2BYTES(KEY_F1,    0),
    OP2BYTES(KEY_F2,    0),
    OP2BYTES(KEY_F3,    0),
    OP2BYTES(KEY_F4,    0),
    OP2BYTES(KEY_F5,    0),
    OP2BYTES(KEY_F6,    0),

    OP2BYTES(KEY_SCREENSHOT, 0),
    OP2BYTES(KEY_SH_UP, 0),
    OP2BYTES(KEY_SH_DOWN, 0),
};


static const byte defaultSecondShiftedCommand[2*input::NUM_KEYS] =
// ----------------------------------------------------------------------------
//   RPL code for the commands assigned by default to long-shifted keys
// ----------------------------------------------------------------------------
//   All the default assigned commands fit in one or two bytes
{
    OP2BYTES(KEY_SIGMA, 0),
    OP2BYTES(KEY_INV,   0),
    OP2BYTES(KEY_SQRT,  0),
    OP2BYTES(KEY_LOG,   function::ID_expm1),
    OP2BYTES(KEY_LN,    function::ID_log1p),
    OP2BYTES(KEY_XEQ,   menu::ID_TestsMenu),
    OP2BYTES(KEY_STO,   command::ID_GarbageCollect),
    OP2BYTES(KEY_RCL,   command::ID_FreeMemory),
    OP2BYTES(KEY_RDN,   0),
    OP2BYTES(KEY_SIN,   function::ID_sinh),
    OP2BYTES(KEY_COS,   function::ID_cosh),
    OP2BYTES(KEY_TAN,   function::ID_tanh),
    OP2BYTES(KEY_ENTER, 0),
    OP2BYTES(KEY_SWAP,  0),
    OP2BYTES(KEY_CHS,   0),
    OP2BYTES(KEY_E,     0),
    OP2BYTES(KEY_BSP,   0),
    OP2BYTES(KEY_UP,    0),
    OP2BYTES(KEY_7,     0),
    OP2BYTES(KEY_8,     0),
    OP2BYTES(KEY_9,     0),
    OP2BYTES(KEY_DIV,   0),
    OP2BYTES(KEY_DOWN,  0),
    OP2BYTES(KEY_4,     0),
    OP2BYTES(KEY_5,     menu::ID_UnitsConversionsMenu),
    OP2BYTES(KEY_6,     0),
    OP2BYTES(KEY_MUL,   0),
    OP2BYTES(KEY_SHIFT, 0),
    OP2BYTES(KEY_1,     0),
    OP2BYTES(KEY_2,     0),
    OP2BYTES(KEY_3,     0),
    OP2BYTES(KEY_SUB,   0),
    OP2BYTES(KEY_EXIT,  0),
    OP2BYTES(KEY_0,     0),
    OP2BYTES(KEY_DOT,   0),
    OP2BYTES(KEY_RUN,   0),
    OP2BYTES(KEY_ADD,   0),

    OP2BYTES(KEY_F1,    0),
    OP2BYTES(KEY_F2,    0),
    OP2BYTES(KEY_F3,    0),
    OP2BYTES(KEY_F4,    0),
    OP2BYTES(KEY_F5,    0),
    OP2BYTES(KEY_F6,    0),

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
    defaultSecondShiftedCommand,
};


object_p input::object_for_key(int key)
// ----------------------------------------------------------------------------
//    Return the object for a given key
// ----------------------------------------------------------------------------
{
    uint plane = shift_plane();
    if (key >= KEY_F1 && key <= KEY_F6 && plane >= menuPlanes())
        plane = 0;

    object_p obj   = function[plane][key - 1];
    if (!obj)
    {
        const byte *ptr = defaultCommand[plane] + 2 * (key - 1);
        if (*ptr)
            obj = (object_p) ptr;
    }
    return obj;
}


bool input::handle_functions(int key)
// ----------------------------------------------------------------------------
//   Check if we have one of the soft menu functions
// ----------------------------------------------------------------------------
{
    if (!key)
        return false;

    record(input, "Handle function for key %d (plane %d) ", key, shift_plane());
    if (object_p obj = object_for_key(key))
    {
        evaluating = key;
        object::id ty = obj->type();
        bool imm = ty >= object::FIRST_IMMEDIATE && ty <= object::LAST_COMMAND;
        if (RT.editing() && !imm)
        {
            if (key == KEY_ENTER || key == KEY_BSP)
                return false;

            if (key >= KEY_F1 && key <= KEY_F6 && autoComplete)
            {
                size_t start = 0;
                size_t size  = 0;
                if (currentWord(start, size))
                {
                    RT.remove(start, size);
                    cursor = start;
                }
            }

            switch (mode)
            {
            case PROGRAM:
                if (obj->is_command())
                    return obj->insert(this) != object::ERROR;
                break;

            case ALGEBRAIC:
                if (obj->is_algebraic())
                    return obj->insert(this) != object::ERROR;
                break;

            default:
                // If we have the editor open, need to close it
                if (!end_edit())
                    return false;
                break;
            }

        }
        obj->execute();
        return true;
    }

    return false;
}


bool input::currentWord(size_t &start, size_t &size)
// ----------------------------------------------------------------------------
//   REturn position of word under the cursor if there is one
// ----------------------------------------------------------------------------
{
    utf8 sed = nullptr;
    bool result = currentWord(sed, size);
    if (result)
        start = sed - RT.editor();
    return result;
}


bool input::currentWord(utf8 &start, size_t &size)
// ----------------------------------------------------------------------------
//   Find the word under the cursor in the editor, if there is one
// ----------------------------------------------------------------------------
{
    if (size_t sz = RT.editing())
    {
        byte *ed = RT.editor();
        uint  c  = cursor;
        c = utf8_previous(ed, c);
        while (c > 0 && !command::is_separator_or_digit(ed + c))
            c = utf8_previous(ed, c);
        if (command::is_separator_or_digit(ed + c))
            c = utf8_next(ed, c, sz);
        uint spos = c;
        while (c < sz && !command::is_separator(ed + c))
            c = utf8_next(ed, c, sz);
        uint end = c;
        if (end > spos)
        {
            start = ed + spos;
            size = end - spos;
            return true;
        }
    }
    return false;
}
