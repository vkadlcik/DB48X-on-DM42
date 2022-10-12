#ifndef OBJECT_H
#define OBJECT_H
// ****************************************************************************
//  object.h                                                      DB48X project
// ****************************************************************************
//
//   File Description:
//
//     The basic RPL object
//
//     An RPL object is a bag of bytes densely encoded using LEB128
//
//     It is important that the base object be empty, sizeof(object) == 1
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
//
//  RPL objects are encoded using sequence of LEB128 values
//  An LEB128 value is a variable-length encoding with 7 bits per byte,
//  the last byte having its high bit clear. This, values 0-127 are coded
//  as 0-127, values up to 16384 are coded on two bytes, and so on.
//
//  The reason for this encoding is that the DM42 is very memory-starved
//  (~70K available to DMCP programs), so chances of a size exceeding 2 bytes
//  are exceedingly rare. We can also use the lowest opcodes for the most
//  frequently used features, ensuring that 128 of them can be encoded on one
//  byte only. Similarly, all constants less than 128 can be encoded in two
//  bytes only (one for the opcode, one for the value), and all constants less
//  than 16384 are encoded on three bytes.
//
//  The opcode is an index in an object-handler table, so they act either as
//  commands or as types


#include <types.h>

struct runtime;

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


struct object
// ----------------------------------------------------------------------------
//  The basic RPL object
// ----------------------------------------------------------------------------
{
    object() {}
    ~object() {}

    enum command
    {
        EVAL,
        SIZE,
        PARSE,
        RENDER,
        LAST,
    };

    int run(runtime &rt, command cmd, void *arg = nullptr)
    // ------------------------------------------------------------------------
    //  Run an arbitrary command
    // ------------------------------------------------------------------------
    {
        void *ptr = this;
        uint type = leb128(ptr);
        if (type >= handlers)
            return -1;
        return handler[type](rt, cmd, arg, this, (object *) ptr);
    }

    size_t  size(runtime &rt = RT) { return (size_t) run(rt, SIZE); }
    object *skip(runtime &rt = RT) { return this + size(rt); }


    struct parser
    {
        cstring begin;
        cstring end;
        enum result { OK = 0, SKIP = 1, ERROR = -1 };
    };

    parser::result parse(cstring begin, cstring end, runtime &rt = RT)
    {
        parser p = { .begin = begin, .end = end };
        return (parser::result) run(rt, PARSE, &p);
    }

    struct renderer
    {
        char *begin;
        cstring end;
    };

    size_t render(char *begin, cstring end, runtime &rt = RT)
    {
        renderer r = { .begin = begin, .end = end };
        return run(rt, RENDER, &r);
    }

    // The actual work is done here
    static int handle(runtime &rt,
                      command  cmd,
                      void    *arg,
                      object  *obj,
                      object  *payload);

  protected:
    typedef int (*handler_fn)(runtime &rt,
                              command cmd, void *arg,
                              object *obj, object *payload);
    static const handler_fn handler[];
    static const size_t handlers;
    static runtime &RT;
};


#endif // OBJECT_H
