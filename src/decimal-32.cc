// ****************************************************************************
//  decimal32.cc                                                 DB48X project
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

#include "decimal-32.h"

#include "runtime.h"
#include "settings.h"
#include "parser.h"
#include "renderer.h"
#include "utf8.h"

#include <bid_conf.h>
#include <bid_functions.h>
#include <cstdio>
#include <algorithm>
#include <cstdlib>

using std::min;
using std::max;

RECORDER(decimal32, 32, "Decimal32 data type");


OBJECT_HANDLER_BODY(decimal32)
// ----------------------------------------------------------------------------
//    Handle commands for decimal32s
// ----------------------------------------------------------------------------
{
    record(decimal32, "Command %+s on %p", name(op), obj);
    switch(op)
    {
    case EXEC:
    case EVAL:
        // Decimal32 values evaluate as self
        rt.push(obj);
        return OK;
    case SIZE:
        return ptrdiff(payload, obj) + sizeof(bid32);
    case PARSE:
        return object_parser(OBJECT_PARSER_ARG(), rt);
    case RENDER:
        return obj->object_renderer(OBJECT_RENDERER_ARG(), rt);
    case HELP:
        return (intptr_t) "decimal";

    default:
        // Check if anyone else knows how to deal with it
        return DELEGATE(object);
    }

}


OBJECT_PARSER_BODY(decimal32)
// ----------------------------------------------------------------------------
//    Try to parse this as an decimal32
// ----------------------------------------------------------------------------
{
    record(decimal32, "Parsing [%s]", (utf8) p.source);

    utf8 source = p.source;
    utf8 s = source;
    utf8 last = source + p.length;

    // Skip leading sign
    if (*s == '+' || *s == '-')
        s++;

    // Skip digits
    utf8 digits = s;
    while (s < last && (*s >= '0' && *s <= '9'))
        s++;

    // If we had no digits, check for special names or exit
    if (s == digits)
    {
        if (strncasecmp(cstring(s), "inf", sizeof("inf") - 1) != 0 &&
            strncasecmp(cstring(s), "NaN", sizeof("NaN") - 1) != 0)
            return SKIP;
        record(decimal32, "Recognized NaN or Inf", s);
    }

    // Check decimal dot
    bool hadDecimalDot = *s == Settings.decimalDot;
    if (hadDecimalDot)
    {
        s++;
        while (s < last && (*s >= '0' && *s <= '9'))
            s++;
    }

    // Check how many digits were given
    uint mantissa = s - digits - hadDecimalDot;
    record(decimal32, "Had %u digits, max %u", mantissa, BID32_MAXDIGITS);
    if (mantissa >= BID32_MAXDIGITS)
    {
        rt.error("Too many digits", digits + BID32_MAXDIGITS);
        return WARN;                    // Try again with higher-precision
    }

    // Check exponent
    utf8 exponent = nullptr;
    if (*s == 'e' || *s == 'E' || utf8_codepoint(s) == Settings.exponentChar)
    {
        s = utf8_next(s);
        exponent = s;
        if (*s == '+' || *s == '-')
            s++;
        utf8 expval = s;
        while (s < last && (*s >= '0' && *s <= '9'))
            s++;
        if (s == expval)
        {
            rt.error("Malformed exponent");
            return ERROR;
        }
    }

    // Check if exponent is withing range, if not skip to wider format
    if (exponent)
    {
        int expval = atoi(cstring(exponent));
        int maxexp = 32 == 127+1 ? 6144 : 32 == 63+1 ? 384 : 96;
        record(decimal32, "Exponent is %d, max is %d", expval, maxexp);
        if (expval < -(maxexp-1) || expval > maxexp)
        {
            rt.error("Exponent out of range");
            return WARN;
        }
    }

    // Patch the input to the BID library
    char buf[50];
    char *b = buf;
    for (utf8 u = source; u < s && b < buf+sizeof(buf) - 1; u++)
    {
        if (*u == Settings.decimalDot)
        {
            *b++ = '.';
        }
        else if (utf8_codepoint(u) == Settings.exponentChar)
        {
            *b++ = 'E';
            u = utf8_next(u) - 1;
        }
        else
        {
            *b++ = *u;
        }
    }
    *b++ = 0;

    // Create the number
    p.end = s - source;
    p.out = rt.make<decimal32>(ID_decimal32, buf);

    return OK;
}


// Trick to only put the decimal_format function inside decimal32.cc
#if 32 == 64 + 64                      // Check if we are in decimal32.cc

#define MAX_NEG_FILL    4               // Limit before switching to SCI

static bool round_string(char *s, int digit, char rounding_digit)
// ----------------------------------------------------------------------------
//   Checks which digit was responsible for overflow
// ----------------------------------------------------------------------------
{
    if (rounding_digit + 5 <= '9')
        return false;

    for (; digit >= 0; digit--)
    {
        if (s[digit] == '.')
            continue; // Skip decimal point
        s[digit]++;
        if (s[digit] <= '9')
            return false;
        s[digit] -= 10;
    }

    return true;
}


void decimal_format(char *str, size_t len, int digits)
// ----------------------------------------------------------------------------
//   Format the number according to our preferences
// ----------------------------------------------------------------------------
//   This is largely inspired by the code in the DM42 SDKdemo
{
    // First make a copy of the raw output of the library
    char s[50];                 // Enough even for bid32
    strcpy(s, str);             // Make a local copy first

    settings::display mode = Settings.display_mode;
    for (;;)
    {
        char *ep = strchr(s, 'E');
        if (!ep)
        {
            // No exponent -> expecting special number like Inf or NaN
            // Just copy (we may have eliminated mantissa sign
            strcpy(str, s);
            return;
        }

        int   ms     = s[0] == '-'; // Mantissa negative
        int   mexp   = ep - s - 1;  // Mantissa exponent (e.g. 4 for +1234E+10)
        bool  hasexp = true;
        int   exp    = atoi(ep + 1) + mexp;

        // Terminate mantissa string
        char *mend   = ep - 1; // Mantissa end
        char *mant   = s + 1;  // Mantissa string

        // Ignore mantissa trailing zeros
        while (ep > mant && mend[0] == '0')
            mend--;
        *(++mend) = 0;

        // Check if we need to display an exponent ("scientific" mode)
        switch (mode)
        {
        case settings::display::FIX:
            if (exp <= 0 && Settings.displayed <= -exp)
                break;           // Zero fill bigger than mode digits

        case settings::display::NORMAL:
        {
            // Check if exponent is needed
            int sz = exp;
            if (exp >= (-MAX_NEG_FILL + 1))
            {
                // Check number requires a 0. and zero padding after decimal
                if (exp <= 0)
                    sz += 2 - exp + 1;
                if (ms)
                    sz++;                // One place for sign
                hasexp = sz > digits + 2;
            }
            break;
        }

        case settings::display::SCI:
        case settings::display::ENG:
            // Here, we always have an exponent
            break;
        }

        int digitsBeforePoint  = hasexp ? 1 : exp;
        int mantissaLen = strlen(s + 1);       // Available mantissa digits

        // Exponent correction for ENG mode
        exp--; // fix for digitsBeforePoint==1
        if (mode == settings::display::ENG)
        {
            // Lower the exponent to nearest multiple of 3
            int adjust = exp >= 0 ? exp % 3 : 2 + (exp - 2) % 3;
            exp -= adjust;
            digitsBeforePoint += adjust;
        }

        int zeroFillAfterDot = max(0, -digitsBeforePoint);

        // Prepare exponent
        int elen;
        if (hasexp)
        {
            // Do not interfere with mantissa end
            ep++;
            sprintf(ep, "%c%i", 'E', exp);
            elen = strlen(ep);
        }
        else
        {
            ep[0] = 0;
            elen  = 0;
        }

        //  // Frac digits available
        int frac = mantissaLen - digitsBeforePoint;

        // Add Mantissa
        char *p = str;
        if (ms)
            *p++ = '-';

        // Copy digits before point
        char *mp = s + 1;
        if (digitsBeforePoint > 0)
        {
            p = stpncpy(p, s + 1, digitsBeforePoint);
            mp += min(digitsBeforePoint, (int) strlen(s+1));
        }

        // Add trailing zeros in integer part
        for (int z = 0; z < -frac; z++)
            *p++ = '0';
        *p = 0;

        // Available space for fraction
        int avail = len - strlen(str) - elen;

        // Add fractional part
        digits = min(min(digits, frac), avail - 1 - (digitsBeforePoint > 0 ? 0 : 1));
        if (digits > 0)
        {
            // We have digits and have room for at least one frac digit
            p = stpcpy(p, digitsBeforePoint > 0 ? "." : "0.");
            frac = max(0, frac);
            for (int z = 0; z < zeroFillAfterDot; z++)
                *p++ = '0';
            digits -= zeroFillAfterDot;
            int msz = min(frac, digits);
            p = stpncpy(p, mp, msz);
            mp += msz;
            int trailingZeroes = max(digits + zeroFillAfterDot - frac, 0);
            for (int z = 0; z < trailingZeroes; z++)
                *p++ = '0';
            *p = 0;
        }

        if (*mp)
        {
            // More mantissa digits rounding
            int rix  = mp - s;
            bool ovfl = round_string(str + ms, strlen(str + ms) - 1, s[rix]);
            if (ovfl)
            {
                // Retry -
                snprintf(s,
                         s + sizeof(s)-1 - ep,
                         "%c1%c%c%i",
                         ms ? '-' : '+',
                         'E',
                         exp < 0 ? '-' : '+',
                         abs(exp + 1));
                continue;
            }
            if (mode == settings::display::NORMAL)
            {
                // Remove trailing zeros
                int ix = strlen(str) - 1;
                while (ix && str[ix] == '0')
                    ix--;
                if (str[ix] == '.')
                    ix--;
                str[ix + 1] = 0;
            }
        }

        // Add exponent
        p = strcpy(p, ep);
        break;
    }
}
#endif // In original decimal32.cc


OBJECT_RENDERER_BODY(decimal32)
// ----------------------------------------------------------------------------
//   Render the decimal32 into the given string buffer
// ----------------------------------------------------------------------------
{
    // Align the value
    bid32 num = value();

    // Render in a separate buffer to avoid overflows
    char buffer[50];            // bid32 with 34 digits takes at most 42 chars
    byte expbuf[4];
    bid32_to_string(buffer, &num.value);
    record(decimal32, "Render raw output [%s]", buffer);

    int digits = r.editing ? BID32_MAXDIGITS : Settings.displayed;
    decimal_format(buffer, min(sizeof(buffer), r.length), digits);
    record(decimal32, "Render formatted output [%s]", buffer);

    // Adjust special characters
    char *ep = nullptr;
    for (char *p = buffer; *p && p < buffer + sizeof(buffer); p++)
    {
        if (*p == 'e' || *p == 'E')
            ep = p;
        else if (*p == '.')
            *p = Settings.decimalDot;
    }

    if (ep)
    {
        *ep++ = 0;
        utf8_encode(Settings.exponentChar, expbuf);
    }
    else
    {
        expbuf[0] = 0;
        ep = (char *) "";
    }

    // And return it to the caller
    return snprintf(r.target, r.length, "%s%s%s", buffer, expbuf, ep);
}



// ============================================================================
//
//   Arithmetic wrappers
//
// ============================================================================
//   Define mod and rem in a way that matches mathematical definition

void bid32_mod(BID_UINT32 *pres, BID_UINT32 *px, BID_UINT32 *py)
// ----------------------------------------------------------------------------
//   The fmod function is really a remainder, adjust it for negative input
// ----------------------------------------------------------------------------
{
    int zero = 0;
    bid32_fmod(pres, px, py);
    bid32_isZero(&zero, pres);
    if (!zero)
    {
        bool xneg = decimal32::is_negative(px);
        bool yneg = decimal32::is_negative(py);
        if (xneg != yneg)
        {
            BID_UINT32 tmp = *pres;
            bid32_add(pres, &tmp, py);
        }
    }
}


void bid32_rem(BID_UINT32 *pres, BID_UINT32 *px, BID_UINT32 *py)
// ----------------------------------------------------------------------------
//   The fmod function is really a remainder, use it as is
// ----------------------------------------------------------------------------
{
    bid32_fmod(pres, px, py);
}
