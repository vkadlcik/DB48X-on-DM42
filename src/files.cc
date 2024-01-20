// ****************************************************************************
//  files.cc                                                      DB48X project
// ****************************************************************************
//
//   File Description:
//
//
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

#include "files.h"

#include "array.h"
#include "file.h"
#include "list.h"
#include "program.h"
#include "runtime.h"
#include "settings.h"

#include <dmcp.h>


// ============================================================================
//
//   Storing an object to disk
//
// ============================================================================

bool files::store(text_p name, object_p value, cstring defext) const
// ----------------------------------------------------------------------------
//   Check if the name ends in 'B' for binary, otherwise use source format
// ----------------------------------------------------------------------------
{
    size_t len  = 0;
    utf8   path = name->value(&len);
    if (!len || !path)
    {
        rt.invalid_file_name_error();
        return false;
    }

    // Select format based on extension
    size_t extpos = len;
    while (extpos > 0 && path[extpos-1] != '.')
        extpos--;
    if (!extpos)
    {
        text_g nmg = text_g(name)
            + text_g(text::make(".")) + text_g(text::make(defext));
        return store(nmg, value, "48s");
    }

    // Check how to save
    cstring ext = cstring(path + extpos);

    // Save as binary?
    if (strncasecmp(ext, "48b", 3) == 0)
        return store_binary(name, value);

    // Save as text?
    if (strncasecmp(ext, "txt", 3) == 0)
    {
        if (text_p txt = value->as<text>())
            return store_text(name, txt);
        return store_text(name, value->as_text(true, false));
    }

    // Save as comma-separated value
    if (strncasecmp(ext, "csv", 3) == 0)
    {
        id ty = value->type();
        if (ty == ID_array || ty == ID_list)
            return store_list(name, list_p(value));
    }

    // Default to saving as source
    return store_source(name, value);
}


// The checksum we use for all commands
static uint32_t computed_checksum = 0;
static byte     file_magic[]      = FILE_MAGIC;


static uint32_t id_checksum()
// ----------------------------------------------------------------------------
//   A checksum of all ID names, used to identify changes in binary format
// ----------------------------------------------------------------------------
{
    if (computed_checksum == 0)
    {
        for (uint i = 0; i < object::NUM_IDS; i++)
            for (utf8 p = object::fancy(object::id(i)); *p; p++)
                computed_checksum = 0x1081 * computed_checksum ^ *p;
    }
    return computed_checksum;
}


bool files::store_binary(text_p name, object_p value) const
// ----------------------------------------------------------------------------
//  Store object in binary format
// ----------------------------------------------------------------------------
{
    if (value)
    {
        file f(filename(name, true), true);
        if (f.valid())
        {
            uint32_t checksum = id_checksum();
            if (f.write(cstring(file_magic), sizeof(file_magic))    &&
                f.write(cstring(&checksum), sizeof(checksum))       &&
                f.write(cstring(value), value->size()))
                return true;
        }
        rt.error(f.error());
    }
    return false;
}


bool files::store_source(text_p name, object_p value) const
// ----------------------------------------------------------------------------
//   Store object in source format
// ----------------------------------------------------------------------------
{
    if (value)
    {
        file f(filename(name, true), true);
        if (f.valid())
        {
            text_p source = value->as_text(true, false);
            size_t len = 0;
            utf8   txt    = source->value(&len);
            if (f.write(cstring(txt), len))
                return true;
        }
        rt.error(f.error());
    }
    return false;
}


bool files::store_text(text_p name, text_p value) const
// ----------------------------------------------------------------------------
//   Stores a text value directly
// ----------------------------------------------------------------------------
{
    if (value)
    {
        file f(filename(name, true), true);
        if (f.valid())
        {
            size_t len = 0;
            utf8   txt    = value->value(&len);
            if (f.write(cstring(txt), len))
                return true;
        }
        rt.error(f.error());
    }
    return false;
}


bool files::store_list(text_p name, list_p value) const
// ----------------------------------------------------------------------------
//   Store a list or array in CSV format, using ';' as separator
// ----------------------------------------------------------------------------
{
    if (value)
    {
        file f(filename(name, true), true);
        if (f.valid())
        {
            bool ok = true;
            for (object_p row : *value)
            {
                id     type = row->type();
                list_p li   = nullptr;
                if (type == ID_list || type == ID_array)
                    li = list_p(row);;
                if (li)
                {
                    bool first = true;
                    for (object_p col : *li)
                    {
                        if (!first)
                            ok = f.write(";", 1);
                        if (ok)
                        {
                            text_p item = col->as_text(true, false);
                            size_t len = 0;
                            utf8   txt  = item->value(&len);
                            ok = f.write(cstring(txt), len);
                        }
                        first = false;
                        if (!ok)
                            break;
                    }
                    if (ok)
                        ok = f.write("\n", 1);
                }
                else
                {
                    text_p item = row->as_text(true, false);
                    size_t len = 0;
                    utf8   txt  = item->value(&len);
                    ok = f.write(cstring(txt), len);
                    if (!ok)
                        ok = f.write("\n", 1);
                }
                if (!ok)
                    break;
            }
            if (ok)
                return true;
        }
        rt.error(f.error());
    }
    return false;

}


object_p files::recall(text_p name, cstring defext) const
// ----------------------------------------------------------------------------
//    Recall an object from disk
// ----------------------------------------------------------------------------
{
    size_t len  = 0;
    utf8   path = name->value(&len);
    if (!len || !path)
    {
        rt.invalid_file_name_error();
        return nullptr;
    }

    // Select format based on extension
    size_t extpos = len;
    while (extpos > 0 && path[extpos-1] != '.')
        extpos--;
    if (!extpos)
    {
        text_g nmg = text_g(name)
            + text_g(text::make(".")) + text_g(text::make(defext));
        return recall(nmg, "48s");
    }

    // Check the format of the file
    cstring ext = cstring(path + extpos);

    // Load from binary?
    if (strncasecmp(ext, "48b", 3) == 0)
        return recall_binary(name);

    // Load from text?
    if (strncasecmp(ext, "txt", 3) == 0)
        return recall_text(name);

    // Load from comma-separated value
    if (strncasecmp(ext, "csv", 3) == 0)
        return recall_list(name, true);

    // Default to loading from source
    return recall_source(name);
}


object_p files::recall_binary(text_p name) const
// ----------------------------------------------------------------------------
//   Recall an object from binary file
// ----------------------------------------------------------------------------
{
    if (rt.allocated())
    {
        rt.unable_to_allocate_error();
        return nullptr;
    }

    file f(filename(name), false);
    if (f.valid())
    {
        scribble scr;
        uint32_t checksum = id_checksum();
        char     buf[sizeof(file_magic)];
        uint32_t check = 0;
        char     c;
        size_t   sz;
        object_p result;
        if (!f.read(buf, sizeof(file_magic)))
            goto err;
        if (memcmp(buf, file_magic, sizeof(file_magic)) != 0)
        {
            rt.invalid_magic_number_error();
            return nullptr;
        }
        if (!f.read((char *) &check, sizeof(checksum)))
            goto err;
        if (check != checksum)
        {
            rt.incompatible_binary_error();
            return nullptr;
        }

        while (true)
        {
            if (!f.read(&c, 1))
                break;
            byte *ptr = rt.allocate(1);
            *ptr = c;
        }

        sz = rt.allocated();
        result = rt.temporary();
        if (result->type() >= NUM_IDS || result->size() != sz)
        {
            rt.invalid_object_in_file_error();
            return nullptr;
        }

        return result;

    err:
        rt.error(f.error());
    }
    return nullptr;
}


object_p files::recall_source(text_p name) const
// ----------------------------------------------------------------------------
//   Recall an object from source file
// ----------------------------------------------------------------------------
{
    file prog(filename(name), false);
    if (!prog.valid())
    {
        rt.error(prog.error());
        return nullptr;
    }

    // Loop on the input file and process it as if it was being typed
    uint bytes = 0;
    rt.clear();

    for (unicode c = prog.get(); c; c = prog.get())
    {
        byte buffer[4];
        size_t count = utf8_encode(c, buffer);
        rt.insert(bytes, buffer, count);
        bytes += count;
    }

    // End of file: execute the command we typed
    size_t edlen = rt.editing();
    if (edlen)
    {
        text_g edstr = rt.close_editor(true);
        if (edstr)
        {
            gcutf8 editor = edstr->value();
            bool dc = Settings.DecimalComma();
            Settings.DecimalComma(false);
            object_p obj = object::parse(editor, edlen);
            Settings.DecimalComma(dc);
            return obj;
        }
    }

    rt.invalid_object_error();
    return nullptr;
}


text_p files::recall_text(text_p name) const
// ----------------------------------------------------------------------------
//   Recall text from a text file
// ----------------------------------------------------------------------------
{
    file f(filename(name), false);
    if (!f.valid())
    {
        rt.error(f.error());
        return nullptr;
    }

    // Loop on the input file and process it as if it was being typed
    size_t bytes = 0;
    rt.clear();
    for (unicode c = f.get(); c; c = f.get())
    {
        byte buffer[4];
        size_t count = utf8_encode(c, buffer);
        rt.insert(bytes, buffer, count);
        bytes += count;
    }

    // End of file: execute the command we typed
    return rt.close_editor(true, false);
}


list_p files::recall_list(text_p name, bool as_array) const
// ----------------------------------------------------------------------------
//  Recall list from a CSV file
// ----------------------------------------------------------------------------
{
    file f(filename(name), false);
    if (!f.valid())
    {
        rt.error(f.error());
        return nullptr;
    }

    id       ty     = as_array ? ID_array : ID_list;
    list_g   result = list::make(ty, nullptr, 0);
    object_g item   = nullptr;
    list_g   row    = nullptr;
    int      cols   = 0;
    int      kcols  = -1;
    bool     intxt  = false;
    bool     ineqn  = false;
    uint     paren  = 0;
    uint     brack  = 0;
    uint     curly  = 0;

    // Loop on the input file and process it as if it was being typed
    size_t   bytes  = 0;
    rt.clear();
    for (unicode c = f.get(); c; c = f.get())
    {
        switch(c)
        {
        case '(':       paren++; break;
        case ')':       paren--; break;
        case '[':       brack++; break;
        case ']':       brack--; break;
        case '{':       curly++; break;
        case '}':       curly--; break;
        case '"':       intxt = !intxt; break;
        case '\'':      ineqn = !ineqn; break;
        }
        bool sepok = !paren && !brack && !curly && !intxt && !ineqn;

        if (c == ';' || (sepok && (c == ',' || c == '\n')))
        {
            text_p parsed = rt.close_editor(true);
            size_t len    = 0;
            utf8   txt    = parsed->value(&len);
            item = object::parse(txt, len);
            if (!item)
                break;
            byte_p b  = byte_p(+item);
            size_t sz = item->size();
            list_g li = rt.make<list>(ty, b, sz);

            if (row || c == ';' || c == ',')
            {
                row = row ? row + li : li;
                if (c == ';' || c == ',')
                    cols++;
            }
            if (c == '\n')
            {
                // Check if we have a rectangular input
                if (kcols < 0)
                    kcols = cols;
                if (cols != kcols)
                {
                    if (ty != ID_list)
                    {
                        ty = ID_list;
                        list_g copy = list::make(ty, nullptr, 0);
                        for (object_p obj : *result)
                        {
                            list_g ci;
                            id     oty = obj->type();
                            bool   isl = oty == ID_list || oty == ID_array;
                            list_p li  = isl ? list_p(obj) : nullptr;
                            if (li)
                            {
                                b = byte_p(li->objects(&sz));
                                ci = list::make(b, sz);
                                obj = ci;
                            }
                            b = byte_p(obj);
                            ci = list::make(b, obj->size());
                            copy = copy + ci;
                        }
                        if (row)
                        {
                            b = byte_p(row->objects(&sz));
                            row = list::make(b, sz);
                        }
                        result = copy;
                    }
                }
                if (row)
                {
                    b = byte_p(+row);
                    sz = row->size();
                    li = rt.make<list>(ty, b, sz);
                }
                result = result + li;
                row = nullptr;
                cols = 0;
            }
            rt.clear();
            bytes = 0;
        }
        else
        {
            byte buffer[4];
            size_t count = utf8_encode(c, buffer);
            rt.insert(bytes, buffer, count);
            bytes += count;
        }
    }
    if (row)
        result = result + row;

    return result;
}


bool files::purge(text_p name) const
// ----------------------------------------------------------------------------
//   Purge a file (unlink it)
// ----------------------------------------------------------------------------
{
    text_p path = filename(name);
    return file::unlink(path);
}


bool files::mkdir() const
// ----------------------------------------------------------------------------
//   Create a directory at the current location
// ----------------------------------------------------------------------------
{
    size_t len = 0;
    byte * txt = (byte *) value(&len);
    if (len == 0 || (len == 1 && *txt == '/'))
        return true;
    save<byte> zero(txt[len], 0); // A bit hacky
    return check_create_dir(cstring(txt));
}


static inline bool find_colon(utf8 txt, size_t len)
// ----------------------------------------------------------------------------
//   Check if there is a colon in the given text
// ----------------------------------------------------------------------------
{
    while (len--)
        if (*txt++ == ':')
            return true;
    return false;
}


text_p files::filename(text_p name, bool writing) const
// ----------------------------------------------------------------------------
//   Build a filename from given input
// ----------------------------------------------------------------------------
{
    // If name is an absolute path, use it directly
    size_t len = 0;
    utf8   txt = name->value(&len);
    if (txt[0] == '/' || find_colon(txt, len))
        return name;

    // Check if length of this one is zero or if it's just '/'
    txt = value(&len);
    if (len == 0 || (len == 1 && *txt == '/'))
        return name;

    // If we want to write, make sur that the directory exists
    if (writing)
        mkdir();

    // Concatenate the two texts
    text_g base = this;
    text_g sep  = text::make("/", 1);
    text_g path = name;
    return base + sep + path;
}
