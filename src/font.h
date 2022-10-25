#ifndef FONT_H
#define FONT_H
// ****************************************************************************
//  font.h                                                        DB48X project
// ****************************************************************************
//
//   File Description:
//
//     RPL font objects
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

#include "object.h"
#include "utf8.h"


RECORDER_DECLARE(fonts);
RECORDER_DECLARE(fonts_error);

struct font : object
// ----------------------------------------------------------------------------
//   Shared by all font objects
// ----------------------------------------------------------------------------
{
    typedef int16_t  fint;
    typedef uint16_t fuint;

    font(id type): object(type) { }

    struct glyph_info
    {
        byte_p bitmap;          // Bitmap we get the glyph from
        fint   bx;              // X position in bitmap
        fint   by;              // Y position in bitmap (always 0 today?)
        fuint  bw;              // Width of bitmap
        fuint  bh;              // Height of bitmap
        fint   x;               // X position of glyph when drawing
        fint   y;               // Y position of glyph when drawing
        fuint  w;               // Width of glyph
        fuint  h;               // Height of glyph
        fuint  advance;         // X advance to next character
        fuint  height;          // Y advance to next line
    };
    bool  glyph(utf8code codepoint, glyph_info &g) const;
    fuint width(utf8code codepoint) const
    {
        glyph_info g;
        if (glyph(codepoint, g))
            return g.advance;
        return 0;
    }
    fuint width(utf8 text) const
    {
        fuint result = 0;
        for (utf8 p = text; *p; p = utf8_next(p))
            result += width(utf8_codepoint(p));
        return result;
    }
    fuint width(utf8 text, size_t len) const
    {
        fuint result = 0;
        utf8 last = text + len;
        for (utf8 p = text; p < last; p = utf8_next(p))
            result += width(utf8_codepoint(p));
        return result;
    }
    fuint height(utf8code codepoint) const
    {
        glyph_info g;
        if (glyph(codepoint, g))
            return g.advance;
        return 0;
    }
    fuint height() const;


    OBJECT_HANDLER(font);
    OBJECT_PARSER(font);
    OBJECT_RENDERER(font);
};
typedef const font *font_p;


struct sparse_font : font
// ----------------------------------------------------------------------------
//   An object representing a sparse font (one bitmap per character)
// ----------------------------------------------------------------------------
{
    sparse_font(id type = ID_sparse_font): font(type) {}
    static id static_type() { return ID_sparse_font; }
    bool glyph(utf8code codepoint, glyph_info &g) const;
    fuint height();
};
typedef const sparse_font *sparse_font_p;


struct dense_font : font
// ----------------------------------------------------------------------------
//   An object representing a dense font (a single bitmap for all characters)
// ----------------------------------------------------------------------------
{
    dense_font(id type = ID_dense_font): font(type) {}
    static id static_type() { return ID_dense_font; }
    bool glyph(utf8code codepoint, glyph_info &g) const;
    fuint height();
};
typedef const dense_font *dense_font_p;


struct dmcp_font : font
// ----------------------------------------------------------------------------
//   An object accessing the DMCP built-in fonts (and remapping to Unicode)
// ----------------------------------------------------------------------------
{
    dmcp_font(fint index, id type = ID_dense_font): font(type)
    {
        byte *p = payload();
        leb128(p, index);
    }
    static size_t required_memory(id i, fint index)
    {
        return leb128size(i) + leb128size(index);
    }

    static id static_type() { return ID_dmcp_font; }
    fint index() const      { byte *p = payload(); return leb128<fint>(p); }

    bool glyph(utf8code codepoint, glyph_info &g) const;
    fuint height();
};
typedef const dmcp_font *dmcp_font_p;


inline bool font::glyph(utf8code codepoint, glyph_info &g) const
// ----------------------------------------------------------------------------
//   Dynamic dispatch to the available font classes
// ----------------------------------------------------------------------------
{
    switch(type())
    {
    case ID_sparse_font: return ((sparse_font *)this)->glyph(codepoint, g);
    case ID_dense_font:  return ((dense_font *)this)->glyph(codepoint, g);
    case ID_dmcp_font:   return ((dmcp_font *)this)->glyph(codepoint, g);
    default:
        record(fonts_error, "Unexpectd font type %d", type());
    }
    return false;
}

inline font::fuint font::height() const
// ----------------------------------------------------------------------------
//   Dynamic dispatch to the available font classes
// ----------------------------------------------------------------------------
{
    switch(type())
    {
    case ID_sparse_font: return ((sparse_font *)this)->height();
    case ID_dense_font:  return ((dense_font *)this)->height();
    case ID_dmcp_font:   return ((dmcp_font *)this)->height();
    default:
        record(fonts_error, "Unexpectd font type %d", type());
    }
    return false;
}


// Generated during the build process
extern const font_p EditorFont;
extern const font_p StackFont;

// Fonts for various parts of the user interface
extern const font_p HeaderFont;
extern const font_p CursorFont;
extern const font_p ErrorFont;
extern const font_p MenuFont;
extern const font_p HelpFont;
extern const font_p HelpBoldFont;
extern const font_p HelpItalicFont;
extern const font_p HelpCodeFont;
extern const font_p HelpTitleFont;
extern const font_p HelpSubTitleFont;

// In the DM42 DMCP - Not fully Unicode capable
extern const dmcp_font_p LibMonoFont10x17;
extern const dmcp_font_p LibMonoFont11x18;
extern const dmcp_font_p LibMonoFont12x20;
extern const dmcp_font_p LibMonoFont14x22;
extern const dmcp_font_p LibMonoFont17x25;
extern const dmcp_font_p LibMonoFont17x28;
extern const dmcp_font_p SkrMono13x18;
extern const dmcp_font_p SkrMono18x24;
extern const dmcp_font_p Free42Font;

#endif // FONT_H
