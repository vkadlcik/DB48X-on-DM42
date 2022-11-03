// ****************************************************************************
//  catalog.cc                                                    DB48X project
// ****************************************************************************
//
//   File Description:
//
//     Auto-completion for commands (Catalog)
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

#include "catalog.h"
#include "runtime.h"
#include "input.h"

#include <string.h>
#include <stdlib.h>


OBJECT_HANDLER_BODY(Catalog)
// ----------------------------------------------------------------------------
//   Process the MENU command for Catalog
// ----------------------------------------------------------------------------
{
    switch(op)
    {
    case MENU:
    {
        info &mi     = *((info *) arg);
        uint  nitems = count_commands();
        items_init(mi, nitems, 1);
        Input.autoCompleteMenu();
        list_commands(mi);
        return OK;
    }
    default:
        return DELEGATE(menu);

    }
}


#define NUM_COMMANDS    (object::LAST_COMMAND - object::FIRST_COMMAND + 1)

static object::id sorted_ids[NUM_COMMANDS];


static int sort_ids(const void *left, const void *right)
// ----------------------------------------------------------------------------
//   Sort the IDs alphabetically based on their fancy name
// ----------------------------------------------------------------------------
{
    object::id l = *((object::id *) left);
    object::id r = *((object::id *) right);
    return strcasecmp(cstring(object::fancy(l)), cstring(object::fancy(r)));
}


static void initialize_sorted_ids()
// ----------------------------------------------------------------------------
//   Sort IDs alphabetically
// ----------------------------------------------------------------------------
{
    for (uint i = 0; i < NUM_COMMANDS; i++)
        sorted_ids[i] = object::id(i + object::FIRST_COMMAND);
    qsort(sorted_ids, NUM_COMMANDS, sizeof(sorted_ids[0]), sort_ids);
}


static bool matches(utf8 start, size_t size, cstring name)
// ----------------------------------------------------------------------------
//   Check if what was typed matches the name
// ----------------------------------------------------------------------------
{
    utf8 ref   = utf8(name);
    bool found = true;
    for (uint i = 0; found && i < size; i++)
        found = tolower(start[i]) == tolower(ref[i]);
    return found;
}


uint Catalog::count_commands()
// ----------------------------------------------------------------------------
//    Count the variables in the current directory
// ----------------------------------------------------------------------------
{
    if (!sorted_ids[0])
        initialize_sorted_ids();

    utf8   start  = 0;
    size_t size   = 0;
    bool   filter = Input.currentWord(start, size);
    uint   count  = 0;

    for (uint i = 0; i < NUM_COMMANDS; i++)
    {
        uint sorted = sorted_ids[i];
        if (!filter                                     ||
            matches(start, size, id_name[sorted])       ||
            matches(start, size, fancy_name[sorted]))
            count++;
    }

    return count;
}


void Catalog::list_commands(info &mi)
// ----------------------------------------------------------------------------
//   Fill the menu with variable names
// ----------------------------------------------------------------------------
{
    utf8   start  = nullptr;
    size_t size   = 0;
    bool   filter = Input.currentWord(start, size);

    for (uint i = 0; i < NUM_COMMANDS; i++)
    {
        object::id sorted = sorted_ids[i];
        if (!filter                                     ||
            matches(start, size, id_name[sorted])       ||
            matches(start, size, fancy_name[sorted]))
            menu::items(mi, fancy_name[sorted], command::static_object(sorted));
    }
    Input.menuNeedsRefresh();
}
