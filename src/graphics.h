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
//     These routines are designed to be highly-optimizable while being able
//     to deal with 1, 4 and 16 bits per pixel as found on various calculators
//     To achieve that objective, the code is parameterized at compile-time,
//     so it makes relatively heavy use of templates and inlining.
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
//
//   In the code, BPP stands for "Bits per pixels", and BPW for "Bits per word"
//   Pixel buffer words are assumed to be 32-bit as on most calculators today.
//

#include "types.h"

#define PACKED  __attribute__((packed))


struct graphics
// ----------------------------------------------------------------------------
//    The graphics class managing bitmap operations
// ----------------------------------------------------------------------------
{
    // ========================================================================
    //
    //    Types and constants
    //
    // ========================================================================

    // Basic types
    typedef int16_t  coord;
    typedef uint16_t size;
    typedef size_t   offset;
    typedef uint32_t pixword;
    typedef uint16_t palette_index;
    typedef uint64_t pattern_bits;

    enum { BPW = 8 * sizeof(pixword) }; // Bits per pixword


    // ========================================================================
    //
    //    Color representation
    //
    // ========================================================================
    //  Colors have a generic RGB-based interface, even on monochrome systems
    //  like the DM42, or on grayscale systems like the HP50G.
    //  The code presently supports 1BPP, 4BPP and 16BPP colors
    //  32BPP would be trivial to add.

    template <uint BPP> union color
    // ------------------------------------------------------------------------
    //   The generic color representation
    // ------------------------------------------------------------------------
    {
        // The constructor takes red/green/blue values
        color(uint8_t red, uint8_t green, uint8_t blue);

        // Return the components
        uint8_t red();
        uint8_t green();
        uint8_t blue();
    };



    // ========================================================================
    //
    //   Pattern representation
    //
    // ========================================================================
    //   A pattern is a NxN set of pixels on screen, corresponding to a fixed
    //   number of bits. This is used to simulate gray scales on monochrome
    //   machines like the DM42, but can also create visual effects on grayscale
    //   or color systems.
    //   Patterns are presently always stored as a 64-bit value for efficient
    //   processing during drawing. For 1BPP, patterns represent 8x8 pixel,
    //   for 4BPP they represent 4x4 pixels, and for 16BPP 2x2 pixels.
    //   This means it is always possible to create two-color and four-color
    //   patterns in a way that is independent of the target system.
    //   A pattern can be built from RGB values, which will build a pattern
    //   with the corresponding pixel density on a monochrome system.

    template <uint BPP> union pattern
    // ------------------------------------------------------------------------
    //   Pattern representation for fills
    // ------------------------------------------------------------------------
    {
        uint64_t  bits;              // All patterns are represented as 64 bit

        enum { SIZE=1 };             // Size of the pattern
        enum:uint64_t { SOLID = 1 }; // Multiplier for solids
        using color = graphics::color<BPP>;
    public:
        // Build a solid pattern from a single color
        pattern(color c);

        // Build a checkered pattern for a given RGB level
        pattern(uint8_t  red, uint8_t  green, uint8_t  blue);

        // Build a checkerboard from 2 or 4 colors in an array
        template<uint N>
        pattern(color colors[N]);

        // Build a pattern with two or four alternating colors
        pattern(color a, color b);
        pattern(color a, color b, color c, color d);

        // Some pre-defined shades of gray
        static const pattern black;
        static const pattern gray25;
        static const pattern gray50;
        static const pattern gray75;
        static const pattern white;
    };




    // ========================================================================
    //
    //    Points and rectangles
    //
    // ========================================================================

    struct point
    // ------------------------------------------------------------------------
    //    A point holds a pair of coordinates
    // ------------------------------------------------------------------------
    {
        point(coord x = 0, coord y = 0): x(x), y(y) {}
        point(const point &o) = default;
        coord x, y;
    };


    struct rect
    // ------------------------------------------------------------------------
    //   A rectangle is a point with a width and a height
    // ------------------------------------------------------------------------
    {
        rect(coord x1 = 0, coord y1 = 0, coord x2 = 0, coord y2 = 0)
            : x1(x1), y1(y1), x2(x2), y2(y2) {}
        rect(size w, size h): rect(0, 0, w-1, h-1) {}
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

    // blitop: a blitting operation
    typedef pixword (*blitop)(pixword dst, pixword src, pixword arg);

    template <clipping Clip, typename Dst, typename Src, uint CBPP>
    static void blit(Dst          &dst,
                     const Src    &src,
                     const rect   &drect,
                     const point  &spos,
                     blitop        op,
                     pattern<CBPP> colors)
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
        bool       clip_src = Clip & CLIP_SRC;
        bool       clip_dst = Clip & CLIP_DST;
        bool       skip_src = Clip & SKIP_SOURCE;
        bool       skip_col = Clip & SKIP_COLOR;

        coord      x1       = drect.x1;
        coord      y1       = drect.y1;
        coord      x2       = drect.x2;
        coord      y2       = drect.y2;
        coord      x        = spos.x;
        coord      y        = spos.y;

        const uint SBPP     = Src::BPP;
        const uint DBPP     = Dst::BPP;

        if (clip_src)
        {
            if (x < src.drawable.x1)
            {
                x1 += src.drawable.x1 - x;
                x = src.drawable.x1;
            }
            if (x + x2 - x1 > src.drawable.x2)
                x2 = src.drawable.x2 - x + x1;
            if (y < src.drawable.y1)
            {
                y1 += src.drawable.y1 - y;
                y = src.drawable.y1;
            }
            if (y + y2 - y1 > src.drawable.y2)
                y2 = src.drawable.y2 - y + y1;
        }

        if (clip_dst)
        {
            // Clipping based on target
            if (x1 < dst.drawable.x1)
            {
                x += dst.drawable.x1 - x1;
                x1 = dst.drawable.x1;
            }
            if (x2 > dst.drawable.x2)
                x2 = dst.drawable.x2;
            if (y1 < dst.drawable.y1)
            {
                y += dst.drawable.y1 - y1;
                y1 = dst.drawable.y1;
            }
            if (y2 > dst.drawable.y2)
                y2 = dst.drawable.y2;
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
        unsigned   cshift  = (CBPP == 16  ? 48
                              : CBPP == 4 ? 20
                              : CBPP == 1 ? 9
                              : 0);
        unsigned   cys    = ydir * (int) cshift;
        unsigned   cxs    = xdir * BPW * CBPP / DBPP;

        // Shift adjustment from source to destinaation
        unsigned   sadj   = (int) (sws * DBPP - dws * SBPP) / (int) DBPP;
        unsigned   sxadj  = xdir * (int) (SBPP * BPW / DBPP);

        // X1 and x2 masks
        pixword    ones   = ~0U;
        pixword    lmask  = ones << dls;
        pixword    rmask  = shrc(ones, drs + DBPP);
        pixword    dmask1 = xback ? rmask : lmask;
        pixword    dmask2 = xback ? lmask : rmask;

        // Adjust the color pattern based on starting point
        uint64_t   cdata64 = colors.bits;
        if (!skip_col)
            cdata64 = rotate(cdata64, dx1 * CBPP + dy1 * cshift - dws);

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
                    int skip = nextsadj >= BPW;
                    if (skip)
                    {
                        sp += xdir;
                        smem = snew;
                        snew = sp[0];
                    }

                    nextsadj %= BPW;
                    sadj %= BPW;
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
                    cdata64 = rotate(cdata64, cxs);
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
                dp1     = dst.pixel_address(dx1, dy1);
                dp2     = dst.pixel_address(dx2, dy1);
                dls     = dst.pixel_shift(x1, dy1);
                drs     = dst.pixel_shift(x2, dy1);
                dws     = xback ? drs : dls;
                lmask   = ones << dls;
                rmask   = shrc(ones, drs + DBPP);
                dmask1  = xback ? rmask : lmask;
                dmask2  = xback ? lmask : rmask;
                cdata64 = rotate(cdata64, dx1 * CBPP + dy1 * cshift - dws);
            }
            else
            {
                dp1 += dyoff;
                dp2 += dyoff;
                if (!skip_col)
                    cdata64 = rotate(csave, cys);
            }

            // Check if we can directly move to next line
            if (sslant || dslant)
            {
                sy1 += ydir;
                sp   = src.pixel_address(sx1, sy1);
                sls  = src.pixel_shift(sl, sy1);
                srs  = src.pixel_shift(sr, sy1);
                sws  = xback ? srs : sls;
                sadj = (int) (sws * DBPP - dws * SBPP) / (int) DBPP;
            }
            else
            {
                sp   = ssave + syoff;
                sadj = sadjsave;
            }
        }
    }


    // ========================================================================
    //
    //   Surface: a bitmap for graphic operations
    //
    // ========================================================================

    template<uint BITS_PER_PIXEL>
    struct surface
    // -------------------------------------------------------------------------
    //   Structure representing a surface on screen
    // -------------------------------------------------------------------------
    {
        surface(pixword *p, size w, size h, size scanline)
            : pixels(p),
              width(w), height(h), scanline(scanline),
              drawable(w, h) {}
        surface(pixword *p, size w, size h) : surface(p, w, h, w) {}
        surface(const surface &o) = default;

        // Operations used by the blitting routine
        enum { BPP = BITS_PER_PIXEL };

        using color   = graphics::color<BPP>;
        using pattern = graphics::pattern<BPP>;

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

        template<clipping Clip = FILL_SAFE>
        void fill(const rect &r, pattern colors = pattern::black)
        // --------------------------------------------------------------------
        //   Fill a rectangle
        // --------------------------------------------------------------------
        {
            blit<Clip>(*this, *this, r, point(), blitop_set, colors);
        }

        template<clipping Clip = FILL_SAFE>
        void fill(coord x1, coord y1, coord x2, coord y2,
                  pattern colors = pattern::black)
        // --------------------------------------------------------------------
        //   Fill a rectangle with a color pattern
        // --------------------------------------------------------------------
        {
            fill<Clip>(rect(x1, y1, x2, y2), colors);
        }

        template<clipping Clip = FILL_SAFE>
        void fill(pattern colors = pattern::black)
        // --------------------------------------------------------------------
        //   Fill the entire area with the chosen color
        // --------------------------------------------------------------------
        {
            fill<Clip>(drawable, colors);
        }

        template<clipping Clip = COPY>
        void copy(surface &src, const rect &r,
                  const point &spos = point(0,0),
                  pattern clear = pattern::black)
        // --------------------------------------------------------------------
        //   Copy a rectangular area from the source
        // --------------------------------------------------------------------
        {
            blit<Clip>(*this, src, r, spos, blitop_source, clear);
        }


    protected:
        offset pixel_offset(coord x, coord y) const
        // ---------------------------------------------------------------------
        //   Offset in words in a given surface for the given coordinates
        // ---------------------------------------------------------------------
        {
            return ((offset) scanline * y + x) * (offset) BPP / BPW;
        }

        offset pixel_shift(coord x, coord y) const
        // ---------------------------------------------------------------------
        //   Shift in bits in the word for the given coordinates
        // ---------------------------------------------------------------------
        {
            return ((offset) scanline * y + x) * (offset) BPP % BPW;
        }

        pixword *pixel_address(coord x, coord y) const
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
        size     scanline;      // Scanline for the buffer (can be > width)
        rect     drawable;      // Draw area (clipping outside)
    };


protected:
    // ========================================================================
    //
    //   Helper routines
    //
    // ========================================================================

    static inline pixword shl(pixword value, unsigned shift)
    // -------------------------------------------------------------------------
    //   Shift right guaranteeing a zero for a large shift (even on x86)
    // -------------------------------------------------------------------------
    {
        return shift < BPW ? value << shift : 0;
    }


    static inline pixword shr(pixword value, unsigned shift)
    // -------------------------------------------------------------------------
    //   Shift right guaranteeing a zero for a large shift (even on x86)
    // -------------------------------------------------------------------------
    {
        return shift < BPW ? value >> shift : 0;
    }


    static inline pixword shlc(pixword value, unsigned shift)
    // -------------------------------------------------------------------------
    //   Shift x1 by the complement of the given shift
    // -------------------------------------------------------------------------
    {
        return shl(value, BPW - shift);
    }


    static pixword shrc(pixword value, unsigned shift)
    // -------------------------------------------------------------------------
    //   Shift x2 by the complement of the given shift
    // -------------------------------------------------------------------------
    {
        return shr(value, BPW - shift);
    }


    template <typename Int>
    static inline uint64_t rotate(Int bits, uint shift)
    // --------------------------------------------------------------------
    //   Rotate a 64-bit pattern by the given amount
    // --------------------------------------------------------------------
    {
        enum { BIT_SIZE = sizeof(bits) * 8 };
        shift %= BIT_SIZE;
        bits = ((bits >> shift) | (bits << (BIT_SIZE - shift)));
        return bits;
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

    template<uint BPP>
    static pixword blitop_mono_fg(pixword dst, pixword src, pixword arg);
    // ------------------------------------------------------------------------
    //   1-BPP to N-BPP foreground color conversion (used for text drawing)
    // ------------------------------------------------------------------------



    template<uint BPP>
    static pixword blitop_mono_bg(pixword dst, pixword src, pixword arg)
    // -------------------------------------------------------------------------
    //   Bitmap baground colorization (1bpp destination)
    // -------------------------------------------------------------------------
    {
        return blitop_mono_fg<BPP>(dst, ~src, arg);
    }


    static pixword blitop_invert(pixword dst, pixword src, pixword arg)
    // -------------------------------------------------------------------------
    //   Inverting colors can always be achieved with a simple xor
    // -------------------------------------------------------------------------
    {
        dst = src ^ arg;
        return dst;
    }
};


// ============================================================================
//
//   Color template specializations for various BPP
//
// ============================================================================

template<> union graphics::color<1>
// ------------------------------------------------------------------------
//  Color representation (1-bit, e.g. DM42)
// ------------------------------------------------------------------------
//  On the DM42. white is 0 and black is 1
{
    color(uint8_t red, uint8_t green, uint8_t blue):
        value((red + green + green + blue) / 4 < 128) {}

    uint8_t red()          { return !value * 255; }
    uint8_t green()        { return !value * 255; }
    uint8_t blue()         { return !value * 255; }

public:
    bool value : 1;         // The color value is 0 or 1
};


template<> union graphics::color<4>
// ------------------------------------------------------------------------
//  Color representation (4-bit, e.g. HP50G)
// ------------------------------------------------------------------------
//  On the HP50G, 0xF is black, 0x0 is white
{
    color(uint8_t  red, uint8_t green, uint8_t   blue):
        value(0xF - (red + green + green + blue) / 64) {}

    uint8_t  red()              { return (0xF - value) * 0x11; }
    uint8_t  green()            { return red(); }
    uint8_t  blue()             { return red(); }

public:
    uint8_t value : 4;
};


template<> union graphics::color<16>
// ------------------------------------------------------------------------
//  Color representation (16-bit, e.g. HP Prime)
// ------------------------------------------------------------------------
{
    struct rgb16
    {
        rgb16(uint8_t  red, uint8_t  green, uint8_t  blue)
            : blue(blue), green(green), red(red) {}
        uint8_t blue  : 5;
        uint8_t green : 6;
        uint8_t red   : 5;
    } PACKED rgb16;
    uint16_t value    : 16;

public:
    // Build a color from normalized RGB values
    color(uint8_t red, uint8_t green, uint8_t blue)
        : rgb16(red >> 3, green >> 2, blue >> 3) {}

    uint8_t  red()              { return rgb16.blue << 3;  }
    uint8_t  green()            { return rgb16.red << 2;   }
    uint8_t  blue()             { return rgb16.green << 3; }
} PACKED;


// ============================================================================
//
//   Pattern template specializations for various BPP
//
// ============================================================================

template<uint BPP> template<uint N>
inline graphics::pattern<BPP>::pattern(graphics::color<BPP> colors[N])
// ------------------------------------------------------------------------
//  Build a checkerboard from 2 or 4 colors in an array
// ------------------------------------------------------------------------
{
    for (uint shift = 0; shift < 64 / BPP; shift++)
    {
        uint index = (shift + ((shift / SIZE) % N)) % N;
        bits |= uint64_t(colors[index].value) << (shift * BPP);
    }
}

template<uint BPP>
inline graphics::pattern<BPP>::pattern(color a, color b)
// ------------------------------------------------------------------------
//   Build a pattern from two colors
// ------------------------------------------------------------------------
//   Is there any way to use delegating constructors here?
{
    color colors[2] = { a, b };
    *this = pattern(colors);
}

template<uint BPP>
inline graphics::pattern<BPP>::pattern(color a, color b, color c, color d)
// ------------------------------------------------------------------------
//   Build a pattern from four colors
// ------------------------------------------------------------------------
{
    color colors[4] = { a, b, c, d };
    *this = pattern(colors);
}

// Pre-built patterns for five shades of grey
#define GPAT      graphics::pattern<BPP>
template<uint BPP> const GPAT GPAT::black  = GPAT(  0,   0,   0);
template<uint BPP> const GPAT GPAT::gray25 = GPAT( 64,  64,  64);
template<uint BPP> const GPAT GPAT::gray50 = GPAT(128, 128, 128);
template<uint BPP> const GPAT GPAT::gray75 = GPAT(192, 192, 192);
template<uint BPP> const GPAT GPAT::white  = GPAT(255, 255, 255);
#undef GPAT


template <> union graphics::pattern<1>
// ------------------------------------------------------------------------
//   Pattern for 1-bit screens (DM42)
// ------------------------------------------------------------------------
{
    uint64_t  bits;

    enum              { BPP = 1, SIZE = 8 }; // 64-bit = 8x8 1-bit pattern
    enum:uint64_t     { SOLID = 0xFFFFFFFFFFFFFFFFull };
    using color = graphics::color<1>;

public:
    // Build a solid pattern from a single color
    pattern(color c) : bits(c.value * SOLID) {}

    // Build a checkered pattern for a given RGB level
    pattern(uint8_t  red, uint8_t  green, uint8_t  blue ) : bits(0)
    {
        // Compute a gray value beteen 0 and 64, number of pixels to fill
        uint16_t gray = (red + green + green + blue + 4) / 16;
        if (gray == 32)             // Hand tweak 50% gray
            bits = 0xAAAAAAAAAAAAAAAAull;
        else
            // Generate a pattern with "gray" bits lit "at random"
            for (int bit = 0; bit < 64 && gray; bit++, gray--)
                bits |= 1ULL << (79 * bit % 64);
    }

    // Shared constructors
    template<uint N> pattern(color colors[N]);
    pattern(color a, color b);
    pattern(color a, color b, color c, color d);

    // Some pre-defined shades of gray
    static const pattern black;
    static const pattern gray25;
    static const pattern gray50;
    static const pattern gray75;
    static const pattern white;
};


template <> union graphics::pattern<4>
// ------------------------------------------------------------------------
//   Pattern for 4-bit screens (HP50G)
// ------------------------------------------------------------------------
{
    uint64_t  bits;

    enum              { BPP = 4, SIZE = 4 }; // 64-bit = 4x4 4-bit pattern
    enum:uint64_t     { SOLID = 0x1111111111111111ull };
    using color = graphics::color<4>;

  public:
    // Build a solid pattern from a single color
    pattern(color c) : bits(c.value * SOLID) {}

    // Build a checkered pattern for a given RGB level
    pattern(uint8_t  red, uint8_t  green, uint8_t  blue ) : bits(0)
    {
        // Compute a gray value beteen 0 and 15
        uint16_t gray = (red + green + green + blue + 4) / 64;
        bits = SOLID * (0xF - gray);
    }

    // Shared constructors
    template<uint N> pattern(color colors[N]);
    pattern(color a, color b);
    pattern(color a, color b, color c, color d);

    // Some pre-defined shades of gray
    static const pattern black;
    static const pattern gray25;
    static const pattern gray50;
    static const pattern gray75;
    static const pattern white;
};


template <> union graphics::pattern<16>
// ------------------------------------------------------------------------
//   Pattern for 16-bit screens (HP Prime)
// ------------------------------------------------------------------------
{
    uint64_t  bits;

    enum              { BPP = 16, SIZE = 2 }; // 64-bit = 4x4 4-bit pattern
    enum:uint64_t     { SOLID = 0x0001000100010001ull };
    using color = graphics::color<16>;

public:
    // Build a solid pattern from a single color
    pattern(color c) : bits(c.value * SOLID) {}

    // Build a checkered pattern for a given RGB level
    pattern(uint8_t  red, uint8_t  green, uint8_t  blue ) : bits(0)
    {
        // Compute a gray value beteen 0 and 15
        color c(red, green, blue);
        bits = SOLID * c.value;
    }

    // Shared constructors
    template<uint N> pattern(color colors[N]);
    pattern(color a, color b);
    pattern(color a, color b, color c, color d);

    // Some pre-defined shades of gray
    static const pattern black;
    static const pattern gray25;
    static const pattern gray50;
    static const pattern gray75;
    static const pattern white;
};



// ============================================================================
//
//   Template specializations for blitops
//
// ============================================================================


template <>
inline graphics::pixword graphics::blitop_mono_fg<1>(graphics::pixword dst,
                                                     graphics::pixword src,
                                                     graphics::pixword arg)
// -------------------------------------------------------------------------
//   Bitmap foreground colorization (1bpp destination)
// -------------------------------------------------------------------------
{
    // For 1bpp, 'arg' is simply a bit mask from the source
    return (dst & ~src) | (arg & src);
}

template <>
inline graphics::pixword graphics::blitop_mono_fg<4>(graphics::pixword dst,
                                                     graphics::pixword src,
                                                     graphics::pixword arg)
// -------------------------------------------------------------------------
//   Bitmap foreground colorization (4bpp destination)
// -------------------------------------------------------------------------
{
    // Expand the 8 bits from the source into a 32-bit mask
    pixword mask = 0;
    for (unsigned shift = 0; shift < 8; shift++)
        if (src & (1 << shift))
            mask |= 0xF << (4 * shift);
    return (dst & ~mask) | (arg & mask);
}

template <>
inline graphics::pixword graphics::blitop_mono_fg<16>(graphics::pixword dst,
                                                      graphics::pixword src,
                                                      graphics::pixword arg)
// -------------------------------------------------------------------------
//   Bitmap foreground colorization (16bpp destination)
// -------------------------------------------------------------------------
{
    // Expand the low 2 bits from the source into a 32-bit mask
    pixword mask = 0;
    for (unsigned shift = 0; shift < 2; shift++)
        if (src & (1 << shift))
            mask |= 0xFFFF << (16 * shift);
    return (dst & ~mask) | (arg & mask);
}

#endif // GRAPHICS_H
