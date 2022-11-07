#ifndef UTF8_H
#define UTF8_H
// ****************************************************************************
//  utf8.h                                                        DB48X project
// ****************************************************************************
//
//   File Description:
//
//    Simple utilities to manipulate UTF-8 text
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

#include <ctype.h>
#include <string.h>


inline bool is_utf8_first(byte b)
// ----------------------------------------------------------------------------
//   Check if this is the first byte in a UTF-8 sequence
// ----------------------------------------------------------------------------
{
    return b >= 0xC0 && b <= 0xFD;
}


inline bool is_utf8_next(byte b)
// ----------------------------------------------------------------------------
//   Check if this is a follow-up byte in a UTF-8 sequence
// ----------------------------------------------------------------------------
{
    return b >= 0x80 && b <= 0xBF;
}


inline bool is_utf8_or_alpha(char c)
// ----------------------------------------------------------------------------
//   When splitting words, we arbitrarily take any UTF8 as being "alpha"...
// ----------------------------------------------------------------------------
{
    return isalpha(c) || is_utf8_first(c) || is_utf8_next(c);
}


inline uint utf8_previous(utf8 text, uint position)
// ----------------------------------------------------------------------------
//   Finds the previous position in the text, assumed to be UTF-8
// ----------------------------------------------------------------------------
{
    if (position > 0)
    {
        position--;
        while (position > 0 && is_utf8_next(text[position]))
            position--;
    }
    return position;
}


inline utf8 utf8_previous(utf8 text)
// ----------------------------------------------------------------------------
//   Finds the previous position in the text, assumed to be UTF-8
// ----------------------------------------------------------------------------
{
    do
        text--;
    while (is_utf8_next(*text));
    return text;
}


inline uint utf8_next(utf8 text, uint position, size_t len)
// ----------------------------------------------------------------------------
//   Find the next position in the text, assumed to be UTF-8
// ----------------------------------------------------------------------------
{
    if (position < len)
    {
        position++;
        while (position < len && is_utf8_next(text[position]))
            position++;
    }
    return position;
}


inline uint utf8_next(utf8 text, uint position)
// ----------------------------------------------------------------------------
//   Find the next position in the text, assumed to be UTF-8
// ----------------------------------------------------------------------------
{
    return utf8_next(text, position, strlen(cstring(text)));
}


inline utf8 utf8_next(utf8 text)
// ----------------------------------------------------------------------------
//   Find the next position in the text, assumed to be UTF-8
// ----------------------------------------------------------------------------
{
    text++;
    while (*text && is_utf8_next(*text))
        text++;
    return text;
}


inline unicode utf8_codepoint(utf8 text, uint position, size_t len = 0)
// ----------------------------------------------------------------------------
//   Return the Unicode value for the character at the given position
// ----------------------------------------------------------------------------
{
    if (!len && text[len])
        len = strlen(cstring(text));

    unicode code = 0;
    if (position < len)
    {
        code = text[position];
        if (code & 0x80)
        {
            // Reference: Wikipedia UTF-8 description
            if ((code & 0xE0) == 0xC0 && position+1 < len)
                code = ((code & 0x1F)             << 6)
                    |  (text[position+1] & 0x3F);
            else if ((code & 0xF0) == 0xE0 && position + 2 < len)
                code = ((code & 0xF)              << 12)
                    |  ((text[position+1] & 0x3F) << 6)
                    |   (text[position+2] & 0x3F);
            else if ((code & 0xF8) == 0xF0 && position + 3 < len)
                code = ((code & 0xF)              << 18)
                    |  ((text[position+1] & 0x3F) << 12)
                    |  ((text[position+2] & 0x3F) << 6)
                    |   (text[position+3] & 0x3F);
        }
    }
    return code;
}


inline unicode utf8_codepoint(utf8 text)
// ----------------------------------------------------------------------------
//   Return the Unicode value for the character at the given position
// ----------------------------------------------------------------------------
{
    unicode code = *text;
    if (code & 0x80)
    {
        // Reference: Wikipedia UTF-8 description
        if ((code & 0xE0) == 0xC0 && text[1])
            code = ((code & 0x1F)    << 6)
                |  (text[1] & 0x3F);
        else if ((code & 0xF0) == 0xE0 && text[1] && text[2])
            code = ((code & 0xF)     << 12)
                |  ((text[1] & 0x3F) << 6)
                |   (text[2] & 0x3F);
        else if ((code & 0xF8) == 0xF0 && text[1] && text[2] && text[3])
            code = ((code & 0xF)     << 18)
                |  ((text[1] & 0x3F) << 12)
                |  ((text[2] & 0x3F) << 6)
                |   (text[3] & 0x3F);
    }
    return code;
}


inline size_t utf8_encode(unicode cp, byte buffer[4])
// ----------------------------------------------------------------------------
//   Encode the code point into the buffer, return number of bytes needed
// ----------------------------------------------------------------------------
{
    if (cp < 0x80)
    {
        buffer[0] = cp;
        return 1;
    }
    else if (cp <  0x800)
    {
        buffer[0] = (cp >> 6)         | 0xC0;
        buffer[1] = (cp & 0x3F)       | 0x80;
        return 2;
    }
    else if (cp <  0x10000)
    {
        buffer[0] =  (cp >> 12)         | 0xE0;
        buffer[1] = ((cp >>  6) & 0x3F) | 0x80;
        buffer[2] = ((cp >>  0) & 0x3F) | 0x80;
        return 3;
    }
    else
    {
        buffer[0] = ((cp >> 18) & 0x07) | 0xF0;
        buffer[1] = ((cp >> 12) & 0x3F) | 0x80;
        buffer[2] = ((cp >>  6) & 0x3F) | 0x80;
        buffer[3] = ((cp >>  0) & 0x3F) | 0x80;
        return 4;
    }
}


inline uint utf8_length(utf8 text)
// ----------------------------------------------------------------------------
//    Return the length of the text in Unicode characters
// ----------------------------------------------------------------------------
{
    uint result = 0;
    for (utf8 p = text; *p; p++)
        result += !is_utf8_next(*p);
    return result;
}

#endif // UTF8_H
