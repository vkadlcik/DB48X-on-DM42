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

#include "algebraic.h"
#include "arithmetic.h"
#include "compare.h"
#include "expression.h"
#include "file.h"
#include "functions.h"
#include "integer.h"
#include "parser.h"
#include "renderer.h"
#include "settings.h"
#include "user_interface.h"


RECORDER(units,         16, "Unit objects");
RECORDER(units_error,   16, "Error on unit objects");


// Units loaded from CONFIG/UNITS.CSV file
static cstring *file_units       = nullptr;
static size_t   file_units_count = 0;
static bool     file_loaded      = false;


PARSE_BODY(unit)
// ----------------------------------------------------------------------------
//    Try to parse this as an unit
// ----------------------------------------------------------------------------
{
    // Actual work is done in the complex parser
    return SKIP;
}


algebraic_p unit::parse_uexpr(gcutf8 source, size_t len)
// ----------------------------------------------------------------------------
//  Parse a uexpr as an expression without quotes
// ----------------------------------------------------------------------------
{
    parser p(source, len, MULTIPLICATIVE);
    object::result result = list::list_parse(ID_expression, p, 0, 0);
    if (result == object::OK)
        if (algebraic_p alg = p.out->as_algebraic())
            return alg;
    return nullptr;
}


unit_p unit::make(algebraic_g v, algebraic_g u, id ty)
// ----------------------------------------------------------------------------
//   Build a unit object from its components
// ----------------------------------------------------------------------------
{
    if (!v.Safe() || !u.Safe())
        return nullptr;

    while (unit_g vu = v->as<unit>())
    {
        u = vu->uexpr() * u;
        v = vu->value();
        while (unit_g uu = u->as<unit>())
        {
            v = uu->value() * v;
            u = uu->uexpr();
        }
    }
    if (expression_p eq = u->as<expression>())
        u = eq->simplify_products();
    return rt.make<unit>(ty, v, u);
}


algebraic_p unit::simple(algebraic_g v, algebraic_g u, id ty)
// ----------------------------------------------------------------------------
//   Build a unit object from its components, simplify if it ends up numeric
// ----------------------------------------------------------------------------
{
    unit_g uobj = make(v, u, ty);
    if (uobj)
    {
        algebraic_g uexpr = uobj->uexpr();
        if (expression_p eq = uexpr->as<expression>())
            if (object_p q = eq->quoted())
                if (q->is_real())
                    uexpr = algebraic_p(q);
        if (uexpr->is_real())
        {
            algebraic_g uval = uobj->value();
            if (!uexpr->is_one())
                uval = uval * uexpr;
            return uval;
        }
    }
    return uobj;
}


RENDER_BODY(unit)
// ----------------------------------------------------------------------------
//   Do not emit quotes around unit objects
// ----------------------------------------------------------------------------
{
    algebraic_g value = o->value();
    algebraic_g uexpr = o->uexpr();
    value->render(r);
    r.put(r.editing() ? unicode('_') : unicode(settings::SPACE_UNIT));
    save<bool> m(mode, true);
    if (expression_p ueq = uexpr->as<expression>())
        ueq->render(r, false);
    else
        uexpr->render(r);
    return r.size();
}


EVAL_BODY(unit)
// ----------------------------------------------------------------------------
//   Evaluate the value, and if in unit mode, evaluate the uexpr as well
// ----------------------------------------------------------------------------
{
    algebraic_g value = o->value();
    algebraic_g uexpr = o->uexpr();
    value = value->evaluate();
    if (!value)
        return ERROR;
    if (unit::mode)
    {
        uexpr = uexpr->evaluate();
        if (!uexpr)
            return ERROR;

        while (unit_g u = uexpr->as<unit>())
        {
            algebraic_g scale = u->value();
            uexpr = u->uexpr();
            value = scale * value;
        }
    }
    value = unit::simple(value, uexpr);
    return rt.push(value.Safe()) ? OK : ERROR;
}


HELP_BODY(unit)
// ----------------------------------------------------------------------------
//   Help topic for units
// ----------------------------------------------------------------------------
{
    return utf8("Units");
}



// ============================================================================
//
//   Unit lookup
//
// ============================================================================

// This variable is true while evaluating a uexpr
bool unit::mode = false;


static const cstring basic_units[] =
// ----------------------------------------------------------------------------
//   List of basic units
// ----------------------------------------------------------------------------
//   The value of these units is taken from Wikipedia.
//   In many cases, e.g. parsec or au, it does not match the HP48 value
{
    // Length and area
    "m",        "1_m",                  // meter, based for SI lengths
    "yd",       "9144/10000_m",         // yard
    "ft",       "3048/10000_m",         // foot
    "in",       "254/10000_m",          // inch
    "pc",       "30856775814913673_m",  // Parsec
    "ls",       "299792458_m",          // Light-second
    "lyr",      "31557600_ls",          // Light year
    "au",       "149597870700_m",       // Astronomical unit
    "nmi",      "1852_m",               // Nautical mile
    "miUS",     "",                     // US mile
    "Å",        "100_pm",               // Angstroem is 100pm, 1E-10m
    "μ",        "1_μm",                 // A micron can be written as μ
    "fermi",    "1_fm",                 // fermi is another name for femtometer
    "mil",      "254/10000000_m",       // A thousands of a inch (min is taken)
    "a",        "100_m²",               // Acre
    "b",        "100_fermi²",           // Barn, 1E-28 m^2

    // US Survey funny set of units
    // See https://www.northamptonma.gov/740/US-Survey-Foot-vs-Meter and
    // https://www.nist.gov/pml/us-surveyfoot/revised-unit-conversion-factors
    // for details about this insanity.
    // The bottom line is that on January 1, 2023, all US units changed
    // to align to the "metric foot". So all units below have two variants,
    // a US (U.S. Survey, pre 2023) and non US variant. Yadi Yada.
    // The HP48 had a single ftUS unit, which was imprecise, because it did
    // not have fractions to represent it precisely. This unit is the only
    // one kept here. Otherwise, you can use the US unit, e.g. using
    // 1_cable*US will give you the U.S. Survey version of the cable.
    "ftUS",     "1200/3937_m",          // US survey foot
    "US",       "1_ftUS/ft",            // Conversion factor
    "cable",    "720_ft",               // Cable's length (US navy)
    "ch",       "66_ft",                // Chain
    "chain",    "1_ch",                 // Chain
    "fath",     "6_ft",                 // Fathom
    "fathom",   "1_fath",               // Fathom
    "fur",      "660_ft",               // Furlong
    "furlong",  "1_fur",                // Furlong
    "league",   "3_mi",                 // League
    "li",       "1/100_ch",             // Link
    "link",     "1_li",                 // Link
    "mi",       "5280_ft",              // Mile
    "miUS",     "1_mi*US",              // Mile (US Survey)
    "rd",       "1/4_ch",               // Rod, pole, perch
    "rod",      "1_rd",                 // Alternate spelling
    "pole",     "1_rd",                 // Pole
    "perch",    "1_rd",                 // Perch

    "ac",       "10_ch²",               // Acre
    "acre",     "10_ac",                // Acre
    "acUS",     "10_ch²*US²",           // Acre (pre-2023)
    "acreUS",   "1_acUS",               // Acre (pre-2023)

    "acable",   "18532/100_m",          // Cable's length (Imperial/Admiralty)
    "icable",   "1852/10_m",            // Cable's length ("International")

    // Duration
    "s",        "1_s",
    "min",      "60_s",
    "minute",   "1_min",
    "h",        "3600_s",
    "hour",     "1_h",
    "d",        "86400_s",
    "day",      "1_d",
    "yr",       "36524219/100000_d",    // Mean tropical year
    "year",     "1_y",                  // Mean tropical year
    "Hz",       "1_s⁻¹",                // Hertz
    "rpm",      "60_Hz",                // Rotations per minute

    // Speed
    "kph",      "1_km/h",               // US common spelling for km/h
    "mph",      "1_mi/h",               // Miles per hour
    "knot",     "1_nmi/h",              // 1 knot is 1 nautical mile per hour
    "c",        "299792458_m/s",        // Speed of light
    "ga",       "980665/100000_m/s^2",  // Standard freefall acceleration
    "G",        "1_ga",                 // Alternate spelling (1_G)

    // Mass
    "g",        "1_g",                  // Gram
    "t",        "1000_kg",              // Metric ton
    "ct",       "200_mg",               // Carat
    "carat",    "1_ct",                 // Carat
    "lb",       "45359237/100000_g",    // Avoirdupois pound
    "dr",       "1/256_lb",             // Drachm
    "dram",     "1_dr",                 // Alternate spelling
    "drachm",   "1_dr",                 // Alternate spelling
    "oz",       "1/16_lb",              // Ounce
    "stone",    "14_lb",                // Stone
    "qrUK",     "28_lb",                // Quarter (UK)
    "qrUS",     "25_lb",                // Quarter (US)
    "cwtUK",    "112_lb",               // Long hundredweight (UK)
    "cwtUS",    "100_lb",               // Short hundredweight (US)
    "tonUK",    "20_cwtUK",             // Long ton
    "tonUS",    "20_cwtUS",             // Short ton
    "ton",      "1_tonUS",              // Short ton
    "grain",    "1/7000_lb",            // Grain (sometimes "gr")
    "gr",       "1_grain",              // Grain
    "slug",     "1_lbf*s^2/ft",         // Slug
    "blob",     "12_slug",              // Blob (seriously????)
    "dwt",      "24_grain",             // Pennyweight (Troy weight system)
    "ozt",      "20_dwt",               // Troy ounce
    "lbt",      "12_ozt",               // Troy pound
    "u",        "1.6605402E-27_kg",     // Unified atomic mass
    "mol",      "1_mol",                // Mole (quantity of matter)
    "mole",     "1_mol",                // Mole (quantity of matter)
    "Avogadro", "6.02214076E23",        // Avogadro constant (# units in 1_mol)

    // Force
    "N",        "1_kg*m/s^2",           // Newton
    "dyn",      "1/100000_N",           // Dyne
    "gf",       "980665/100000000_N",   // Gram-force
    "kip",      "1000_lbf",             // Kilopound-force
    "lbf",      "44482216152605/10000000000000_N",    // Pound-force
    "pdl",      "138254954376/1000000000000_N",       // Poundal

    // Energy
    "J",        "1_kg*m^2/s^2",         // Joule
    "erg",      "1/10000000_J",         // erg
    "calth",    "4184/1000_J",          // Thermochemical Calorie
    "cal4",     "4204/1000_J",          // 4°C calorie
    "cal15",    "41855/10000_J",        // 15°C calorie
    "cal20",    "4182/1000_J",          // 20°C calorie
    "calmean",  "4190/1000_J",          // 4°C calorie
    "cal",      "41868/10000_J",        // International calorie (1929, 1956)
    "Btu",      "1055.05585262_J",      // British thermal unit
    "therm",    "105506000_J",          // EEC therm
    "eV",       "1.60217733E-19_J",     // electron-Volt

    // Power
    "W",        "1_J/s",                // Watt
    "hp",       "745.699871582_W",      // Horsepower

    // Pressure
    "Pa",       "1_N/m^2",              // Pascal
    "atm",      "101325_Pa",            // Atmosphere
    "bar",      "100000_Pa",            // bar
    "psi",      "6894.75729317_Pa",     // Pound per square inch
    "ksi",      "1000_psi",             // Kilopound per square inch
    "torr",     "1/760_atm",            // Torr = 1/760 standard atm
    "mmHg",     "1_torr",               // millimeter of mercury
    "inHg",     "1_in/mm*mmHg",         // inch of mercury
    "inH2O",    "249.0889_Pa",          // Inch of H2O

    // Temperature
    "K",        "1_K",                  // Kelvin
    "°C",       "1_K",                  // Celsius
    "°R",       "9/5_K",                // Rankin
    "°F",       "9/5_K",                // Fahrenheit

    // Electricity
    "A",        "1_A",                  // Ampere
    "V",        "1_kg*m^2/(A*s^3)",     // Volt
    "C",        "1_A*s",                // Coulomb
    "Ω",        "1_V/A",                // Ohm
    "ohm",      "1_Ω",                  // Ohm
    "F",        "1_C/V",                // Farad
    "Fdy",      "96487_A*s",            // Faraday
    "H",        "1_ohm*s",              // Henry
    "mho",      "1_S",                  // Ohm spelled backwards
    "S",        "1_A/V",                // Siemens
    "T",        "1_V*s/m^2",            // Tesla
    "Wb",       "1_V*s",                // Weber

    // Angles
    "turn",     "1_turn",               // Full turns
    "°",        "1/360_turn",           // Degree
    "grad",     "1/400_turn",           // Grad
    "r",        "0.1591549430918953357688837633725144_turn", // Radian
    "arcmin",   "1/60_°",               // Arc minute
    "arcs",     "1/60_arcmin",          // Arc second
    "sr",       "1_sr",                 // Steradian
    "ℼr",       "1/2_turn",             // Pi radians
    "pir",      "1/2_turn",             // Pi radians

    // Light
    "cd",       "1_cd",                 // Candela
    "lm",       "1_cd*sr",              // Lumen
    "lx"        "1_lm/m^2",             // Lux
    "fc",       "1_lm/ft^2",            // Footcandle
    "flam",     "1_cd/ft^2*r/pir",      // Foot-Lambert
    "ph",       "10000_lx",             // Phot
    "sb",       "10000_cd/m^2",         // Stilb
    "lam",      "1_cd/cm^2*r/pir",      // Lambert
    "nit",      "1_cd/m^2",             // Nit
    "nt",       "1_cd/m^2",             // Nit

    // Radiation
    "Gy",       "1_m^2/s^2",            // Gray
    "rad",      "1/100_m^2/s^2",        // rad
    "rem",      "1_rad",                // rem
    "Sv",       "1_Gy",                 // Sievert
    "Bq",       "1_Hz",                 // Becquerel
    "Ci",       "37_GBq",               // Curie
    "R",        "258_µC/kg"             // Roentgen

    // Viscosity
    "P",        "1/10_Pa*s",            // Poise
    "St",       "1_cm^2/s",             // Stokes

    // Computing
    "bit",      "1_bit",                // Bit
    "byte",     "8_bit",                // Byte
    "B",        "1_byte",               // Byte
    "bps",      "1_bit/s",              // bit per second
    "baud",     "1_bps/SR",             // baud
    "Bd",       "1_baud",               // baud (standard unit)
    "mips",     "1_mips",               // Million instructions per second
    "flops",    "1_flops",              // Floating point operation per second
    "SR",       "1",                    // Symbol rate (default is 1)
    "dB",       "1_dB",                 // decibel
};


struct si_prefix
// ----------------------------------------------------------------------------
//   Representation of a SI prefix
// ----------------------------------------------------------------------------
{
    cstring     prefix;
    int         exponent;
};


static const si_prefix si_prefixes[] =
// ----------------------------------------------------------------------------
//  List of standard SI prefixes
// ----------------------------------------------------------------------------
{
    { "",       0 },                    // No prefix
    { "da",     1 },                    // deca (the only one with 2 letters)
    { "d",     -1 },                    // deci
    { "c",     -2 },                    // centi
    { "h",      2 },                    // hecto
    { "m",     -3 },                    // milli
    { "k",      3 },                    // kilo
    { "K",      3 },                    // kilo (computer-science)
    { "µ",     -6 },                    // micro (0xB5)
    { "μ",     -6 },                    // micro (0x3BC)
    { "M",      6 },                    // mega
    { "n",     -9 },                    // nano
    { "G",      9 },                    // giga
    { "p",    -12 },                    // pico
    { "T",     12 },                    // tera
    { "f",    -15 },                    // femto
    { "P",     15 },                    // peta
    { "a",    -18 },                    // atto
    { "E",     18 },                    // exa
    { "z",    -21 },                    // zepto
    { "Z",     21 },                    // zetta
    { "y",    -24 },                    // yocto
    { "Y",     24 },                    // yotta
    { "r",    -27 },                    // ronna
    { "R",     27 },                    // ronto
    { "q",    -30 },                    // quetta
    { "Q",     30 },                    // quecto
};



unit_p unit::lookup(symbol_p name, int *prefix_info)
// ----------------------------------------------------------------------------
//   Lookup a built-in unit
// ----------------------------------------------------------------------------
{
    size_t maxf  = load_file() ? file_units_count : 0;
    size_t len   = 0;
    gcutf8 gtxt  = name->value(&len);
    uint   maxs  = sizeof(si_prefixes) / sizeof(si_prefixes[0]);
    for (uint si = 0; si < maxs; si++)
    {
        utf8    ntxt   = gtxt;
        cstring prefix = si_prefixes[si].prefix;
        size_t  plen   = strlen(prefix);
        if (memcmp(prefix, ntxt, plen) != 0)
            continue;

        int    e       = si_prefixes[si].exponent;
        size_t maxu    = sizeof(basic_units) / sizeof(basic_units[9]);
        size_t maxkibi = 1 + (e > 0 && e % 3 == 0 &&
                              ntxt[plen] == 'i' && len > plen+1);
        for (uint kibi = 0; kibi < maxkibi; kibi++)
        {
            size_t  rlen = len - plen - kibi;
            utf8    txt  = ntxt + plen + kibi;
            cstring utxt = nullptr;
            cstring udef = nullptr;
            size_t  ulen = 0;

            // Check in-file units
            for (size_t u = 0; !udef && u < maxf; u += 3)
            {
                if (strcasecmp(file_units[u], "cycle"))
                {
                    // If definition is empty, it's a menu-only entry
                    cstring def = file_units[u+2];
                    if (def && *def)
                    {
                        utxt = file_units[u+1];
                        if (memcmp(utxt, txt, rlen) == 0 && utxt[rlen] == 0)
                        {
                            udef = def;
                            ulen  = strlen(udef);
                        }
                    }
                }
            }

            // Check built-in units
            for (size_t u = 0; !udef && u < maxu; u += 2)
            {
                utxt = basic_units[u];
                if (memcmp(utxt, txt, rlen) == 0 && utxt[rlen] == 0)
                {
                    udef = basic_units[u + 1];
                    ulen  = strlen(udef);
                }
            }

            // If we found a definition, use that
            if (udef)
            {
                if (object_p obj = object::parse(utf8(udef), ulen))
                {
                    if (unit_g u = obj->as<unit>())
                    {
                        // Record prefix info if we need it
                        if (prefix_info)
                            *prefix_info = kibi ? -si : si;

                        // Apply multipliers
                        if (e)
                        {
                            // Convert SI exp into value, e.g cm-> 1/100
                            // If kibi mode, use powers of 2
                            algebraic_g exp   = integer::make(e);
                            algebraic_g scale = integer::make(10);
                            if (kibi)
                            {
                                scale = integer::make(3);
                                exp = exp / scale;
                                scale = integer::make(1024);
                            }
                            scale = pow(scale, exp);
                            exp = u.Safe();
                            scale = scale * exp;
                            if (scale)
                                if (unit_p us = scale->as<unit>())
                                    u = us;
                        }

                        // Check if we have a terminal unit
                        algebraic_g uexpr = u->uexpr();
                        if (symbol_g sym = uexpr->as_quoted<symbol>())
                        {
                            size_t slen = 0;
                            utf8   stxt = sym->value(&slen);
                            if (slen == rlen &&
                                memcmp(stxt, utxt, slen) == 0)
                                return u;
                        }

                        // Check if we must evaluate, e.g. 1_min -> seconds
                        uexpr = u->evaluate();
                        if (!uexpr || uexpr->type() != ID_unit)
                        {
                            rt.inconsistent_units_error();
                            return nullptr;
                        }
                        u = unit_p(uexpr.Safe());
                        return u;
                    }
                }
            }
        }
    }
    return nullptr;
}



// ============================================================================
//
//   Unit conversion
//
// ============================================================================

bool unit::convert(algebraic_g &x) const
// ----------------------------------------------------------------------------
//   Convert the object to the given unit
// ----------------------------------------------------------------------------
{
    if (!x.Safe())
        return false;

    // If we already have a unit object, perform a conversion
    if (x->type() == ID_unit)
        return convert((unit_g &) x);

    // Otherwise, convert to a unity unit
    algebraic_g one = algebraic_p(integer::make(1));
    unit_g u = unit::make(x, one);
    if (!convert(u))
        return false;
    x = u.Safe();
    return true;
}


bool unit::convert(unit_g &x) const
// ----------------------------------------------------------------------------
//   Convert a unit object to the current unit
// ----------------------------------------------------------------------------
{
    if (!x.Safe())
        return false;
    algebraic_g u   = uexpr();
    algebraic_g o   = x->uexpr();
    algebraic_g svu = u;

    // Check error case
    if (!u || !o)
        return false;

    // Common case where we have the exact same unit
    if (u->is_same_as(o.Safe()))
        return true;

    if (!unit::mode)
    {
        save<bool> save(unit::mode, true);

        // Evaluate the unit expression for this one
        u = u->evaluate();
        if (!u)
            return false;

        // Evaluate the unit expression for x
        o = o->evaluate();
        if (!o)
            return false;

        // Compute conversion factor
        bool as = Settings.auto_simplify;
        Settings.auto_simplify = true;
        o = o / u;
        Settings.auto_simplify = as;

        // Check if this is a unit and if so, make sure the unit is 1
        while (unit_p cf = o->as<unit>())
        {
            algebraic_g cfu = cf->uexpr();
            if (!cfu->is_real())
            {
                rt.inconsistent_units_error();
                return false;
            }
            o = cf->value();
            if (!cfu->is_one(false))
                o = o * cfu;
        }

        algebraic_g v = x->value();
        v = v * o;
        x = unit_p(unit::simple(v, svu)); // Wrong cast, but OK above
        return true;
    }

    // For now, the rest is not implemented
    return false;
}


unit_p unit::cycle() const
// ----------------------------------------------------------------------------
//   Cycle the unit SI prefix across the closest appropriate ones
// ----------------------------------------------------------------------------
{
    unit_g      u       = this; // GC may move this
    algebraic_g value   = u->value();
    algebraic_g uexpr   = u->uexpr();
    int         max     = sizeof(si_prefixes) / sizeof(si_prefixes[0]);
    bool        decimal = value->is_decimal();
    bool        frac    = value->is_real() && !decimal;

    // Check if we can cycle through the prefixes
    if (symbol_g sym = uexpr->as_quoted<symbol>())
    {
        int index = 0;
        if (lookup(sym, &index))
        {
            bool kibi = index < 0;
            if (kibi)
                index = -index;
            int         exp       = si_prefixes[index].exponent;
            cstring     opfx      = si_prefixes[index].prefix;
            size_t      olen      = strlen(opfx);
            int         candidate = -1;

            if (decimal)
            {
                // Try to see if we can go up in exponents
                int bexp = -1000;
                for (int i = 0; i < max; i++)
                {
                    int nexp = si_prefixes[i].exponent;
                    if (nexp < exp && nexp > bexp)
                    {
                        candidate = i;
                        bexp = nexp;
                    }
                }
            }
            else if (frac)
            {
                // Fraction: go down until we hit exponent mode
                int bexp = 1000;
                for (int i = 0; i < max; i++)
                {
                    int nexp = si_prefixes[i].exponent;
                    if (nexp > exp && nexp < bexp)
                    {
                        candidate = i;
                        bexp = nexp;
                    }
                }
            }

            if (candidate >= 0)
            {
                cstring  nprefix = si_prefixes[candidate].prefix;
                size_t   oulen   = 0;
                utf8     outxt   = sym->value(&oulen);
                scribble scr;
                renderer r;
                r.put(nprefix);
                r.put(outxt + olen, oulen - olen);
                algebraic_g nuexpr = parse_uexpr(r.text(), r.size());
                unit_g nunit = unit::make(integer::make(1), nuexpr);
                if (nunit->convert(u))
                {
                    algebraic_g mag   = integer::make(Settings.standard_exp);
                    algebraic_g range = integer::make(10);
                    algebraic_g nvalue = u->value();
                    range = pow(range, mag);
                    mag = abs::run(nvalue);

                    if (decimal)
                    {
                        algebraic_g test = mag >= range;
                        if (!test->as_truth(false))
                            if (arithmetic::to_decimal(nvalue))
                                return unit::make(nvalue, nuexpr);
                    }
                    else if (frac)
                    {
                        range = inv::run(range);
                        algebraic_g test = mag <= range;
                        if (!test->as_truth(false))
                            return unit::make(nvalue, nuexpr);
                    }
                }
            }
        }
    }

    // Check if we have a fraction or an integer, if so convert to decimal
    if (frac)
    {
        if (arithmetic::to_decimal(value, true))
            u = unit::make(value, uexpr);
    }
    else if (decimal)
    {
        if (arithmetic::decimal_to_fraction(value))
            u = unit::make(value, uexpr);
    }
    return u;
}


bool unit::load_file()
// ----------------------------------------------------------------------------
//   Load the units file
// ----------------------------------------------------------------------------
//   In order to avoid memory fragmentation, and since we load the file once,
//   we do two passes on the file:
//   - the first one where we compute memory requirements,
//   - the second one where we load data into allocated memory
//   This also ensures we deal gracefully with out-of-memory cases
{
    if (!file_loaded)
    {
        file_loaded = true;

        // Try to open the units file
        file units_file("CONFIG/UNITS.CSV", false);
        if (units_file.valid())
        {
            size_t   strings   = 0;
            size_t   chars     = 0;
            char    *text      = nullptr;
            cstring  value     = nullptr;
            cstring *values    = nullptr;
            bool     malformed = false;

            for (int pass = 0; pass < 2; pass++)
            {
                uint     column = 0;
                bool     quoted = false;

                if (pass)
                {
                    size_t memsize = strings * sizeof(cstring) + chars;
                    file_units = (cstring *) malloc(memsize);
                    file_units_count = strings / 3;
                    values = file_units;
                    text = (char *) (file_units + strings);
                    value = text;
                }

                // Restart from beginning of file
                units_file.seek(0);
                while(units_file.valid())
                {
                    unicode c = units_file.get();
                    if (!c)
                        break;

                    if (c == '"')
                    {
                        quoted = !quoted;
                        if (!quoted)
                        {
                            // Defensive coding: ignore anything after column 3
                            if (column < 3)
                            {
                                if (pass)
                                {
                                    *text++ = 0;
                                    *values++ = value;
                                    value = text;
                                }
                                else
                                {
                                    // New entry - Account for trailing 0
                                    chars++;
                                    strings++;
                                }
                            }
                            column++;
                        }
                    }
                    else if (c == '\n')
                    {
                        malformed = (column > 0 && column < 3) || quoted;
                        if (malformed)
                        {
                            if (!pass)
                                record(units_error,
                                       "Malformed row after %u strings, "
                                       "%u columns, %+s",
                                       strings, column,
                                   quoted ? "quoted" : "unquoted");
                            if (quoted)
                            {
                                quoted = false;
                                if (pass)
                                    text = (char *) value;
                                column++;
                            }

                            // Ignore this line, it's malformed
                            if (pass)
                            {
                                values -= column;
                                for (size_t c = 0; c < 3; c++)
                                    values[c] = "";
                            }
                            else
                            {
                                strings -= column;
                            }

                        }
                        column = 0;
                    }
                    else if (quoted)
                    {
                        if (pass)
                            text += utf8_encode(c, (byte *) text);
                        else
                            chars += utf8_size(c);
                    }
                }

                // If last row was malformed, reading it might overflow
                // into the texts> Avoid that by overallocating 3 entries
                if (malformed)
                    strings += 3;

            }
            units_file.close();
        }
    }

    return file_units_count > 0;
}



// ============================================================================
//
//   Build a units menu
//
// ============================================================================

void unit_menu::units(info &mi, cstring name, cstring utable[], size_t count)
// ----------------------------------------------------------------------------
//   Build a units menu
// ----------------------------------------------------------------------------
{
    // Use the units loaded from the units file
    size_t file_entries = unit::load_file() ? file_units_count : 0;
    size_t matching     = 0;
    for (size_t i = 0; i < file_entries; i++)
        if (strcasecmp(file_units[3 * i], name) == 0)
            matching++;

    items_init(mi, count + matching, 3, 1);

    // Insert the built-in units after the ones from the file
    uint skip = mi.skip;
    mi.plane  = 0;
    mi.planes = 1;
    for (uint i = 0; i < matching; i++)
        items(mi, file_units[3 * i + 1], ID_ApplyUnit);
    for (uint i = 0; i < count; i++)
        items(mi, utable[i], ID_ApplyUnit);

    mi.plane  = 1;
    mi.planes = 2;
    mi.skip   = skip;
    mi.index  = mi.plane * ui.NUM_SOFTKEYS;
    for (uint i = 0; i < matching; i++)
        items(mi, file_units[3 * i + 1], ID_ConvertToUnit);
    for (uint i = 0; i < count; i++)
        items(mi, utable[i], ID_ConvertToUnit);

    mi.plane  = 2;
    mi.planes = 3;
    mi.index  = mi.plane * ui.NUM_SOFTKEYS;
    mi.skip   = skip;
    for (uint i = 0; i < matching; i++)
        items(mi, file_units[3 * i + 1], ID_ApplyInverseUnit);
    for (uint i = 0; i < count; i++)
        items(mi, utable[i], ID_ApplyInverseUnit);

    for (uint k = 0; k < ui.NUM_SOFTKEYS - (mi.pages > 1); k++)
    {
        ui.marker(k + 1 * ui.NUM_SOFTKEYS, L'→', true);
        ui.marker(k + 2 * ui.NUM_SOFTKEYS, '/', false);
    }
}


#define UNITS(Name, ...)                                                \
    MENU_BODY(Name##UnitsMenu)                                          \
/* ---------------------------------------------------------------- */  \
/*   Create a system menu                                           */  \
/* ---------------------------------------------------------------- */  \
{                                                                       \
    cstring units_table[] = { __VA_ARGS__ };                            \
    size_t count = sizeof(units_table) / sizeof(units_table[0]);        \
    units(mi, #Name, units_table, count);                               \
    return true;                                                        \
}



// ============================================================================
//
//   Unit-related commands
//
// ============================================================================

COMMAND_BODY(Convert)
// ----------------------------------------------------------------------------
//   Convert level 2 into unit of level 1
// ----------------------------------------------------------------------------
{
    if (!rt.args(2))
        return ERROR;

    unit_p y = rt.stack(1)->as<unit>();
    unit_p x = rt.stack(0)->as<unit>();
    if (!y || !x)
    {
        rt.type_error();
        return ERROR;
    }
    algebraic_g r = y;
    if (!x->convert(r))
        return ERROR;
    if (!r || !rt.drop() || !rt.top(r))
        return ERROR;
    return OK;
}


COMMAND_BODY(UBase)
// ----------------------------------------------------------------------------
//   Convert level 1 to the base SI units
// ----------------------------------------------------------------------------
{
    if (!rt.args(1))
        return ERROR;

    unit_p x = rt.stack(0)->as<unit>();
    if (!x)
    {
        rt.type_error();
        return ERROR;
    }
    algebraic_g r = x;
    save<bool> ueval(unit::mode, true);
    r = r->evaluate();
    if (!r || !rt.top(r))
        return ERROR;
    return OK;
}


COMMAND_BODY(UFact)
// ----------------------------------------------------------------------------
//   Factor level 1 unit out of level 2 unit
// ----------------------------------------------------------------------------
{
    if (!rt.args(2))
        return ERROR;

    unit_p x = rt.stack(0)->as<unit>();
    unit_p y = rt.stack(1)->as<unit>();
    if (!x || !y)
    {
        rt.type_error();
        return ERROR;
    }

    algebraic_g xa = x;
    algebraic_g ya = y;
    save<bool> ueval(unit::mode, true);
    algebraic_g r = xa * (ya / xa);
    if (r->is_same_as(ya))
    {
        algebraic_g d = xa->evaluate();
        ya = ya->evaluate();
        r = xa * (ya / d);
    }
    if (!r || !rt.drop() || !rt.top(r))
        return ERROR;
    return OK;
}


FUNCTION_BODY(UVal)
// ----------------------------------------------------------------------------
//   Extract value from unit object in level 1
// ----------------------------------------------------------------------------
{
    if (!x.Safe())
        return nullptr;
    if (x->is_symbolic())
        return symbolic(ID_UVal, x);
    if (unit_p u = x->as<unit>())
        return u->value();
    rt.type_error();
    return nullptr;
}


COMMAND_BODY(ToUnit)
// ----------------------------------------------------------------------------
//   Combine a value and a unit object to build a new unit object
// ----------------------------------------------------------------------------
{
    if (!rt.args(2))
        return ERROR;

    object_p y = rt.stack(1);
    unit_p x = rt.stack(0)->as<unit>();
    if (!x || !y || !y->is_algebraic())
    {
        rt.type_error();
        return ERROR;
    }
    algebraic_g u = algebraic_p(y);
    algebraic_g result = unit::simple(u, x->uexpr());
    if (result && rt.pop() && rt.top(result))
        return OK;
    return ERROR;
}


static algebraic_p key_unit(uint key)
// ----------------------------------------------------------------------------
//   Return a softkey label as a unit value
// ----------------------------------------------------------------------------
{
    if (key >= KEY_F1 && key <= KEY_F6)
    {
        if (cstring label = ui.label_text(key - KEY_F1))
        {
            char buffer[16];
            save<bool> umode(unit::mode, true);
            size_t     len = strlen(label);
            buffer[0] = '1';
            buffer[1] = '_';
            memcpy(buffer+2, label, len);
            len += 2;
            if (object_p uobj = object::parse(utf8(buffer), len))
                if (unit_p u = uobj->as<unit>())
                    return u->uexpr();
        }
    }
    return nullptr;
}


COMMAND_BODY(ApplyUnit)
// ----------------------------------------------------------------------------
//   Apply a unit from a unit menu
// ----------------------------------------------------------------------------
{
    int key = ui.evaluating;
    if (rt.editing())
    {
        if (ui.editing_mode() != ui.DIRECT)
            return ui.insert_softkey(key, "_", " ");
        if (!ui.end_edit())
            return object::ERROR;
    }

    if (!rt.args(1))
        return ERROR;

    if (algebraic_g uname = key_unit(key))
        if (object_p value = rt.top())
            if (algebraic_g alg = value->as_algebraic())
                if (algebraic_g uobj = unit::simple(alg, uname))
                    if (rt.top(uobj.Safe()))
                        return OK;

    if (!rt.error())
        rt.type_error();
    return ERROR;
}


COMMAND_BODY(ApplyInverseUnit)
// ----------------------------------------------------------------------------
//   Apply the invserse of a unit from a unit menu
// ----------------------------------------------------------------------------
{
    int key = ui.evaluating;
    if (rt.editing())
    {
        if (ui.editing_mode() != ui.DIRECT)
            return ui.insert_softkey(key, "_(", ")⁻¹ ");
        if (!ui.end_edit())
            return object::ERROR;
    }

    if (!rt.args(1))
        return ERROR;

    if (algebraic_g uname = key_unit(key))
        if (object_p value = rt.top())
            if (algebraic_g alg = value->as_algebraic())
                if (algebraic_g uobj = unit::simple(alg, inv::run(uname)))
                    if (rt.top(uobj.Safe()))
                        return OK;

    if (!rt.error())
        rt.type_error();
    return ERROR;
}


COMMAND_BODY(ConvertToUnit)
// ----------------------------------------------------------------------------
//   Apply conversion to a given menu unit
// ----------------------------------------------------------------------------
{
    int key = ui.evaluating;
    if (rt.editing())
    {
        if (ui.editing_mode() != ui.DIRECT)
            return ui.insert_softkey(key, " 1_", " Convert ");
        if (!ui.end_edit())
            return object::ERROR;
    }

    if (!rt.args(1))
        return ERROR;

    if (algebraic_g uname = key_unit(key))
        if (object_p value = rt.top())
            if (algebraic_g alg = value->as_algebraic())
                if (algebraic_g one = integer::make(1))
                    if (unit_g uobj = unit::make(one, uname))
                        if (uobj->convert(alg))
                            if (rt.top(alg.Safe()))
                                return OK;

    return ERROR;
}


static symbol_p unit_name(object_p obj)
// ----------------------------------------------------------------------------
//    If the object is a simple unit like `1_m`, return `m`
// ----------------------------------------------------------------------------
{
    if (obj)
    {
        if (unit_p uobj = obj->as<unit>())
        {
            algebraic_p uexpr = uobj->uexpr();
            symbol_p name = uexpr->as<symbol>();
            if (!name)
                if (expression_p eq = uexpr->as<expression>())
                    if (symbol_p inner = eq->as_quoted<symbol>())
                        name = inner;
            return name;
        }
    }
    return nullptr;
}


COMMAND_BODY(ConvertToUnitPrefix)
// ----------------------------------------------------------------------------
//   Convert to a given unit prefix
// ----------------------------------------------------------------------------
{
    int key = ui.evaluating;
    if (rt.editing())
    {
        if (ui.editing_mode() != ui.DIRECT)
            return ui.insert_softkey(key, "_", "", ' ');
        if (!ui.end_edit())
            return object::ERROR;
    }
    if (key < KEY_F1 || key > KEY_F6)
        return object::OK;

    if (!rt.args(1))
        return ERROR;

    // Read the prefix (e.g. "c") from the softkey label,
    uint index = key - KEY_F1 + ui.NUM_SOFTKEYS * ui.shift_plane();
    cstring prefix = ui.label_text(index);
    if (!prefix)
    {
        rt.undefined_operation_error();
        return ERROR;
    }

    // Read the stack value
    object_p value = rt.top();
    if (!value)
        return ERROR;

    // This must be a unit type with a simple name
    unit_g   un  = value->as<unit>();
    symbol_p sym = unit_name(un);
    if (!sym)
    {
        rt.type_error();
        return ERROR;
    }

    // Lookup the name to get the underlying unit, e.g. 1_km -> 1000_m
    unit_p   base = unit::lookup(sym);
    symbol_g bsym = unit_name(base);
    if (!bsym)
    {
        rt.inconsistent_units_error();
        return ERROR;
    }

    // Build a unit with the prefix and the base
    gcutf8 ptxt = utf8(prefix);
    size_t plen = strlen(prefix);
    if (cstring space = strchr(prefix, ' '))
    {
        size_t offset = space - prefix;
        if (plen > offset)
            plen = offset;
    }

    // Render 1_cm if the prefix is c
    renderer r;
    r.put("1_");
    r.put(ptxt, plen);
    ptxt = bsym->value(&plen);
    r.put(ptxt, plen);

    plen = r.size();
    object_p scaled = object::parse(r.text(), plen);
    if (!scaled)
        return ERROR;
    unit_p target = scaled->as<unit>();
    if (!target)
    {
        rt.inconsistent_units_error();
        return ERROR;
    }

    // Perform the conversion to the desired unit
    algebraic_g x = un.Safe();
    if (!target->convert(x))
    {
        rt.inconsistent_units_error();
        return ERROR;
    }

    if (!rt.top(x))
        return ERROR;
    return OK;
}



// ============================================================================
//
//   Units menus
//
// ============================================================================

UNITS(Length,
// ----------------------------------------------------------------------------
//   Length units menu
// ----------------------------------------------------------------------------
      "m",      "yd",   "ft",   "ftUS", "US",           // Human scale
      "cm",     "mm",   "in",   "mil",  "μm",           // Small stuff
      "km",     "mi",   "nmi",  "miUS", "fur",          // Short travel distance
      "ch",     "rd",   "cable","fath", "league",       // US Survey
      "Mpc",    "pc",   "lyr",  "au",   "ls",           // Astronomy
      "mi",     "miUS", "ft",   "ftUS", "US",           // US.Survey, pre-2023
      "cable",  "link", "icable","acable", "nmi",       // Nautical
      "Å",      "fermi", "μm",   "nm",   "pm"           // Microscopic
    );


UNITS(Area,
// ----------------------------------------------------------------------------
//   Area units menu
// ----------------------------------------------------------------------------
      "m^2",    "yd^2", "ft^2", "in^2", "cm^2",         // Human scale
      "km^2",   "mi^2", "ha",   "a",    "acre",         // Surveying
      "m^2",    "cm^2", "km^2", "ha",   "a",            // Metric
      "b",      "miUS^2","ftUS^2"                       // Misc
    );


UNITS(Volume,
// ----------------------------------------------------------------------------
//   Volume
// ----------------------------------------------------------------------------
      "m^3", "st", "cm^3", "yd^3", "ft^3", "in^3",
      "l", "galUK", "galC", "gal", "qt", "pt",
      "ml", "cu", "ozfl", "ozUK", "tbsp", "tsp",
      "bbl", "bu", "pk", "fbm"
    );


UNITS(Time,
// ----------------------------------------------------------------------------
//   Time
// ----------------------------------------------------------------------------
      "s", "min", "h", "d", "yr", "Hz"
    );


UNITS(Speed,
// ----------------------------------------------------------------------------
//   Speed
// ----------------------------------------------------------------------------
      "m/s", "km/h", "ft/s", "mph", "knot",
      "c", "ga"
    );


UNITS(Mass,
// ----------------------------------------------------------------------------
//   Mass
// ----------------------------------------------------------------------------
      "kg",     "g",    "t",    "ct",   "mol",
      "lb",     "oz",   "dr",   "stone","grain",
      "qrUS",   "cwtUS","tonUS","slug", "blob",
      "lbt",    "ozt",  "dwt",  "tonUK","u"
    );


UNITS(Force,
// ----------------------------------------------------------------------------
//   Force
// ----------------------------------------------------------------------------
      "N", "dyn", "gf", "kip", "lbf", "pdl",
    );


UNITS(Energy,
// ----------------------------------------------------------------------------
//   Energy
// ----------------------------------------------------------------------------
      "J",      "erg",  "Kcal", "cal",  "Btu",
      "ft×lb",  "therm","MeV",  "eV"
    );


UNITS(Power,
// ----------------------------------------------------------------------------
//   Power
// ----------------------------------------------------------------------------
      "W", "kW", "MW", "GW", "hp"
    );


UNITS(Pressure,
// ----------------------------------------------------------------------------
//   Pressure
// ----------------------------------------------------------------------------
      "Pa", "atm", "bar", "psi", "torr", "mmHg",
      "inHg", "inH2O"
    );


UNITS(Temperature,
// ----------------------------------------------------------------------------
//   Temperature
// ----------------------------------------------------------------------------
      "°C", "°F", "K", "°R"
    );


UNITS(Electricity,
// ----------------------------------------------------------------------------
//   Electricity
// ----------------------------------------------------------------------------
      "V", "A", "C", "Ω", "F", "W",
      "Fdy", "H", "mho", "S", "T", "Wb"
    );


UNITS(Angle,
// ----------------------------------------------------------------------------
//   Angles
// ----------------------------------------------------------------------------
      "°",      "r",    "grad",         "arcmin",       "arcs",
      "turn",   "sr",   "ℼr"
    );


UNITS(Light,
// ----------------------------------------------------------------------------
//   Light and radiations
// ----------------------------------------------------------------------------
      "cd", "lm", "lx", "fc", "flam",
      "ph", "sb", "lam", "nit"
    );


UNITS(Radiation,
// ----------------------------------------------------------------------------
//   Radiations
// ----------------------------------------------------------------------------
      "Gy", "rad", "rem", "Sv", "Bq",
      "Ci", "R"

    );


UNITS(Viscosity,
// ----------------------------------------------------------------------------
//   Viscosity
// ----------------------------------------------------------------------------
      "P", "St"
    );



UNITS(Computer,
// ----------------------------------------------------------------------------
//   Units for computer use
// ----------------------------------------------------------------------------
      "B",      "byte", "bit",  "flops", "mips",
      "baud",   "bps"   "SR",   "dB",
    );
