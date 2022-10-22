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

struct font : object
// ----------------------------------------------------------------------------
//   Shared by all font objects
// ----------------------------------------------------------------------------
{
    font(id type): object(type) { }

    byte *bitmap(uint codepoint, uint *x, uint *y, uint *w, uint *h, uint *adv);

    OBJECT_HANDLER(font);
    OBJECT_PARSER(font);
    OBJECT_RENDERER(font);
};


struct sparse_font : font
// ----------------------------------------------------------------------------
//   An object representing a sparse font (one bitmap per character)
// ----------------------------------------------------------------------------
{
    sparse_font(id type = ID_sparse_font): font(type) {}
    byte *bitmap(uint codepoint, uint *x, uint *y, uint *w, uint *h, uint *adv);
};


struct dense_font : font
// ----------------------------------------------------------------------------
//   An object representing a dense font (a single bitmap for all characters)
// ----------------------------------------------------------------------------
{
    dense_font(id type = ID_dense_font): font(type) {}
    byte *bitmap(uint codepoint, uint *x, uint *y, uint *w, uint *h, uint *adv);
};


struct dmcp_font : font
// ----------------------------------------------------------------------------
//   An object accessing the DMCP built-in fonts (and remapping to Unicode)
// ----------------------------------------------------------------------------
{
    dmcp_font(id type = ID_dense_font): font(type) {}
    byte *bitmap(uint codepoint, uint *x, uint *y, uint *w, uint *h, uint *adv);
};

#endif // FONT_H
