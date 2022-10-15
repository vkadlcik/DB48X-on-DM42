#ifndef DISPLAY_H
#define DISPLAY_H
// ****************************************************************************
//  display.h                                                     DB48X project
// ****************************************************************************
//
//   File Description:
//
//     Some utilities for display
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

#include <types.h>
#include <dmcp.h>


struct display
// ----------------------------------------------------------------------------
//   A thin wrapper to access DMCP data structures
// ----------------------------------------------------------------------------
{
    display(disp_stat_t *ds)
        : ds(*ds),
          s_x(ds->x),
          s_y(ds->y),
          s_fixed(ds->fixed),
          s_invert(ds->inv),
          s_fill(ds->bgfill),
          s_clear(ds->lnfill),
          s_newlines(ds->newln)
    { }
    ~display()
    {
        ds.x = s_x;
        ds.y = s_y;
        ds.fixed = s_fixed;
        ds.inv = s_invert;
        ds.bgfill = s_fill;
        ds.lnfill = s_clear;
        ds.newln = s_newlines;
    }

    // Wrapping operations
    template <typename ...Args>
    display &write(cstring format, Args &...args)
    {
        lcd_print(&ds, format, args...);
        return *this;
    }

    display &write(cstring t)           { lcd_writeText(&ds, t); return *this; }
    display &newline()                  { lcd_writeNl(&ds); return *this; }
    display &prevln()                   { lcd_prevLn(&ds); return *this; }
    display &clear()                    { lcd_writeClr(&ds); return *this; }
    display &font(int f)                { lcd_switchFont(&ds,f); return *this; }

    int      lineHeight()               { return lcd_lineHeight(&ds); }
    int      baseHeight()               { return lcd_baseHeight(&ds); }
    int      fontWidth()                { return lcd_fontWidth(&ds); }
    int      width(cstring t)           { return lcd_textWidth(&ds, t); }
    int      width(byte c)              { return lcd_charWidth(&ds, c); }


    // Getters
    int16_t x()                         { return ds.x; }
    int16_t y()                         { return ds.y; }
    bool    fixed()                     { return ds.fixed; }
    bool    inverted()                  { return ds.inv; }
    bool    background()                { return ds.bgfill; }
    bool    clearing()                  { return ds.lnfill; }
    bool    newlines()                  { return ds.newln; }

    // Setters
    display &x(int16_t nx)              { ds.x = nx; return *this; }
    display &y(int16_t ny)              { ds.y = ny; return *this; }
    display &xy(int16_t nx, int16_t ny) { return x(nx).y(ny); }
    display &fixed(bool fx = true)      { ds.fixed = fx; return *this; }
    display &inverted(bool inv = true)  { ds.inv = inv; return *this; }
    display &background(bool bg = true) { ds.bgfill = bg; return *this; }
    display &clearing(bool c = true)    { ds.lnfill = c; return *this; }
    display &newlines(bool nl = true)   { ds.newln = nl; return *this; }

    // Implicit conversion to use DMCP functions directly
    operator disp_stat_t *()            { return &ds; }
    operator disp_stat_t &()            { return ds; }

protected:
    disp_stat_t &ds;
    int16_t      s_x, s_y;
    bool         s_fixed    : 1;
    bool         s_invert   : 1;
    bool         s_fill     : 1;
    bool         s_clear    : 1;
    bool         s_newlines : 1;
};

// Easier-to-remember names
enum { LCD_W = LCD_X, LCD_H = LCD_Y };

#endif // DISPLAY_H
