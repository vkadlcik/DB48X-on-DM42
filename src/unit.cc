// ****************************************************************************
//  unit.h                                                        DB48X project
// ****************************************************************************
//
//   File Description:
//
//    Unit objects represent objects such as 1_km/s.
//
//    The representation is an equation where the outermost operator is _
//    which is different from the way the HP48 does it, but simplify
//    many other operations
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

#include "unit.h"

#include "parser.h"
#include "settings.h"
#include "renderer.h"


PARSE_BODY(unit)
// ----------------------------------------------------------------------------
//    Try to parse this as an unit
// ----------------------------------------------------------------------------
{
    // If already parsing an equation, let upper parser deal with quotes
    if (p.precedence)
        return SKIP;

    utf8   s      = p.source;
    size_t len    = p.length;
    utf8   e      = s + len;
    bool   unit   = false;
    bool   signok = true;

    for (utf8 p = s; p < e; p = utf8_next(p))
    {
        unicode c = utf8_codepoint(p);
        unit = c == '_' || c == settings::SPACE_UNIT;
        if (unit)
            break;
        bool sign = c == '+' || c == '-';
        if (!signok && sign)
            break;
        signok = c == 'E' || c == 'e' || c == L'⁳';
        if (!signok && !sign && (c < '0' || c > '9') && c != '.' && c != ',')
            break;
    }

    object::result result = SKIP;
    if (unit)
    {
        p.precedence = MKUNIT;
        result = list_parse(ID_unit, p, 0, 0);
        p.precedence = 0;
    }

    return result;
}


RENDER_BODY(unit)
// ----------------------------------------------------------------------------
//   Do not emit quotes around unit objects
// ----------------------------------------------------------------------------
{
    return render(o, r, false);
}


HELP_BODY(unit)
// ----------------------------------------------------------------------------
//   Help topic for units
// ----------------------------------------------------------------------------
{
    return utf8("Units");
}
