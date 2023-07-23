#ifndef ARRAY_H
#define ARRAY_H
// ****************************************************************************
//  array.h                                                       DB48X project
// ****************************************************************************
//
//   File Description:
//
//     Implementation of arrays (vectors, matrices and maybe tensors)
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

#include "list.h"

GCP(array);

struct array : list
// ----------------------------------------------------------------------------
//   An array is a list with [ and ] as delimiters
// ----------------------------------------------------------------------------
{
    array(gcbytes bytes, size_t len, id type = ID_array)
        : list(bytes, len, type) {}



public:
    OBJECT_DECL(array);
    PARSE_DECL(array);
    RENDER_DECL(array);
};


array_g operator-(array_r x);
array_g operator+(array_r x, array_r y);
array_g operator-(array_r x, array_r y);
array_g operator*(array_r x, array_r y);
array_g operator/(array_r x, array_r y);

#endif // ARRAY_H
