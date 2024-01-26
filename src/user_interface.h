#ifndef USER_INTERFACE_H
#define USER_INTERFACE_H
// ****************************************************************************
//  user_interface.h                                             DB48X project
// ****************************************************************************
//
//   File Description:
//
//    Calculator user interface
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

#include "blitter.h"
#include "dmcp.h"
#include "file.h"
#include "object.h"
#include "runtime.h"
#include "text.h"
#include "types.h"

#include <string>
#include <vector>

GCP(menu);

struct user_interface
// ----------------------------------------------------------------------------
//    Calculator user_interface state
// ----------------------------------------------------------------------------
{
    user_interface();

    enum modes
    // ------------------------------------------------------------------------
    //   Current user_interface mode
    // ------------------------------------------------------------------------
    {
        STACK,                  // Showing the stack, not editing
        DIRECT,                 // Keys like 'sin' evaluate directly
        TEXT,                   // Alphanumeric entry, e.g. in strings
        PROGRAM,                // Keys like 'sin' show as 'sin' in the editor
        ALGEBRAIC,              // Keys like 'sin' show as 'sin()' in eqs
        PARENTHESES,            // Space inserts a semi-colon in eqs
        POSTFIX,                // Keys like '!' or 'x²' are postfix in eqs
        INFIX,                  // Keys like '+' are treated as infix in eqs
        CONSTANT,               // Entities like ⅈ or π have no parentheses
        MATRIX,                 // Matrix/vector mode
        BASED,                  // Based number: A-F map switch to alpha
    };

    enum
    // ------------------------------------------------------------------------
    //   Dimensioning constants
    // ------------------------------------------------------------------------
    {
        HISTORY         = 8,    // Number of menus and commands kept in history
        NUM_PLANES      = 3,    // NONE, Shift and "extended" shift
        NUM_KEYS        = 46,   // Including SCREENSHOT, SH_UP and SH_DN
        NUM_SOFTKEYS    = 6,    // Number of softkeys
        NUM_MENUS = NUM_PLANES * NUM_SOFTKEYS,
    };

    using result = object::result;
    using id     = object::id;

    typedef blitter::coord coord;
    typedef blitter::size  size;
    typedef blitter::rect  rect;


    bool        key(int key, bool repeating, bool transalpha);
    bool        repeating()     { return repeat; }
    void        assign(int key, uint plane, object_p code);
    object_p    assigned(int key, uint plane);

    void        update_mode();

    void        menu(menu_p menu, uint page = 0);
    menu_p      menu();
    void        menu_pop();
    uint        page();
    void        page(uint p);
    uint        pages();
    void        pages(uint p);
    uint        menu_planes();
    void        menus(uint count, cstring labels[], object_p function[]);
    void        menu(uint index, cstring label, object_p function);
    void        menu(uint index, symbol_p label, object_p function);
    void        marker(uint index, unicode mark, bool alignLeft = false);
    bool        menu_refresh();
    bool        menu_refresh(object::id menu);
    void        menu_auto_complete()    { autoComplete = true; }
    symbol_p    label(uint index);
    cstring     label_text(uint index);

    void        draw_start(bool force, uint refresh = ~0U);
    void        draw_refresh(uint delay);
    void        draw_dirty(const rect &r);
    void        draw_dirty(coord x1, coord y1, coord x2, coord y2);
    uint        draw_refresh()          { return nextRefresh; }
    rect        draw_dirty()            { return dirty; }
    void        draw_clean()            { dirty = rect(); }
    bool        draw_graphics();

    bool        draw_header();
    bool        draw_annunciators();
    bool        draw_editor();
    bool        draw_stack();
    bool        draw_error();
    bool        draw_message(utf8 header, uint count, utf8 msg[]);
    bool        draw_message(cstring header, cstring = 0, cstring = 0);
    bool        draw_help();
    bool        draw_command();
    void        draw_user_command(utf8 cmd, size_t sz);
    bool        draw_stepping_object();

    bool        draw_menus();
    bool        draw_battery();
    bool        draw_cursor(int show, uint ncursor);
    bool        draw_busy(unicode glyph = L'▶');
    bool        draw_idle();

    modes       editing_mode()          { return mode; }
    int         stack_screen_bottom()   { return stack; }
    int         menu_screen_bottom()    { return menuHeight; }
    bool        showing_help()          { return help + 1 != 0; }
    uint        cursor_position()       { return cursor; }
    void        cursor_position(uint p) { cursor = p; dirtyEditor = true; edRows = 0; }
    bool        current_word(size_t &start, size_t &size);
    bool        current_word(utf8 &start, size_t &size);

    uint        shift_plane()   { return xshift ? 2 : shift ? 1 : 0; }
    void        clear_help();
    void        clear_menu();
    object_p    object_for_key(int key);
    void        edit(unicode c, modes m, bool autoclose = true);
    result      edit(utf8 s, size_t len, modes m, int off = 0);
    result      edit(utf8 s, modes m, int off = 0);
    bool        end_edit();
    void        clear_editor();
    text_p      editor_save(text_r ed, bool rewinding = false);
    text_p      editor_save(bool rewinding = false);
    void        editor_history();
    bool        editor_select();
    bool        editor_word_left();
    bool        editor_word_right();
    bool        editor_begin();
    bool        editor_end();
    bool        editor_cut();
    bool        editor_copy();
    bool        editor_paste();
    bool        editor_search();
    bool        editor_replace();
    bool        editor_clear();
    bool        editor_selection_flip();
    size_t      insert(size_t offset, utf8 data, size_t len);
    size_t      insert(size_t offset, unicode c);
    size_t      insert(size_t offset, byte c) { return insert(offset, &c, 1); }
    size_t      insert(size_t offset, char c) { return insert(offset, byte(c)); }
    size_t      remove(size_t offset, size_t len);
    result      insert_softkey(int key,
                               cstring before, cstring after,
                               char term = 0);
    void        load_help(utf8 topic, size_t len = 0);

protected:
    bool        handle_screen_capture(int key);
    bool        handle_shifts(int &key, bool talpha);
    bool        handle_help(int &key);
    bool        handle_editing(int key);
    bool        handle_alpha(int key);
    bool        handle_functions(int key);
    bool        handle_digits(int key);
    bool        noHelpForKey(int key);
    bool        do_search(unicode with = 0, bool restart = false);

public:
    int      evaluating;        // Key being evaluated

protected:
    utf8     command;           // Command being executed
    uint     help;              // Offset of help being displayed in help file
    uint     line;              // Line offset in the help display
    uint     topic;             // Offset of topic being highlighted
    uint     topics_history;    // History depth
    uint     topics[8];         // Topics history
    uint     cursor;            // Cursor position in buffer
    uint     select;            // Cursor position for selection marker
    uint     searching;         // Searching start point
    coord    xoffset;           // Offset of the cursor
    modes    mode;              // Current editing mode
    int      last;              // Last key
    int      stack;             // Vertical bottom of the stack
    coord    cx, cy;            // Cursor position on screen
    uint     edRows;            // Editor rows
    int      edRow;             // Current editor row
    int      edColumn;          // Current editor column (in pixels)
    id       menuStack[HISTORY];// Current and past menus
    uint     pageStack[HISTORY];// Current and past menus pages
    uint     menuPage;          // Current menu page
    uint     menuPages;         // Number of menu pages
    uint     menuHeight;        // Height of the menu
    uint     busy;              // Busy counter
    uint     nextRefresh;       // Time for next refresh
    rect     dirty;             // Dirty rectangles
    object_g editing;           // Object being edited if any
    uint     cmdIndex;          // Command index for next command to save
    uint     cmdHistoryIndex;   // Command index for next command history
    text_g   history[HISTORY];  // Command-line history
    text_g   clipboard;         // Clipboard for copy/paste operations
    bool     shift        : 1;  // Normal shift active
    bool     xshift       : 1;  // Extended shift active (simulate Right)
    bool     alpha        : 1;  // Alpha mode active
    bool     transalpha   : 1;  // Transitory alpha (up or down key)
    bool     lowercase    : 1;  // Lowercase
    bool     shift_drawn  : 1;  // Cache of drawn annunciators
    bool     xshift_drawn : 1;  // Cache
    bool     alpha_drawn  : 1;  // Cache
    bool     lowerc_drawn : 1;  // Cache
    bool     down         : 1;  // Move one line down
    bool     up           : 1;  // Move one line up
    bool     repeat       : 1;  // Repeat the key
    bool     longpress    : 1;  // We had a long press of the key
    bool     blink        : 1;  // Cursor blink indicator
    bool     follow       : 1;  // Follow a help topic
    bool     force        : 1;  // Force a redraw of everything
    bool     dirtyMenu    : 1;  // Menu label needs redraw
    bool     dirtyStack   : 1;  // Need to redraw the stack
    bool     dirtyCommand : 1;  // Need to redraw the command
    bool     dirtyEditor  : 1;  // Need to redraw the text editor
    bool     dirtyHelp    : 1;  // Need to redraw the help
    bool     autoComplete : 1;  // Menu is auto-complete
    bool     adjustSeps   : 1;  // Need to adjust separators
    bool     graphics     : 1;  // Displaying user-defined graphics screen
    bool     dbl_release  : 1;  // Double release

protected:
    // Key mappings
    object_p function[NUM_PLANES][NUM_KEYS];
    cstring  menu_label[NUM_PLANES][NUM_SOFTKEYS];
    uint16_t menu_marker[NUM_PLANES][NUM_SOFTKEYS];
    bool     menu_marker_align[NUM_PLANES][NUM_SOFTKEYS];
    file     helpfile;
    friend struct tests;
    friend struct runtime;
};


enum { TIMER0, TIMER1, TIMER2, TIMER3 };

extern user_interface ui;

#endif // INPUT_H
