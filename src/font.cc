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
    case EVAL:
        // Integer values evaluate as self
        rt.push(obj);
        return 0;
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
    char buffer[32];
    size_t result = snprintf(buffer, sizeof(buffer), "Font %p", this);
    record(fonts, "Render %p [%s]", this, (cstring) r.target);
    return result;
}


struct font_cache
// ----------------------------------------------------------------------------
//   A data structure to accelerate access to font offsets for a given font
// ----------------------------------------------------------------------------
{
    // Use same size as font data
    using fint = font::fint;

    struct data
    // ------------------------------------------------------------------------
    //   Data in the cache
    // ------------------------------------------------------------------------
    {
        void set(byte *bitmap, fint x, fint y, fint w, fint h)
        {
            this->bitmap = bitmap;
            this->x = x;
            this->y = y;
            this->w = w;
            this->h = h;
        }

        byte_p bitmap;          // Bitmap data for glyph
        fint   x;               // X position (meaning depends on font type)
        fint   y;               // Y position (meaning depends on font type)
        fint   w;               // Width (meaning depends on font type)
        fint   h;               // Height (meaning depends on font type)
    };

    font_cache(): fobj(), first(0), last(0), cache() {}

    font *cachedFont()
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


    data *range(font *f, fint firstCP, fint lastCP)
    // ------------------------------------------------------------------------
    //   Set the currently cached range in the font
    // ------------------------------------------------------------------------
    {
        if (f != fobj || firstCP != first || lastCP != last)
        {
            fint count = lastCP - firstCP;
            fobj = f;
            first = firstCP;
            last = lastCP;
            cache = (data *) realloc(cache, count * sizeof(data));
            data *ecache = cache + count;
            for (data *p = cache; p < ecache; p++)
                p->set(nullptr, 0, 0, 0, 0);
        }
        return cache;
    }

    data *get(fint glyph)
    // ------------------------------------------------------------------------
    //  Return cached data
    // ------------------------------------------------------------------------
    {
        if (glyph >= first && glyph < last)
            return cache + glyph - first;
        return nullptr;
    }


    bool set(fint glyph, byte *bm, fint x, fint y, fint w, fint h)
    // ------------------------------------------------------------------------
    //   Set the offset of the glyph in the font
    // ------------------------------------------------------------------------
    {
        if (glyph >= first && glyph < last)
        {
            data *p = cache + glyph - first;
            p->set(bm, x, y, w, h);
            return true;
        }
        return false;
    }

private:
    font *fobj;
    fint  first;
    fint  last;
    data *cache;
} FontCache;


bool sparse_font::glyph(utf8code codepoint, glyph_info &g)
// ----------------------------------------------------------------------------
//   Return the bitmap address and update coordinate info for a sparse font
// ----------------------------------------------------------------------------
{
    // Scan the font data
    byte         *p      = payload();
    size_t UNUSED size   = leb128<size_t>(p);
    fint          height = leb128<fint>(p);

    // Check if cached
    font_cache::data *data = FontCache.cachedFont() == this
        ? FontCache.get(codepoint)
        : nullptr;

    record(sparse_fonts, "Looking up %u, got cache %p", codepoint, data);
    while (!data)
    {
        // Check code point range
        fint firstCP = leb128<fint>(p);
        fint numCPs = leb128<fint>(p);
        record(sparse_fonts,
               "  Range %u-%u (%u codepoints)",
               firstCP, firstCP + numCPs, numCPs);

        // Check end of font ranges
        if (!firstCP && !numCPs)
            return false;

        // Check if we are past the current code point
        if (firstCP > codepoint)
            return false;

        fint lastCP = firstCP + numCPs;
        bool in = codepoint >= firstCP && codepoint < lastCP;

        // Initialize cache for range of current code point
        font_cache::data *cache = in
            ? FontCache.range(this, firstCP, lastCP)
            : nullptr;
        if (cache)
            record(sparse_fonts, "Caching in %p", cache);
        for (fint cp = firstCP; cp < lastCP; cp++)
        {
            fint x = leb128<fint>(p);
            fint y = leb128<fint>(p);
            fint w = leb128<fint>(p);
            fint h = leb128<fint>(p);
            if (cache)
            {
                cache->set(p, x, y, w, h);
                cache++;
                if (cp == codepoint)
                {
                    record(sparse_fonts, "Cache data is at %p", cache);
                    data = cache;
                }
            }
            int sparseBitmapBits = w * h;
            int sparseBitmapBytes = (sparseBitmapBits + 7) / 8;
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
    g.advance = data->x + data->w;
    g.height  = height;
    record(sparse_fonts,
           "For glyph %u, x=%u y=%u w=%u h=%u bw=%u bh=%u adv=%u hgh=%u",
           codepoint, g.x, g.y, g.w, g.h, g.bw, g.bh, g.advance, g.height);
    return true;
}


bool dense_font::glyph(utf8code codepoint, glyph_info &g)
// ----------------------------------------------------------------------------
//   Return the bitmap address and update coordinate info for a dense font
// ----------------------------------------------------------------------------
{
    // Scan the font data
    byte         *p      = payload();
    size_t UNUSED size   = leb128<size_t>(p);
    fint          height = leb128<fint>(p);
    fint          width  = leb128<fint>(p);
    byte         *bitmap = p;

    // Check if cached
    font_cache::data *data = FontCache.cachedFont() == this
        ? FontCache.get(codepoint)
        : nullptr;

    // Scan the font data
    fint  x          = 0;
    fint  bitmapSize = (height * width + 7) / 8;

    p += bitmapSize;
    while (!data)
    {
        // Check code point range
        fint firstCP = leb128<fint>(p);
        fint numCPs = leb128<fint>(p);

        // Check end of font ranges
        if (!firstCP && !numCPs)
            return false;

        // Check if we are past the current code point
        if (firstCP > codepoint)
            return false;

        fint lastCP = firstCP + numCPs;
        bool in = codepoint >= firstCP && codepoint < lastCP;

        // Initialize cache for range of current code point
        font_cache::data *cache = in
            ? FontCache.range(this, firstCP, lastCP)
            : nullptr;
        for (fint cp = firstCP; cp < lastCP; cp++)
        {
            fint cw = leb128<fint>(p);
            if (cache)
            {
                cache->set(bitmap, x, 0, cw, height);
                cache++;
                if (cp == codepoint)
                    data = cache;
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
    g.advance = g.w;
    g.height  = height;
    return true;
}


bool dmcp_font::glyph(utf8code codepoint, glyph_info &g)
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
    byte_p synthesized = nullptr;
    switch(codepoint)
    {
    case L'÷': codepoint = 0x80; break;
    case L'×': codepoint = 0x81; break;
    case L'√': codepoint = 0x82; break;
    case L'∫': codepoint = 0x83; break;
    case L'░': codepoint = 0x84; break;
    case L'Σ': codepoint = 0x85; break;
    case L'▶': codepoint = 0x86; break;
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
    case L'«':
    {
        // Invent this character for the font
        static const byte bitmap[] = {
            0x05,               // ----*-*-
            0x0A,               // ---*-*--
            0x14,               // --*-*---
            0x28,               // -*-*----
            0x14,               // --*-*---
            0x0A,               // ---*-*--
            0x05,               // ----*-*-
            0x00,               // --------
        };
        synthesized = bitmap;
        break;
    }
    case L'»':
    {
        // Invent this character for the font
        static const byte bitmap[] = {
            0x50,               // -*-*----
            0x28,               // --*-*---
            0x14,               // ---*-*--
            0x0A,               // ----*-*-
            0x14,               // ---*-*--
            0x28,               // --*-*---
            0x50,               // -*-*----
            0x00,               // --------
        };
        synthesized = bitmap;
        break;
    }
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
        if (synthesized)
        {
            // Special case of characters we synthesize
            g.bitmap = synthesized;
            g.bx = 0;
            g.by = 0;
            g.bw = 8;
            g.bh = 8;
            g.x =  0;
            g.y =  (f->height - 8) / 2;
            g.w =  8;
            g.h =  8;
            g.advance = g.w;
            return true;
        }
        return false;
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
