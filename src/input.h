#ifndef INPUT_H
#define INPUT_H
// ****************************************************************************
//  input.h                                                       DB48X project
// ****************************************************************************
//
//   File Description:
//
//    Calculator input
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

#include "graphics.h"
#include "object.h"
#include "types.h"
#include "dmcp.h"

#include <string>
#include <vector>

struct runtime;
struct menu;
typedef const menu *menu_p;

struct input
// ----------------------------------------------------------------------------
//    Calculator input state
// ----------------------------------------------------------------------------
{
    input();

    enum modes
    // ------------------------------------------------------------------------
    //   Current input mode
    // ------------------------------------------------------------------------
    {
        STACK,                  // Showing the stack, not editing
        DIRECT,                 // Keys like 'sin' evaluate directly
        TEXT,                   // Alphanumeric entry, e.g. in strings
        PROGRAM,                // Keys like 'sin' show as 'sin' in the editor
        ALGEBRAIC,              // Keys like 'sin' show as 'sin()'
        MATRIX,                 // Matrix/vector mode
    };

    enum
    // ------------------------------------------------------------------------
    //   Dimensioning constants
    // ------------------------------------------------------------------------
    {
        NUM_PLANES      = 3,    // NONE, Shift and "extended" shift
        NUM_KEYS        = 46,   // Including SCREENSHOT, SH_UP and SH_DN
        NUM_SOFTKEYS    = 6,    // Number of softkeys
        NUM_LABEL_CHARS = 12,   // Number of characters per menu label
        NUM_MENUS = NUM_PLANES * NUM_SOFTKEYS,
    };


    bool        key(int key, bool repeating);
    bool        repeating()     { return repeat; }
    void        assign(int key, uint plane, object_p code);
    object_p    assigned(int key, uint plane);

    void        menu(menu_p menu, uint page = 0);
    menu_p      menu();
    uint        page();
    void        page(uint p);
    uint        pages();
    void        pages(uint p);
    void        menus(uint count, cstring labels[], object_p function[]);
    void        menu(uint index, cstring label, object_p function);
    void        menu(uint index, symbol_p label, object_p function);
    symbol_p    label(uint index);

    void        draw_annunciators();
    void        draw_editor();
    void        draw_error();
    bool        draw_help();
    void        draw_command();

    int         draw_menus(uint time, uint &period);
    int         draw_battery(uint time, uint &period);
    int         draw_cursor(uint time, uint &period);

    int         stack_screen_bottom()   { return stack; }
    int         menu_screen_bottom()    { return menuHeight; }
    bool        showingHelp()           { return help + 1 != 0; }

    uint        shift_plane()   { return xshift ? 2 : shift ? 1 : 0; }
    void        clear_help();
    object_p    object_for_key(int key);
    void        edit(unicode c, modes m);
    bool        end_edit();
    void        clear_editor();
    void        load_help(utf8 topic);

protected:
    bool        handle_shifts(int key);
    bool        handle_help(int &key);
    bool        handle_editing(int key);
    bool        handle_alpha(int key);
    bool        handle_functions(int key);
    bool        handle_digits(int key);

protected:
    typedef graphics::coord     coord;
    typedef graphics::size      size;

public:
    int      evaluating;    // Key being evaluated

protected:
    utf8     command;       // Command being executed
    uint     help;          // Offset of help being displayed in help file
    uint     line;          // Line offset in the help display
    uint     topic;         // Offset of topic being highlighted
    uint     history;       // History depth
    uint     topics[8];     // Topics history
    uint     cursor;        // Cursor position in buffer
    coord    xoffset;       // Offset of the cursor
    modes    mode;          // Current editing mode
    int      last;          // Last key
    int      stack;         // Vertical bottom of the stack
    coord    cx, cy;        // Cursor position on screen
    menu_p   menuObject;    // Current menu being shown
    uint     menuPage;      // Current menu page
    uint     menuPages;     // Number of menu pages
    uint     menuHeight;    // Height of the menu
    bool     shift     : 1; // Normal shift active
    bool     xshift    : 1; // Extended shift active (simulate Right)
    bool     alpha     : 1; // Alpha mode active
    bool     lowercase : 1; // Lowercase
    bool     down      : 1; // Move one line down
    bool     up        : 1; // Move one line up
    bool     repeat    : 1; // Repeat the key
    bool     longpress : 1; // We had a long press of the key
    bool     blink     : 1; // Cursor blink indicator
    bool     follow    : 1; // Follow a help topic
    bool     dirtyMenu : 1; // Menu label needs redraw

protected:
    // Key mappings
    object_p function[NUM_PLANES][NUM_KEYS];
    cstring  menu_label[NUM_PLANES][NUM_SOFTKEYS];
    static runtime &RT;
    friend struct tests;
    friend struct runtime;

protected:
    struct file
    // ------------------------------------------------------------------------
    //   Direct access to the help file
    // ------------------------------------------------------------------------
    {
        file();
        ~file();

        void    open(cstring path);
        bool    valid();
        void    close();
        unicode get();
        unicode get(uint offset);
        void    seek(uint offset);
        unicode peek();
        uint    position();
        uint    find(unicode cp);
        uint    rfind(unicode cp);

      protected:
#ifdef SIMULATOR
        FILE    *data;
#else
        FIL      data;
#endif
    } helpfile;
};


enum { TIMER0, TIMER1, TIMER2, TIMER3 };

extern input Input;


#endif // INPUT_H
