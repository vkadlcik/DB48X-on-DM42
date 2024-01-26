// ****************************************************************************
//  user_interface.cc                                            DB48X project
// ****************************************************************************
//
//   File Description:
//
//     User interface for the calculator
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

#include "user_interface.h"

#include "arithmetic.h"
#include "blitter.h"
#include "command.h"
#include "functions.h"
#include "list.h"
#include "menu.h"
#include "precedence.h"
#include "program.h"
#include "runtime.h"
#include "settings.h"
#include "stack.h"
#include "symbol.h"
#include "sysmenu.h"
#include "target.h"
#include "utf8.h"
#include "util.h"

#ifdef SIMULATOR
#include "tests.h"
#endif // SIMULATOR

#include <ctype.h>
#include <dmcp.h>
#include <stdio.h>
#include <unistd.h>
#include <wctype.h>

// The primary user interface of the calculator
user_interface ui;

using std::max;
using std::min;

RECORDER(user_interface, 16, "ui processing");
RECORDER(text_editor, 16, "Text editor");
RECORDER(menus, 16, "Menu operations");
RECORDER(help,  16, "On-line help");

#define NUM_TOPICS      (sizeof(topics) / sizeof(topics[0]))

user_interface::user_interface()
// ----------------------------------------------------------------------------
//   Initialize the user interface
// ----------------------------------------------------------------------------
    : command(),
      help(-1u),
      line(0),
      topic(0),
      topics_history(0),
      topics(),
      cursor(0),
      select(~0U),
      searching(~0U),
      xoffset(0),
      mode(STACK),
      last(0),
      stack(LCD_H),
      cx(0),
      cy(0),
      edRows(0),
      edRow(0),
      edColumn(0),
      menuStack(),
      pageStack(),
      menuPage(),
      menuPages(),
      menuHeight(),
      busy(0),
      nextRefresh(~0U),
      dirty(),
      editing(),
      cmdIndex(0),
      clipboard(),
      shift(false),
      xshift(false),
      alpha(false),
      transalpha(false),
      lowercase(false),
      shift_drawn(false),
      xshift_drawn(false),
      alpha_drawn(false),
      lowerc_drawn(false),
      down(false),
      up(false),
      repeat(false),
      longpress(false),
      blink(false),
      follow(false),
      force(false),
      dirtyMenu(false),
      dirtyStack(false),
      dirtyCommand(false),
      dirtyHelp(false),
      autoComplete(false),
      adjustSeps(false),
      graphics(false),
      dbl_release(false),
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


static inline bool is_algebraic(user_interface::modes mode)
// ----------------------------------------------------------------------------
//   Return true for algebraic and parentheses mode
// ----------------------------------------------------------------------------
{
    return (mode == user_interface::ALGEBRAIC ||
            mode == user_interface::PARENTHESES);
}


void user_interface::edit(unicode c, modes m, bool autoclose)
// ----------------------------------------------------------------------------
//   Begin editing with a given character
// ----------------------------------------------------------------------------
{
    dirtyEditor = true;

    // If already editing, keep current mode
    if (rt.editing())
        m = mode;

    uint savec = cursor;
    insert(cursor, c);

    // Test delimiters
    unicode closing = 0;
    switch(c)
    {
    case '(':                   closing = ')';  m = PARENTHESES;        break;
    case '[':                   closing = ']';  m = MATRIX;             break;
    case '{':                   closing = '}';  m = PROGRAM;            break;
    case ':':  if (m != TEXT)   closing = ':';  m = DIRECT;             break;
    case '"':                   closing = '"';  m = TEXT;               break;
    case '\'':                  closing = '\''; m = ALGEBRAIC;          break;
    case L'«':                  closing = L'»'; m = PROGRAM;            break;
    case '\n': edRows = 0;                    break; // Recompute rows
    }
    if (closing && autoclose)
    {
        byte *ed = rt.editor();
        if (mode == PROGRAM || mode == DIRECT || is_algebraic(mode))
            if (savec > 0 && ed[savec] != ' ')
                insert(savec, ' ');
        size_t back = insert(cursor, closing);
        cursor -= back;
    }

    mode = m;
    adjustSeps = true;
}


object::result user_interface::edit(utf8 text, size_t len, modes m, int offset)
// ----------------------------------------------------------------------------
//   Enter the given text on the command line
// ----------------------------------------------------------------------------
{
    dirtyEditor = true;

    bool editing = rt.editing();
    byte *ed = rt.editor();
    bool skip = m == POSTFIX && is_algebraic(mode);

    // Skip the x in postfix operators (x⁻¹, x², x³ or x!)
    if (skip)
    {
        text++;
        len--;
    }

    if (!editing)
    {
        cursor = 0;
        select = ~0U;
        dirtyStack = true;
    }
    else if ((!is_algebraic(mode) || !is_algebraic(m)) &&
             cursor > 0 && ed[cursor-1] != ' ')
    {
        if (!skip && (!is_algebraic(mode) || (m != INFIX && m != CONSTANT)))
            insert(cursor, ' ');
    }

    size_t added = insert(cursor, text, len);

    if ((m == POSTFIX || m == INFIX || m == CONSTANT) && is_algebraic(mode))
    {
        /* nothing */
    }
    else if (!is_algebraic(mode) || !is_algebraic(m))
    {
        insert(cursor, ' ');
    }
    else if (m != INFIX)
    {
        if (insert(cursor, utf8("()"), 2) == 2)
            cursor--;
    }

    // Offset from beginning or end of inserted text
    if (offset > 0 && cursor > len)
        cursor = cursor - len + offset;
    else if (offset < 0 && cursor > uint(-offset))
        cursor = cursor + offset;

    dirtyEditor = true;
    adjustSeps = true;
    update_mode();
    return added == len ? object::OK : object::ERROR;
}


object::result user_interface::edit(utf8 text, modes m, int offset)
// ----------------------------------------------------------------------------
//   Edit a null-terminated text
// ----------------------------------------------------------------------------
{
    return edit(text, strlen(cstring(text)), m, offset);
}


bool user_interface::end_edit()
// ----------------------------------------------------------------------------
//   Clear the editor
// ----------------------------------------------------------------------------
{
    alpha       = false;
    lowercase   = false;
    shift       = false;
    xshift      = false;
    dirtyEditor = true;
    dirtyStack  = true;
    edRows      = 0;
    last        = 0;
    select      = ~0;

    clear_help();
    rt.clear_error();

    size_t edlen = rt.editing();
    if (edlen)
    {
        gcutf8  ed   = rt.editor();
        size_t  o    = 0;
        bool    text = false;
        unicode nspc = Settings.NumberSeparator();
        unicode hspc = Settings.BasedSeparator();

        draw_busy();

        // Save the command-line history (prior to removing spaces)
        text_g saved = text::make(ed, edlen);

        // Remove all additional decorative number spacing
        while (o < edlen)
        {
            unicode cp = utf8_codepoint(ed + o);
            if (cp == '"')
            {
                text = !text;
                o += 1;
            }
            else if (!text && (cp == nspc || cp == hspc))
            {
                size_t ulen = utf8_size(cp);
                ulen = remove(o, ulen);
                edlen -= ulen;
            }
            else
            {
                o += utf8_size(cp);
            }
        }

        text_g edstr = rt.close_editor();
        if (edstr)
        {
            gcutf8 editor = edstr->value();
            program_g cmds = program::parse(editor, edlen);
            if (cmds)
            {
                // We successfully parsed the line
                editor_save(saved, false);
                clear_editor();
                this->editing = nullptr;
                if (Settings.SaveStack())
                    rt.save();
                save<bool> no_halt(program::halted, false);
                cmds->run(Settings.SaveLastArguments());
            }
            else
            {
                // Move cursor to error if there is one
                utf8   pos  = rt.source();
                utf8   ed   = editor;
                size_t slen = rt.source_length();
                if (pos >= editor && pos <= ed + edlen)
                    cursor = pos - ed;
                select = slen ? cursor + slen : ~0U;
                if (!rt.edit(ed, edlen))
                {
                    cursor = 0;
                    select = ~0U;
                }
                draw_idle();
                beep(3300, 100);
                return false;
            }
        }
        draw_idle();
    }

    return true;
}


void user_interface::clear_editor()
// ----------------------------------------------------------------------------
//   Clear the editor either after edit, or when pressing EXIT
// ----------------------------------------------------------------------------
{
    rt.clear();
    cursor      = 0;
    select      = ~0U;
    xoffset     = 0;
    edRows      = 0;
    alpha       = false;
    shift       = false;
    xshift      = false;
    lowercase   = false;
    longpress   = false;
    repeat      = false;
    dirtyEditor = true;
    dirtyStack  = true;
    clear_help();
}


text_p user_interface::editor_save(bool rewinding)
// ----------------------------------------------------------------------------
//   Save current editor content for history
// ----------------------------------------------------------------------------
{
    if (rt.editing())
        if (text_g editor = rt.close_editor(false, false))
            return editor_save(editor, rewinding);
    return nullptr;
}


text_p user_interface::editor_save(text_r &editor, bool rewinding)
// ----------------------------------------------------------------------------
//   Save text as editor content for history
// ----------------------------------------------------------------------------
{
    bool found = false;
    uint base = rewinding ? cmdHistoryIndex : cmdIndex;
    for (uint h = 1; !found && h < HISTORY; h++)
    {
        uint i = (base + HISTORY - h) % HISTORY;
        if (history[i] && editor->is_same_as(history[i]))
        {
            std::swap(history[base], history[i]);
            found = true;
        }
    }
    if (!found)
        history[base] = editor;
    if (!rewinding)
    {
        cmdIndex = (cmdIndex + 1) % HISTORY;
        cmdHistoryIndex = cmdIndex;
    }
    return editor;
}


void user_interface::editor_history()
// ----------------------------------------------------------------------------
//   Restore editor buffer from history
// ----------------------------------------------------------------------------
{
    editor_save(true);
    for (uint h = 0; h < HISTORY; h++)
    {
        cmdHistoryIndex = (cmdHistoryIndex + HISTORY - 1) % HISTORY;
        if (history[cmdHistoryIndex])
        {
            size_t sz = 0;
            gcutf8 ed = history[cmdHistoryIndex]->value(&sz);
            rt.edit(ed, sz);
            cursor = 0;
            select = ~0U;
            alpha = xshift = shift = false;
            edRows = 0;
            dirtyEditor = true;
            break;
        }
    }
}


void user_interface::clear_help()
// ----------------------------------------------------------------------------
//   Clear help data
// ----------------------------------------------------------------------------
{
   command     = nullptr;
    help        = -1u;
    line        = 0;
    topic       = 0;
    follow      = false;
    last        = 0;
    longpress   = false;
    repeat      = false;
    dirtyMenu   = true;
    dirtyHelp   = true;         // Need to redraw what is behind help
    dirtyEditor = true;
    dirtyStack  = true;
    helpfile.close();
}


void user_interface::clear_menu()
// ----------------------------------------------------------------------------
//   Clear the menu
// ----------------------------------------------------------------------------
{
    menu(nullptr, 0);
    menus(0, nullptr, nullptr);
}

bool user_interface::key(int key, bool repeating, bool talpha)
// ----------------------------------------------------------------------------
//   Process an input key
// ----------------------------------------------------------------------------
{
    int skey = key;

    if (handle_screen_capture(key))
        return true;

    longpress = key && repeating;
    record(user_interface,
           "Key %d shifts %d longpress", key, shift_plane(), longpress);
    repeat = false;

    if (rt.error())
    {
        if (key == KEY_EXIT || key == KEY_ENTER || key == KEY_BSP)
            rt.clear_error();
        else if (key == KEY_SHIFT)
            handle_shifts(key, talpha);
        else if (key)
            beep(2200, 75);
        dirtyStack = true;
        dirtyEditor = true;
        return true;
    }

    // Handle keys
    bool result =
        handle_shifts(key, talpha)      ||
        handle_help(key)                ||
        handle_editing(key)             ||
        handle_alpha(key)               ||
        handle_digits(key)              ||
        handle_functions(key)           ||
        key == 0;

    if (rt.editing())
        update_mode();

    if (!skey && last != KEY_SHIFT)
    {
        shift = false;
        xshift = false;
        menu_refresh(menu::ID_Catalog);
    }

    if (!skey)
        command = nullptr;

    return result;
}


void user_interface::assign(int key, uint plane, object_p code)
// ----------------------------------------------------------------------------
//   Assign an object to a given key
// ----------------------------------------------------------------------------
{
    if (key >= 1 && key <= NUM_KEYS && plane <= NUM_PLANES)
        function[plane][key - 1] = code;
}


object_p user_interface::assigned(int key, uint plane)
// ----------------------------------------------------------------------------
//   Assign an object to a given key
// ----------------------------------------------------------------------------
{
    if (key >= 1 && key <= NUM_KEYS && plane <= NUM_PLANES)
        return function[plane][key - 1];
    return nullptr;
}


void user_interface::update_mode()
// ----------------------------------------------------------------------------
//   Scan the command line to check what the state is at the cursor
// ----------------------------------------------------------------------------
{
    utf8    ed    = rt.editor();
    utf8    last  = ed + cursor;
    uint    progs = 0;
    uint    lists = 0;
    uint    algs  = 0;
    uint    txts  = 0;
    uint    cmts  = 0;
    uint    vecs  = 0;
    uint    based = 0;
    uint    syms  = 0;
    uint    inum  = 0;
    uint    fnum  = 0;
    uint    hnum  = 0;
    uint    parn  = 0;
    unicode nspc  = Settings.NumberSeparator();
    unicode hspc  = Settings.BasedSeparator();
    unicode dmrk  = Settings.DecimalSeparator();
    unicode emrk  = Settings.ExponentSeparator();
    utf8    num   = nullptr;

    mode = DIRECT;
    for (utf8 p = ed; p < last; p = utf8_next(p))
    {
        unicode code = utf8_codepoint(p);

        if (!txts && !cmts)
        {
            if ((inum || fnum) && (code == emrk || code == '-'))
            {

            }
            else if (code == nspc || code == hspc)
            {
                // Ignore all extra spacing in numbers
                if (!num)
                    num = p;
            }
            else if (based)
            {
                if  (code < '0'
                 || (code > '9' && code < 'A')
                 || (code > 'Z' && code < 'a')
                 || (code > 'z'))
                {
                    based = 0;
                }
                else
                {
                    if (!num)
                        num = p;
                    hnum++;
                }
            }
            else if (!syms && code >= '0' && code <= '9')
            {
                if (!num)
                    num = p;
                if (fnum)
                    fnum++;
                else
                    inum++;
            }
            else if (code == dmrk)
            {
                if (!num)
                    num = p;
                fnum = 1;
            }
            else if (code == '@')
            {
                cmts++;
            }
            else
            {
                // All other characters: reset numbering
                based = inum = fnum = hnum = 0;
                num = nullptr;
                if (is_valid_as_name_initial(code))
                    syms = true;
                else if (syms && !is_valid_in_name(code))
                    syms = false;
            }

            switch(code)
            {
            case '\'':      algs = 1 - algs;                break;
            case '"':       txts = 1 - txts;                break;
            case '{':       lists++;                        break;
            case '}':       lists--;                        break;
            case '[':       vecs++;                         break;
            case ']':       vecs--;                         break;
            case '(':       parn++;                         break;
            case ')':       parn--;                         break;
            case L'«':      progs++;                        break;
            case L'»':      progs--;                        break;
            case '#':       based++;
                            hnum = inum = syms = 0;
                            num = nullptr;                  break;
            }
        }
        else if (txts && code == '"')
        {
            txts = 1 - txts;
        }
        else if (cmts && code == '\n')
        {
            cmts = 0;
        }
    }

    if (txts)
        mode = TEXT;
    else if (based)
        mode = BASED;
    else if (parn)
        mode = PARENTHESES;
    else if (vecs)
        mode = MATRIX;
    else if (algs)
        mode = ALGEBRAIC;
    else if (lists || progs)
        mode = PROGRAM;
    else
        mode = DIRECT;

    if (adjustSeps)
    {
        if  ((inum || fnum || hnum) && num)
        {
            // We are editing some kind of number. Insert relevant spacing.
            size_t len = rt.editing();

            // First identify the number range and remove all extra spaces in it
            bool   isnum = true;
            size_t frpos = 0;
            size_t start = num - ed;
            size_t o     = start;

            while (o < len && isnum)
            {
                unicode code = utf8_codepoint(ed + o);

                // Remove all spacing in the range
                if (code == nspc || code == hspc)
                {
                    size_t rlen = utf8_size(code);
                    rlen = remove(o, rlen);
                    len -= rlen;
                    ed = rt.editor(); // Defensive coding (no move on remove)
                    continue;
                }

                isnum = ((code >= '0' && code <= '9')
                         || (hnum && ((code >= 'A' && code <= 'Z')
                                      || (code >= 'a' && code <= 'z')))
                         || code == '+'
                         || code == '-'
                         || code == '#'
                         || code == dmrk);
                if (code == dmrk)
                    frpos = o + 1;
                if (isnum)
                    o += utf8_size(code);
            }

            // Insert markers on the fractional part if necessary
            if (frpos)
            {
                byte   encoding[4];
                size_t ulen = utf8_encode(nspc, encoding);
                uint   sf   = Settings.FractionSpacing();
                size_t end  = o;

                o = frpos - 1;
                if (sf)
                {
                    frpos += sf;
                    while (frpos < end)
                    {
                        if (!insert(frpos, encoding, ulen))
                            break;
                        frpos += sf + ulen;
                        len += ulen;
                        end += ulen;
                    }
                }
            }

            // Then insert markers on the integral part
            byte encoding[4];
            uint sp = hnum ? Settings.BasedSpacing()
                           : Settings.MantissaSpacing();
            if (sp)
            {
                unicode spc  = hnum ? Settings.BasedSeparator()
                                    : Settings.NumberSeparator();
                size_t ulen = utf8_encode(spc, encoding);
                while (o > start + sp)
                {
                    o -= sp;
                    if (!insert(o, encoding, ulen))
                        break;
                }
            }
        }
        adjustSeps = false;
    }
}


void user_interface::menu(menu_p menu, uint page)
// ----------------------------------------------------------------------------
//   Set menu and page
// ----------------------------------------------------------------------------
{
    menu::id mid = menu ? menu->type() : menu::ID_object;

    record(menus, "Selecting menu %t page %u", menu, page);

    if (mid != *menuStack)
    {
        pageStack[0] = menuPage;
        memmove(menuStack + 1, menuStack, sizeof(menuStack) - sizeof(*menuStack));
        memmove(pageStack + 1, pageStack, sizeof(pageStack) - sizeof(*pageStack));
        if (menu)
        {
            menuStack[0] = mid;
            pageStack[0] = page;
            menu->update(page);
        }
        else
        {
            menuStack[0] = menu::ID_object;
        }
        menuPage = page;
        dirtyMenu = true;
    }

    for (uint i = 0; i < HISTORY; i++)
        record(menus, "  History %u %+s.%u",
               i, menu::name(menuStack[i]), pageStack[i]);
}


menu_p user_interface::menu()
// ----------------------------------------------------------------------------
//   Return the current menu
// ----------------------------------------------------------------------------
{
    return menuStack[0] ? menu_p(menu::static_object(menuStack[0])) : nullptr;
}


void user_interface::menu_pop()
// ----------------------------------------------------------------------------
//   Pop last menu in menu history
// ----------------------------------------------------------------------------
{
    id current = menuStack[0];
    uint cpage = pageStack[0];

    record(menus, "Popping menu %+s", menu::name(current));

    memmove(menuStack, menuStack + 1, sizeof(menuStack) - sizeof(*menuStack));
    memmove(pageStack, pageStack + 1, sizeof(pageStack) - sizeof(*pageStack));
    menuStack[HISTORY-1] = menu::ID_object;
    pageStack[HISTORY-1] = 0;
    for (uint i = 1; i < HISTORY; i++)
    {
        if (menuStack[i] == menu::ID_object)
        {
            menuStack[i] = current;
            pageStack[i] = cpage;
            break;
        }
    }
    menuPage = 0;
    if (menu::id mty = menuStack[0])
    {
        menu_p m = menu_p(menu::static_object(mty));
        menuPage = pageStack[0];
        m->update(menuPage);
    }
    else
    {
        menus(0, nullptr, nullptr);
    }
    dirtyMenu = true;

    for (uint i = 0; i < HISTORY; i++)
        record(menus, "  History %u %+s.%u",
               i, menu::name(menuStack[i]), pageStack[i]);
}


uint user_interface::page()
// ----------------------------------------------------------------------------
//   Return the currently displayed page
// ----------------------------------------------------------------------------
{
    return menuPage;
}


void user_interface::page(uint p)
// ----------------------------------------------------------------------------
//   Set the menu page to display
// ----------------------------------------------------------------------------
{
    menuPage = (p + menuPages) % menuPages;
    if (menu_p m = menu())
        m->update(menuPage);
    dirtyMenu = true;
}


uint user_interface::pages()
// ----------------------------------------------------------------------------
//   Return number of menu pages
// ----------------------------------------------------------------------------
{
    return menuPages;
}


void user_interface::pages(uint p)
// ----------------------------------------------------------------------------
//   Return number of menu pages
// ----------------------------------------------------------------------------
{
    menuPages = p ? p : 1;
}


void user_interface::menus(uint count, cstring labels[], object_p function[])
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
    autoComplete = false;
}


void user_interface::menu(uint menu_id, cstring label, object_p fn)
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


void user_interface::menu(uint id, symbol_p label, object_p fn)
// ----------------------------------------------------------------------------
//   The drawing of menus recognizes symbols
// ----------------------------------------------------------------------------
{
    menu(id, (cstring) label, fn);
}


bool user_interface::menu_refresh()
// ----------------------------------------------------------------------------
//   Udpate current menu
// ----------------------------------------------------------------------------
{
    if (menuStack[0])
    {
        menu_p m = menu_p(menu::static_object(menuStack[0]));
        return m->update(menuPage) == object::OK;
    }
    return false;
}


bool user_interface::menu_refresh(object::id menu)
// ----------------------------------------------------------------------------
//   Request a refresh of a menu
// ----------------------------------------------------------------------------
{
    if (menuStack[0] == menu)
        return menu_refresh();
    return false;
}

void user_interface::marker(uint menu_id, unicode mark, bool alignLeft)
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


symbol_p user_interface::label(uint menu_id)
// ----------------------------------------------------------------------------
//   Return the label for a given menu ID
// ----------------------------------------------------------------------------
{
    cstring lbl = label_text(menu_id);
    if (*lbl == object::ID_symbol)
        return (symbol_p) lbl;
    return nullptr;
}


cstring user_interface::label_text(uint menu_id)
// ----------------------------------------------------------------------------
//   Return the label for a given menu ID
// ----------------------------------------------------------------------------
{
    int     softkey_id = menu_id % NUM_SOFTKEYS;
    int     plane      = menu_id / NUM_SOFTKEYS;
    cstring lbl        = menu_label[plane][softkey_id];
    return lbl;
}


uint user_interface::menu_planes()
// ----------------------------------------------------------------------------
//   Count menu planes
// ----------------------------------------------------------------------------
{
    int planes = 3;
    if (showing_help())
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


void user_interface::draw_start(bool forceRedraw, uint refresh)
// ----------------------------------------------------------------------------
//   Start a drawing cycle
// ----------------------------------------------------------------------------
{
    dirty = rect();
    force = forceRedraw;
    nextRefresh = refresh;
    graphics = false;
}


void user_interface::draw_refresh(uint delay)
// ----------------------------------------------------------------------------
//   Indicates that a component expects a refresh in the given delay
// ----------------------------------------------------------------------------
{
    if (nextRefresh > delay)
        nextRefresh = delay;
}


void user_interface::draw_dirty(coord x1, coord y1, coord x2, coord y2)
// ----------------------------------------------------------------------------
//   Indicates that a component dirtied a given area of the screen
// ----------------------------------------------------------------------------
{
    draw_dirty(rect(min(x1,x2), min(y1,y2), max(x1,x2)+1, max(y1,y2)+1));
}


void user_interface::draw_dirty(const rect &r)
// ----------------------------------------------------------------------------
//   Indicates that a component dirtied a given area of the screen
// ----------------------------------------------------------------------------
{
    if (dirty.empty())
        dirty = r;
    else
        dirty |= r;
}


bool user_interface::draw_graphics()
// ----------------------------------------------------------------------------
//   Start graphics mode
// ----------------------------------------------------------------------------
{
    if (!graphics)
    {
        draw_start(false);
        graphics = true;
        Screen.fill(pattern::white);
        draw_dirty(0, 0, LCD_W, LCD_H);
        return true;
    }
    return false;
}


bool user_interface::draw_menus()
// ----------------------------------------------------------------------------
//   Draw the softkey menus
// ----------------------------------------------------------------------------
{
    static int  lastp   = 0;
    static uint lastt   = 0;
    static uint animate = 0;
    uint        time    = sys_current_ms();
    int         shplane = shift_plane();
    uint        period  = usb_powered() ? 200 : 850;

    bool animating = animate && (time - lastt > period);
    bool redraw = dirtyMenu || shplane != lastp || animating;
    if (!force && !redraw)
        return false;

    if (force || dirtyMenu || shplane != lastp)
    {
        animate = 0;
        animating = false;
    }

    lastp = shplane;
    lastt = time;
    dirtyMenu = false;

    font_p font  = MenuFont;
    int    mh    = font->height() + 2;
    int    mw    = (LCD_W - 10) / 6;
    int    sp    = (LCD_W - 5) - 6 * mw;
    rect   clip  = Screen.clip();
    bool   help  = showing_help();

    if (period > time - last)
        period = time - last;

    static unsigned menuShift = 0;
    menuShift++;

    int  planes        = menu_planes();
    id   menuStyle     = Settings.MenuAppearance();
    bool single        = menuStyle == object::ID_SingleRowMenus;
    bool flat          = menuStyle == object::ID_FlatMenus;
    bool square        = Settings.SquareMenus();
    int  visiblePlanes = single ? 1 : planes;
    uint newMenuHeight = 1 + visiblePlanes * mh;
    if (newMenuHeight != menuHeight)
    {
        menuHeight = newMenuHeight;
        dirtyStack = true;
        dirtyEditor = true;
    }

    if (flat)
    {
        object_p prevo = command::static_object(command::ID_MenuPreviousPage);
        object_p nexto = command::static_object(command::ID_MenuNextPage);
        object_p what = function[0][KEY_F6-1];
        bool prev = what == prevo;
        bool next = what == nexto;
        if (prev || next)
        {
            if (shplane != prev)
            {
                if (shplane)
                {
                    function[0][KEY_F6-1] = prevo;
                    menu_label[0][NUM_SOFTKEYS-1] = "◀︎";
                }
                else
                {
                    function[0][KEY_F6-1] = nexto;
                    menu_label[0][NUM_SOFTKEYS-1] = "▶";
                }
            }
        }
        shplane = 0;
    }


    for (int plane = 0; plane < planes; plane++)
    {
        cstring *labels = menu_label[plane];
        if (help)
        {
            static cstring helpMenu[] =
            {
                "Home", "Page▲", "Page▼", "Link▲", "Link▼", "← Topic"
            };
            labels = helpMenu;
        }

        if (single)
            if (plane != shplane)
                continue;

        int my = LCD_H - (plane * !single + 1) * mh;
        for (int m = 0; m < NUM_SOFTKEYS; m++)
        {
            uint animask = (1<<(m + plane * NUM_SOFTKEYS));
            if (animating && (~animate & animask))
                continue;

            coord x = (2 * m + 1) * mw / 2 + (m * sp) / 5 + 2;
            size mcw = mw;
            rect mrect(x - mw/2-1, my, x + mw/2, my+mh-1);
            if (animating)
                draw_dirty(mrect);

            bool alt = planes > 1 && plane != shplane;
            pattern color = pattern::black;

            if (square)
            {
                mrect.x2++;
                mrect.y2++;
                Screen.fill(mrect, alt ? pattern::gray50 : pattern::black);
                mrect.inset(1, 1);
                Screen.fill(mrect, pattern::white);
                if (!alt)
                {
                    rect trect(x - mw/2-1, my, x + mw/2, my+1);
                    Screen.fill(trect, pattern::black);
                    trect.offset(0, mh-2);
                    Screen.fill(trect, pattern::black);
                }
            }
            else
            {
                if (!alt)
                    color = pattern::white;
                Screen.fill(mrect, pattern::white);
                mrect.inset(3,  1);
                Screen.fill(mrect, pattern::black);
                mrect.inset(-1, 1);
                Screen.fill(mrect, pattern::black);
                mrect.inset(-1, 1);
                Screen.fill(mrect, pattern::black);
                mrect.inset(2, 0);
                if (alt)
                    Screen.fill(mrect, pattern::white);
            }


            utf8 label = utf8(labels[m]);
            if (label)
            {
                unicode marker = 0;
                coord   mkw    = 0;
                coord   mkx    = 0;

                size_t len = 0;
                if (*label == object::ID_symbol)
                {
                    COMPILE_TIME_ASSERT(object::ID_symbol < ' ');

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
                rect trect = mrect;
                if (!help)
                {
                    if (unicode mark = menu_marker[plane][m])
                    {
                        if (mark == 1)
                        {
                            mark = settings::MARK;
                        }
                        if (mark == L'░')
                        {
                            color = pattern::gray50;
                        }
                        else
                        {
                            bool alignLeft = menu_marker_align[plane][m];
                            marker         = mark;
                            mkw            = (marker == '/'
                                              ? 0
                                              : font->width(marker));
                            mkx            = alignLeft ? x - mw / 2 + 2
                                                       : x + mw / 2 - mkw - 2;
                            mcw -= mkw;
                            if (alignLeft)
                                trect.x1 += mkw;
                            else
                                trect.x2 -= mkw;
                        }
                    }
                }

                Screen.clip(trect);
                size tw = font->width(label, len);
                if (marker == '/')
                    tw += font->width(utf8("⁻¹"));
                if (tw + 2 >= mcw)
                {
                    animate |= animask;
                    x = mrect.x1 - menuShift % (tw - mcw + 5);
                }
                else
                {
                    x = (mrect.x1 + mrect.x2 - tw) / 2;
                }
                coord ty = mrect.y1 - (3 - square);
                x = Screen.text(x, ty, label, len, font, color);
                if (marker)
                {
                    Screen.clip(mrect);
                    bool dossier = marker==L'◥';
                    if (dossier)
                    {
                        if (alt || square)
                            Screen.glyph(mkx+3, ty-3, marker, font, color);
                        Screen.clip(clip);
                        Screen.glyph(mkx+4, ty-4, marker, font, pattern::white);
                    }
                    else if (marker == '/')
                    {
                        Screen.text(x, ty,
                                    utf8("⁻¹"), sizeof("⁻¹")-1, font, color);
                    }
                    else
                    {
                        Screen.glyph(mkx, ty, marker, font, color);
                    }
                }
                Screen.clip(clip);
            }
        }
    }
    if (square && shplane < visiblePlanes)
    {
        int my = LCD_H - (shplane * !single + 1) * mh;
        Screen.fill(0, my, LCD_W-1, my, pattern::black);
    }

    if (animate)
        draw_refresh(period);
    if (!animating)
        draw_dirty(0, LCD_H - menuHeight, LCD_W, LCD_H);

    return true;
}


bool user_interface::draw_header()
// ----------------------------------------------------------------------------
//   Draw the header with the state name
// ----------------------------------------------------------------------------
{
    static uint day = 0, month = 0, year = 0;
    static uint hour = 0, minute = 0, second = 0;
    static uint dow = 0;
    bool changed = force;

    if (!changed)
    {
        dt_t dt;
        tm_t tm;
        rtc_wakeup_delay();
        rtc_read(&tm, &dt);

        if (day != dt.day || month != dt.month || year != dt.year)
        {
            day = dt.day;
            month = dt.month;
            year = dt.year;
            changed = true;
        }
        if (hour != tm.hour || minute != tm.min || second != tm.sec)
        {
            hour = tm.hour;
            minute = tm.min;
            second = tm.sec;
            changed = true;
        }
        if (dow != tm.dow)
        {
            dow = tm.dow;
            changed = true;
        }
    }

    if (changed)
    {
        size h = HeaderFont->height() + 1;
        rect clip = Screen.clip();
        rect header(0, 0, LCD_W, h);

        Screen.clip(0, 0, 260, h);
        Screen.fill(header, pattern::black);

        char buffer[MAX_LCD_LINE_LEN];
        size_t sz = 0;

#define EMIT(...)                                               \
        do                                                      \
        {                                                       \
            char  *to  = buffer + sz;                           \
            size_t max = MAX_LCD_LINE_LEN - sz;                 \
            size_t wr  = snprintf(to, max, __VA_ARGS__);        \
            sz += wr;                                           \
        } while (0)

        // Read the real-time clock
        if (Settings.ShowDate())
        {
            char mname[4];
            if (Settings.ShowMonthName())
                snprintf(mname, 4, "%s", get_month_shortcut(month));
            else
                snprintf(mname, 4, "%d", month);
            char ytext[6];
            if (Settings.TwoDigitYear())
                snprintf(ytext, 6, "%02d", year % 100);
            else
                snprintf(ytext, 6, "%d", year);

            if (Settings.ShowDayOfWeek())
                EMIT("%s ", get_wday_shortcut(dow));

            char sep   = Settings.DateSeparator();
            uint index = 2 * Settings.YearFirst() + Settings.MonthBeforeDay();
            switch(index)
            {
            case 0: EMIT("%d%c%s%c%s ", day,   sep, mname, sep, ytext); break;
            case 1: EMIT("%s%c%d%c%s ", mname, sep, day,   sep, ytext); break;
            case 2: EMIT("%s%c%d%c%s ", ytext, sep, day,   sep, mname); break;
            case 3: EMIT("%s%c%s%c%d ", ytext, sep, mname, sep, day);   break;
            }
        }
        if (Settings.ShowTime())
        {
            EMIT("%d", Settings.Time24H() ? hour : hour % 12);
            EMIT(":%02d", minute);
            if (Settings.ShowSeconds())
                EMIT(":%02d", second);
            if (Settings.Time12H())
                EMIT("%c", hour < 12 ? 'A' : 'P');
            EMIT(" ");
            draw_refresh(Settings.ShowSeconds() ? 1000 : 1000 * (60 - second));
        }

        EMIT("%s", state_name());

        Screen.text(1, 0, utf8(buffer), HeaderFont, pattern::white);
        Screen.clip(clip);
        draw_dirty(header);
        return true;
    }
    return false;
}


static const uint  ann_width  = 15;
static const uint  ann_height = 12;


static const byte ann_right[] =
// ----------------------------------------------------------------------------
//   Right-shift annunciator
// ----------------------------------------------------------------------------
{
    0xfe, 0x3f, 0xff, 0x7f, 0x9f, 0x7f,
    0xcf, 0x7f, 0xe7, 0x7f, 0x03, 0x78,
    0x03, 0x70, 0xe7, 0x73, 0xcf, 0x73,
    0x9f, 0x73, 0xff, 0x73, 0xfe, 0x33
};


static const byte ann_left[] =
// ----------------------------------------------------------------------------
//   Left-shift annunciator
// ----------------------------------------------------------------------------
{
    0xfe, 0x3f, 0xff, 0x7f, 0xff, 0x7c,
    0xff, 0x79, 0xff, 0x73, 0x0f, 0x60,
    0x07, 0x60, 0xe7, 0x73, 0xe7, 0x79,
    0xe7, 0x7c, 0xe7, 0x7f, 0xe6, 0x3f
};


bool user_interface::draw_annunciators()
// ----------------------------------------------------------------------------
//    Draw the annunciators for Shift, Alpha, etc
// ----------------------------------------------------------------------------
{
    bool result = false;

    size lh = HeaderFont->height();
    if (force || alpha != alpha_drawn || lowercase != lowerc_drawn)
    {
        utf8 label = utf8(lowercase ? "abc" : "ABC");
        size lw = HeaderFont->width(label);
        Screen.fill(280, 0, 280+lw, 1+lh, pattern::black);
        if (alpha)
            Screen.text(280, 0, label, HeaderFont, pattern::white);
        draw_dirty(280, 0, 280+lw, 1+lh);
        alpha_drawn = alpha;
        lowerc_drawn = lowercase;
        result = true;
    }

    if (!force && shift == shift_drawn && xshift == xshift_drawn)
        return result;

    coord       ann_y      = (lh - ann_height) / 2;
    const byte *source     = xshift ? ann_right : shift ? ann_left : nullptr;
    if (source)
    {
        pixword *sw = (pixword *) source;
        surface  s(sw, ann_width, ann_height, 16);
        Screen.copy(s, 260, ann_y);
    }
    else if (!force)
    {
        Screen.fill(260, ann_y, 260+ann_width, ann_y+ann_height);
    }
    draw_dirty(260, ann_y, 260+ann_width, ann_y+ann_height);
    shift_drawn = shift;
    xshift_drawn = xshift;
    return true;
}


bool user_interface::draw_battery()
// ----------------------------------------------------------------------------
//    Draw the battery information
// ----------------------------------------------------------------------------
{
    static uint last       = 0;
    uint        time       = sys_current_ms();

    const uint  ann_height = 12;
    size        hfh        = HeaderFont->height();
    coord       ann_y      = (hfh - ann_height) / 2;

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
    }
    else if (!force)
    {
        return false;
    }

    // Experimentally, battery voltage below 2.6V cause calculator flakiness
    const int vmax = BATTERY_VMAX;
    const int vmin = BATTERY_VMIN;
    const int vlow = BATTERY_VLOW;

    bool showv = Settings.ShowVoltage();
    coord x = showv ? 311 : 370;
    rect bat(x + 3, ann_y+2, x + 25, ann_y + ann_height);
    Screen.fill(x-3, 0, LCD_W, hfh + 1, pattern::black);
    if (showv)
    {
        char buffer[64];
        snprintf(buffer, sizeof(buffer), "%d.%03dV", vdd / 1000, vdd % 1000);
        pattern vpat = low         ? pattern::gray25
                     : vdd <= vlow ? pattern::gray50
                                   : pattern::white;
        Screen.text(340, 1, utf8(buffer), HeaderFont, vpat);
    }
    Screen.fill(x, ann_y + 4, x+4, ann_y + ann_height - 2, pattern::white);

    Screen.fill(bat, pattern::white);
    bat.inset(1,1);
    Screen.fill(bat, pattern::black);
    bat.inset(1,1);

    size batw = bat.width();
    size w = (vdd - vmin) * batw / (vmax - vmin);
    if (w > batw)
        w = batw;
    else if (w < 1)
        w = 1;
    bat.x1 = bat.x2 - w;

    Screen.fill(bat, usb ? pattern::gray50 : pattern::white);
    if (!usb)
    {
        bat.x2 += 1;
        while (bat.x2 > x + 8)
        {
            bat.x2 -= 4;
            bat.x1 = bat.x2;
            Screen.fill(bat, pattern::black);
        }
    }

    draw_dirty(x, 0, LCD_W, hfh);
    draw_refresh(2000);

    // Power off if battery power is really low
    if (vdd < BATTERY_VOFF)
        power_off();

    return true;
}


bool user_interface::draw_busy(unicode glyph)
// ----------------------------------------------------------------------------
//    Draw the busy flying cursor
// ----------------------------------------------------------------------------
{
    if (graphics)
        return false;

    size w  = 32;
    size h  = HeaderFont->height();
    size x  = 279 - w;
    size y  = 0;

    rect r(x, y, x + w, y + h + 1);
    Screen.fill(r, pattern::black);
    if (glyph)
    {
        rect clip = Screen.clip();
        Screen.clip(r);
        coord gx = x + sys_current_ms() / 16 % w;
        Screen.glyph(gx, y, glyph, HeaderFont, pattern::white);
        Screen.clip(clip);
    }
    draw_dirty(r);
    refresh_dirty();
    return true;
}


bool user_interface::draw_idle()
// ----------------------------------------------------------------------------
//   Clear busy indicator
// ----------------------------------------------------------------------------
{
    if (graphics)
    {
        graphics = false;
        wait_for_key_press();
        redraw_lcd(true);
    }
    draw_busy(0);
    alpha_drawn = !alpha_drawn;
    shift_drawn = !shift;
    xshift_drawn = !xshift;
    draw_annunciators();
    refresh_dirty();
    return true;
}


bool user_interface::draw_editor()
// ----------------------------------------------------------------------------
//   Draw the editor
// ----------------------------------------------------------------------------
{
    if (!force && !dirtyEditor)
        return false;

    record(text_editor, "Redrawing %+s %+s curs=%d, offset=%d cx=%d",
           dirtyEditor ? "dirty" : "clean",
           force ? "forced" : "lazy",
           cursor, xoffset, cx);

    // Get the editor area
    utf8   ed   = rt.editor();
    size_t len  = rt.editing();
    utf8   last = ed + len;
    dirtyEditor = false;

    if (!len)
    {
        // Editor is not open, compute stack bottom
        int ns = LCD_H - menuHeight;
        if (stack != ns)
        {
            stack = ns;
            dirtyStack = true;
        }
        return false;
    }

    // Select font
    font_p font = Settings.editor_font(false);

    // Count rows and colums
    int  rows   = 1;            // Number of rows in editor
    int  cwidth = 0;            // Column width
    int  edrow  = 0;            // Row number of line being edited
    int  cursx  = 0;            // Cursor X position
    bool found  = false;

    byte *wed = (byte *) ed;
    wed[len] = 0;               // Ensure utf8_next does not go into the woods

    // Count rows to check if we need to switch to stack font
    if (!edRows)
    {
        for (utf8 p = ed; p < last; p = utf8_next(p))
            if (*p == '\n')
                rows++;
        edRows = rows;

        font = Settings.editor_font(rows > 2);

        rows = 1;
        for (utf8 p = ed; p < last; p = utf8_next(p))
        {
            if (p - ed == (int) cursor)
            {
                edrow = rows - 1;
                cursx = cwidth;
                found = true;
            }

            if (*p == '\n')
            {
                rows++;
                cwidth = 0;
            }
            else
            {
                unicode cp  = utf8_codepoint(p);
                cwidth     += font->width(cp);
            }
        }
        if (!found)
        {
            edrow = rows - 1;
            cursx = cwidth;
        }

        edRow = edrow;

        record(text_editor, "Computed: row %d/%d cursx %d (%d+%d=%d)",
               edrow, rows, cursx, cx, xoffset, cx+xoffset);
    }
    else
    {
        rows  = edRows;
        edrow = edRow;
        cursx = cx + xoffset;
        font = Settings.editor_font(rows > 2);

        record(text_editor, "Cached: row %d/%d cursx %d (%d+%d)",
               edrow, rows, cursx, cx, xoffset, cx+xoffset);
    }

    // Check if we want to move the cursor up or down
    if (up || down)
    {
        int   r    = 0;
        coord c    = 0;
        int   tgt  = edrow - (up && edrow > 0) + down;
        bool  done = up && edrow == 0;

        record(text_editor,
               "Moving %+s%+s edrow=%d target=%d curs=%d cursx=%d edcx=%d",
               up ? "up" : "", down ? "down" : "",
               edrow, tgt, cursor, cursx, edColumn);

        for (utf8 p   = ed; p < last && !done; p = utf8_next(p))
        {
            if (*p == '\n')
            {
                r++;
                if (r > tgt)
                {
                    cursor = p - ed;
                    edrow  = tgt;
                    done   = true;
                }
            }
            else if (r == tgt)
            {
                unicode cp = utf8_codepoint(p);
                c += font->width(cp);
                if (c > edColumn)
                {
                    cursor = p - ed;
                    edrow = r;
                    done = true;
                }
            }
        }
        if (!done && down)
        {
            cursor = len;
            edrow = rows - 1;
        }
        record(text_editor, "Moved %+s%+s row=%d curs=%d",
               up ? "up" : "", down ? "down" : "",
               edrow, cursor);

        up   = false;
        down = false;
        edRow = edrow;
    }
    else
    {
        edColumn = cursx;
    }

    // Draw the area that fits on the screen
    int   lineHeight      = font->height();
    int   errorHeight     = rt.error() ? LCD_H / 3 + 10 : 0;
    int   top             = HeaderFont->height() + errorHeight + 2;
    int   bottom          = LCD_H - menuHeight;
    int   availableHeight = bottom - top;
    int   fullRows        = availableHeight / lineHeight;
    int   clippedRows     = (availableHeight + lineHeight - 1) / lineHeight;
    utf8  display         = ed;
    coord y               = bottom - rows * lineHeight;

    blitter::rect clip = Screen.clip();
    Screen.clip(0, top, LCD_W, bottom);
    record(text_editor, "Clip between %d and %d", top, bottom);
    if (rows > fullRows)
    {
        // Skip rows to show the cursor
        int half = fullRows / 2;
        int skip = edrow < half         ? 0
                 : edrow >= rows - half ? rows - fullRows
                                        : edrow - half;
        record(text_editor,
               "Available %d, ed %d, displaying %d, skipping %d",
               fullRows,
               edrow,
               clippedRows,
               skip);

        for (int r = 0; r < skip; r++)
        {
            do
                display = utf8_next(display);
            while (*display != '\n');
        }
        if (skip)
            display = utf8_next(display);
        record(text_editor, "Truncated from %d to %d, text=%s",
               rows, clippedRows, display);
        rows = clippedRows;
        y = top;
    }


    // Draw the editor rows
    int  hskip = 180;
    size cursw = font->width('M');
    if (xoffset > cursx)
        xoffset = (cursx > hskip) ? cursx - hskip : 0;
    else if (coord(xoffset + LCD_W - cursw) < cursx)
        xoffset = cursx - LCD_W + cursw + hskip;

    coord x = -xoffset;
    int   r = 0;

    if (y < top)
        y = top;
    if (stack != y - 1)
    {
        stack      = y - 1;
        dirtyStack = true;
    }
    Screen.fill(0, stack, LCD_W, bottom, pattern::white);
    draw_dirty(0, stack, LCD_W, bottom);

    while (r < rows && display <= last)
    {
        bool atCursor = display == ed + cursor;
        if (atCursor)
        {
            cx = x;
            cy = y;
        }
        if (display >= last)
            break;

        unicode c   = utf8_codepoint(display);
        uint    pos = display - ed;
        bool    sel = ~select && int((pos - cursor) ^ (pos - select)) < 0;
        display     = utf8_next(display);
        if (c == '\n')
        {
            if (sel && x >= 0 && x < LCD_W)
                Screen.fill(x, y, LCD_W, y + lineHeight - 1, pattern::black);
            y += lineHeight;
            x  = -xoffset;
            r++;
            continue;
        }
        int cw = font->width(c);
        if (x + cw >= 0 && x < LCD_W)
        {
            pattern fg  = sel ? pattern::white : pattern::black;
            pattern bg  = sel ? (~searching ? pattern::gray25 : pattern::black)
                              : pattern::white;
            x = Screen.glyph(x, y, c, font, fg, bg);
        }
        else
        {
            x += cw;
        }
    }
    if (cursor >= len)
    {
        cx = x;
        cy = y;
    }

    Screen.clip(clip);

    return true;
}


bool user_interface::draw_cursor(int show, uint ncursor)
// ----------------------------------------------------------------------------
//   Draw the cursor at the location
// ----------------------------------------------------------------------------
//   This function returns the cursor vertical position for screen refresh
{
    // Do not draw if not editing or if help is being displayed
    if (!rt.editing() || showing_help())
        return false;

    static uint lastT = 0;
    uint time = sys_current_ms();
    const uint period = Settings.CursorBlinkRate();

    if (!force && !show && time - lastT < period)
    {
        draw_refresh(lastT + period - time);
        return false;
    }
    lastT = time;
    if (show)
        blink = show > 0;

    // Select editor font
    bool   ml         = edRows > 2;
    utf8   ed         = rt.editor();
    font_p edFont     = Settings.editor_font(ml);
    font_p cursorFont = Settings.cursor_font(ml);
    size_t len        = rt.editing();
    utf8   last       = ed + len;

    // Select cursor character
    unicode cursorChar = ~searching          ? 'S'
                       : mode == DIRECT      ? 'D'
                       : mode == TEXT        ? (lowercase ? 'L' : 'C')
                       : mode == PROGRAM     ? 'P'
                       : mode == ALGEBRAIC   ? 'A'
                       : mode == PARENTHESES ? 'E'
                       : mode == MATRIX      ? 'M'
                       : mode == BASED       ? 'B'
                                             : 'X';
    size    csrh       = cursorFont->height();
    coord   csrw       = cursorFont->width(cursorChar);
    size    ch         = edFont->height();

    coord   x          = cx;
    utf8    p          = ed + cursor;
    rect    clip       = Screen.clip();
    coord   ytop       = HeaderFont->height() + 2;
    coord   ybot       = LCD_H - menuHeight;

    Screen.clip(0, ytop, LCD_W, ybot);
    bool spaces = false;
    while (x <= cx + csrw + 1)
    {
        unicode cchar  = p < last ? utf8_codepoint(p) : ' ';
        if (cchar == '\n')
            spaces = true;
        if (spaces)
            cchar = ' ';
        size cw = edFont->width(cchar);
        bool gray = x == cx && !show;
        Screen.fill(x, cy, x + cw - 1, cy + ch - 1,
                    gray ? pattern::gray75 : pattern::white);
        draw_dirty(x, cy, x + cw - 1, cy + ch - 1);

        // Write the character under the cursor
        uint pos = p - ed;
        bool sel = ~select && int((pos - ncursor) ^ (pos - select)) < 0;
        pattern fg = sel ? pattern::white : pattern::black;
        pattern bg  = sel ? (~searching ? pattern::gray25 : pattern::black)
                          : pattern::white;
        x = Screen.glyph(x, cy, cchar, edFont, fg, bg);
        if (p < last)
            p = utf8_next(p);
    }

    if (blink)
    {
        coord csrx = cx + 1;
        coord csry = cy + (ch - csrh)/2;
        Screen.invert(csrx, cy, csrx+1, cy + ch - 1);
        rect  r(csrx, csry - 1, csrx+csrw, csry + csrh);
        if (alpha)
        {
            Screen.fill(r, pattern::black);
            r.inset(2,2);
            Screen.fill(r, pattern::white);
            Screen.glyph(csrx, csry, cursorChar, cursorFont, pattern::black);
        }
        else
        {
            Screen.fill(r, pattern::black);
            Screen.glyph(csrx, csry, cursorChar, cursorFont, pattern::white);
        }
        draw_dirty(r);
    }

    blink = !blink;
    Screen.clip(clip);
    draw_refresh(period);
    return true;
}


bool user_interface::draw_command()
// ----------------------------------------------------------------------------
//   Draw the current command
// ----------------------------------------------------------------------------
{
    if (force || dirtyCommand)
    {
        dirtyCommand = false;
        if (command && !rt.error())
        {
            font_p font = StackFont;
            size   w    = font->width(command);
            size   h    = font->height();
            coord  x    = 25;
            coord  y    = HeaderFont->height() + 6;

            Screen.fill(x-2, y-1, x+w+2, y+h+1, pattern::black);
            Screen.text(x, y, command, font, pattern::white);
            draw_dirty(x-2, y-1, x+w+2, y+h+1);
            return true;
        }
    }

    return false;
}


void user_interface::draw_user_command(utf8 cmd, size_t len)
// ----------------------------------------------------------------------------
//   Draw the current command
// ----------------------------------------------------------------------------
{
    font_p font = StackFont;
    size   w    = command ? font->width(command) : 0;
    size   h    = font->height();
    coord  x    = 25;
    coord  y    = HeaderFont->height() + 6;

    // Erase normal command
    Screen.fill(x-2, y-1, x + w + 2, y + h + 1, pattern::gray50);

    // Draw user command
    size nw = font->width(cmd, len);
    if (nw > w)
        w = nw;

    // User-defined command, display in white
    rect r(x-2, y-1, x+w+2, y+h+1);
    draw_dirty(r);
    Screen.fill(r, pattern::black);
    r.inset(1,1);
    Screen.fill(r, pattern::white);
    Screen.text(x + (w - nw) / 2, y, cmd, len, font, pattern::black);

    // Update screen
    refresh_dirty();
}


bool user_interface::draw_stepping_object()
// ----------------------------------------------------------------------------
//   Draw the next command to evaluate while stepping
// ----------------------------------------------------------------------------
{
    if (object_p obj = rt.run_stepping())
    {
        char buffer[40];
        size_t length = obj->render(buffer, sizeof(buffer));
        draw_user_command(utf8(buffer), length);
        ui.draw_busy(L'♦');
        return true;
    }
    return false;
}


bool user_interface::draw_error()
// ----------------------------------------------------------------------------
//   Draw the error message if there is one
// ----------------------------------------------------------------------------
{
    if (utf8 err = rt.error())
    {
        const int border = 4;
        coord     top    = HeaderFont->height() + 10;
        coord     height = LCD_H / 3;
        coord     width  = LCD_W - 8;
        coord     x      = LCD_W / 2 - width / 2;
        coord     y      = top;

        rect clip = Screen.clip();
        rect r(x, y, x + width - 1, y + height - 1);
        draw_dirty(r);
        Screen.fill(r, pattern::gray50);
        r.inset(border);
        Screen.fill(r, pattern::white);
        r.inset(2);

        Screen.clip(r);
        if (text_p cmd = rt.command())
        {
            size_t sz = 0;
            utf8 cmdt = cmd->value(&sz);
            coord x = Screen.text(r.x1, r.y1, cmdt, sz, ErrorFont);
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
    return true;
}


bool user_interface::draw_message(utf8 header, uint count, utf8 msgs[])
// ----------------------------------------------------------------------------
//   Draw an immediate message
// ----------------------------------------------------------------------------
{
    font_p font   = LibMonoFont10x17;
    size   h      = font->height();
    size   ch     = h * 5 / 2 + h * count + 10;
    coord  top    = HeaderFont->height() + 10;
    size   height = ch < LCD_H / 3 ? LCD_H / 3 : ch;
    size   width  = LCD_W - 8;
    coord  x      = LCD_W / 2 - width / 2;
    coord  y      = top;
    rect   clip   = Screen.clip();
    rect   r(x, y, x + width - 1, y + height - 1);

    draw_dirty(r);
    Screen.fill(r, pattern::gray50);
    r.inset(1);
    Screen.fill(r, pattern::white);
    r.inset(1);
    Screen.fill(r, pattern::black);
    r.inset(2);
    Screen.fill(r, pattern::white);
    r.inset(2);

    Screen.clip(r);
    x = r.x1;
    y = r.y1;

    Screen.text(x+0, y, header, font);
    Screen.text(x+1, y, header, font);
    y += h * 3 / 2;

    for (uint i = 0; i < count; i++)
        if (msgs[i])
            Screen.text(x, y + i * h, msgs[i], font);

    Screen.clip(clip);
    refresh_dirty();

    return true;
}


bool user_interface::draw_message(cstring header, cstring msg1, cstring msg2)
// ----------------------------------------------------------------------------
//   Draw an immediate message in C string mode
// ----------------------------------------------------------------------------
{
    utf8 msgs[] = { utf8(msg1), utf8(msg2) };
    return draw_message(utf8(header), 2, msgs);
}


bool user_interface::draw_stack()
// ----------------------------------------------------------------------------
//   Redraw the stack if dirty
// ----------------------------------------------------------------------------
{
    if (!force && !dirtyStack)
        return false;
    draw_busy();
    Stack.draw_stack();
    draw_dirty(0, HeaderFont->height() + 2, stack, LCD_H);
    draw_idle();
    dirtyStack = false;
    dirtyCommand = true;
    return true;
}


void user_interface::load_help(utf8 topic, size_t len)
// ----------------------------------------------------------------------------
//   Find the help message associated with the topic
// ----------------------------------------------------------------------------
{
    record(help, "Loading help topic %s", topic);

    if (!len)
        len = strlen(cstring(topic));
    command   = nullptr;
    follow    = false;
    dirtyHelp = true;

    // Need to have the help file open here
    if (!helpfile.valid())
    {
        helpfile.open(HELPFILE_NAME);
        if (!helpfile.valid())
        {
            help = -1u;
            line = 0;
            return;
        }
    }
    dirtyMenu = true;

    // Look for the topic in the file
    int  matching = 0;
    uint level    = 0;
    bool hadcr    = true;
    uint topicpos = 0;

#if SIMULATOR
    char debug[80];
    uint debugindex = 0;
#endif // SIMULATOR

    helpfile.seek(0);
    for (char c = helpfile.getchar(); c; c = helpfile.getchar())
    {
        if (hadcr)
        {
            if (c == '#')
                topicpos = helpfile.position() - 1;
            matching = level = 0;
        }

#if SIMULATOR
        if (matching && debugindex < sizeof(debug) - 1)
        {
            debug[debugindex++] = c;
            if (RECORDER_TRACE(help) > 2)
            {
                debug[debugindex] = 0;
                record(help, "Matching %2d: Scanning %s", matching, debug);
            }
        }
#endif // SIMULATOR

        if (((hadcr || matching == 1) && c == '#') ||
            (matching == 1 && c == ' '))
        {
            level += c == '#';
            matching = 1;
#if SIMULATOR
            debugindex = 0;
#endif // SIMULATOR
        }
        else if (matching < 0)
        {
            if (c == '(' || c == ',')
            {
                matching = -2;
                matching = 1;
            }
            else if (matching == -2 && c == ' ')
            {
                matching = 1;
            }

#if SIMULATOR
            if (matching == 1 || c =='\n' || c == ')')
            {
                if (RECORDER_TRACE(help) > 1)
                {
                    debug[debugindex-1] = 0;
                    if (debugindex > 1)
                        record(help, "Scanning topic %s", debug);
                }
                debugindex = 0;
            }
#endif // SIMULATOR
        }
        else if (matching)
        {
            if (uint(matching) == len + 1)
            {
                bool match = c == '\n' || c == ')' || c == ',' || c == ' ';
                record(help, "%+s topic len %u at position %u next [%c]",
                       match ? "Matched" : "Mismatched",
                       len, helpfile.position(), c);
                if (match)
                    break;
                matching = -1;
            }

            // Matching is case-independent, and matches markdown hyperlinks
            else if (byte(c) == topic[matching-1] ||
                tolower(c) == tolower(topic[matching-1]) ||
                (c == ' ' && topic[matching-1] == '-'))
            {
                matching++;
            }
            else if (c == '\n')
            {
#if SIMULATOR
                if (RECORDER_TRACE(help) > 1)
                {
                    debug[debugindex - 1] = 0;
                    if (debugindex > 1)
                        record(help, "Scanned topic %s", debug);
                    debugindex = 0;
                }
#endif // SIMULATOR
                matching = level = 0;
            }
            else
            {
#if SIMULATOR
                if (RECORDER_TRACE(help) > 2)
                    record(help, "Mismatch at %u: %u != %u",
                           matching, c, topic[matching-1]);
#endif // SIMULATOR
                matching = c == '(' ? -2 : -1;
            }
        }
        hadcr = c == '\n';
    }

    // Check if we found the topic
    if (uint(matching) == len + 1)
    {
        help = topicpos;
        line = 0;
        record(help, "Found topic %s at position %u level %u",
               topic, helpfile.position(), level);

        if (topics_history >= NUM_TOPICS)
        {
            // Overflow, keep the last topics
            for (uint i         = 1; i < NUM_TOPICS; i++)
                topics[i - 1]   = topics[i];
            topics[topics_history - 1] = help;
        }
        else
        {
            // New topic, store it
            topics[topics_history++] = help;
        }
    }
    else
    {
        static char buffer[50];
        snprintf(buffer, sizeof(buffer), "No help for %.*s", int(len), topic);
        rt.error(buffer);
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


bool user_interface::draw_help()
// ----------------------------------------------------------------------------
//    Draw the help content
// ----------------------------------------------------------------------------
{
    if (!force && !dirtyHelp && !dirtyStack)
        return false;
    dirtyHelp = false;

    if (!showing_help())
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
        { HelpBoldFont,     p::black,  p::white,   true, false, false, false },
        { HelpItalicFont,   p::black,  p::white,  false, true,  false, false },
        { HelpCodeFont,     p::black,  p::gray50, false, false, false, true  },
        { HelpFont,         p::white,  p::black,  false, false, false, false },
        { HelpFont,         p::black,  p::gray50, false, false, true,  false },
        { HelpFont,         p::white,  p::gray10, false, false, false, false },
    };


    // Compute the size for the help display
    coord      ytop   = HeaderFont->height() + 2;
    coord      ybot   = LCD_H - (MenuFont->height() + 2);
    coord      xleft  = 0;
    coord      xright = LCD_W - 1;
    style_name style  = NORMAL;


    // Clear help area and add some decorative elements
    rect clip = Screen.clip();
    rect r(xleft, ytop, xright, ybot);
    draw_dirty(r);
    Screen.fill(r, pattern::gray50);
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
    coord   height    = font->height();
    coord   x         = xleft;
    coord   y         = ytop + 2 - line * height;
    unicode last      = '\n';
    uint    lastTopic = 0;
    uint    shown     = 0;

    // Pun not intended
    helpfile.seek(help);

    // Display until end of help
    while (y < ybot)
    {
        unicode word[60];
        uint    widx       = 0;
        bool    emit       = false;
        bool    newline    = false;
        bool    yellow     = false;
        bool    blue       = false;
        style_name restyle = style;

        if (last  == '\n' && !shown && y >= ytop)
            shown  = helpfile.position();

        while (!emit)
        {
            unicode ch   = helpfile.get();
            bool    skip = false;

            switch (ch)
            {
            case 0:
                emit = true;
                skip = true;
                newline = true;
                break;

            case ' ':
                if (style <= SUBTITLE)
                {
                    skip = last == '#';
                    break;
                }
                skip = last == ' ';
                emit = style != KEY && style != CODE;
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

            case '!':
                // Skip images
                if (last == '\n')
                {
                    unicode c = helpfile.get();
                    while (c != '\n' && c != unicode(EOF))
                        c = helpfile.get();
                    skip = true;
                }
                break;

            case '*':
                if (last == '\n' && helpfile.peek() == ' ')
                {
                    restyle = NORMAL;
                    ch      = L'●'; // L'■'; // L'•';
                    xleft   = r.x1 + 2 + font->width(utf8("● "));
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
                    if (helpfile.peek() != '!')
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
                    else
                    {
                        // Link to a picture, skip it
                        unicode c = helpfile.get();
                        while (c != '\n' && c != unicode(EOF))
                            c = helpfile.get();
                        skip = true;
                    }
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
                            if (topics_history)
                                topics[topics_history-1] = shown;
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
            case L'🟨':
                emit = true;
                yellow = true;
                break;
            case L'🟦':
                emit = true;
                blue = true;
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
        size kwidth = 0;
        if (style == KEY)
        {
            kwidth = 2*font->width(' ');
            width += 2*kwidth;
        }

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
            if (underline)
            {
                xl -= 2;
                xr += 2;
                Screen.fill(xl, yf, xr, yf, bg);
                xl += 2;
                xr -= 2;
            }
            else if (box)
            {
                xl += 1;
                xr += 8;
                Screen.fill(xl, yf, xr, yf, bg);
                Screen.fill(xl, y, xl, yf, bg);
                Screen.fill(xr, y, xr, yf, bg);
                Screen.fill(xl, y, xr, y, bg);
                xl -= 1;
                xr -= 8;
                kwidth += 4;
            }
            else if (bg.bits != pattern::white.bits)
            {
                Screen.fill(xl, y, xr, yf, bg);
            }

            // Draw next word
            for (int i = 0; i < 1 + 3 * italic; i++)
            {
                x = xl + kwidth;
                if (italic)
                {
                    coord yt  = y + (3-i) * height / 4;
                    coord yb  = y + (4-i) * height / 4;
                    x        += i;
                    rect itr(x, yt, xr + i, yb);
                    itr &= r;
                    Screen.clip(itr);
                }
                coord x0 = x;
                for (int b = 0; b <= bold; b++)
                    x = draw_word(x0 + b, y, widx, word, font, color);
                x += kwidth;
            }
            if (italic)
                Screen.clip(r);
        }

        // Check special case of yellow shift key
        if (yellow || blue)
        {
            const byte *source     = blue ? ann_right : ann_left;
            pixword    *sw         = (pixword *) source;
            surface     s(sw, ann_width, ann_height, 16);

            rect shkey(x, y, x + ann_width + 7, y + height);
            Screen.fill(shkey, pattern::black);
            Screen.copy(s, x + 4, y + (height - ann_height)/2);
            yellow = blue = false;
            x += ann_width + 7 + font->width(' ');
        }

        // Select style for next round
        style = restyle;

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


bool user_interface::noHelpForKey(int key)
// ----------------------------------------------------------------------------
//   Return true if key requires immediate action, no help displayed
// ----------------------------------------------------------------------------
{
    bool editing  = rt.editing();

    // Show help for Duplicate and Drop only if not editing
    if (key == KEY_ENTER || key == KEY_BSP)
        return editing;

    // No help in alpha mode
    if (alpha && key < KEY_F1)
        return true;

    if (editing)
    {
        // No help for ENTER or BSP key while editing
        if (key == KEY_ENTER || key == KEY_BSP ||
            key == KEY_UP || key == KEY_DOWN || key == KEY_RUN)
            return true;

        // No help for A-F keys in hexadecimal entry mode
        if (mode == BASED && (key >= KB_A && key <= KB_F))
            return true;
    }

    // No help for digits entry
    if (!shift && !xshift)
        if (key > KEY_ENTER && key < KEY_ADD &&
            key != KEY_SUB && key != KEY_MUL && key != KEY_DIV && key != KEY_RUN)
            return true;

    // Other cases are regular functions, we can display help
    return false;
}



bool user_interface::handle_screen_capture(int key)
// ----------------------------------------------------------------------------
//   Check if we need to do a screen capture
// ----------------------------------------------------------------------------
{
    if (key >= KEY_SCREENSHOT)
    {
        if (key == KEY_SCREENSHOT)
        {
            shift = xshift = alpha = longpress = repeat = false;
            last = 0;
            draw_annunciators();
            refresh_dirty();
            if (!screenshot())
                rt.screenshot_capture_error();
        }
        if (key == KEY_DOUBLE_RELEASE)
            dbl_release = true; // Ignore next key
        return true;
    }
    if (!key && dbl_release)
    {
        dbl_release = false;
        return true;
    }
    return false;
}


bool user_interface::handle_help(int &key)
// ----------------------------------------------------------------------------
//   Handle help keys when showing help
// ----------------------------------------------------------------------------
{
    if (!showing_help())
    {
        // Exit if we are editing or entering digits
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
                    record(help, "Help topic is %s\n", htopic);
                    command = htopic;
                    dirtyCommand = true;
                    if (longpress)
                    {
                        rt.command(command::static_object(object::ID_Help));
                        load_help(htopic);
                        if (rt.error())
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
            key = 0;
        }
        else
        {
            if (!noHelpForKey(last))
                key = last;     // Time to evaluate
            last    = 0;
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
        dirtyHelp = true;
        break;

    case KEY_F3:
        count   = 8;
        // Fall through
    case KEY_DOWN:
    case KEY_2:
    case KEY_ADD:
        line   += count;
        repeat  = true;
        dirtyHelp = true;
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
        dirtyHelp = true;
        break;
    case KEY_F5:
    case KEY_3:
    case KEY_MUL:
        helpfile.seek(topic);
        while (count--)
            helpfile.find('[');
        topic  = helpfile.position();
        repeat = true;
        dirtyHelp = true;
        break;

    case KEY_ENTER:
        follow = true;
        dirtyHelp = true;
        break;

    case KEY_F6:
    case KEY_BSP:
        if (topics_history)
        {
            --topics_history;
            if (topics_history)
            {
                help = topics[topics_history-1];
                line = 0;
                dirtyHelp = true;
                break;
            }
        }
        // Otherwise fall-through and exit

    case KEY_EXIT:
        clear_help();
        dirtyHelp = true;
        break;
    }
    return true;
}


bool user_interface::handle_shifts(int &key, bool talpha)
// ----------------------------------------------------------------------------
//   Handle status changes in shift keys
// ----------------------------------------------------------------------------
{
    bool consumed = false;

    // Transient alpha management
    if (!transalpha)
    {
        // Not yet in trans alpha mode, check if we need to enable it
        if (talpha)
        {
            if (key == KEY_UP || key == KEY_DOWN)
            {
                // Let menu and normal keys go through
                if (xshift)
                    return false;

                // Delay processing of up or down until after delay
                if (longpress)
                {
                    repeat = true;
                    return false;
                }

                last = key;
                repeat = true;
                lowercase = key == KEY_DOWN;
                return true;
            }
            else if (key)
            {
                // A non-arrow key was pressed while arrows are down
                alpha = true;
                transalpha = true;
                last = 0;
                return false;
            }
            else
            {
                // Swallow the last key pressed (up or down)
                key = 0;
                last = 0;
                return true;
            }
        }
        else if (!key && (last == KEY_UP || last == KEY_DOWN))
        {
            if (!longpress)
                key = last;
            last = 0;
            return false;
        }
    }
    else
    {
        if (!talpha)
        {
            // We released the up/down key
            transalpha = false;
            alpha = false;
            lowercase = false;
            key = 0;
            last = 0;
            return true;
        }
        else if (key == KEY_UP || key == KEY_DOWN || key == 0)
        {
            // Ignore up/down or release in trans-alpha mode
            last = 0;
            return true;
        }

        // Other keys will be processed as alpha
    }


    if (key == KEY_SHIFT)
    {
        if (longpress)
        {
            alpha = !alpha;
            lowercase = false;
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
        menu_refresh(menu::ID_Catalog);
#undef SHM
#undef SHD
    }
    else if (shift && key == KEY_ENTER)
    {
        // Cycle ABC -> abc -> non alpha
        if (alpha)
        {
            if (lowercase)
                alpha = lowercase = false;
            else
                lowercase = true;
        }
        else
        {
            alpha     = true;
            lowercase = false;
        }
        consumed = true;
        shift = false;
        key = last = 0;
    }

    if (key)
        last = key;
    return consumed;
}


bool user_interface::handle_editing(int key)
// ----------------------------------------------------------------------------
//   Some keys always deal with editing
// ----------------------------------------------------------------------------
{
    bool   consumed = false;
    size_t editing  = rt.editing();

    // Some editing keys that do not depend on data entry mode
    if (!alpha)
    {
        switch(key)
        {
        case KEY_XEQ:
            // XEQ is used to enter algebraic / equation objects
            if ((!editing  || mode != BASED) && !shift && !xshift)
            {
                bool is_eqn = editing && is_algebraic(mode);
                edit(is_eqn ? '(' : '\'', ALGEBRAIC);
                last = 0;
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
            else if (xshift)
            {
                edit('{', PROGRAM);
                last = 0;
                return true;
            }
            else if (editing)
            {
                // Stick to space role while editing, do not EVAL, repeat
                if (mode == PARENTHESES)
                    edit(';', PARENTHESES);
                else if (mode == ALGEBRAIC)
                    edit('=', ALGEBRAIC);
                else
                    edit(' ', PROGRAM);
                repeat = true;
                return true;
            }
            break;

        case KEY_9:
            if (shift)
            {
                // Shift-9 enters a matrix
                edit('[', MATRIX);
                last = 0;
                return true;
            }
            break;
        }
    }

    if (editing)
    {
        record(user_interface, "Editing key %d", key);
        switch (key)
        {
        case KEY_BSP:
            if (xshift)
                return false;
            repeat = true;
            if (~searching)
            {
                utf8 ed = rt.editor();
                if (cursor > select)
                    cursor = utf8_previous(ed, cursor);
                else
                    select = utf8_previous(ed, select);
                if (cursor == select)
                    cursor = select = searching;
                else
                    do_search(0, true);
            }
            else
            {
                utf8 ed = rt.editor();
                if (shift && cursor < editing)
                {
                    // Shift + Backspace = Delete to right of cursor
                    uint after = utf8_next(ed, cursor, editing);
                    if (utf8_codepoint(ed + cursor) == '\n')
                        edRows = 0;
                    remove(cursor, after - cursor);
                }
                else if (!shift && cursor > 0)
                {
                    // Backspace = Erase on left of cursor
                    utf8 ed      = rt.editor();
                    uint before  = cursor;
                    cursor       = utf8_previous(ed, cursor);
                    if (utf8_codepoint(ed + cursor) == '\n')
                        edRows = 0;
                    remove(cursor, before - cursor);
                }
                else
                {
                    // Limits of line: beep
                    repeat = false;
                    beep(4400, 50);
                }

                dirtyEditor = true;
                adjustSeps = true;
                menu_refresh(object::ID_Catalog);
            }

            // Do not stop editing if we delete last character
            if (!rt.editing())
                edit(' ', DIRECT);
            last = 0;
            return true;
        case KEY_ENTER:
        {
            // Finish editing and parse the result
            if (!shift && !xshift)
            {
                if (~searching)
                {
                    searching = ~0U;
                    dirtyEditor = true;
                    edRows = 0;
                }
                else
                {
                    end_edit();
                }
                return true;
            }
        }
        case KEY_EXIT:
            // Clear error if there is one, else clear editor
            if (shift || xshift)
                return false;

            if (rt.error())
            {
                rt.clear_error();
                dirtyEditor = true;
                dirtyStack = true;
            }
            else
            {
                editor_save(false);
                clear_editor();
                if (this->editing)
                {
                    rt.push(this->editing);
                    this->editing = nullptr;
                    dirtyEditor = true;
                    dirtyStack = true;
                }
            }
            return true;

        case KEY_UP:
            repeat = true;
            if (shift)
            {
                up = true;
                dirtyEditor = true;
            }
            else if (xshift)
            {
                // Command-line history
                editor_history();
                return true;
            }
            else if (cursor > 0)
            {
                font_p edFont = Settings.editor_font(edRows > 2);
                utf8 ed = rt.editor();
                uint pcursor  = utf8_previous(ed, cursor);
                unicode cp = utf8_codepoint(ed + pcursor);
                if (cp != '\n')
                {
                    draw_cursor(-1, pcursor);
                    cursor = pcursor;
                    cx -= edFont->width(cp);
                    edColumn = cx;
                    draw_cursor(1, pcursor);
                    if (cx < 0)
                        dirtyEditor = true;
                }
                else
                {
                    cursor = pcursor;
                    edRows = 0;
                    dirtyEditor = true;
                }
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
                dirtyEditor = true;
            }
            else if (xshift)
            {
                return false;
            }
            else if (cursor < editing)
            {
                font_p edFont = Settings.editor_font(edRows > 2);
                utf8 ed = rt.editor();
                unicode cp = utf8_codepoint(ed + cursor);
                uint ncursor = utf8_next(ed, cursor, editing);
                if (cp != '\n')
                {
                    draw_cursor(-1, ncursor);
                    cursor = ncursor;
                    cx += edFont->width(cp);
                    edColumn = cx;
                    draw_cursor(1, ncursor);
                    if (cx >= LCD_W - edFont->width('M'))
                        dirtyEditor = true;
                }
                else
                {
                    cursor = ncursor;
                    edRows = 0;
                    dirtyEditor = true;
                }
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
            if (xshift)
            {
                // Insert quotes and begin editing
                edit('\"', TEXT);
                alpha = true;
                return true;
            }
            break;
        case KEY_EXIT:
            if (shift || xshift)
                return false;
            alpha = false;
            clear_menu();
            return true;
        case KEY_DOWN:
            // Key down to edit last object on stack
            if (!shift && !xshift && !alpha)
            {
                if (rt.depth())
                {
                    if (object_p obj = rt.pop())
                    {
                        this->editing = obj;
                        obj->edit();
                        dirtyEditor = true;
                        return true;
                    }
                }
            }
            break;
        case KEY_UP:
            if (xshift)
            {
                editor_history();
                return true;
            }
            break;
        }
    }

    return consumed;
}


bool user_interface::handle_alpha(int key)
// ----------------------------------------------------------------------------
//    Handle alphabetic user_interface
// ----------------------------------------------------------------------------
{
    // Things that we never handle in alpha mode
    if (!key || (key >= KEY_F1 && key <= KEY_F6) || key == KEY_EXIT)
        return false;

    // Allow "alpha" mode for keys A-F in based number mode
    // xshift-ENTER inserts quotes, xshift-BSP inserts \n
    bool editing = rt.editing();
    bool hex = editing && !alpha && mode == BASED && key >= KB_A && key <= KB_F;
    bool special = xshift && (key == KEY_ENTER || (key == KEY_BSP && editing));
    if (!alpha && !hex && !special)
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
        '_', L'⇄', L'±', L'∡', '_',
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
        '_',  '&',   '@', '$',  L'…',
        '_',  ';',  L'·', '{',  '!'
    };

    // Special case: + in alpha mode shows the catalog
    if (key == KEY_ADD && !shift && !xshift)
    {
        object_p cat = command::static_object(menu::ID_Catalog);
        cat->evaluate();
        return true;
    }

    key--;
    unicode c =
        hex       ? upper[key]    :
        xshift    ? xshifted[key] :
        shift     ? shifted[key]  :
        lowercase ? lower[key]    :
        upper[key];
    if (~searching)
    {
        if (!do_search(c))
            beep(2400, 100);
    }
    else
    {
        edit(c, DIRECT);
        if (c == '"')
            alpha = true;
        repeat = true;
    }
    menu_refresh(object::ID_Catalog);
    return true;
}


bool user_interface::handle_digits(int key)
// ----------------------------------------------------------------------------
//    Handle alphabetic user_interface
// ----------------------------------------------------------------------------
{
    if (alpha || shift || xshift || !key)
        return false;

    static const char numbers[] =
        "______"
        "______"
        "__-__"
        "_789_"
        "_456_"
        "_123_"
        "_0.__";

    if (rt.editing())
    {
        if (key == KEY_CHS)
        {
            // Special case for change of sign
            byte   *ed          = rt.editor();
            byte   *p           = ed + cursor;
            utf8    found       = nullptr;
            unicode c           = utf8_codepoint(p);
            unicode dm          = Settings.DecimalSeparator();
            unicode ns          = Settings.NumberSeparator();
            unicode hs          = Settings.BasedSeparator();
            bool    had_complex = false;
            while (p > ed && !found)
            {
                p = (byte *) utf8_previous(p);
                c = utf8_codepoint(p);
                if (c == complex::I_MARK || c == complex::ANGLE_MARK)
                {
                    had_complex = true;
                    if (c == complex::ANGLE_MARK)
                    {
                        found = utf8_next(p);
                    }
                    else
                    {
                        found = p;
                        p = (byte *) utf8_previous(p);
                        c = utf8_codepoint(p);
                    }
                }
                else if ((c < '0' || c > '9') && c != dm && c != ns && c != hs)
                {
                    found = utf8_next(p);
                }
            }

            if (!found)
                found = ed;
            if (c == 'e' || c == 'E' || c == Settings.ExponentSeparator())
                c  = utf8_codepoint(p);

            if (had_complex)
            {
                if (c == '+' || c == '-')
                    *p = '+' + '-' - c;
                else
                    insert(found - ed, '-');
            }
            else if (c == '-')
            {
                remove(p - ed, 1);
            }
            else
            {
                insert(found - ed, '-');
            }
            last = 0;
            dirtyEditor = true;
            return true;
        }
        else if (key == KEY_E && !~searching)
        {
            byte   buf[4];
            size_t sz = utf8_encode(Settings.ExponentSeparator(), buf);
            insert(cursor, buf, sz);
            last = 0;
            dirtyEditor = true;
            return true;
        }
    }
    if (key > KEY_CHS && key < KEY_F1)
    {
        unicode c = numbers[key - 1];
        if (~searching)
        {
            bool found = false;
            switch (key)
            {
            case KEY_ADD:       found = do_search('+'); break;
            case KEY_SUB:       found = do_search('-'); break;
            case KEY_MUL:       found = do_search('*')||do_search(L'×')
                                                      ||do_search(L'·'); break;
            case KEY_DIV:       found = do_search('/')||do_search(L'÷'); break;
            case KEY_DOT:       found = do_search('.')||do_search(L','); break;
            case KEY_E:         found = do_search('E')||do_search(L'⁳'); break;
            default:
                if (c == '_')
                    return false;
                found = do_search(c);
                break;
            }
            if (!found)
                beep(2400, 100);
            return true;
        }
        if (c == '_')
            return false;
        if (c == '.')
        {
            // Check if we enter a DMS value
            byte   *ed    = rt.editor();
            byte   *p     = ed + cursor;
            utf8    found = nullptr;
            unicode dm    = Settings.DecimalSeparator();
            unicode ns    = Settings.NumberSeparator();
            unicode hs    = Settings.BasedSeparator();

            c = char(dm);
            while (p > ed && !found)
            {
                p = (byte *) utf8_previous(p);
                unicode cp = utf8_codepoint(p);
                if (cp == L'″')
                {
                    found = p;
                    c = '/';
                }
                else if (cp == L'′')
                {
                    found = p;
                    c = L'″';
                }
                else if (cp == L'°')
                {
                    found = p;
                    if (uint(found - ed) == cursor - utf8_size(cp))
                    {
                        remove(found - ed, utf8_size(cp));
                        c = dm;

                        size_t edlen = rt.editing();
                        ed = rt.editor();
                        if (cursor + 4 <= edlen &&
                            memcmp(ed + cursor, "_dms", 4) == 0)
                            remove(cursor, 4);
                    }
                    else
                    {
                        c = L'′';
                    }
                }
                else if (cp == dm)
                {
                    found = p;
                    if (uint(found - ed) == cursor - utf8_size(cp))
                    {
                        remove (found - ed, utf8_size(cp));
                        c = L'°';
                    }
                    else
                    {
                        remove(found - ed, utf8_size(cp));
                        insert(found - ed, unicode(L'°'));
                        c = L'′';
                    }
                    size_t edlen = rt.editing();
                    ed = rt.editor();
                    if (cursor + 4 > edlen ||
                        memcmp(ed + cursor, "_dms", 4) != 0)
                    {
                        size_t add = insert(cursor, utf8("_dms"), 4);
                        cursor -= add;
                    }
                }
                else if ((cp < '0' || cp > '9') && cp != ns && cp != hs)
                {
                    break;
                }
            }
        }
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

static const byte defaultUnshiftedCommand[2*user_interface::NUM_KEYS] =
// ----------------------------------------------------------------------------
//   RPL code for the commands assigned by default to each key
// ----------------------------------------------------------------------------
//   All the default-assigned commands fit in one or two bytes
{
#define OP2BYTES(key, id)                                              \
    [2*(key) - 2] = (id) < 0x80 ? (id) & 0x7F : ((id) & 0x7F) | 0x80,  \
    [2*(key) - 1] = (id) < 0x80 ?           0 : ((id) >> 7)

    OP2BYTES(KEY_SIGMA, menu::ID_ToolsMenu),
    OP2BYTES(KEY_INV,   function::ID_inv),
    OP2BYTES(KEY_SQRT,  function::ID_sqrt),
    OP2BYTES(KEY_LOG,   function::ID_exp),
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
    OP2BYTES(KEY_E,     function::ID_Cycle),
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

    OP2BYTES(KEY_SCREENSHOT, command::ID_ScreenCapture),
    OP2BYTES(KEY_SH_UP,  0),
    OP2BYTES(KEY_SH_DOWN, 0),
};


static const byte defaultShiftedCommand[2*user_interface::NUM_KEYS] =
// ----------------------------------------------------------------------------
//   RPL code for the commands assigned by default to shifted keys
// ----------------------------------------------------------------------------
//   All the default assigned commands fit in one or two bytes
{
    OP2BYTES(KEY_SIGMA, menu::ID_LastMenu),
    OP2BYTES(KEY_INV,   arithmetic::ID_pow),
    OP2BYTES(KEY_SQRT,  arithmetic::ID_sq),
    OP2BYTES(KEY_LOG,   function::ID_exp10),
    OP2BYTES(KEY_LN,    function::ID_log10),
    OP2BYTES(KEY_XEQ,   menu::ID_LoopsMenu),
    OP2BYTES(KEY_STO,   menu::ID_ComplexMenu),
    OP2BYTES(KEY_RCL,   menu::ID_FractionsMenu),
    OP2BYTES(KEY_RDN,   menu::ID_ConstantsMenu),
    OP2BYTES(KEY_SIN,   function::ID_asin),
    OP2BYTES(KEY_COS,   function::ID_acos),
    OP2BYTES(KEY_TAN,   function::ID_atan),
    OP2BYTES(KEY_ENTER, 0),     // Alpha
    OP2BYTES(KEY_SWAP,  menu::ID_LastArg),
    OP2BYTES(KEY_CHS,   menu::ID_ModesMenu),
    OP2BYTES(KEY_E,     menu::ID_DisplayModesMenu),
    OP2BYTES(KEY_BSP,   menu::ID_ClearThingsMenu),
    OP2BYTES(KEY_UP,    0),
    OP2BYTES(KEY_7,     menu::ID_SolverMenu),
    OP2BYTES(KEY_8,     menu::ID_IntegrationMenu),
    OP2BYTES(KEY_9,     0),     // Insert []
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
    OP2BYTES(KEY_SUB,   menu::ID_PrintingMenu),
    OP2BYTES(KEY_EXIT,  command::ID_Off),
    OP2BYTES(KEY_0,     command::ID_SystemSetup),
    OP2BYTES(KEY_DOT,   0),
    OP2BYTES(KEY_RUN,   0),
    OP2BYTES(KEY_ADD,   menu::ID_Catalog),

    OP2BYTES(KEY_F1,    0),
    OP2BYTES(KEY_F2,    0),
    OP2BYTES(KEY_F3,    0),
    OP2BYTES(KEY_F4,    0),
    OP2BYTES(KEY_F5,    0),
    OP2BYTES(KEY_F6,    0),

    OP2BYTES(KEY_SCREENSHOT, command::ID_ScreenCapture),
    OP2BYTES(KEY_SH_UP, 0),
    OP2BYTES(KEY_SH_DOWN, 0),
};


static const byte defaultSecondShiftedCommand[2*user_interface::NUM_KEYS] =
// ----------------------------------------------------------------------------
//   RPL code for the commands assigned by default to long-shifted keys
// ----------------------------------------------------------------------------
//   All the default assigned commands fit in one or two bytes
{
    OP2BYTES(KEY_SIGMA, menu::ID_MainMenu),
    OP2BYTES(KEY_INV,   command::ID_xroot),
    OP2BYTES(KEY_SQRT,  menu::ID_PolynomialsMenu),
    OP2BYTES(KEY_LOG,   menu::ID_ExpLogMenu),
    OP2BYTES(KEY_LN,    menu::ID_PartsMenu),
    OP2BYTES(KEY_XEQ,   menu::ID_EquationsMenu),
    OP2BYTES(KEY_STO,   menu::ID_MemMenu),
    OP2BYTES(KEY_RCL,   menu::ID_LibsMenu),
    OP2BYTES(KEY_RDN,   menu::ID_MathMenu),
    OP2BYTES(KEY_SIN,   menu::ID_HyperbolicMenu),
    OP2BYTES(KEY_COS,   menu::ID_CircularMenu),
    OP2BYTES(KEY_TAN,   menu::ID_RealMenu),
    OP2BYTES(KEY_ENTER, 0),     // Text
    OP2BYTES(KEY_SWAP,  function::ID_Undo),
    OP2BYTES(KEY_CHS,   menu::ID_ObjectMenu),
    OP2BYTES(KEY_E,     menu::ID_PlotMenu),
    OP2BYTES(KEY_BSP,   function::ID_updir),
    OP2BYTES(KEY_UP,    0),
    OP2BYTES(KEY_7,     menu::ID_SymbolicMenu),
    OP2BYTES(KEY_8,     menu::ID_DifferentiationMenu),
    OP2BYTES(KEY_9,     menu::ID_MatrixMenu),
    OP2BYTES(KEY_DIV,   menu::ID_FinanceSolverMenu),
    OP2BYTES(KEY_DOWN,  menu::ID_EditMenu),
    OP2BYTES(KEY_4,     menu::ID_TextMenu),
    OP2BYTES(KEY_5,     menu::ID_UnitsConversionsMenu),
    OP2BYTES(KEY_6,     menu::ID_TimeMenu),
    OP2BYTES(KEY_MUL,   menu::ID_NumbersMenu),
    OP2BYTES(KEY_SHIFT, 0),
    OP2BYTES(KEY_1,     menu::ID_DebugMenu),
    OP2BYTES(KEY_2,     menu::ID_CharsMenu),
    OP2BYTES(KEY_3,     menu::ID_TestsMenu),
    OP2BYTES(KEY_SUB,   menu::ID_IOMenu),
    OP2BYTES(KEY_EXIT,  command::ID_SaveState),
    OP2BYTES(KEY_0,     menu::ID_FilesMenu),
    OP2BYTES(KEY_DOT,   menu::ID_GraphicsMenu),
    OP2BYTES(KEY_RUN,   0),
    OP2BYTES(KEY_ADD,   command::ID_Help),

    OP2BYTES(KEY_F1,    0),
    OP2BYTES(KEY_F2,    0),
    OP2BYTES(KEY_F3,    0),
    OP2BYTES(KEY_F4,    0),
    OP2BYTES(KEY_F5,    0),
    OP2BYTES(KEY_F6,    0),

    OP2BYTES(KEY_SCREENSHOT, command::ID_ScreenCapture),
    OP2BYTES(KEY_SH_UP, 0),
    OP2BYTES(KEY_SH_DOWN, 0),
};


static const byte *const defaultCommand[user_interface::NUM_PLANES] =
// ----------------------------------------------------------------------------
//   Pointers to the default commands
// ----------------------------------------------------------------------------
{
    defaultUnshiftedCommand,
    defaultShiftedCommand,
    defaultSecondShiftedCommand,
};


object_p user_interface::object_for_key(int key)
// ----------------------------------------------------------------------------
//    Return the object for a given key
// ----------------------------------------------------------------------------
{
    uint plane = shift_plane();
    if (key >= KEY_F1 && key <= KEY_F6 && plane >= menu_planes())
        plane = 0;

    object_p obj = function[plane][key - 1];
    if (!obj)
    {
        const byte *ptr = defaultCommand[plane] + 2 * (key - 1);
        if (*ptr)
            obj = (object_p) ptr;
    }
    return obj;
}


bool user_interface::handle_functions(int key)
// ----------------------------------------------------------------------------
//   Check if we have one of the soft menu functions
// ----------------------------------------------------------------------------
{
    if (!key)
        return false;

    record(user_interface,
           "Handle function for key %d (plane %d) ", key, shift_plane());
    if (object_p obj = object_for_key(key))
    {
        evaluating = key;
        object::id ty = obj->type();
        bool imm = object::is_immediate(ty);
        bool editing = rt.editing();
        if (editing && !imm)
        {
            if (key == KEY_ENTER || key == KEY_BSP)
                return false;

            if (autoComplete && key >= KEY_F1 && key <= KEY_F6)
            {
                size_t start = 0;
                size_t size  = 0;
                if (current_word(start, size))
                    remove(start, size);
            }

            switch (mode)
            {
            case PROGRAM:
                if (obj->is_command())
                {
                    dirtyEditor = true;
                    edRows = 0;
                    return obj->insert(*this) != object::ERROR;
                }
                break;

            case ALGEBRAIC:
            case PARENTHESES:
                if (obj->type() == object::ID_Sto)
                {
                    if (!end_edit())
                        return false;
                }
                else if (obj->is_algebraic() || obj->is_command())
                {
                    dirtyEditor = true;
                    edRows = 0;
                    return obj->insert(*this) != object::ERROR;
                }
                break;

            default:
                // If we have the editor open, need to close it
                if (ty != object::ID_SelfInsert)
                {
                    if (!end_edit())
                        return false;
                }
                break;
            }

        }
        draw_busy();
        if (!imm && !rt.editing())
        {
            if (Settings.SaveStack())
                rt.save();
            if (Settings.SaveLastArguments())
                rt.need_save();
        }
        obj->evaluate();
        draw_idle();
        dirtyStack = true;
        if (!imm)
            alpha = false;
        xshift = false;
        shift = false;
        return true;
    }

    return false;
}


bool user_interface::current_word(size_t &start, size_t &size)
// ----------------------------------------------------------------------------
//   REturn position of word under the cursor if there is one
// ----------------------------------------------------------------------------
{
    utf8 sed = nullptr;
    bool result = current_word(sed, size);
    if (result)
        start = sed - rt.editor();
    return result;
}


bool user_interface::current_word(utf8 &start, size_t &size)
// ----------------------------------------------------------------------------
//   Find the word under the cursor in the editor, if there is one
// ----------------------------------------------------------------------------
{
    if (size_t sz = rt.editing())
    {
        byte *ed = rt.editor();
        uint  c  = cursor;
        c = utf8_previous(ed, c);
        while (c > 0 && !is_separator_or_digit(ed + c))
            c = utf8_previous(ed, c);
        if (is_separator_or_digit(ed + c))
            c = utf8_next(ed, c, sz);
        uint spos = c;
        while (c < sz && !is_separator(ed + c))
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



// ============================================================================
//
//   Editor menu commands
//
// ============================================================================

bool user_interface::editor_select()
// ----------------------------------------------------------------------------
//   Set selection to current cursor position
// ----------------------------------------------------------------------------
{
    if (select == cursor)
        select = ~0U;
    else
        select = cursor;
    dirtyEditor = true;
    return true;
}


bool user_interface::editor_word_left()
// ----------------------------------------------------------------------------
//   Move cursor one word to the left
// ----------------------------------------------------------------------------
{
    if (rt.editing())
    {
        utf8 ed = rt.editor();

        // Skip whitespace
        while (cursor > 0)
        {
            unicode code = utf8_codepoint(ed + cursor);
            if (!isspace(code))
                break;
            cursor = utf8_previous(ed, cursor);
        }

        // Skip word
        while (cursor > 0)
        {
            unicode code = utf8_codepoint(ed + cursor);
            if (isspace(code))
                break;
            cursor = utf8_previous(ed, cursor);
        }

        edRows = 0;
        dirtyEditor = true;
    }
    return true;
}


bool user_interface::editor_word_right()
// ----------------------------------------------------------------------------
//   Move cursor one word to the right
// ----------------------------------------------------------------------------
{
    if (size_t editing = rt.editing())
    {
        utf8 ed = rt.editor();

        // Skip whitespace
        while (cursor < editing)
        {
            unicode code = utf8_codepoint(ed + cursor);
            if (!isspace(code))
                break;
            cursor = utf8_next(ed, cursor, editing);
        }

        // Skip word
        while (cursor < editing)
        {
            unicode code = utf8_codepoint(ed + cursor);
            if (isspace(code))
                break;
            cursor = utf8_next(ed, cursor, editing);
        }

        edRows = 0;
        dirtyEditor = true;
    }
    return true;
}

bool user_interface::editor_begin()
// ----------------------------------------------------------------------------
//   Move cursor to beginning of buffer
// ----------------------------------------------------------------------------
{
    cursor = 0;
    edRows = 0;
    dirtyEditor = true;
    return true;
}

bool user_interface::editor_end()
// ----------------------------------------------------------------------------
//   Move cursor one word to the right
// ----------------------------------------------------------------------------
{
    cursor = rt.editing();
    edRows = 0;
    dirtyEditor = true;
    return true;
}


bool user_interface::editor_cut()
// ----------------------------------------------------------------------------
//   Cut to clipboard (most recent history)
// ----------------------------------------------------------------------------
{
    editor_copy();
    editor_clear();
    return true;
}


bool user_interface::editor_copy()
// ----------------------------------------------------------------------------
//   Copy to clipboard
// ----------------------------------------------------------------------------
{
    if (~select && select != cursor)
    {
        uint start = cursor;
        uint end = select;
        if (start > end)
            std::swap(start, end);
        utf8 ed = rt.editor();
        clipboard = text::make(ed + start, end - start);
    }
    return true;
}


bool user_interface::editor_paste()
// ----------------------------------------------------------------------------
//   Paste from clipboard
// ----------------------------------------------------------------------------
{
    if (clipboard)
    {
        size_t len = 0;
        utf8 ed = clipboard->value(&len);
        insert(cursor, ed, len);
        edRows = 0;
        dirtyEditor = true;
    }
    return true;
}


bool user_interface::do_search(unicode with, bool restart)
// ----------------------------------------------------------------------------
//   Perform the actual search
// ----------------------------------------------------------------------------
{
    // Already search, find next position
    size_t max = rt.editing();
    utf8   ed  = rt.editor();
    if (!max || !ed)
        return false;
    if (!~select)
        select = cursor;

    bool   forward  = cursor >= select;
    size_t selected = forward ? cursor - select : select - cursor;
    if (selected > max)
    {
        selected = 0;
        select = cursor;
    }
    size_t found  = ~0;
    uint   ref    = forward ? select : cursor;
    uint   start  = restart ? searching : ref;
    uint   search = start;

    // Skip current location (search next) or not (incremental search)
    bool   skip   = with == 0;

    // Loop until we either find a new spot or we wrap around
    for (uint count = 0; !~found && count < max; count++)
    {
        if (skip)
        {
            // Move search forward or backward respecting unicode boundaries
            if (forward)
            {
                search = utf8_next(ed, search, max);
                if (search == max)
                    search = 0;
            }
            else
            {
                search = utf8_previous(ed, search);
                if (search == 0)
                    search = utf8_previous(ed, max);
            }
        }
        else
        {
            skip = true;
        }

        // Check if there is a match at the current location
        bool check  = true;
        uint last = search + selected;

        // Never match if past end of buffer
        if (last + (with != 0) > max)
            continue;

        // Otherwise, loop inside buffer
        for (uint s = search; check && s < last; s = utf8_next(ed, s, max))
        {
            unicode sc = utf8_codepoint(ed + s);
            unicode rc = utf8_codepoint(ed + ref + s - search);
            check = towlower(sc) == towlower(rc);
        }

        if (check && with)
        {
            unicode sc = utf8_codepoint(ed + last);
            check = towlower(sc) == towlower(with);
        }
        if (check)
        {
            found = search;
            break;
        }
    }

    if (~found)
    {
        if (with)
            selected += utf8_size(with);

        if (forward)
        {
            select = found;
            cursor = select + selected;
        }
        else
        {
            cursor = found;
            select = cursor + selected;
        }
        edRows = 0;
        dirtyEditor = true;
        return true;
    }
    return false;
}


bool user_interface::editor_search()
// ----------------------------------------------------------------------------
//   Start or repeat a search a search
// ----------------------------------------------------------------------------
{
    if (~select && cursor != select)
    {
        // Keep searching
        if (~searching == 0)
            searching = cursor > select ? select : cursor;
        if (!do_search())
            beep(2500, 100);
        edRows = 0;
        dirtyEditor = true;
    }
    else
    {
        // Start search
        searching = select = cursor;
        alpha = true;
        lowercase = false;
        shift = xshift = false;
    }
    return true;
}


bool user_interface::editor_replace()
// ----------------------------------------------------------------------------
//   Perform a search replacement
// ----------------------------------------------------------------------------
{
    bool result = true;
    if (~searching && ~select && cursor != select && clipboard)
    {
        uint start = cursor;
        uint end = select;
        if (start > end)
            std::swap(start, end);
        result = do_search();
        remove(start, end - start);

        size_t len = 0;
        utf8 ed = clipboard->value(&len);
        insert(start, ed, len);

        if (!result)
            select = ~0U;

        edRows = 0;
        dirtyEditor = true;
    }
    return result;
}


bool user_interface::editor_clear()
// ----------------------------------------------------------------------------
//   Paste from clipboard
// ----------------------------------------------------------------------------
{
    if (~select && select != cursor)
    {
        uint start = cursor;
        uint end = select;
        if (start > end)
            std::swap(start, end);
        remove(start, end - start);
        select = ~0U;
        edRows = 0;
        dirtyEditor = true;
    }
    return true;
}


bool user_interface::editor_selection_flip()
// ----------------------------------------------------------------------------
//   Flip cursor and selection point
// ----------------------------------------------------------------------------
{
    if (~select)
        std::swap(select, cursor);
    edRows = 0;
    dirtyEditor = true;
    return true;
}


size_t user_interface::insert(size_t offset, utf8 data, size_t len)
// ----------------------------------------------------------------------------
//   Insert data in the editor
// ----------------------------------------------------------------------------
{
    size_t d = rt.insert(offset, data, len);
    if (~select && select >= offset)
        select += d;
    if (cursor >= offset)
        cursor += d;
    return d;
}


size_t user_interface::insert(size_t offset, unicode c)
// ----------------------------------------------------------------------------
//   Insert a Unicode glyph in the editor
// ----------------------------------------------------------------------------
{
    byte buffer[4];
    size_t sz = utf8_encode(c, buffer);
    return insert(offset, buffer, sz);
}


object::result user_interface::insert_softkey(int     key,
                                              cstring before,
                                              cstring after,
                                              char    term)
// ----------------------------------------------------------------------------
//   Insert the name associated with the key if editing
// ----------------------------------------------------------------------------
{
    if (cstring text = label_text(key - KEY_F1))
    {
        if (*text)
        {
            size_t length = 0;
            uint   cursor = cursor_position();
            if (symbol_p name = label(key - KEY_F1))
            {
                text = (cstring) name->value(&length);
            }
            else if (term)
            {
                cstring found = strchr(text, term);
                length = found ? found - text : strlen(text);
            }
            else
            {
                length = strlen(text);
            }

            insert(cursor, utf8(before), strlen(before));
            insert(cursor, utf8(text), length);
            insert(cursor, utf8(after), strlen(after));

            cursor_position(cursor);

            return object::OK;
        }
    }

    return object::ERROR;
}


size_t user_interface::remove(size_t offset, size_t len)
// ----------------------------------------------------------------------------
//   Remove data from the editor
// ----------------------------------------------------------------------------
{
    len = rt.remove(offset, len);
    if (~select && select >= offset)
    {
        if (select >= offset + len)
            select -= len;
        else
            select = offset;
    }
    if (cursor >= offset)
    {
        if (cursor >= offset + len)
            cursor -= len;
        else
            cursor = offset;
    }
    return len;
}


// ============================================================================
//
//   Debugging tool (printing on screen)
//
// ============================================================================

#include "font.h"
#include <cstdarg>

void debug_printf(int row, cstring format, ...)
// ----------------------------------------------------------------------------
//   Debug printf on the given row
// ----------------------------------------------------------------------------
{
    if (HelpFont)
    {
        char buffer[256];
        va_list va;
        va_start(va, format);
        vsnprintf(buffer, sizeof(buffer), format, va);
        va_end(va);
        size  h = HelpFont->height();
        coord y = row * h;
        Screen.text(0, y, utf8(buffer), HelpFont, pattern::white, pattern::black);
        ui.draw_dirty(0, y, LCD_W, y + h - 1);
    }
}


void debug_wait(int delay)
// ----------------------------------------------------------------------------
//   Wait for the given delay, or until key is pressed
// ----------------------------------------------------------------------------
{
    refresh_dirty();
    if (delay > 0)
        sys_delay(delay);
    else if (delay < 0)
        wait_for_key_press();
}
