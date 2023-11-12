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
#include "user_interface.h"

#include <string.h>
#include <stdlib.h>


MENU_BODY(Catalog)
// ----------------------------------------------------------------------------
//   Process the MENU command for Catalog
// ----------------------------------------------------------------------------
{
    uint  nitems = count_commands();
    items_init(mi, nitems, 1);
    ui.menu_auto_complete();
    list_commands(mi);
    return OK;
}


static uint16_t sorted_ids[object::NUM_IDS];


static uint       NUM_COMMANDS = 0;

static int sort_ids(const void *left, const void *right)
// ----------------------------------------------------------------------------
//   Sort the IDs alphabetically based on their fancy name
// ----------------------------------------------------------------------------
{
    object::id l = object::id(*((uint16_t *) left));
    object::id r = object::id(*((uint16_t *) right));
    return strcasecmp(cstring(object::fancy(l)), cstring(object::fancy(r)));
}


static void initialize_sorted_ids()
// ----------------------------------------------------------------------------
//   Sort IDs alphabetically
// ----------------------------------------------------------------------------
{
    uint count = 0;
    for (uint i = 0; i < object::NUM_IDS; i++)
        if (object::is_command(object::id(i)))
            sorted_ids[count++] = object::id(i);
    qsort(sorted_ids, count, sizeof(sorted_ids[0]), sort_ids);
    NUM_COMMANDS = count;
}


static bool matches(utf8 start, size_t size, utf8 name)
// ----------------------------------------------------------------------------
//   Check if what was typed matches the name
// ----------------------------------------------------------------------------
{
    size_t len   = strlen(cstring(name));
    bool   found = false;
    // printf("[%.*s] vs [%s] is ", int(size), start, name);
    for (uint o = 0; !found && o + size < len; o++)
    {
        found = true;
        for (uint i = 0; found && i < size; i++)
            found = tolower(start[i]) == tolower(name[i + o]);
    }
    // printf("%s\n", found ? "true" : "false");
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
    bool   filter = ui.current_word(start, size);
    uint   count  = 0;

    for (uint i = 0; i < NUM_COMMANDS; i++)
    {
        id sorted = object::id(sorted_ids[i]);
        if (!filter                             ||
            matches(start, size, name(sorted))  ||
            matches(start, size, fancy(sorted)))
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
    bool   filter = ui.current_word(start, size);

    for (uint i = 0; i < NUM_COMMANDS; i++)
    {
        id sorted = object::id(sorted_ids[i]);
        if (!filter                                  ||
            matches(start, size, name(sorted))       ||
            matches(start, size, fancy(sorted)))
            menu::items(mi, cstring(fancy(sorted)),
                        command::static_object(sorted));
    }
}
