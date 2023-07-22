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


GCP(list);
GCP(program);
GCP(block);
GCP(equation);
GCP(array);


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

    static list_p make(gcbytes bytes, size_t len)
    {
        return rt.make<list>(bytes, len);
    }


    // Iterator, built in a way that is robust to garbage collection in loops
    struct iterator
    {

        typedef object_p value_type;
        typedef size_t difference_type;

        explicit iterator(list_p list, bool atend = false)
            : first(object_p(list->value())),
              size(list->length()),
              index(atend ? size : 0) {}
        explicit iterator(list_p list, size_t skip)
            : first(object_p(list->value())),
              size(list->length()),
              index(0)
        {
            while (skip && index < size)
            {
                operator++();
                skip--;
            }
        }

    public:
        iterator& operator++()
        {
            if (index < size)
            {
                object_p obj = first.Safe() + index;
                size_t objsize = obj->size();
                index += objsize;
            }

            return *this;
        }
        iterator operator++(int)
        {
            iterator prev = *this;
            ++(*this);
            return prev;
        }
        bool operator==(iterator other) const
        {
            return
                index == other.index &&
                first.Safe() == other.first.Safe() &&
                size == other.size;
        }
        bool operator!=(iterator other) const
        {
            return !(*this == other);
        }
        value_type operator*() const
        {
            return first.Safe() + index;
        }

    public:
        object_g first;
        size_t   size;
        size_t   index;
    };
    iterator begin() const      { return iterator(this); }
    iterator end() const        { return iterator(this, true); }

public:
    // Shared code for parsing and rendering, taking delimiters as input
    static result list_parse(id type, parser &p, unicode open, unicode close);
    intptr_t      list_render(renderer &r, unicode open, unicode close) const;

public:
    OBJECT_DECL(list);
    PARSE_DECL(list);
    RENDER_DECL(list);
};
typedef const list *list_p;


struct program : list
// ----------------------------------------------------------------------------
//   A program is a list with « and » as delimiters
// ----------------------------------------------------------------------------
{
    program(gcbytes bytes, size_t len, id type = ID_program)
        : list(bytes, len, type) {}

    static bool      interrupted(); // Program interrupted e.g. by EXIT key
    static program_p parse(utf8 source, size_t size);

public:
    OBJECT_DECL(program);
    PARSE_DECL(program);
    RENDER_DECL(program);
    EVAL_DECL(program);
    EXEC_DECL(program);
};
typedef const program *program_p;


struct block : program
// ----------------------------------------------------------------------------
//   A block inside a program, e.g. in loops
// ----------------------------------------------------------------------------
{
    block(gcbytes bytes, size_t len, id type = ID_block)
        : program(bytes, len, type) {}

public:
    OBJECT_DECL(block);
    PARSE_DECL(block);
    RENDER_DECL(block);
    EVAL_DECL(block);
};
typedef const block *block_p;


struct equation : program
// ----------------------------------------------------------------------------
//   An equation is a program with ' and ' as delimiters
// ----------------------------------------------------------------------------
//   We also need special parsing and rendering of algebraic objects
{
    equation(gcbytes bytes, size_t len, id type = ID_equation)
        : program(bytes, len, type) {}
    static size_t required_memory(id i, gcbytes UNUSED bytes, size_t len)
    {
        return program::required_memory(i, bytes, len);
    }

    // Building an equation from an object
    equation(algebraic_r arg, id type = ID_equation);
    static size_t required_memory(id i, algebraic_r arg);

    // Building equations from one or two arguments
    equation(id op, algebraic_r arg, id type = ID_equation);
    static size_t required_memory(id i, id op, algebraic_r arg);
    equation(id op, algebraic_r x, algebraic_r y, id type = ID_equation);
    static size_t required_memory(id i, id op, algebraic_r x, algebraic_r y);

    object_p quoted(id type) const;
    static size_t size_in_equation(object_p obj);

    static int precedence(id type);
    static int precedence(object_p obj) { return precedence(obj->type()); }

    static equation_p make(algebraic_r x,
                           id type = ID_equation)
    {
        if (!x.Safe())
            return nullptr;
        return rt.make<equation>(type, x);
    }

    static equation_p make(id op, algebraic_r x,
                           id type = ID_equation)
    {
        if (!x.Safe())
            return nullptr;
        return rt.make<equation>(type, op, x);
    }

    static equation_p make(id op, algebraic_r x, algebraic_r y,
                           id type = ID_equation)
    {
        if (!x.Safe() || !y.Safe())
            return nullptr;
        return rt.make<equation>(type, op, x, y);
    }

protected:
    static symbol_g render(uint depth, int &precedence, bool edit);
    static symbol_g parentheses(symbol_g what);
    static symbol_g space(symbol_g what);

public:
    OBJECT_DECL(equation);
    PARSE_DECL(equation);
    RENDER_DECL(equation);
};


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
typedef const array *array_p;

#endif // LIST_H
