#ifndef GRAPHICS_H
#define GRAPHICS_H
// ****************************************************************************
//  graphics.h                                                    DB48X project
// ****************************************************************************
//
//   File Description:
//
//     Graphic routines for the DB48X project
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

#include "types.h"


template <uint BITS_PER_PIXEL>
struct graphics
// ----------------------------------------------------------------------------
//    The graphics class managing bitmap operations
// ----------------------------------------------------------------------------
{
    // Some basic types for graphics
    typedef int16_t  coord;
    typedef uint16_t size;
    typedef size_t   offset;
    typedef uint32_t pixword;
    typedef uint16_t palette_index;
    typedef uint64_t pattern_bits;

    enum
    // ------------------------------------------------------------------------
    //   Size of the side of a 64-bit pattern
    // ------------------------------------------------------------------------
    {
        PATTERN_SIZE = BITS_PER_PIXEL == 1  ? 8
                     : BITS_PER_PIXEL == 4  ? 4
                     : BITS_PER_PIXEL == 16 ? 2
                                            : 1
    };


    union color
    // ------------------------------------------------------------------------
    //  1-bit color representation (e.g. DM42)
    // ------------------------------------------------------------------------
    {
        struct bits
        {
            uint8_t bit0 : 1;
            uint8_t bit1 : 1;
            uint8_t bit2 : 1;
            uint8_t bit3 : 1;
            uint8_t bit4 : 1;
            uint8_t bit5 : 1;
            uint8_t bit6 : 1;
            uint8_t bit7 : 1;
        } __attribute__((packed)) bits;
        bool value;             // The color value is 0 or 1
    };


    union pattern
    // ------------------------------------------------------------------------
    //   Pattern representation for fills
    // ------------------------------------------------------------------------
    //   All patterns are represented by a 64-bit value, but that same
    //   pattern can represent a 8x8 square on a 1-bit screen like the DM42,
    //   a 4x4 square on a 4-bit screen like the HP50G, and a 2x2 square on
    //   a 16-bit screen like the HP Prime.
    {
        // Build a pattern from one, two or four colors
        pattern(color c) : bits(c.value * multiplier()) {}

        template<uint N>
        pattern(color c[N]) : bits(0)
        {
            for (uint shift = 0; shift < 64 / BITS_PER_PIXEL; shift++)
            {
                uint index = (shift + ((shift / PATTERN_SIZE) % N)) % N;
                bits |= uint64_t(c[index].value) << (shift * BITS_PER_PIXEL);
            }
        }
        pattern(color a, color b)
        {
            color colors[2] = { a, b };
            *this = pattern(colors);
        }
        pattern(color a, color b, color c, color d)
        {
            color colors[4] = { a, b, c, d };
            *this = pattern(colors);
        }

        // Build a pattern from RGB levels
        pattern(uint8_t red, uint8_t green, uint8_t blue) : bits(0)
        {
            uint16_t gray = (red + green + green + blue + 4) / 16;
            if (gray == 32)             // Hand tweak 50% gray
                bits = 0xAAAAAAAAAAAAAAAAull;
            else
                for (int bit = 0; bit < 64 && gray; bit++, gray--)
                    bits |= 1ULL << (79 * bit % 64);
        }

        uint64_t rotate(uint64_t pat, uint shift)
        // --------------------------------------------------------------------
        //   Rotate a pattern by the given amount
        // --------------------------------------------------------------------
        {
            shift %= BITS_PER_PATTERN;
            bits = ((pat >> shift) | (pat << (BITS_PER_PATTERN - shift)));
            return bits;
        }

        uint64_t rotate(uint shift)
        // --------------------------------------------------------------------
        //   Rotate a pattern by the given amount
        // --------------------------------------------------------------------
        {
            return rotate(bits, shift);
        }

        uint64_t multiplier()
        // --------------------------------------------------------------------
        //   Return the value to multiply color bits by to get a filled pattern
        // --------------------------------------------------------------------
        {
            // Value for 1-bpp
            return 0xFFFFFFFFFFFFFFFFull;
        }

        static const pattern black()    { return pattern(  0,   0,   0); }
        static const pattern gray25()   { return pattern( 64,  64,  64); }
        static const pattern gray50()   { return pattern(128, 128, 128); }
        static const pattern gray75()   { return pattern(192, 192, 192); }
        static const pattern white()    { return pattern(255, 255, 255); }

        uint64_t  bits;
    };


    enum
    // ------------------------------------------------------------------------
    //   Some constants we derive from the pattern size
    // ------------------------------------------------------------------------
    {
        BITS_PER_PATTERN       = (8 * sizeof(pattern_bits)),
        BITS_PER_PATTERN_ROW   = BITS_PER_PIXEL * PATTERN_SIZE,
        BYTES_PER_PATTERN_ROW  = BITS_PER_PATTERN_ROW / 8,
        COLORS_PER_PATTERN_ROW = BYTES_PER_PATTERN_ROW / sizeof(color),
        BITS_PER_WORD          = (8 * sizeof(pixword)),
    };


    struct point
    // ------------------------------------------------------------------------
    //    A point holds a pair of coordinates
    // ------------------------------------------------------------------------
    {
        point(coord x, coord y): x(x), y(y) {}
        point(const point &o) = default;
        coord x, y;
    };


    struct rect
    // ------------------------------------------------------------------------
    //   A rectangle is a point with a width and a height
    // ------------------------------------------------------------------------
    {
        rect(coord x1, coord y1, coord x2, coord y2)
            : x1(x1), x2(x2), y1(y1), y2(y2) {}
        rect(size w, size h): x1(0), x2(w-1), y1(0), y2(h-1) {}

        rect(const rect &o) = default;

        rect &operator &=(const rect &o)
        // --------------------------------------------------------------------
        //   Intersection of two rectangles
        // --------------------------------------------------------------------
        {
            if (x1 < o.x1) x1 = o.x1;
            if (x2 > o.x2) x2 = o.x2;
            if (y1 < o.y1) y1 = o.y1;
            if (y2 > o.y2) y2 = o.y2;
            return *this;
        }

        rect &operator |=(const rect &o)
        // --------------------------------------------------------------------
        //   Union of two rectangles
        // --------------------------------------------------------------------
        {
            if (x1 > o.x1) x1 = o.x1;
            if (x2 < o.x2) x2 = o.x2;
            if (y1 > o.y1) y1 = o.y1;
            if (y2 < o.y2) y2 = o.y2;
            return *this;
        }

        friend rect operator& (const rect &a, const rect &b)
        // --------------------------------------------------------------------
        //   Intersection of two rectangle
        // --------------------------------------------------------------------
        {
            rect r(a);
            r &= b;
            return r;
        }

        friend rect operator| (const rect &a, const rect &b)
        // --------------------------------------------------------------------
        //   Union of two rectangle
        // --------------------------------------------------------------------
        {
            rect r(a);
            r |= b;
            return r;
        }

    public:
        coord x1, y1, x2, y2;
    };


public:
    // =========================================================================
    //
    //    Core blitting routine
    //
    // =========================================================================

    enum clipping
    // ------------------------------------------------------------------------
    //   Hints to help the compiler drop useless code
    // ------------------------------------------------------------------------
    {
        CLIP_NONE   = 0,
        CLIP_SRC    = 1,
        CLIP_DST    = 2,
        CLIP_ALL    = 3,
        SKIP_SOURCE = 4,
        SKIP_COLOR  = 8,
        FILL_QUICK  = SKIP_SOURCE,
        FILL_SAFE   = SKIP_SOURCE | CLIP_DST,
        COPY        = CLIP_ALL | SKIP_COLOR,
    };

    // Blitting operations
    typedef pixword (*blitop)(pixword dst, pixword src, pixword arg);

    template <clipping clip,
              typename Dst,
              typename Src,
              uint cbpp = BITS_PER_PIXEL>
    static void blit(Dst         &dst,
                     const Src   &src,
                     const rect  &drect,
                     const point &spos,
                     blitop       op,
                     pattern      colors)
    // -------------------------------------------------------------------------
    //   Generalized multi-bpp blitting routine
    // -------------------------------------------------------------------------
    //   This transfers pixels from 'src' to 'dst' (which can be equal)
    //   - targeting a rectangle defined by (x1, x2, y1, y2)
    //   - fetching pixels from (x,y) in the source
    //   - applying the given operation in 'op'
    //
    //   Everything is so that the compiler can optimize code away
    //
    //   The code selects the correct direction for copies within the same
    //   surface, so it is safe to use for operations like scrolling.
    //
    //   We pass the bits-per-pixel as arguments to make it easier for the
    //   optimizer to replace multiplications with shifts when we pass a
    //   constant. Additional flags can be used to statically disable sections
    //   of the code.
    //
    //   An arbitrary blitop is passed, which can be used to process each set of
    //   pixels in turn. That operation is dependent on the respective bits per
    //   pixelss, and can be used e.g. to do bit-plane conversions. See how this
    //   is used in DrawText to colorize 1bpp bitplanes. The source and color
    //   pattern are both aligned to match the destination before the operator
    //   is called. So if the source is 1bpp and the destination is 4bpp, you
    //   might end up with the 8 low bits of the source being the bit pattern
    //   that applies to the 8 nibbles in the destination word.
    {
        bool       clip_src = clip & CLIP_SRC;
        bool       clip_dst = clip & CLIP_DST;
        bool       skip_src = clip & SKIP_SOURCE;
        bool       skip_col = clip & SKIP_COLOR;

        coord      x1       = drect.x1;
        coord      y1       = drect.y1;
        coord      x2       = drect.x2;
        coord      y2       = drect.y2;
        coord      x        = spos.x;
        coord      y        = spos.y;
        const uint sbpp     = Src::BPP;
        const uint dbpp     = Dst::BPP;

        if (clip_src)
        {
            if (x < src.x1)
            {
                x1 += src.x1 - x;
                x = src.x1;
            }
            if (x + x2 - x1 > src.x2)
                x2 = src.x2 - x + x1;
            if (y < src.y1)
            {
                y1 += src.y1 - y;
                y = src.y1;
            }
            if (y + y2 - y1 > src.y2)
                y2 = src.y2 - y + y1;
        }

        if (clip_dst)
        {
            // Clipping based on target
            if (x1 < dst.x1)
            {
                x += dst.x1 - x1;
                x1 = dst.x1;
            }
            if (x2 > dst.x2)
                x2 = dst.x2;
            if (y1 < dst.y1)
            {
                y += dst.y1 - y1;
                y1 = dst.y1;
            }
            if (y2 > dst.y2)
                y2 = dst.y2;
        }

        // Bail out if there is no effect
        if (x1 > x2 || y1 > y2)
            return;

        // Check whether we need to go forward or backward along X or Y
        int        xback  = x < x1;
        int        yback  = y < y1;
        int        xdir   = xback ? -1 : 1;
        int        ydir   = yback ? -1 : 1;
        coord      dx1    = xback ? x2 : x1;
        coord      dx2    = xback ? x1 : x2;
        coord      dy1    = yback ? y2 : y1;
        coord      sl     = x;
        coord      sr     = sl + x2 - x1;
        coord      st     = y;
        coord      sb     = st + y2 - y1;
        coord      sx1    = xback ? sr : sl;
        coord      sy1    = yback ? sb : st;
        coord      ycount = y2 - y1;

        // Offset in words for a displacement along Y
        offset     dyoff  = dst.pixel_offset(0, ydir);
        offset     syoff  = skip_src ? 0 : src.pixel_offset(0, ydir);
        size       swidth = src.width;
        unsigned   sslant = skip_src ? 0 : src.pixel_shift(swidth, 0);
        size       dwidth = dst.width;
        unsigned   dslant = dst.pixel_shift(dwidth, 0);

        // Pointers to word containing start and end pixel
        pixword   *dp1    = dst.pixel_address(dx1, dy1);
        pixword   *dp2    = dst.pixel_address(dx2, dy1);
        pixword   *sp     = skip_src ? dp1 : src.pixel_address(sx1, sy1);

        // X1 and x2 pixel shift
        unsigned   dls    = dst.pixel_shift(x1, dy1);
        unsigned   drs    = dst.pixel_shift(x2, dy1);
        unsigned   dws    = xback ? drs : dls;
        unsigned   sls    = src.pixel_shift(sl, sy1);
        unsigned   srs    = src.pixel_shift(sr, sy1);
        unsigned   sws    = xback ? srs : sls;
        const size bpw    = BITS_PER_WORD;
        unsigned   cshift  = (cbpp == 16  ? 48
                              : cbpp == 4 ? 20
                              : cbpp == 1 ? 9
                              : 0);
        unsigned   cys    = ydir * (int) cshift;
        unsigned   cxs    = xdir * bpw * cbpp / dbpp;

        // Shift adjustment from source to destinaation
        unsigned   sadj   = (int) (sws * dbpp - dws * sbpp) / (int) dbpp;
        unsigned   sxadj  = xdir * (int) (sbpp * bpw / dbpp);

        // X1 and x2 masks
        pixword    ones   = ~0U;
        pixword    lmask  = ones << dls;
        pixword    rmask  = shrc(ones, drs + dbpp);
        pixword    dmask1 = xback ? rmask : lmask;
        pixword    dmask2 = xback ? lmask : rmask;

        // Adjust the color pattern based on starting point
        uint64_t   cdata64 = colors.bits;
        if (!skip_col)
            cdata64 = colors.rotate(dx1 * cbpp + dy1 * cshift - dws);

        while (ycount-- >= 0)
        {
            uint64_t csave    = cdata64;
            unsigned sadjsave = sadj;
            pixword *ssave    = sp;
            pixword *dp       = dp1;
            pixword  dmask    = dmask1;
            int      xdone    = 0;
            pixword  smem     = sp[0];
            pixword  snew     = smem;
            pixword  sdata    = 0;
            pixword  cdata    = 0;

            if (xback)
                sadj -= sxadj;

            do
            {
                xdone = dp == dp2;
                if (xdone)
                    dmask &= dmask2;

                if (!skip_src)
                {
                    unsigned nextsadj = sadj + sxadj;

                    // Check if we change source word
                    int skip = nextsadj >= bpw;
                    if (skip)
                    {
                        sp += xdir;
                        smem = snew;
                        snew = sp[0];
                    }

                    nextsadj %= bpw;
                    sadj %= bpw;
                    if (sadj)
                        if (xback)
                            sdata = shlc(smem, nextsadj) | shr(snew, nextsadj);
                        else
                            sdata = shlc(snew, sadj) | shr(smem, sadj);
                    else
                        sdata = xback ? snew : smem;

                    sadj = nextsadj;
                }
                if (!skip_col)
                {
                    cdata   = cdata64;
                    cdata64 = colors.rotate(cxs);
                }

                pixword ddata = dp[0];
                pixword tdata = op(ddata, sdata, cdata);
                *dp           = (tdata & dmask) | (ddata & ~dmask);

                dp += xdir;
                dmask = ~0U;
                smem = snew;
            } while (!xdone);

            // Move to next line
            if (dslant)
            {
                dy1 += ydir;
                dp1     = pixel_address(dst, dbpp, dx1, dy1);
                dp2     = pixel_address(dst, dbpp, dx2, dy1);
                dls     = pixel_shift(dst, dbpp, x1, dy1);
                drs     = pixel_shift(dst, dbpp, x2, dy1);
                dws     = xback ? drs : dls;
                lmask   = ones << dls;
                rmask   = shrc(ones, drs + dbpp);
                dmask1  = xback ? rmask : lmask;
                dmask2  = xback ? lmask : rmask;
                cdata64 = colors.rotate(dx1 * cbpp + dy1 * cshift - dws);
            }
            else
            {
                dp1 += dyoff;
                dp2 += dyoff;
                if (!skip_col)
                    cdata64 = colors.rotate(csave, cys);
            }

            // Check if we can directly move to next line
            if (sslant || dslant)
            {
                sy1 += ydir;
                sp   = pixel_address(src, sbpp, sx1, sy1);
                sls  = pixel_shift(src, sbpp, sl, sy1);
                srs  = pixel_shift(src, sbpp, sr, sy1);
                sws  = xback ? srs : sls;
                sadj = (int) (sws * dbpp - dws * sbpp) / (int) dbpp;
            }
            else
            {
                sp   = ssave + syoff;
                sadj = sadjsave;
            }
        }
    }


    struct surface
    // -------------------------------------------------------------------------
    //   Structure representing a surface on screen
    // -------------------------------------------------------------------------
    {
        surface(pixword *p, size w, size h)
            : pixels(p), width(w), height(h), drawable(w, h) {}
        surface(const surface &o) = default;

        // Operations used by the blitting routine
        enum { BPP = BITS_PER_PIXEL, BPW = BITS_PER_WORD };

    public:
        void clip(const rect &r)
        // --------------------------------------------------------------------
        //    Limit drawing to the given rectangle
        // --------------------------------------------------------------------
        {
            drawable = r;
            drawable &= rect(width, height);
        }

        void clip(coord x1, coord y1, coord x2, coord y2)
        // --------------------------------------------------------------------
        //   Clip an area given in coordinates
        // --------------------------------------------------------------------
        {
            clip(rect(x1, y1, x2, y2));
        }

        const rect &clip()
        // --------------------------------------------------------------------
        //   Return the clipping area
        // --------------------------------------------------------------------
        {
            return drawable;
        }


        void fill(const rect &r,
                  pattern     colors = pattern::black,
                  bool        clip   = true)
        // --------------------------------------------------------------------
        //   Fill a rectangle
        // --------------------------------------------------------------------
        {
            if (clip)
                blit<FILL_SAFE>(*this, *this, r, point(), blitop_set, colors);
            else
                blit<FILL_QUICK>(*this, *this, r, point(), blitop_set, colors);
        }

        void fill(coord x1, coord y1, coord x2, coord y2,
                  pattern colors = pattern::black,
                  bool clip = true)
        // --------------------------------------------------------------------
        //   Fill a rectangle with a color pattern
        // --------------------------------------------------------------------
        {
            fill(rect(x1, y1, x2, y2), colors, clip);
        }

        void fill(pattern colors = pattern::black, bool clip = true)
        // --------------------------------------------------------------------
        //   Fill the entire area with the chosen color
        // --------------------------------------------------------------------
        {
            fill(drawable, colors, clip);
        }

        void copy(surface &src, const rect &r, const point &spos = point(0,0))
        // --------------------------------------------------------------------
        //   Copy a rectangular area from the soruce
        // --------------------------------------------------------------------
        {
            pattern clear = pattern::black;
            blit(*this, src, r, spos, blitop_source, clear, COPY);
        }


    protected:

        offset pixel_offset(coord x, coord y)
        // ---------------------------------------------------------------------
        //   Offset in words in a given surface for the given coordinates
        // ---------------------------------------------------------------------
        {
            return ((offset) width * y + x) * (offset) BPP / BPW;
        }

        offset pixel_shift(coord x, coord y)
        // ---------------------------------------------------------------------
        //   Shift in bits in the word for the given coordinates
        // ---------------------------------------------------------------------
        {
            return ((offset) width * y + x) * (offset) BPP % BPW;
        }

        pixword *pixel_address(coord x, coord y)
        // ---------------------------------------------------------------------
        //   Get the address of a word representing the pixel in a surface
        // ---------------------------------------------------------------------
        {
            return pixels + pixel_offset(x, y);
        }

    protected:
        friend struct graphics;
        pixword *pixels;        // Word-aligned address of surface buffer
        size     width;         // Pixel width of buffer
        size     height;        // Pixel height of buffer
        rect     drawable;      // Draw area (clipping outside)
    };


protected:
    // ========================================================================
    //
    //   Helper routines
    //
    // ========================================================================

    static pixword shl(pixword value, unsigned shift)
    // -------------------------------------------------------------------------
    //   Shift right guaranteeing a zero for a large shift (even on x86)
    // -------------------------------------------------------------------------
    {
        return shift < BITS_PER_WORD ? value << shift : 0;
    }


    static pixword shr(pixword value, unsigned shift)
    // -------------------------------------------------------------------------
    //   Shift right guaranteeing a zero for a large shift (even on x86)
    // -------------------------------------------------------------------------
    {
        return shift < BITS_PER_WORD ? value >> shift : 0;
    }


    static pixword shlc(pixword value, unsigned shift)
    // -------------------------------------------------------------------------
    //   Shift x1 by the complement of the given shift
    // -------------------------------------------------------------------------
    {
        return shl(value, BITS_PER_WORD - shift);
    }


    static pixword shrc(pixword value, unsigned shift)
    // -------------------------------------------------------------------------
    //   Shift x2 by the complement of the given shift
    // -------------------------------------------------------------------------
    {
        return shr(value, BITS_PER_WORD - shift);
    }


    // =========================================================================
    //
    //   Operators for blit
    //
    // =========================================================================

    static pixword blitop_set(pixword UNUSED dst,
                              pixword UNUSED src,
                              pixword        arg)
    // -------------------------------------------------------------------------
    //   This simply sets the color passed in arg
    // -------------------------------------------------------------------------
    {
        return arg;
    }

    static pixword blitop_source(pixword UNUSED dst,
                                 pixword        src,
                                 pixword UNUSED arg)
    // -------------------------------------------------------------------------
    //   This simly sets the color from the source
    // -------------------------------------------------------------------------
    {
        return src;
    }

    static pixword blitop_mono_fg(pixword dst, pixword src, pixword arg)
    // -------------------------------------------------------------------------
    //   Bitmap foreground colorization (1bpp destination)
    // -------------------------------------------------------------------------
    {
        // For 1bpp, 'arg' is simply a bit mask from the source
        return (dst & ~src) | (arg & src);
    }


    static pixword blitop_mono_bg(pixword dst, pixword src, pixword arg)
    // -------------------------------------------------------------------------
    //   Bitmap baground colorization (1bpp destination)
    // -------------------------------------------------------------------------
    {
        return blitop_mono_fg(dst, ~src, arg);
    }


    static pixword blitop_invert(pixword dst, pixword src, pixword arg)
    // -------------------------------------------------------------------------
    //   Inverting colors can always be achieved with a simple xor
    // -------------------------------------------------------------------------
    {
        dst = src ^ arg;
        return dst;
    }


public:
    // =========================================================================
    //
    //    Core blitting routine
    //
    // =========================================================================




};


#if 0 // FUTURE
// ============================================================================
//
//   Specialization for 4-BPP graphics (HP50G)
//
// ============================================================================

template<> union graphics<4>::color
// ----------------------------------------------------------------------------
//   4-bit color representation (e.g. grayscales on HP50 and the like)
// ----------------------------------------------------------------------------
{
    color(uint8_t v): value(v) {}
    struct nibbles
    {
        uint8_t low  : 4;
        uint8_t high : 4;
    } __attribute__((packed)) nibbles;
    uint8_t value;
};


template<> uint64_t graphics<4>::pattern::multiplier()
// ----------------------------------------------------------------------------
//   For 4-bpp, we have one color per nibble
// ----------------------------------------------------------------------------
{
    return 0x1111111111111111ull;
}


template<> graphics<4>::pattern::pattern(uint8_t r, uint8_t g, uint8_t b)
// ----------------------------------------------------------------------------
//   Create a grayscale pattern from RGB values
// ----------------------------------------------------------------------------
{
    uint16_t gray = (r + g + g + b) / 4;

    // On the HP48, 0xF is black, not white
    bits = (0xF - gray / 16) * multiplier();
}

template<>
pixword graphics<4>::blitop_mono_fg(pixword dst, pixword src, pixword arg)
// ----------------------------------------------------------------------------
//   Bitmap foreground colorization (4bpp destination)
// ----------------------------------------------------------------------------
{
    // Expand the 8 bits from the source into a 32-bit mask
    pixword mask = 0;
    for (unsigned shift = 0; shift < 8; shift++)
        if (src & (1 << shift))
            mask |= 0xF << (4 * shift);
    return (dst & ~mask) | (arg & mask);
}


template<>
pixword graphics<4>::blitop_mono_bg(pixword dst, pixword src, pixword arg)
// ----------------------------------------------------------------------------
//   Bitmap background colorization (4bpp destination)
// ----------------------------------------------------------------------------
{
    return blitop_mono_fg(dst, ~src, arg);
}


// ============================================================================
//
//   Specialization for 4-BPP graphics (HP50G) and 16-BPP graphics (HP Prime)
//
// ============================================================================

template<> union graphics<16>::color
// ----------------------------------------------------------------------------
//   16-bit color representation, e.g. Prime1
// ----------------------------------------------------------------------------
{
    // Build a color from normalized RGB values
    color(uint8_t red, uint8_t green, uint8_t blue)
        : red(red >> 3), green(green >> 2), blue(blue >> 3)
    {}


    struct rgb16
    {
        uint8_t blue  : 5;
        uint8_t green : 6;
        uint8_t red   : 5;
    } __attribute__((packed)) rgb16;
    uint16_t value;
};


template<> uint64_t graphics<4>::pattern::multiplier()
// ----------------------------------------------------------------------------
//   For 16-bpp, we have one color per 16-bit word
// ----------------------------------------------------------------------------
{
    return 0x0001000100010001ull;
}

template<> graphics<4>::pattern::pattern(uint8_t r, uint8_t g, uint8_t b)
// ----------------------------------------------------------------------------
//   Create a grayscale pattern from RGB values
// ----------------------------------------------------------------------------
{
    color c(r, g, b);
    bits = c.value * multiplier();
}

template<>
pixword graphics<4>::blitop_mono_fg(pixword dst, pixword src, pixword arg)
// ----------------------------------------------------------------------------
//   Bitmap foreground colorization (16bpp destination)
// ----------------------------------------------------------------------------
{
    // Expand the low 2 bits from the source into a 32-bit mask
    pixword mask = 0;
    for (unsigned shift = 0; shift < 2; shift++)
        if (src & (1 << shift))
            mask |= 0xFFFF << (16 * shift);
    return (dst & ~mask) | (arg & mask);
}


template<>
pixword graphics<4>::blitop_mono_bg(pixword dst, pixword src, pixword arg)
// ----------------------------------------------------------------------------
//   Bitmap background colorization (16bpp destination)
// ----------------------------------------------------------------------------
{
    return blitop_mono_fg_16bpp(dst, ~src, arg);
}
#endif // 0

#endif // GRAPHICS_H
