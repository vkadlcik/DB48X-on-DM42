// ****************************************************************************
//  constants.cc                                                  DB48X project
// ****************************************************************************
//
//   File Description:
//
//     Constant values loaded from a constants file
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

#include "constants.h"

#include "algebraic.h"
#include "arithmetic.h"
#include "compare.h"
#include "expression.h"
#include "file.h"
#include "functions.h"
#include "parser.h"
#include "renderer.h"
#include "settings.h"
#include "unit.h"
#include "user_interface.h"
#include "utf8.h"


RECORDER(constants,         16, "Constant objects");
RECORDER(constants_error,   16, "Error on constant objects");

#define CFILE   "config/constants.csv"


PARSE_BODY(constant)
// ----------------------------------------------------------------------------
//    Try to parse this as a constant
// ----------------------------------------------------------------------------
{
    utf8    source = p.source;
    size_t  max    = p.length;
    size_t  parsed = 0;

    // First character must be a constant marker
    unicode cp = utf8_codepoint(source);
    if (cp != settings::CONSTANT_MARKER)
        return SKIP;
    parsed = utf8_next(source, parsed, max);
    size_t first = parsed;

    // Other characters must be alphabetic
    while (parsed < max && is_valid_in_name(source + parsed))
        parsed = utf8_next(source, parsed, max);
    if (parsed <= first)
        return SKIP;

    gcutf8 text   = source + first;
    p.end         = parsed;
    p.out         = rt.make<constant>(ID_constant, text, parsed - first);

    return OK;
}


RENDER_BODY(constant)
// ----------------------------------------------------------------------------
//   Render the constant into the given constant buffer
// ----------------------------------------------------------------------------
{
    size_t len    = 0;
    utf8   txt    = o->name(&len);
    auto   format = r.editing() ? ID_LongFormNames : Settings.NameDisplayMode();
    if (r.editing())
        r.put(unicode(settings::CONSTANT_MARKER));
    r.put(format, txt, len);
    return r.size();
}


GRAPH_BODY(constant)
// ----------------------------------------------------------------------------
//   Do not italicize constants
// ----------------------------------------------------------------------------
{
    return object::do_graph(o, g);
}


EVAL_BODY(constant)
// ----------------------------------------------------------------------------
//   Check if we need to convert to numeric
// ----------------------------------------------------------------------------
{
    // Check if we should preserve the constant as is
    if (!Settings.NumericalConstants() && !Settings.NumericalResults())
        return rt.push(o) ? OK : ERROR;
    algebraic_g value = o->value();
    return rt.push(+value) ? OK : ERROR;
}


HELP_BODY(constant)
// ----------------------------------------------------------------------------
//   Help topic for constants
// ----------------------------------------------------------------------------
{
    return utf8("Constants");
}



// ============================================================================
//
//   Constant lookup
//
// ============================================================================

static const cstring basic_constants[] =
// ----------------------------------------------------------------------------
//   List of basic constants
// ----------------------------------------------------------------------------
//   clang-format off
{
    // ------------------------------------------------------------------------
    // MATH CONSTANTS MENU
    // ------------------------------------------------------------------------
    "Math",   nullptr,

    "π",        "=",                    // Evaluated specially (decimal-pi.h)
    "e",        "=",                    // Evaluated specially (decimal-e.h)
    "i",        "0+ⅈ1",                 // Imaginary unit
    "∞",        "9.99999E999999",       // A small version of infinity
    "?",        "Undefined",            // Undefined result


    // ------------------------------------------------------------------------
    //   Chemistry
    // ------------------------------------------------------------------------

    "Chem",     nullptr,

    "NA",       "6.0221367E23_mol⁻¹",   // Avogradro's number
    "k",        "1.380658E-23_J/K",     // Boltzmann
    "Vm",       "22.4141_mol⁻¹",        // Molar volume
    "R",        "8.31451_J/(mol*K)",    // Universal gas constant
    "StdT",     "273.15_K",             // Standard temperature
    "StdP",     "101.325_kPa",          // Standard temperature
    "σ",        "5.67051E-8_W/(m^2*K^4)", // Stefan-Boltzmann

    // ------------------------------------------------------------------------
    //   Physics
    // ------------------------------------------------------------------------

    "Phys",     nullptr,

    "c",        "299792458_m/s",        // Speed of light
    "ε0",       "8.85418781761E-12_F/m",// Vaccuum permittivity
    "μ0",       "1.25663706144E-6_H/m", // Vaccuum permeability
    "g",        "9.80665_m/s²",         // Acceleration of Earth gravity
    "G",        "6.67259E-11_m^3/(s^2•kg)",// Gravitation constant
    "h",        "6.6260755E-34_J*s",    // Planck
    "hbar",     "1.05457266E-34_J*s",   // Dirac
    "q",        "1.60217733E-19_C",     // Electronic charge
    "me",       "9.1093897E-31_kg",     // Electron mass
    "qme",      "175881962000_C/kg",    // q/me ratio
    "mp",       "1.6726231E-27_kg",     // proton mass
    "mpme",     "1836.152701",          // mp/me ratio
    "α",        "0.00729735308",        // fine structure
    "ø",        "2.06783461E-15_Wb",    // Magnetic flux quantum
    "F",        "96485.309_C/mol",      // Faraday
    "R∞",       "10973731.534_m⁻¹",     // Rydberg
    "a0",       "0.0529177249_nm",      // Bohr radius
    "μB",       "9.2740154E-24_J/T",    // Bohr magneton
    "μN",       "5.0507866E-27_J/T",    // Nuclear magneton
    "λ0",       "1239.8425_nm",         // Photon wavelength
    "f0",       "2.4179883E14_Hz",      // Photon frequency
    "λc",       "0.00242631058_nm",     // Compton wavelength
    "rad",      "1_r",                  // One radian
    "twoπ",     "π_2*r",                // Two pi radian
    "angl",     "180_°",                // Half turn
    "c3",       "0.002897756_m*K",      // Wien's
    "kq",       "0.00008617386_J/(K*C)",// k/q
    "ε0q",      "55263469.6_F/(m*C)",   // ε0/q
    "qε0",      "1.4185978E-30_F*C/ m", // q*ε0
    "εsi",      "11.9",                 // Dielectric constant
    "εox",      "3.9",                  // SiO2 dielectric constant
    "I0",       "0.000000000001_W/m^2"  // Ref intensity
};
//   clang-format on



algebraic_p constant::value() const
// ----------------------------------------------------------------------------
//   Lookup a built-in constant
// ----------------------------------------------------------------------------
{
    size_t    len  = 0;
    gcutf8    gtxt = name(&len);
    unit_file cfile(CFILE);
    size_t    maxu = sizeof(basic_constants) / sizeof(basic_constants[0]);

    utf8      txt  = +gtxt;
    cstring   ctxt = nullptr;
    cstring   cdef = nullptr;
    size_t    clen = 0;

    // Check in-file constants
    if (cfile.valid())
    {
        bool first = true;
        while (symbol_p def = cfile.lookup(txt, len, false, first))
        {
            first     = false;
            utf8 fdef = def->value(&clen);

            // If definition begins with '=', only show constant in menus
            if (*fdef != '=')
            {
                cdef = cstring(fdef);
                ctxt = cstring(txt);
                break;
            }
        }
    }

    // Check built-in constants
    for (size_t u = 0; !cdef && u < maxu; u += 2)
    {
        ctxt = basic_constants[u];
        if (memcmp(ctxt, txt, len) == 0 && ctxt[len] == 0)
        {
            cdef = basic_constants[u + 1];
            if (cdef)
                clen = strlen(cdef);
        }
    }

    // If we found a definition, use that unless it begins with '='
    if (cdef)
    {
        // Special cases for pi and e where we have built-in constants
        if (cdef[0] == '=' && cdef[1] == 0)
        {
            if (!strcmp(ctxt, "π"))
                return decimal::pi();
            else if (!strcmp(ctxt, "e"))
                return decimal::e();
        }
        else
        {
            if (object_p obj = object::parse(utf8(cdef), clen))
                if (algebraic_p alg = obj->as_algebraic())
                    return alg;
        }
    }
    rt.invalid_constant_error();
    return nullptr;
}



// ============================================================================
//
//   Build a constants menu
//
// ============================================================================

utf8 constant_menu::name(id type, size_t &len)
// ----------------------------------------------------------------------------
//   Return the name associated with the type
// ----------------------------------------------------------------------------
{
    uint count = type - ID_ConstantsMenu00;
    unit_file cfile(CFILE);

    // List all preceding entries
    if (cfile.valid())
        while (symbol_p mname = cfile.next(true))
            if (*mname->value() != '=')
                if (!count--)
                    return mname->value(&len);

    if (Settings.ShowBuiltinConstants())
    {
        size_t maxu = sizeof(basic_constants) / sizeof(basic_constants[0]);
        for (size_t u = 0; u < maxu; u += 2)
        {
            if (!basic_constants[u+1] || !*basic_constants[u+1])
            {
                if (!count--)
                {
                    len = strlen(basic_constants[u]);
                    return utf8(basic_constants[u]);
                }
            }
        }
    }

    return nullptr;
}


MENU_BODY(constant_menu)
// ----------------------------------------------------------------------------
//   Build a constants menu
// ----------------------------------------------------------------------------
{
    // Use the constants loaded from the constants file
    unit_file cfile(CFILE);
    size_t    matching = 0;
    size_t    maxu     = sizeof(basic_constants) / sizeof(basic_constants[0]);
    uint      position = 0;
    uint      count    = 0;
    size_t    first    = 0;
    size_t    last     = maxu;
    id        type     = o->type();
    id        menu     = ID_ConstantsMenu00;

    if (cfile.valid())
    {
        while (symbol_p mname = cfile.next(true))
        {
            if (*mname->value() == '=')
                continue;
            if (menu == type)
            {
                position = cfile.position();
                while (cfile.next(false))
                    matching++;
                break;
            }
            menu = id(menu + 1);
        }
    }

     // Disable built-in constants if we loaded a file
    if (!matching || Settings.ShowBuiltinConstants())
    {
        bool found = false;
        for (size_t u = 0; u < maxu; u += 2)
        {
            if (!basic_constants[u+1] || !*basic_constants[u+1])
            {
                if (found)
                {
                    last = u;
                    break;
                }
                if (menu == type)
                {
                    found = true;
                    first = u + 2;
                }
                menu = id(menu + 1);
            }
        }
        count = (last - first) / 2;
    }

    items_init(mi, count + matching, 2, 1);

    // Insert the built-in constants after the ones from the file
    uint skip = mi.skip;
    for (uint plane = 0; plane < 2; plane++)
    {
        static const id ids[2] = { ID_ConstantName, ID_ConstantValue };
        mi.plane  = plane;
        mi.planes = plane + 1;
        mi.index  = plane * ui.NUM_SOFTKEYS;
        mi.skip   = skip;
        id type = ids[plane];

        if (matching)
        {
            cfile.seek(position);
            if (plane == 0)
            {
                while (symbol_g mentry = cfile.next(false))
                    items(mi, mentry, type);
            }
            else
            {
                while (symbol_g mentry = cfile.next(false))
                {
                    uint posafter = cfile.position();
                    size_t mlen = 0;
                    utf8 mtxt = mentry->value(&mlen);
                    cfile.seek(position);
                    mentry = cfile.lookup(mtxt, mlen, false, false);
                    cfile.seek(posafter);
                    if (mentry)
                    {
                        size_t vlen = 0;
                        utf8 vtxt = mentry->value(&vlen);
                        if (vlen == 1 && *vtxt == '=')
                        {
                            decimal_g value = nullptr;
                            settings::SaveDisplayDigits sdd(6);
                            if (!memcmp(mtxt, "π", mlen))
                                value = decimal::pi();
                            else if (!memcmp(mtxt, "e", mlen))
                                value = decimal::e();
                            if (value)
                                mentry = value->as_symbol(false);
                            else
                                mentry = (symbol_p) "???";
                        }
                        items(mi, mentry, type);
                    }
                }
            }
        }
        for (uint i = 0; i < count; i++)
        {
            cstring label = basic_constants[first + 2*i + plane];
            if (label[0] == '=' && label[1] == 0)
            {
                cstring ctxt = basic_constants[first + 2*i];
                decimal_g value = nullptr;
                settings::SaveDisplayDigits sdd(6);
                if (!strcmp(ctxt, "π"))
                    value = decimal::pi();
                else if (!strcmp(ctxt, "e"))
                    value = decimal::e();
                if (value)
                    label = (cstring) value->as_symbol(false);
                else
                    label = "???";
            }
            items(mi, label, type);
        }
    }

    return true;
}


MENU_BODY(ConstantsMenu)
// ----------------------------------------------------------------------------
//   The constants menu is dynamically populated
// ----------------------------------------------------------------------------
{
    uint      infile   = 0;
    uint      count    = 0;
    uint      maxmenus = ID_ConstantsMenu99 - ID_ConstantsMenu00;
    size_t    maxu     = sizeof(basic_constants) / sizeof(basic_constants[0]);
    unit_file cfile(CFILE);

    // List all menu entries in the file (up to 100)
    if (cfile.valid())
        while (symbol_p mname = cfile.next(true))
            if (*mname->value() != '=')
                if (infile++ >= maxmenus)
                    break;

    // Count built-in constant menu titles
    if (!infile || Settings.ShowBuiltinConstants())
    {
        for (size_t u = 0; u < maxu; u += 2)
            if (!basic_constants[u+1] || !*basic_constants[u+1])
                count++;
        if (infile + count > maxmenus)
            count = maxmenus - infile;
    }

    items_init(mi, 1 + infile + count);
    infile = 0;
    if (cfile.valid())
    {
        cfile.seek(0);
        while (symbol_p mname = cfile.next(true))
        {
            if (*mname->value() == '=')
                continue;
            if (infile >= maxmenus)
                break;
            items(mi, mname, id(ID_ConstantsMenu00 + infile++));
        }
    }
    if (!infile || Settings.ShowBuiltinConstants())
    {
        for (size_t u = 0; u < maxu; u += 2)
        {
            if (!basic_constants[u+1] || !*basic_constants[u+1])
            {
                if (infile >= maxmenus)
                    break;
                items(mi, basic_constants[u], id(ID_ConstantsMenu00+infile++));
            }
        }
    }

    return true;
}



// ============================================================================
//
//   Constant-related commands
//
// ============================================================================

static constant_p key_constant(uint key)
// ----------------------------------------------------------------------------
//   Return a softkey label as a constant value
// ----------------------------------------------------------------------------
{
    if (key >= KEY_F1 && key <= KEY_F6)
    {
        size_t   len = 0;
        utf8     txt = nullptr;
        symbol_p sym = ui.label(key - KEY_F1);
        if (sym)
        {
            txt = sym->value(&len);
        }
        else if (cstring label = ui.label_text(key - KEY_F1))
        {
            txt = utf8(label);
            len = strlen(label);
        }

        if (txt)
        {
            char   buffer[32];
            size_t sz = utf8_encode(unicode(settings::CONSTANT_MARKER),
                                    (byte *) buffer);
            if (len + sz <= sizeof(buffer))
            {
                memcpy(buffer+sz, txt, len);
                len += sz;
                if (object_p uobj = object::parse(utf8(buffer), len))
                    if (constant_p u = uobj->as<constant>())
                        return u;
            }
            rt.invalid_constant_error();
            return nullptr;
        }
    }
    return nullptr;
}


COMMAND_BODY(ConstantName)
// ----------------------------------------------------------------------------
//   Put the name of a constant on the stack
// ----------------------------------------------------------------------------
{
    int key = ui.evaluating;
    if (constant_p constant = key_constant(key))
        if (rt.push(constant))
            return OK;
    if (!rt.error())
        rt.type_error();
    return ERROR;
}


INSERT_BODY(ConstantName)
// ----------------------------------------------------------------------------
//   Put the name of a constant in the editor
// ----------------------------------------------------------------------------
{
    int key = ui.evaluating;
    return ui.insert_softkey(key, "₭", " ", false);
}


COMMAND_BODY(ConstantValue)
// ----------------------------------------------------------------------------
//   Put the value of a constant on the stack
// ----------------------------------------------------------------------------
{
    int key = ui.evaluating;
    if (constant_p constant = key_constant(key))
        if (object_p value = constant->value())
            if (rt.push(value))
                return OK;
    if (!rt.error())
        rt.type_error();
    return ERROR;
}


INSERT_BODY(ConstantValue)
// ----------------------------------------------------------------------------
//   Insert the value of a constant
// ----------------------------------------------------------------------------
{
    int key = ui.evaluating;
    if (constant_p constant = key_constant(key))
        if (object_p value = constant->value())
            return ui.insert_object(value, " ", " ");
    return ERROR;
}
