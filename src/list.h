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

#include "command.h"
#include "object.h"
#include "text.h"


GCP(list);


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

    static list_p make(id ty, gcbytes bytes, size_t len)
    {
        return rt.make<list>(ty, bytes, len);
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
            return index < size ? first.Safe() + index : nullptr;
        }

    public:
        object_g first;
        size_t   size;
        size_t   index;
    };
    iterator begin() const      { return iterator(this); }
    iterator end() const        { return iterator(this, true); }


    size_t items() const
    // ------------------------------------------------------------------------
    //   Return number of items in the list
    // ------------------------------------------------------------------------
    {
        size_t result = 0;
        for (object_p obj UNUSED : *this)
            result++;
        return result;
    }


    size_t expand() const
    // ------------------------------------------------------------------------
    //   Expand items to the stack, and return number of them
    // ------------------------------------------------------------------------
    {
        size_t result = 0;
        for (object_p obj : *this)
        {
            if (!rt.push(obj))
            {
                if (result)
                    rt.drop(result);
                return 0;
            }
            result++;
        }
        return result;
    }


    object_p operator[](size_t index) const
    // ------------------------------------------------------------------------
    //   Return the n-th element in the list
    // ------------------------------------------------------------------------
    {
        return at(index);
    }


    object_p at(size_t index) const
    // ------------------------------------------------------------------------
    //   Return the n-th element in the list
    // ------------------------------------------------------------------------
    {
        return *iterator(this, index);
    }


    template<typename ...args>
    object_p at(size_t index, args... rest) const
    // ------------------------------------------------------------------------
    //   N-dimensional array access
    // ------------------------------------------------------------------------
    {
        object_p inner = at(index);
        if (!inner)
            return nullptr;
        id type = inner->type();
        if (type != ID_array && type != ID_list)
            return nullptr;
        list_p list = list_p(inner);
        return list->at(rest...);
    }

    // Apply an algebraic function to all elements in list
    static list_g map(algebraic_fn fn, list_r x) { return x->map(fn); }
    list_g map(algebraic_fn fn) const;

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

COMMAND_DECLARE(Get);

inline list_g operator+(list_r x, list_r y)
// ----------------------------------------------------------------------------
//   Concatenate two lists, leveraging text concatenation
// ----------------------------------------------------------------------------
{
    text_r xt = (text_r) x;
    text_r yt = (text_r) y;
    return list_p((xt + yt).Safe());
}


inline list_g operator*(list_r x, uint y)
// ----------------------------------------------------------------------------
//    Repeat a list, leveraging text repetition
// ----------------------------------------------------------------------------
{
    text_r xt = (text_r) x;
    return list_p((xt * y).Safe());
}

#endif // LIST_H
