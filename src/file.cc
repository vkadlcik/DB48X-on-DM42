// ****************************************************************************
//  file.cc                                                       DB48X project
// ****************************************************************************
//
//   File Description:
//
//      Abstract interface for the zany DMCP filesystem
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

#include "file.h"
#include "recorder.h"

RECORDER(file,          16, "File operations");
RECORDER(file_error,    16, "File errors");


file::file()
// ----------------------------------------------------------------------------
//   Construct a file object
// ----------------------------------------------------------------------------
    : data()
{}


file::~file()
// ----------------------------------------------------------------------------
//   Close the help file
// ----------------------------------------------------------------------------
{
    close();
}


void file::open(cstring path)
// ----------------------------------------------------------------------------
//    Open a help file
// ----------------------------------------------------------------------------
{
#if SIMULATOR
    data = fopen(path, "r");
    if (!data)
    {
        record(file_error, "Error %s opening %s", strerror(errno), path);
        return;
    }
#else
    FRESULT ok = f_open(&data, path, FA_READ);
    if (ok != FR_OK)
    {
        data.obj.objsize = 0;
        return;
    }
#endif                          // SIMULATOR
}


void file::close()
// ----------------------------------------------------------------------------
//    Close the help file
// ----------------------------------------------------------------------------
{
    if (valid())
        fclose(data);
}


unicode file::get()
// ----------------------------------------------------------------------------
//   Read UTF8 code at offset
// ----------------------------------------------------------------------------
{
    unicode code = valid() ? fgetc(data) : unicode(EOF);
    if (code == unicode(EOF))
        return 0;

    if (code & 0x80)
    {
        // Reference: Wikipedia UTF-8 description
        if ((code & 0xE0)      == 0xC0)
            code = ((code & 0x1F)        <<  6)
                |  (fgetc(data) & 0x3F);
        else if ((code & 0xF0) == 0xE0)
            code = ((code & 0xF)         << 12)
                |  ((fgetc(data) & 0x3F) <<  6)
                |   (fgetc(data) & 0x3F);
        else if ((code & 0xF8) == 0xF0)
            code = ((code & 0xF)         << 18)
                |  ((fgetc(data) & 0x3F) << 12)
                |  ((fgetc(data) & 0x3F) << 6)
                |   (fgetc(data) & 0x3F);
    }
    return code;
}


uint file::find(unicode   cp)
// ----------------------------------------------------------------------------
//    Find a given code point in file looking forward
// ----------------------------------------------------------------------------
//    Return position right before code point, position file right after it
{
    unicode c;
    uint    off;
    do
    {
        off          = ftell(data);
        c            = get();
    } while (c && c != cp);
    return off;
}


uint file::rfind(unicode  cp)
// ----------------------------------------------------------------------------
//    Find a given code point in file looking backward
// ----------------------------------------------------------------------------
//    Return position right before code point, position file right after it
{
    uint    off = ftell(data);
    unicode c;
    do
    {
        if (off == 0)
            break;
        fseek(data, --off, SEEK_SET);
        c        = get();
    }
    while (c != cp);
    return off;
}
