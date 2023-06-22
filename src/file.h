#ifndef FILE_H
#  define FILE_H
// ****************************************************************************
//  file.h                                                        DB48X project
// ****************************************************************************
//
//   File Description:
//
//    Abstract the DMCP zany filesystem interface
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

#include "dmcp.h"

#include "types.h"
#include <stdio.h>


struct file
// ----------------------------------------------------------------------------
//   Direct access to the help file
// ----------------------------------------------------------------------------
{
    file();
    ~file();

    void    open(cstring path);
    bool    valid();
    void    close();
    unicode get();
    unicode get(uint offset);
    void    seek(uint offset);
    unicode peek();
    uint    position();
    uint    find(unicode cp);
    uint    rfind(unicode cp);

  protected:
#if SIMULATOR
    FILE *data;
#else
    FIL data;
#endif
};



// ============================================================================
//
//   DMCP wrappers
//
// ============================================================================

#ifndef SIMULATOR
#define ftell(f)     f_tell(&f)
#define fseek(f,o,w) f_lseek(&f,o)
#define fclose(f)    f_close(&f)

static inline int fgetc(FIL &f)
// ----------------------------------------------------------------------------
//   Read one character from a file - Wrapper for DMCP filesystem
// ----------------------------------------------------------------------------
{
    UINT br                     = 0;
    char c                      = 0;
    if (f_read(&f, &c, 1, &br) != FR_OK || br != 1)
        return EOF;
    return c;
}
#endif                          // SIMULATOR




// ============================================================================
//
//    Inline functions for simple stuff
//
// ============================================================================

inline bool file::valid()
// ----------------------------------------------------------------------------
//    Return true if the input file is OK
// ----------------------------------------------------------------------------
{
#if SIMULATOR
    return data          != 0;
#else
    return f_size(&data) != 0;
#endif
}


inline void file::seek(uint off)
// ----------------------------------------------------------------------------
//    Move the read position in the data file
// ----------------------------------------------------------------------------
{
    fseek(data, off, SEEK_SET);
}


inline unicode file::peek()
// ----------------------------------------------------------------------------
//    Look at what is as current position without moving it
// ----------------------------------------------------------------------------
{
    uint off       = ftell(data);
    unicode result = get();
    seek(off);
    return result;
}


inline unicode file::get(uint off)
// ----------------------------------------------------------------------------
//    Get code point at given offset
// ----------------------------------------------------------------------------
{
    seek(off);
    return get();
}


inline uint file::position()
// ----------------------------------------------------------------------------
//   Return current position in help file
// ----------------------------------------------------------------------------
{
    return ftell(data);
}

#endif // FILE_H
