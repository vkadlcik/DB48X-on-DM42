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
#include "text.h"
#include "symbol.h"

struct list : text
// ----------------------------------------------------------------------------
//   RPL list type
// ----------------------------------------------------------------------------
{
    list(gcbytes bytes, size_t len, id type = ID_list): text(bytes, len, type)
    { }

    static size_t required_memory(id i, gcbytes UNUSED bytes, size_t len)
    {
        return text::required_memory(i, bytes, len);
    }

    static list *make(byte_p bytes, size_t len)
    {
        return (list *) text::make(bytes, len);
    }

    OBJECT_HANDLER(list);
    OBJECT_PARSER(list);
    OBJECT_RENDERER(list);

protected:
    // Shared code for parsing and rendering, taking delimiters as input
    intptr_t object_renderer(renderer &r, runtime &rt,
                             unicode open, unicode close) const;
    static result object_parser(id type, parser UNUSED &p, runtime &rt,
                                unicode open, unicode close);
};
typedef const list *list_p;


struct program : list
// ----------------------------------------------------------------------------
//   A program is a list with « and » as delimiters
// ----------------------------------------------------------------------------
{
    program(gcbytes bytes, size_t len, id type = ID_program)
        : list(bytes, len, type) {}

    result           execute(runtime &rt = RT) const;
    static bool      interrupted(); // Program interrupted e.g. by EXIT key
    static program_p parse(utf8 source, size_t size);

    OBJECT_HANDLER(program);
    OBJECT_PARSER(program);
    OBJECT_RENDERER(program);
};
typedef const program *program_p;


struct equation : program
// ----------------------------------------------------------------------------
//   An equation is a program with ' and ' as delimiters
// ----------------------------------------------------------------------------
//   We also need special parsing and rendering of algebraic objects
{
    equation(gcbytes bytes, size_t len, id type = ID_equation)
        : program(bytes, len, type) {}

    symbol_p symbol() const;

    OBJECT_HANDLER(equation);
    OBJECT_PARSER(equation);
    OBJECT_RENDERER(equation);
};
typedef const equation *equation_p;


struct array : list
// ----------------------------------------------------------------------------
//   An array is a list with [ and ] as delimiters
// ----------------------------------------------------------------------------
{
    array(gcbytes bytes, size_t len, id type = ID_array)
        : list(bytes, len, type) {}

    OBJECT_HANDLER(array);
    OBJECT_PARSER(array);
    OBJECT_RENDERER(array);
};
typedef const array *array_p;

#endif // LIST_H
