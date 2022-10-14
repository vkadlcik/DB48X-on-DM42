// ****************************************************************************
//  decimal64.cc                                                 DB48X project
// ****************************************************************************
//
//   File Description:
//
//     Implementation of decimal floating point using Intel's library
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

#include "decimal-64.h"

#include "runtime.h"
#include "settings.h"

#include <bid_conf.h>
#include <bid_functions.h>
#include <cstdio>


OBJECT_HANDLER_BODY(decimal64)
// ----------------------------------------------------------------------------
//    Handle commands for decimal64s
// ----------------------------------------------------------------------------
{
    switch(cmd)
    {
    case EVAL:
        // Decimal64 values evaluate as self
        rt.push(obj);
        return 0;
    case SIZE:
        return ptrdiff(payload, obj) + sizeof(bid64);
    case PARSE:
    {
        parser *p = (parser *) arg;
        return parse(p->begin, &p->end, &p->output, rt);
    }
    case RENDER:
    {
        renderer *r = (renderer *) arg;
        return obj->render(r->begin, r->end, rt);
    }

    default:
        // Check if anyone else knows how to deal with it
        return DELEGATE(object);
    }

}


OBJECT_PARSER_BODY(decimal64)
// ----------------------------------------------------------------------------
//    Try to parse this as an decimal64
// ----------------------------------------------------------------------------
{
    cstring p = begin;

    // Skip leading sign
    if (*p == '+' || *p == '-')
        p++;

    // Skip digits
    cstring digits = p;
    while (*p >= '0' && *p <= '9')
        p++;

    // If we had no digits, check for special names or exit
    if (p == digits)
    {
        if (strncasecmp(p, "infinity", sizeof("infinity") - 1) != 0 &&
            strncasecmp(p, "NaN",      sizeof("NaN")      - 1) != 0)
            return SKIP;
    }

    // Check decimal dot
    char *decimal = nullptr;
    if (*p == '.' || *p == ',')
    {
        decimal = (char *) p++;
        while (*p >= '0' && *p <= '9')
            p++;
    }

    // Check exponent
    char *exponent = nullptr;
    if (*p == 'e' || *p == 'E' || *p == Settings.exponentChar)
    {
        exponent = (char *) p++;
        if (*p == '+' || *p == '-')
            p++;
        cstring expval = p;
        while (*p >= '0' && *p <= '9')
            p++;
        if (p == expval)
        {
            rt.error("Malformed exponent");
            return ERROR;
        }
    }

    // Patch the input to the BID library
    char dot = '.';
    if (decimal)
    {
        dot = *decimal;
        *decimal = '.';
    }

    char exp = 'e';
    if (exponent)
    {
        exp = *exponent;
        *exponent = 'e';
    }

    // Parse the number
    if (end)
        *end = p;
    if (out)
        *out = rt.make<decimal64>(ID_decimal64, begin);

    // Restore the patched input
    if (decimal)
        *decimal = dot;
    if (exponent)
        *exponent = exp;

    return OK;
}


OBJECT_RENDERER_BODY(decimal64)
// ----------------------------------------------------------------------------
//   Render the decimal64 into the given string buffer
// ----------------------------------------------------------------------------
{
    // Align the value
    bid64 num = value();

    // Render in a separate buffer to avoid overflows
    char buffer[50];
    bid64_to_string(buffer, &num);

    // Adjust special characters
    for (char *p = buffer; *p && p < buffer + sizeof(buffer); p++)
        if (*p == 'e' || *p == 'E')
            *p = Settings.exponentChar;
        else if (*p == '.')
            *p = Settings.decimalDot;

    // And return it to the caller
    return snprintf(begin, end - begin, "%s", buffer);
}
