#ifndef LEB128_H
#define LEB128_H
// ****************************************************************************
//  leb128.h                                                      DB48X project
// ****************************************************************************
//
//   File Description:
//
//     Operations on LEB128-encoded data
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

inline uint leb128(void *&p)
// ----------------------------------------------------------------------------
//   Return the leb128 value at pointer
// ----------------------------------------------------------------------------
{
    byte *bp = (byte *) p;
    uint result = 0;
    do
    {
        result = (result << 7) | (*bp & 0x7F);
    } while (*bp++ & 0x80);
    p = (void *) bp;
    return result;
}

#endif // LEB128_H
