// ****************************************************************************
//  characters.cc                                                DB48X project
// ****************************************************************************
//
//   File Description:
//
//     Character tables loaded from a characters file
//
//
//
//
//
//
//
//
// ****************************************************************************
//   (C) 2024 Christophe de Dinechin <christophe@dinechin.org>
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

#include "characters.h"

#include "unit.h"
#include "user_interface.h"
#include "utf8.h"



#define CFILE   "config/characters.csv"


// ============================================================================
//
//   Read data from the characters file
//
// ============================================================================

symbol_g characters_file::next()
// ----------------------------------------------------------------------------
//   Find the next file entry if there is one
// ----------------------------------------------------------------------------
{
    bool     quoted = false;
    symbol_g result = nullptr;
    scribble scr;

    while (valid())
    {
        char c = getchar();
        if (!c)
            break;

        if (c == '"')
        {
            if (quoted && peek() == '"') // Treat double "" as a data quote
                c = getchar();
            else
                quoted = !quoted;
            if (!quoted)
            {
                result = symbol::make(scr.scratch(), scr.growth());
                return result;
            }
        }
        else if (quoted)
        {
            byte *buf = rt.allocate(1);
            *buf = byte(c);
        }
    }
    return result;
}



// ============================================================================
//
//   Character lookup
//
// ============================================================================

static const cstring basic_characters[] =
// ----------------------------------------------------------------------------
//   List of basic characters
// ----------------------------------------------------------------------------
//   clang-format off
{
    "RPL",      "→«»Σ∏∆" "⇄{}≤≠≥" "ⅈ∡_∂∫|",
    "Math",     "Σ∏∆∂∫|" "+-*/×÷" "<=>≤≠≥",
    "Punct",    ".,;:!?" "#$%&'\\" "()[]{}",
    "Greek",    "αβγδεζηθικλμνξοπρςστυφχψωΑΒΓΔΕΖΗΘΙΚΛΜΝΞΟΠΡΣΤΥΦΧΨΩάέήίΰϊϋόύώϐϑϕϖ",
    "LtrLike",  "©®℗™℠№" "ªº℀℁℅℆" "℔℥ℨℬℊ℞",
    "Arrows",   "→←↑↓↔︎↕︎" "↵↩︎↺↻↳↪︎" "↖︎↘︎",
};
//   clang-format on



// ============================================================================
//
//   Build a characters menu
//
// ============================================================================

MENU_BODY(character_menu)
// ----------------------------------------------------------------------------
//   Build a characters menu
// ----------------------------------------------------------------------------
{
    // Use the characters loaded from the characters file
    characters_file cfile(CFILE);
    size_t          matching = 0;
    size_t maxu   = sizeof(basic_characters) / sizeof(basic_characters[0]);
    id     type   = o->type();
    id     menu   = ID_CharactersMenu00;
    symbol_g mchars = nullptr;

    if (cfile.valid())
    {
        while (cfile.next())
        {
            mchars = cfile.next();
            if (mchars)
            {
                if (menu == type)
                {
                    size_t len = 0;
                    utf8 val = mchars->value(&len);
                    utf8 end = val + len;
                    for (utf8 p = val; p < end; p = utf8_next(p))
                        matching++;
                    break;
                }
                menu = id(menu + 1);
            }
        }
    }

     // Disable built-in characters if we loaded a file
    if (!matching || Settings.ShowBuiltinCharacters())
    {
        for (size_t u = 0; u < maxu; u += 2)
        {
            if (menu == type)
            {
                utf8   mtxt = utf8(basic_characters[u + 1]);
                size_t len  = strlen(cstring(mtxt));
                mchars      = symbol::make(mtxt, len);
                for (utf8 p = mtxt; *p; p = utf8_next(p))
                    matching++;
                break;
            }
            menu = id(menu + 1);
        }
    }

    items_init(mi, matching);

    utf8 next = nullptr;
    if (mchars)
    {
        for (utf8 p = mchars->value(); matching--; p = next)
        {
            next = utf8_next(p);
            symbol_g label = symbol::make(p, size_t(next - p));
            items(mi, label, ID_SelfInsert);
        }
    }

    return true;
}


MENU_BODY(CharactersMenu)
// ----------------------------------------------------------------------------
//   The characters menu is dynamically populated
// ----------------------------------------------------------------------------
{
    uint   infile   = 0;
    uint   count    = 0;
    uint   maxmenus = ID_CharactersMenu99 - ID_CharactersMenu00;
    size_t maxu     = sizeof(basic_characters) / sizeof(basic_characters[0]);
    characters_file cfile(CFILE);

    // List all menu entries in the file (up to 100)
    if (cfile.valid())
        while (symbol_g mname = cfile.next())
            if (symbol_g mvalue = cfile.next())
                if (infile++ >= maxmenus)
                    break;

    // Count built-in character menu titles
    if (!infile || Settings.ShowBuiltinCharacters())
    {
        count += maxu / 2;
        if (infile + count > maxmenus)
            count = maxmenus - infile;
    }

    items_init(mi, infile + count);
    infile = 0;
    if (cfile.valid())
    {
        cfile.seek(0);
        while (symbol_g mname = cfile.next())
        {
            if (symbol_g mvalue = cfile.next())
            {
                if (infile >= maxmenus)
                    break;
                items(mi, mname, id(ID_CharactersMenu00 + infile++));
            }
        }
    }
    if (!infile || Settings.ShowBuiltinCharacters())
    {
        for (size_t u = 0; u < maxu; u += 2)
        {
            if (infile >= maxmenus)
                break;
            items(mi, basic_characters[u], id(ID_CharactersMenu00+infile++));
        }
    }

    return true;
}
