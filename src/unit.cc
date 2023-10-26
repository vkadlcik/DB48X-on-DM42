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
#include "equation.h"
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


HELP_BODY(unit)
// ----------------------------------------------------------------------------
//   Help topic for units
// ----------------------------------------------------------------------------
{
    return utf8("Units");
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
    if (unit_p u = x->as<unit>())
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

    // Common case where we have the exact same unit
    if (u.Safe() && o.Safe() && u->is_same_as(o.Safe()))
        return true;

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


COMMAND_BODY(ApplyUnit)
// ----------------------------------------------------------------------------
//   Apply a unit from a unit menu
// ----------------------------------------------------------------------------
{
    int key = ui.evaluating;
    if (rt.editing())
        return ui.insert_softkey(key, "_", " ");

    if (key >= KEY_F1 && key <= KEY_F6)
    {
        if (symbol_p name = ui.label(key - KEY_F1))
        {
            if (object_p value = rt.top())
            {
                if (algebraic_g alg = value->as_algebraic())
                {
                    unit_g uobj = unit::make(alg, name);
                    if (rt.top(uobj.Safe()))
                        return OK;
                }
            }
        }
    }

    return ERROR;
}


COMMAND_BODY(ApplyInverseUnit)
// ----------------------------------------------------------------------------
//   Apply the invserse of a unit from a unit menu
// ----------------------------------------------------------------------------
{
    int key = ui.evaluating;
    if (rt.editing())
        return ui.insert_softkey(key, "_(", ")⁻¹ ");

    if (key >= KEY_F1 && key <= KEY_F6)
    {
        if (symbol_p name = ui.label(key - KEY_F1))
        {
            if (object_p value = rt.top())
            {
                if (algebraic_g alg = value->as_algebraic())
                {
                    unit_g uobj = unit::make(alg, name);
                    if (rt.top(uobj.Safe()))
                        return OK;
                }
            }
        }
    }

    return ERROR;
}


COMMAND_BODY(ConvertToUnit)
// ----------------------------------------------------------------------------
//   Apply conversion to a given menu unit
// ----------------------------------------------------------------------------
{
    int key = ui.evaluating;
    if (rt.editing())
        return ui.insert_softkey(key, " 1_", " Convert ");

    rt.unimplemented_error();
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
