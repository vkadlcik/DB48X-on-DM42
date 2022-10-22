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
{
};

struct sparse_font : font
{
};

struct dense_font : font
{
};

#endif // FONT_H
