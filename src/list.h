#ifndef LIST_H
#define LIST_H
// ****************************************************************************
//  list.h                                                        DB48X project
// ****************************************************************************
//
//   File Description:
//
//     RPL list objects
//
//     A list is a sequence of distinct objects
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
//
// Payload format:
//
//   A list is a sequence of bytes containing:
//   - The type ID
//   - The UTF8-encoded length of the payload
//   - Each object in the list in turn
//
//   To save space, there is no explicit marker for the end of list

#include "object.h"
#include "rplstring.h"

struct list : string
// ----------------------------------------------------------------------------
//   RPL list type
// ----------------------------------------------------------------------------
{
    list(gcbytes bytes, size_t len, id type = ID_list): string(bytes, len, type)
    { }

    static size_t required_memory(id i, gcbytes UNUSED bytes, size_t len)
    {
        return string::required_memory(i, bytes, len);
    }

    static list *make(byte_p bytes, size_t len)
    {
        return (list *) string::make(bytes, len);
    }

    OBJECT_HANDLER(list);
    OBJECT_PARSER(list);
    OBJECT_RENDERER(list);


protected:
    // Shared code for parsing and rendering, taking delimiters as input
    intptr_t object_renderer(renderer &r, runtime &rt,
                             unicode open, unicode close) const;
    static result object_parser(parser UNUSED &p, runtime &rt,
                                unicode open, unicode close);

};


#endif // LIST_H
