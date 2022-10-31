#ifndef MENU_H
#define MENU_H
// ****************************************************************************
//  menu.h                                                        DB48X project
// ****************************************************************************
//
//   File Description:
//
//     An RPL menu object defines the content of the soft menu keys
//
//     It is a catalog which, when evaluated, updates the soft menu keys
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

#include "variables.h"
#include "symbol.h"


struct menu : catalog
// ----------------------------------------------------------------------------
//   An RPL menu defines a bunch of commands with associated labels
// ----------------------------------------------------------------------------
{
    // Construction of menu objects
    menu(id type = ID_menu) : catalog(type) {}

    template<typename ...Args>
    menu(Args... args, id type = ID_menu) : catalog(type)
    {
        byte *p = payload();
        size_t sz = items_size(args...);
        p = leb128(p, sz);
        p = items(p, args...);
    }

    static size_t required_memory(id i) { return catalog::required_memory(i); }
    template<typename ...Args>
    static size_t required_memory(id i, Args... args)
    {
        size_t size = items_size(args...);
        return leb128size(i) + leb128size(size) + size;
    }

public:
    result  evaluate(runtime &rt = RT) const;


protected:
    static size_t items_size() { return 0; }
    static size_t items_size(cstring lbl, object_p obj)
    {
        size_t len = strlen(lbl);
        return leb128size(ID_symbol) + leb128size(len) + len + obj->size();
    }
    static size_t items_size(symbol_p lbl, object_p obj)
    {
        return lbl->object::size() + obj->size();
    }

    template<typename Label, typename ...Args>
    static size_t items_size(Label lbl, object_p obj, Args... args)
    {
        return items_size(lbl, obj) + items_size(args...);
    }

    static byte *items(byte *p, cstring label, object_p obj)
    {
        size_t len = strlen(label);
        p = leb128(p, ID_symbol);
        p = leb128(p, len);
        memmove(p, label, len);
        p += len;
        size_t objsize = obj->size();
        memmove(p, obj, objsize);
        p += objsize;
        return p;
    }
    static byte *items(byte *p, symbol_p label, object_p obj)
    {
        size_t lblsize = label->object::size();
        memmove(p, label, lblsize);
        p += lblsize;
        size_t objsize = obj->size();
        memmove(p, obj, objsize);
        p += objsize;
        return p;
    }

public:
    OBJECT_HANDLER(menu);
    OBJECT_PARSER(menu);
    OBJECT_RENDERER(menu);
};


#endif // MENU_H
