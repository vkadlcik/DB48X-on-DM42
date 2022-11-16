// ****************************************************************************
//  font.cc                                                       DB48X project
// ****************************************************************************
//
//   File Description:
//
//     RPL Font objects
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

#include "font.h"

#include "parser.h"
#include "recorder.h"
#include "renderer.h"
#include "runtime.h"

#include <dmcp.h>
#include <stdio.h>
#include <stdlib.h>

RECORDER(fonts,         16, "Information about fonts");
RECORDER(sparse_fonts,  16, "Information about sparse fonts");
RECORDER(dense_fonts,   16, "Information about dense fonts");
RECORDER(dmcp_fonts,    16, "Information about DMCP fonts");
RECORDER(fonts_error,   16, "Information about fonts");


OBJECT_HANDLER_BODY(font)
// ----------------------------------------------------------------------------
//    Handler for font objects
// ----------------------------------------------------------------------------
{
    record(fonts, "Command %+s on %p", name(op), obj);
    switch(op)
    {
    case EXEC:
    case EVAL:
        // Font values evaluate as self
        return rt.push(obj) ? OK : ERROR;
    case SIZE:
        return ptrdiff(payload, obj) + leb128<size_t>(payload);
    case PARSE:
        return object_parser(OBJECT_PARSER_ARG(), rt);
    case RENDER:
        return obj->object_renderer(OBJECT_RENDERER_ARG(), rt);

    default:
        // Check if anyone else knows how to deal with it
        return DELEGATE(object);
    }

}


OBJECT_PARSER_BODY(font)
// ----------------------------------------------------------------------------
//    Try to parse this as a font object
// ----------------------------------------------------------------------------
{
    record(fonts, "Cannot parse a font (yet)");
    return SKIP;
}


OBJECT_RENDERER_BODY(font)
// ----------------------------------------------------------------------------
//   Render the integer into the given string buffer
// ----------------------------------------------------------------------------
{
    r.put("Font (internal)");
    return r.size();
}


static const byte dmcpFontRPL[]
// ----------------------------------------------------------------------------
//   RPL object representing the various DMCP fonts
// ----------------------------------------------------------------------------
{
#define LEB128(id, fnt)      (id) | 0x80, ((id) >> 7) & 0x7F, (fnt)

    LEB128(object::ID_dmcp_font, 0),            // lib_mono
    LEB128(object::ID_dmcp_font, 1),
    LEB128(object::ID_dmcp_font, 2),
    LEB128(object::ID_dmcp_font, 3),
    LEB128(object::ID_dmcp_font, 4),
    LEB128(object::ID_dmcp_font, 5),

    LEB128(object::ID_dmcp_font, 10),           // Free42 (fixed size, very small)

    LEB128(object::ID_dmcp_font, 18),           // skr_mono
    LEB128(object::ID_dmcp_font, 21),           // skr_mono

};

// In the DM42 DMCP - Not fully Unicode capable
const dmcp_font_p LibMonoFont10x17 = (dmcp_font_p) (dmcpFontRPL +  0);
const dmcp_font_p LibMonoFont11x18 = (dmcp_font_p) (dmcpFontRPL +  3);
const dmcp_font_p LibMonoFont12x20 = (dmcp_font_p) (dmcpFontRPL +  6);
const dmcp_font_p LibMonoFont14x22 = (dmcp_font_p) (dmcpFontRPL +  9);
const dmcp_font_p LibMonoFont17x25 = (dmcp_font_p) (dmcpFontRPL + 12);
const dmcp_font_p LibMonoFont17x28 = (dmcp_font_p) (dmcpFontRPL + 15);
const dmcp_font_p Free42Font       = (dmcp_font_p) (dmcpFontRPL + 18);
const dmcp_font_p SkrMono13x18     = (dmcp_font_p) (dmcpFontRPL + 21);
const dmcp_font_p SkrMono18x24     = (dmcp_font_p) (dmcpFontRPL + 24);


font_p EditorFont;
font_p StackFont;
font_p HeaderFont;
font_p CursorFont;
font_p ErrorFont;
font_p MenuFont;
font_p HelpFont;
font_p HelpBoldFont;
font_p HelpItalicFont;
font_p HelpCodeFont;
font_p HelpTitleFont;
font_p HelpSubTitleFont;


void font_defaults()
// ----------------------------------------------------------------------------
//    Initialize the fonts for the user interface
// ----------------------------------------------------------------------------
{
#define GENERATED_FONT(name)                    \
    extern byte name##_sparse_font_data[];      \
    name = (font_p) name##_sparse_font_data;

    GENERATED_FONT(EditorFont);
    GENERATED_FONT(HelpFont);
    GENERATED_FONT(StackFont);

    HeaderFont       = LibMonoFont10x17;
    CursorFont       = LibMonoFont17x25;
    ErrorFont        = SkrMono13x18;
    MenuFont         = HelpFont;

    HelpBoldFont     = HelpFont;
    HelpItalicFont   = HelpFont;
    HelpCodeFont     = LibMonoFont11x18;
    HelpTitleFont    = StackFont;
    HelpSubTitleFont = HelpFont;
}


struct font_cache
// ----------------------------------------------------------------------------
//   A data structure to accelerate access to font offsets for a given font
// ----------------------------------------------------------------------------
{
    // Use same size as font data
    using fint  = font::fint;
    using fuint = font::fuint;

    struct data
    // ------------------------------------------------------------------------
    //   Data in the cache
    // ------------------------------------------------------------------------
    {
        void set(byte *bitmap, fint x, fint y, fuint w, fuint h, fuint a)
        {
            this->bitmap = bitmap;
            this->x = x;
            this->y = y;
            this->w = w;
            this->h = h;
            this->advance = a;
        }

        byte_p bitmap;          // Bitmap data for glyph
        fint   x;               // X position (meaning depends on font type)
        fint   y;               // Y position (meaning depends on font type)
        fuint  w;               // Width (meaning depends on font type)
        fuint  h;               // Height (meaning depends on font type)
        fuint  advance;         // Advance to next character
    };

    font_cache(): fobj(), first(0), last(0), cache() {}

    font_p cachedFont()
    // ------------------------------------------------------------------------
    //   Return the currently cached font
    // ------------------------------------------------------------------------
    {
        return fobj;
    }

    fint firstCodePoint()
    // ------------------------------------------------------------------------
    //   Return first cached code point
    // ------------------------------------------------------------------------
    {
        return first;
    }


    fint lastCodePoint()
    // ------------------------------------------------------------------------
    //   Return last cached code point
    // ------------------------------------------------------------------------
    {
        return last;
    }


    data *range(font_p f, fuint firstCP, fuint lastCP)
    // ------------------------------------------------------------------------
    //   Set the currently cached range in the font
    // ------------------------------------------------------------------------
    {
        if (f != fobj || firstCP != first || lastCP != last)
        {
            fuint count = lastCP - firstCP;
            fobj = f;
            first = firstCP;
            last = lastCP;
            cache = (data *) realloc(cache, count * sizeof(data));
            data *ecache = cache + count;
            for (data *p = cache; p < ecache; p++)
                p->set(nullptr, 0, 0, 0, 0, 0);
        }
        return cache;
    }

    data *get(fint glyph) const
    // ------------------------------------------------------------------------
    //  Return cached data
    // ------------------------------------------------------------------------
    {
        if (glyph >= first && glyph < last)
            return cache + glyph - first;
        return nullptr;
    }


    bool set(fint glyph, byte *bm, fint x, fint y, fuint w, fuint h, fuint a)
    // ------------------------------------------------------------------------
    //   Set the offset of the glyph in the font
    // ------------------------------------------------------------------------
    {
        if (glyph >= first && glyph < last)
        {
            data *p = cache + glyph - first;
            p->set(bm, x, y, w, h, a);
            return true;
        }
        return false;
    }

private:
    font_p fobj;
    fuint   first;
    fuint   last;
    data  *cache;
} FontCache;


font::fuint sparse_font::height()
// ----------------------------------------------------------------------------
//   Return the font height from its data
// ----------------------------------------------------------------------------
{
    // Scan the font data
    byte         *p      = payload();
    size_t UNUSED size   = leb128<size_t>(p);
    fuint         height = leb128<fuint>(p);
    return height;
}


bool sparse_font::glyph(unicode codepoint, glyph_info &g) const
// ----------------------------------------------------------------------------
//   Return the bitmap address and update coordinate info for a sparse font
// ----------------------------------------------------------------------------
{
    // Scan the font data
    byte         *p      = payload();
    size_t UNUSED size   = leb128<size_t>(p);
    fuint         height = leb128<fuint>(p);

    // Check if cached
    font_cache::data *data = FontCache.cachedFont() == this
        ? FontCache.get(codepoint)
        : nullptr;

    record(sparse_fonts, "Looking up %u, got cache %p", codepoint, data);
    while (!data)
    {
        // Check code point range
        fuint firstCP = leb128<fuint>(p);
        fuint numCPs  = leb128<fuint>(p);
        record(sparse_fonts,
               "  Range %u-%u (%u codepoints)",
               firstCP, firstCP + numCPs, numCPs);

        // Check end of font ranges, or if past current codepoint
        if ((!firstCP && !numCPs) || firstCP > codepoint)
        {
            record(sparse_fonts, "Code point %u not found", codepoint);
            return false;
        }

        fuint lastCP = firstCP + numCPs;
        bool  in = codepoint >= firstCP && codepoint < lastCP;

        // Initialize cache for range of current code point
        font_cache::data *cache = in
            ? FontCache.range(this, firstCP, lastCP)
            : nullptr;
        if (cache)
            record(sparse_fonts, "Caching in %p", cache);
        for (fuint cp = firstCP; cp < lastCP; cp++)
        {
            fint  x = leb128<fint>(p);
            fint  y = leb128<fint>(p);
            fuint w = leb128<fuint>(p);
            fuint h = leb128<fuint>(p);
            fuint a = leb128<fuint>(p);
            if (cache)
            {
                cache->set(p, x, y, w, h, a);
                if (cp == codepoint)
                {
                    record(sparse_fonts, "Cache data is at %p", cache);
                    data = cache;
                }
                cache++;
            }
            size_t sparseBitmapBits = w * h;
            size_t sparseBitmapBytes = (sparseBitmapBits + 7) / 8;
            p += sparseBitmapBytes;
            record(sparse_fonts,
                   "  cp %u x=%u y=%u w=%u h=%u bitmap=%p %u bytes",
                   cp, x, y, w, h, p - sparseBitmapBytes, sparseBitmapBytes);
        }
    }

    g.bitmap  = data->bitmap;
    g.bx      = 0;
    g.by      = 0;
    g.bw      = data->w;
    g.bh      = data->h;
    g.x       = data->x;
    g.y       = data->y;
    g.w       = data->w;
    g.h       = data->h;
    g.advance = data->advance;
    g.height  = height;
    record(sparse_fonts,
           "For glyph %u, x=%u y=%u w=%u h=%u bw=%u bh=%u adv=%u hgh=%u",
           codepoint, g.x, g.y, g.w, g.h, g.bw, g.bh, g.advance, g.height);
    return true;
}


font::fuint dense_font::height()
// ----------------------------------------------------------------------------
//   Return the font height from its data
// ----------------------------------------------------------------------------
{
    // Scan the font data
    byte         *p      = payload();
    size_t UNUSED size   = leb128<size_t>(p);
    fuint         height = leb128<fuint>(p);
    return height;
}


bool dense_font::glyph(unicode codepoint, glyph_info &g) const
// ----------------------------------------------------------------------------
//   Return the bitmap address and update coordinate info for a dense font
// ----------------------------------------------------------------------------
{
    // Scan the font data
    byte         *p      = payload();
    size_t UNUSED size   = leb128<size_t>(p);
    fuint         height = leb128<fuint>(p);
    fuint         width  = leb128<fuint>(p);
    byte         *bitmap = p;

    // Check if cached
    font_cache::data *data = FontCache.cachedFont() == this
        ? FontCache.get(codepoint)
        : nullptr;

    // Scan the font data
    fint   x          = 0;
    size_t bitmapSize = (height * width + 7) / 8;

    p += bitmapSize;
    while (!data)
    {
        // Check code point range
        fuint firstCP = leb128<fuint>(p);
        fuint numCPs  = leb128<fuint>(p);

        // Check end of font ranges, or if past current codepoint
        if ((!firstCP && !numCPs) || firstCP > codepoint)
        {
            record(dense_fonts, "Code point %u not found", codepoint);
             return false;
        }

        fuint lastCP = firstCP + numCPs;
        bool in = codepoint >= firstCP && codepoint < lastCP;

        // Initialize cache for range of current code point
        font_cache::data *cache = in
            ? FontCache.range(this, firstCP, lastCP)
            : nullptr;
        for (fuint cp = firstCP; cp < lastCP; cp++)
        {
            fuint cw = leb128<fuint>(p);
            if (cache)
            {
                cache->set(bitmap, x, 0, cw, height, cw);
                if (cp == codepoint)
                    data = cache;
                cache++;
            }
            x += cw;
        }
    }
    g.bitmap  = bitmap;
    g.bx      = data->x;
    g.by      = data->y;
    g.bw      = width;
    g.bh      = height;
    g.x       = 0;
    g.y       = 0;
    g.w       = data->w;
    g.h       = height;
    g.advance = data->advance;
    g.height  = height;
    return true;
}


font::fuint dmcp_font::height()
// ----------------------------------------------------------------------------
//   Return the font height from its data
// ----------------------------------------------------------------------------
{
    // Switch to the correct DMCP font
    int fontnr = index();
    if (fontnr >= 11 && fontnr <= 16) // Use special DMCP index for Free42 fonts
        fontnr = -(fontnr - 10);
    lcd_switchFont(fReg, fontnr);

    // Check if codepoint is within font range
    const line_font_t *f        = fReg->f;
    return f->height;
}


bool dmcp_font::glyph(unicode utf8cp, glyph_info &g) const
// ----------------------------------------------------------------------------
//   Return the bitmap address and update coordinate info for a DMCP font
// ----------------------------------------------------------------------------
//   On the DM42, DMCP font numbering is a bit wild.
//   There are three font sets, with lcd_nextFontNr and lcd_prevFontNr switching
//   only within a given set, and lcd_toggleFontT switching between sets
//   The lib_mono set has 6 font sizes with the following numbers:
//      0: lib_mono_10x17
//      1: lib_mono_11x18
//      2: lib_mono_12x20
//      3: lib_mono_14x22
//      4: lib_mono_17x25
//      5: lib_mono_17x28
//   lib_mono fonts are the only fonts the simulator has at the moment.
//   The free42 family contains four "HP-style" fonts:
//     -1: free42_2x2   (encoded as dcmp_font::index 11)
//     -3: free42_2x3   (encoded as dmcp_font::index 13)
//     -5: free42_3x3   (encoded as dmcp_font::index 15)
//     -6: free42_3x4   (encoded as dmcp_font::index 16)
//   The skr_mono family has two "bold" fonts with larger size:
//     18: skr_mono_13x18
//     21: skr_mono_18x24
//   The lcd_toggleFontT can sometimes return 16, but that appears to select the
//   same font as index 18 when passed to lcd_switchFont
//
//   Furthermore, the DMCP fonts are not Unicode-compliant.
//   This function automatically performs the remapping of relevant code points
//   prior to caching (in order to keep correct cache locality)
//
//   Finally, the fonts lack a few important characters for RPL, like the
//   program brackets « and ». Those are "synthesized" by this routine.
//
//   The DMCP font does not spoil the cache, keeping it for the other two types.
//   That's because it has a single range and a direct access already.
{
    // Map Unicode code points to corresonding entry in DMCP charset
    unicode codepoint = utf8cp;
    switch(codepoint)
    {
    case L'÷': codepoint = 0x80; break;
    case L'×': codepoint = 0x81; break;
    case L'√': codepoint = 0x82; break;
    case L'∫': codepoint = 0x83; break;
    case L'░': codepoint = 0x84; break;
    case L'Σ': codepoint = 0x85; break;
        // case L'▶': codepoint = 0x86; break;
    case L'π': codepoint = 0x87; break;
    case L'¿': codepoint = 0x88; break;
    case L'≤': codepoint = 0x89; break;
    case L'␊': codepoint = 0x8A; break;
    case L'≥': codepoint = 0x8B; break;
    case L'≠': codepoint = 0x8C; break;
    case L'↲': codepoint = 0x8D; break;
    case L'↓': codepoint = 0x8E; break;
    case L'→': codepoint = 0x8F; break;
    case L'←': codepoint = 0x90; break;
    case L'μ': codepoint = 0x91; break;
    case L'£': codepoint = 0x92; break;
    case L'°': codepoint = 0x93; break;
    case L'Å': codepoint = 0x94; break;
    case L'Ñ': codepoint = 0x95; break;
    case L'Ä': codepoint = 0x96; break;
    case L'∡': codepoint = 0x97; break;
    case L'ᴇ': codepoint = 0x98; break;
    case L'Æ': codepoint = 0x99; break;
    case L'…': codepoint = 0x9A; break;
    case L'␛': codepoint = 0x9B; break;
    case L'Ö': codepoint = 0x9C; break;
    case L'Ü': codepoint = 0x9D; break;
    case L'▒': codepoint = 0x9E; break;
    case L'■': codepoint = 0x9F; break;
    case L'▼': codepoint = 0xA0; break;
    case L'▲': codepoint = 0xA1; break;
    default:
        break;
    }

    // Switch to the correct DMCP font
    int fontnr = index();
    if (fontnr >= 11 && fontnr <= 16) // Use special DMCP index for Free42 fonts
        fontnr = -(fontnr - 10);
    lcd_switchFont(fReg, fontnr);

    // Check if codepoint is within font range
    const line_font_t *f        = fReg->f;
    uint               first    = f->first_char;
    uint               count    = f->char_cnt;
    uint               last     = first + count;
    if (codepoint < first || codepoint >= last)
    {
        font_p alternate = HelpFont;
        switch (fontnr)
        {
        case 2:
        case 3:
        case 4:
        case 5:
            alternate = StackFont;
            break;
        case 18:
        case 21:
            alternate = HelpFont;
            break;
        case 24:
            alternate = StackFont;
            break;
        }
        record(dmcp_fonts, "Code point %u not found (utf8 %u), using alternate",
               codepoint, utf8cp);
        return alternate->glyph(codepoint, g);
    }

    // Get font and glyph properties
    uint           height = f->height;
    const byte    *data   = f->data;
    fint           off    = f->offs[codepoint - first];
    const uint8_t *dp     = data + off;
    fint           cx     = *dp++;
    fint           cy     = *dp++;
    fint           cols   = *dp++;
    fint           rows   = *dp++;

    g.bitmap = dp;
    g.bx = 0;
    g.by = 0;
    g.bw = (cols + 7) / 8 * 8;
    g.bh = rows;
    g.x = cx;
    g.y = cy;
    g.w = cols;
    g.h = rows;
    g.advance = cx + cols;
    g.height = height;

    return true;
}
