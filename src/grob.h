#ifndef GROB_H
#  define GROB_H
// ****************************************************************************
//  grob.h                                                        DB48X project
// ****************************************************************************
//
//   File Description:
//
//    Graphic objects, representing a bitmap in memory
//
//
//
//
//
//
//
//
// ****************************************************************************
//   (C) 2023 Christophe de Dinechin <christophe@dinechin.org>
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
#include "object.h"
#include "runtime.h"
#include "target.h"

GCP(grob);

struct grob : object
// ----------------------------------------------------------------------------
//   Representation of a graphic object
// ----------------------------------------------------------------------------
{
    using pixsize = blitter::size;

    grob(id type, pixsize w, pixsize h, gcbytes bits)
        : object(type)
    // ------------------------------------------------------------------------
    //   Graphic object constructor
    // ------------------------------------------------------------------------
    {
        byte *p = (byte *) payload();
        p = leb128(p, w);
        p = leb128(p, h);
        size_t datasize = (w + 7) / 8 * h;
        if (byte_p s = bits)
        {
            while (datasize--)
                *p++ = *s++;
        }
        else
        {
            while (datasize--)
                *p++ = 0;
        }
    }


    static size_t required_memory(id            type,
                                  pixsize       w,
                                  pixsize       h,
                                  gcutf8 UNUSED bytes)
    // ------------------------------------------------------------------------
    //   Compute required grob memory for the given parameters
    // ------------------------------------------------------------------------
    {
        size_t bodysize = bytesize(w, h);
        return leb128size(type) + bodysize;
    }


    static grob_p make(pixsize w, pixsize h, byte_p bits)
    // ------------------------------------------------------------------------
    //   Build a grob from the given parameters
    // ------------------------------------------------------------------------
    {
        return rt.make<grob>(w, h, bits);
    }


    static size_t bytesize(pixsize w, pixsize h)
    // ------------------------------------------------------------------------
    //   Compute the number of bytes required for a bitmap
    // ------------------------------------------------------------------------
    {
        size_t datasize = (w + 7) / 8 * h;
        return leb128size(w) + leb128size(h) + datasize;
    }


    pixsize width() const
    // ------------------------------------------------------------------------
    //   Return the width of a grob
    // ------------------------------------------------------------------------
    {
        byte_p p = payload();
        return leb128<pixsize>(p);
    }


    pixsize height() const
    // ------------------------------------------------------------------------
    //   Return the height of a grob
    // ------------------------------------------------------------------------
    {
        byte_p p = payload();
        p = leb128skip(p);      // Skip width
        return leb128<pixsize>(p);
    }


    byte_p pixels(pixsize *width, pixsize *height, size_t *datalen = 0) const
    // ------------------------------------------------------------------------
    //   Return the byte pointer to the data in the grob
    // ------------------------------------------------------------------------
    {
        byte_p  p   = payload();
        pixsize w   = leb128<pixsize>(p);
        pixsize h   = leb128<pixsize>(p);
        if (width)
            *width = w;
        if (height)
            *height = h;
        if (datalen)
            *datalen = (w + 7) / 8 * h;
        return p;
    }


    surface pixels() const
    // ------------------------------------------------------------------------
    //   Return a blitter surface for the grob
    // ------------------------------------------------------------------------
    {
        pixsize w = 0;
        pixsize h = 0;
        byte_p bitmap = pixels(&w, &h);
        return surface((pixword *) bitmap, w, h, (w+7)/8*8);
    }


public:
    OBJECT_DECL(grob);
    PARSE_DECL(grob);
    SIZE_DECL(grob);
    RENDER_DECL(grob);
};

#endif // GROB_H
