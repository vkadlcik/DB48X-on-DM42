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
#include "equation.h"
#include "functions.h"
#include "integer.h"
#include "parser.h"
#include "renderer.h"
#include "settings.h"


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
    object::result result = list::list_parse(ID_equation, p, 0, 0);
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

    while (unit_p vu = v->as<unit>())
    {
        u = u * vu->uexpr();
        v = vu->value();
        while (unit_p uu = u->as<unit>())
        {
            v = v * uu->value();
            u = uu->uexpr();
        }
    }
    return rt.make<unit>(ty, v, u);
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
    if (equation_p ueq = uexpr->as<equation>())
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
    value = unit::make(value, uexpr);
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
    // Length
    "m",        "1_m",
    "yd",       "9144/10000_m",
    "ft",       "3048/10000_m",
    "in",       "254/10000_m",
    "pc",       "30856775814913673_m",  // Parsec
    "ls",       "299792458_m",          // Light-second
    "lyr",      "31557600_ls",          // Light year
    "au",       "149597870700_m",       // Astronomical unit

    // Duration
    "s",        "1_s",
    "min",      "60_s",
    "h",        "3600_s",
    "d",        "86400_s",
    "yr",       "36524219/100000_d",    // Mean tropical year
    "Hz",       "1_s⁻¹",
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
    { "μ",     -6 },                    // micro
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



unit_p unit::lookup(symbol_p name)
// ----------------------------------------------------------------------------
//   Lookup a built-in unit
// ----------------------------------------------------------------------------
{
    size_t len  = 0;
    utf8   ntxt = name->value(&len);
    size_t maxs = sizeof(si_prefixes) / sizeof(si_prefixes[0]);
    for (size_t si = 0; si < maxs; si++)
    {
        cstring prefix = si_prefixes[si].prefix;
        size_t  plen   = strlen(prefix);
        if (memcmp(prefix, ntxt, plen) != 0)
            continue;

        size_t rlen = len - plen;
        utf8 txt = ntxt + plen;

        size_t maxu  = sizeof(basic_units) / sizeof(basic_units[9]);
        for (size_t u = 0; u < maxu; u += 2)
        {
            cstring utxt = basic_units[u];
            if (memcmp(utxt, txt, rlen) == 0 && utxt[rlen] == 0)
            {
                cstring udef = basic_units[u + 1];
                size_t  len  = strlen(udef);
                if (object_p obj = object::parse(utf8(udef), len))
                {
                    if (unit_g u = obj->as<unit>())
                    {
                        // Apply multipliers
                        int e = si_prefixes[si].exponent;
                        if (e)
                        {
                            // Convert si exponent into value, e.g cm-> 1/100
                            algebraic_g scale = integer::make(10);
                            algebraic_g exp   = integer::make(e);
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
                            if (slen == rlen && memcmp(stxt, utxt, slen) == 0)
                                return u;
                        }

                        // Check if we need to evaluate, e.g. 1_min -> seconds
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
    return convert(x);
}


bool unit::convert(unit_g &x) const
// ----------------------------------------------------------------------------
//   Convert a unit object to the current unit
// ----------------------------------------------------------------------------
{
    if (!x.Safe())
        return false;
    algebraic_g u = uexpr();
    algebraic_g o = x->uexpr();

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
        u = uexpr();
        x = unit::make(v, u);
        return true;
    }

    // For now, the rest is not implemented
    return false;
}



// ============================================================================
//
//   Build a units menu
//
// ============================================================================

void unit_menu::units(info &mi, cstring utable[], size_t count)
// ----------------------------------------------------------------------------
//   Build a units menu
// ----------------------------------------------------------------------------
{
    items_init(mi, count, 3, 1);

    uint skip = mi.skip;
    mi.plane  = 0;
    mi.planes = 1;
    for (uint i = 0; i < count; i++)
        items(mi, utable[i], ID_ApplyUnit);

    mi.plane  = 1;
    mi.planes = 2;
    mi.skip   = skip;
    mi.index  = mi.plane * ui.NUM_SOFTKEYS;
    for (uint i = 0; i < count; i++)
        items(mi, utable[i], ID_ConvertToUnit);

    mi.plane  = 2;
    mi.planes = 3;
    mi.index  = mi.plane * ui.NUM_SOFTKEYS;
    mi.skip   = skip;
    for (uint i = 0; i < count; i++)
        items(mi, utable[i], ID_ApplyInverseUnit);

    for (uint k = 0; k < ui.NUM_SOFTKEYS - (mi.pages > 1); k++)
    {
        ui.marker(k + 1 * ui.NUM_SOFTKEYS, L'→', true);
        ui.marker(k + 2 * ui.NUM_SOFTKEYS, '/', true);
    }

}


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


static symbol_p key_label(uint key)
// ----------------------------------------------------------------------------
//   Return a unit name as a label
// ----------------------------------------------------------------------------
{
    if (key >= KEY_F1 && key <= KEY_F6)
        if (cstring label = ui.labelText(key - KEY_F1))
            if (symbol_p name = symbol::make(label))
                return name;
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

    if (symbol_p name = key_label(key))
        if (object_p value = rt.top())
            if (algebraic_g alg = value->as_algebraic())
                if (unit_g uobj = unit::make(alg, name))
                    if (rt.top(uobj.Safe()))
                        return OK;

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

    if (symbol_p name = key_label(key))
        if (object_p value = rt.top())
            if (algebraic_g alg = value->as_algebraic())
                if (unit_g uobj = unit::make(alg, inv::run(name)))
                    if (rt.top(uobj.Safe()))
                        return OK;

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

    if (symbol_p name = key_label(key))
        if (object_p value = rt.top())
            if (algebraic_g alg = value->as_algebraic())
                if (algebraic_g one = integer::make(1))
                    if (unit_g uobj = unit::make(one, name))
                        if (uobj->convert(alg))
                            if (rt.top(alg.Safe()))
                                return OK;

    return ERROR;
}



// ============================================================================
//
//   Units menu
//
// ============================================================================

#define UNITS(UnitMenu, ...)                                            \
MENU_BODY(UnitMenu)                                                     \
/* ---------------------------------------------------------------- */  \
/*   Create a system menu                                           */  \
/* ---------------------------------------------------------------- */  \
{                                                                       \
    cstring units_table[] = { __VA_ARGS__ };                            \
    size_t count = sizeof(units_table) / sizeof(units_table[0]);        \
    units(mi, units_table, count);                                      \
    return true;                                                        \
}


UNITS(LengthUnitsMenu,
// ----------------------------------------------------------------------------
//   LengthUnitsMenu
// ----------------------------------------------------------------------------
      "m", "cm", "mm", "km", "μm",
      "yd", "ft", "in", "mi", "miUS",
      "Mpc", "pc", "lyr", "au", "fath",
      "ftUS", "chain", "rd", "mil",
      "Å", "fermi"
    );


UNITS(AreaUnitsMenu,
// ----------------------------------------------------------------------------
//   AreaUnitsMenu
// ----------------------------------------------------------------------------
      "m^2", "cm^2", "b", "yd^2", "ft^2", "in^2",
      "km^2", "ha", "a", "mi^2", "miUS^2", "acre"
    );


UNITS(VolumeUnitsMenu,
// ----------------------------------------------------------------------------
//   VolumeUnitsMenu
// ----------------------------------------------------------------------------
      "m^3", "st", "cm^3", "yd^3", "ft^3", "in^3",
      "l", "galUK", "galC", "gal", "qt", "pt",
      "ml", "cu", "ozfl", "ozUK", "tbsp", "tsp",
      "bbl", "bu", "pk", "fbm"
    );


UNITS(TimeUnitsMenu,
// ----------------------------------------------------------------------------
//   TimeUnitsMenu
// ----------------------------------------------------------------------------
      "yr", "d", "h", "min", "s", "Hz"
    );


UNITS(SpeedUnitsMenu,
// ----------------------------------------------------------------------------
//   SpeedUnitsMenu
// ----------------------------------------------------------------------------
      "m/s", "cm/s", "ft/s", "kph", "mph", "knot",
      "c", "ga"
    );


UNITS(MassUnitsMenu,
// ----------------------------------------------------------------------------
//   MassUnitsMenu
// ----------------------------------------------------------------------------
     "kg", "g", "lb", "oz", "slug",
      "lbt", "ton", "tonUK", "t", "ozt",
      "ct", "grain", "u", "mol"
    );


UNITS(ForceUnitsMenu,
// ----------------------------------------------------------------------------
//   ForceUnitsMenu
// ----------------------------------------------------------------------------
      "N", "dyn", "gf", "kip", "lbf", "pdl",
    );


UNITS(EnergyUnitsMenu,
// ----------------------------------------------------------------------------
//   EnergyUnitsMenu
// ----------------------------------------------------------------------------
      "J", "erg", "Kcal", "cal", "Btu", "ft×lb",
      "therm", "MeV", "eV"
    );


UNITS(PowerUnitsMenu,
// ----------------------------------------------------------------------------
//   PowerUnitsMenu
// ----------------------------------------------------------------------------
      "W", "kW", "MW", "GW", "hp"
    );


UNITS(PressureUnitsMenu,
// ----------------------------------------------------------------------------
//   PressureUnitsMenu
// ----------------------------------------------------------------------------
      "Pa", "atm", "bar", "psi", "torr", "mmHg",
      "inHg", "inH2O"
    );


UNITS(TemperatureUnitsMenu,
// ----------------------------------------------------------------------------
//   TemperatureUnitsMenu
// ----------------------------------------------------------------------------
      "°C", "°F", "K", "°R"
    );


UNITS(ElectricityUnitsMenu,
// ----------------------------------------------------------------------------
//   ElectricityUnitsMenu
// ----------------------------------------------------------------------------
      "V", "A", "C", "Ω", "F", "W",
      "Fdy", "H", "mho", "S", "T", "Wb"
    );


UNITS(AngleUnitsMenu,
// ----------------------------------------------------------------------------
//   AnglesUnitsMenu
// ----------------------------------------------------------------------------
      "°", "r", "grad", "archi", "arcs", "sr", "ℼ×r"
    );


UNITS(LightUnitsMenu,
// ----------------------------------------------------------------------------
//   LightUnitsMenu
// ----------------------------------------------------------------------------
      "fc", "flam", "lx", "ph", "sb", "lm", "cd", "lam"
    );


UNITS(RadiationUnitsMenu,
// ----------------------------------------------------------------------------
//   RadiationsUnitsMenu
// ----------------------------------------------------------------------------
      "Gy", "rad", "rem", "Sv", "Bq", "Ci", "R"

    );


UNITS(ViscosityUnitsMenu,
// ----------------------------------------------------------------------------
//   ViscosityUnitsMenu
// ----------------------------------------------------------------------------
      "P", "St"
    );
