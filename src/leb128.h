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

#include <cstdint>

template <typename Data, typename Int = uint>
inline Int leb128(Data *&p)
// ----------------------------------------------------------------------------
//   Return the leb128 value at pointer
// ----------------------------------------------------------------------------
{
    byte    *bp     = (byte *) p;
    Int      result = 0;
    unsigned shift  = 0;
    bool     sign   = false;
    do
    {
        result |= (*bp & 0x7F) << shift;
        sign = *bp & 0x40;
        shift += 7;
    } while (*bp++ & 0x80);
    p = (Data *) bp;
    if (Int(-1) < Int(0) && sign)
        result |= Int(-1) << (shift - 1);
    return result;
}


template<typename Data, typename Int = uint>
inline Data *leb128(Data *p, Int value)
// ----------------------------------------------------------------------------
//   Write the LEB value at pointer
// ----------------------------------------------------------------------------
{
    byte *bp = (byte *) p;
    do
    {
        *bp++ = (value & 0x7F) | 0x80;
        value = Int(value >> 7);
    } while (value != 0 && (Int(-1) > Int(0) || value != Int(-1)));
    bp[-1] &= ~0x80;
    return (Data *) bp;
}


template<typename Int>
inline size_t leb128size(Int value)
// ----------------------------------------------------------------------------
//   Compute the size required for a given integer value
// ----------------------------------------------------------------------------
{
    size_t result = 0;
    do
    {
        value = Int(value >> 7);
        result++;
    } while (value && (Int(-1) > Int(0) || value != Int(-1)));
    return result;
}


template<typename Data>
inline size_t leb128size(Data *ptr)
// ----------------------------------------------------------------------------
//   Compute the size of an LEB128 value at pointer
// ----------------------------------------------------------------------------
{
    byte *s = (byte *) ptr;
    byte *p = s;
    do { } while (*p++ & 0x80);
    return p - s;
}

#endif // LEB128_H
