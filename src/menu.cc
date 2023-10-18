// ****************************************************************************
//  menu.cc                                                       DB48X project
// ****************************************************************************
//
//   File Description:
//
//    An RPL object describing a soft menu
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
//
//  Payload layout:
//    Each menu entry is a pair with a symbol and the associated object
//    The symbol represents the name for the menu entry

#include "menu.h"

#include "user_interface.h"
#include "settings.h"


RECORDER(menu,          16, "RPL menu class");
RECORDER(menu_error,    16, "Errors handling menus");


EVAL_BODY(menu)
// ----------------------------------------------------------------------------
//   Evaluating a menu puts it in the interface's menus
// ----------------------------------------------------------------------------
{
    ui.menu(o);
    return OK;
}


MARKER_BODY(menu)
// ----------------------------------------------------------------------------
//   A menu has a mark to identify it
// ----------------------------------------------------------------------------
{
    return L'◥';
}


void menu::items_init(info &mi, uint nitems, uint planes)
// ----------------------------------------------------------------------------
//   Initialize the info structure
// ----------------------------------------------------------------------------
{
    if (Settings.menu_flatten)
        planes = 1;
    uint page0 = planes * ui.NUM_SOFTKEYS;
    mi.planes  = planes;
    mi.plane   = 0;
    mi.index   = 0;
    mi.marker  = 0;
    if (nitems <= page0)
    {
        mi.page = 0;
        mi.skip = 0;
        mi.pages = 1;
    }
    else
    {
        uint perpage = planes * (ui.NUM_SOFTKEYS - 1);
        mi.skip = mi.page * perpage;
        mi.pages = (nitems + perpage - 1) / perpage;
    }
    ui.menus(0, nullptr, nullptr);
    ui.pages(mi.pages);
}


void menu::items(info &mi, id action)
// ----------------------------------------------------------------------------
//   Use the object's name as label
// ----------------------------------------------------------------------------
{
    object_p obj = command::static_object(action);
    items(mi, cstring(obj->fancy()), obj);
}


void menu::items(info &mi, cstring label, object_p action)
// ----------------------------------------------------------------------------
//   Add a menu item
// ----------------------------------------------------------------------------
{
    if (mi.skip > 0)
    {
        mi.skip--;
    }
    else
    {
        uint idx = mi.index++;
        if (mi.pages > 1 && mi.plane < mi.planes)
        {
            if (idx == 0)
            {
                // Insert next and previous keys in menu
                if (mi.planes >= 2)
                {
                    ui.menu(1 * ui.NUM_SOFTKEYS - 1, "▶",
                               command::static_object(ID_MenuNextPage));
                    ui.menu(2 * ui.NUM_SOFTKEYS - 1, "◀︎",
                               command::static_object(ID_MenuPreviousPage));
                }
                else if (ui.shift_plane())
                {
                    ui.menu(1 * ui.NUM_SOFTKEYS - 1, "◀︎",
                               command::static_object(ID_MenuPreviousPage));
                }
                else
                {
                    ui.menu(1 * ui.NUM_SOFTKEYS - 1, "▶",
                               command::static_object(ID_MenuNextPage));

                }
            }

            if ((idx + 1) % ui.NUM_SOFTKEYS == 0)
            {
                mi.plane++;
                idx = mi.index++;
                if (mi.plane >= mi.planes)
                    return;
            }
        }
        if (idx < ui.NUM_SOFTKEYS * mi.planes)
        {
            ui.menu(idx, label, action);
            if (action)
            {
                unicode mark = action->marker();
                if (!mark)
                    mark = mi.marker;
                mi.marker = 0;
                if (mark)
                {
                    if ((int) mark < 0)
                        ui.marker(idx, -mark, true);
                    else
                        ui.marker(idx, mark, false);
                }
            }
        }
    }
}



// ============================================================================
//
//   Creation of a menu
//
// ============================================================================

#define MENU(SysMenu, ...)                                              \
    MENU_BODY(SysMenu)                                                  \
    /* ------------------------------------------------------------ */  \
    /*   Create a system menu                                       */  \
    /* ------------------------------------------------------------ */  \
    {                                                                   \
        uint  nitems = count(__VA_ARGS__);                              \
        items_init(mi, nitems, 3);                                      \
        items(mi, ##__VA_ARGS__);                                       \
        return true;                                                    \
    }



// ============================================================================
//
//    Menu hierarchy
//
// ============================================================================

MENU(MainMenu,
// ----------------------------------------------------------------------------
//   Top level menu, reached from Σ- key
// ----------------------------------------------------------------------------
     "Help",    ID_Help,
     "Math",    ID_MathMenu,
     "Prog",    ID_ProgramMenu,
     "Plot",    ID_PlotMenu,
     "Solve",   ID_SolverMenu,
     "Modes",   ID_ModesMenu,

     "Cat",     ID_Catalog,
     "Real",    ID_RealMenu,
     "Matrix",  ID_MatrixMenu,
     "Stack",   ID_StackMenu,
     "Eqns",    ID_EquationsMenu,
     "UI",      ID_UserInterfaceModesMenu,

     "Time",    ID_TimeMenu,
     "I/O",     ID_IOMenu,
     "Chars",   ID_CharsMenu);


MENU(MathMenu,
// ----------------------------------------------------------------------------
//   Math menu, reached from the Σ+ key
// ----------------------------------------------------------------------------
     "Real",    ID_RealMenu,
     "Cmplx",   ID_ComplexMenu,
     "Trig",    ID_CircularMenu,
     "Vector",  ID_VectorMenu,
     "Matrix",  ID_MatrixMenu,
     "Const",   ID_ConstantsMenu,

     "Hyper",   ID_HyperbolicMenu,
     "Eqns",    ID_EquationsMenu,
     "Proba",   ID_ProbabilitiesMenu,
     "Stats",   ID_StatisticsMenu,
     "Solver",  ID_SolverMenu,
     "Symb",    ID_SymbolicMenu,

     "Signal",  ID_SignalProcessingMenu,
     "Bases",   ID_BasesMenu,
     "Powers",  ID_PowersMenu,
     "Angles",  ID_AnglesMenu,
     "Poly",    ID_PolynomialsMenu,
     "Frac",    ID_FractionsMenu);


MENU(RealMenu,
// ----------------------------------------------------------------------------
//   Functions on real numbers
// ----------------------------------------------------------------------------
     "Min",     ID_Unimplemented,
     "Max",     ID_Unimplemented,
     ID_mod,
     ID_abs,
     "→Num",    ID_ToDecimal,
     "→Frac",   ID_ToFraction,

     "Ceil",    ID_Unimplemented,
     "Floor",   ID_Unimplemented,
     ID_rem,
     "%",       ID_Unimplemented,
     "%Chg",    ID_Unimplemented,
     "Parts",   ID_PartsMenu);


MENU(PartsMenu,
// ----------------------------------------------------------------------------
//   Extract parts of a number
// ----------------------------------------------------------------------------
     ID_abs,
     ID_sign,
     ID_re,
     ID_im,
     ID_arg,
     ID_conj,

     "Round",   ID_Unimplemented,
     "Trunc",   ID_Unimplemented,
     "IPart",   ID_Unimplemented,
     "FPart",   ID_Unimplemented,
     "Mant",    ID_Unimplemented,
     "Xpon",    ID_Unimplemented,

     "RowNorm", ID_Unimplemented,
     "ColNorm", ID_Unimplemented,
     "Dim",     ID_Unimplemented,
     "Type",    ID_Unimplemented);


MENU(NumbersMenu,
// ----------------------------------------------------------------------------
//   Number operations
// ----------------------------------------------------------------------------
     "IsPrime", ID_Unimplemented,
     "NextPr",  ID_Unimplemented,
     "PrevPr",  ID_Unimplemented,
     "Factors", ID_Unimplemented,
     "Random",  ID_Unimplemented,
     "Seed",    ID_Unimplemented);

MENU(AnglesMenu,
// ----------------------------------------------------------------------------
//   Operations on angles
// ----------------------------------------------------------------------------
     "Deg",     ID_Deg,
     "Rad",     ID_Rad,
     "Grad",    ID_Grad,
     "n×π",     ID_PiRadians,
     "→Angle",  ID_Unimplemented,
     "Angle→",  ID_Unimplemented,

     "→Deg",    ID_Unimplemented,
     "→Rad",    ID_Unimplemented,
     "→Grad",   ID_Unimplemented,
     "→x×π",    ID_Unimplemented,
     "→Polar",  ID_Unimplemented,
     "→Rect",   ID_Unimplemented,

     "→DMS",    ID_Unimplemented,
     "DMS→",    ID_Unimplemented,
     "DMS+",    ID_Unimplemented,
     "DMS-",    ID_Unimplemented,
     "Math",    ID_MathModesMenu,
     "Modes",   ID_ModesMenu);


MENU(ComplexMenu,
// ----------------------------------------------------------------------------
//   Operation on complex numbers
// ----------------------------------------------------------------------------
     "ⅈ",       ID_SelfInsert,
     "∡",       ID_SelfInsert,
     "ℝ→ℂ",     ID_RealToComplex,
     "ℂ→ℝ",     ID_ComplexToReal,
     ID_re,
     ID_im,

     "→Rect",   ID_ToRectangular,
     ID_ToPolar,
     ID_conj,
     ID_sign,
     ID_abs,
     ID_arg,

     "Auto ℂ",  ID_Unimplemented,
     "Rect",    ID_Unimplemented,
     "Polar",   ID_Unimplemented);


MENU(VectorMenu,
// ----------------------------------------------------------------------------
//   Operations on vectors
// ----------------------------------------------------------------------------
     "Norm",    ID_abs,
     "Dot",     ID_Unimplemented,
     "Cross",   ID_Unimplemented,
     "→Vec2",   ID_Unimplemented,
     "→Vec3",   ID_Unimplemented,
     "Vec→",    ID_Unimplemented,

     "→Cart",   ID_Unimplemented,
     "→Cylin",  ID_Unimplemented,
     "→Spher",  ID_Unimplemented,
     "Cart",    ID_Unimplemented,
     "Cylin",   ID_Unimplemented,
     "Spher",   ID_Unimplemented);


MENU(MatrixMenu,
// ----------------------------------------------------------------------------
//   Matrix operations
// ----------------------------------------------------------------------------
     "→Mat",    ID_Unimplemented,
     "Mat→",    ID_Unimplemented,
     "Det",     ID_det,
     "Transp",  ID_Unimplemented,
     "Conjug",  ID_Unimplemented,
     "Redim",   ID_Unimplemented,

     "→Diag",   ID_Unimplemented,
     "Idnty",   ID_Unimplemented,
     "Resid",   ID_Unimplemented,
     "Norm",    ID_abs,
     "RowNrm",  ID_Unimplemented,
     "ColNrm",  ID_Unimplemented);


MENU(PolynomialsMenu,
// ----------------------------------------------------------------------------
//   Root-finding operations
// ----------------------------------------------------------------------------
     "FCoef",   ID_Unimplemented,
     "FRoots",  ID_Unimplemented,
     "MRoot",   ID_Unimplemented,
     "MSolvr",  ID_Unimplemented,
     "PCoef",   ID_Unimplemented,
     "PRoot",   ID_Unimplemented,
     "Root",    ID_Root,
     "Solve",   ID_Unimplemented,
     "TVMRoot", ID_Unimplemented,
     "XRoot",   ID_xroot,
     "Zeros",   ID_Unimplemented,
     "Square",  ID_sqrt,
     "Cube",    ID_cbrt);

MENU(HyperbolicMenu,
// ----------------------------------------------------------------------------
//   Hyperbolic operations
// ----------------------------------------------------------------------------
     ID_sinh,   ID_cosh,        ID_tanh,
     ID_asinh,  ID_acosh,       ID_atanh);

MENU(ExpLogMenu,
// ----------------------------------------------------------------------------
//   Exp and log operations
// ----------------------------------------------------------------------------
     ID_exp,    ID_log,
     ID_exp10,  ID_log10,
     ID_expm1,  ID_log1p,
     "exp2",    ID_Unimplemented,
     "log2",    ID_Unimplemented,
     "fhbs",    ID_Unimplemented,
     "flbsc",   ID_Unimplemented,
     "popcnt",  ID_Unimplemented);

MENU(CircularMenu,
// ----------------------------------------------------------------------------
//   Circular / trigonometric functions
// ----------------------------------------------------------------------------

     ID_sin,    ID_cos,         ID_tan,
     ID_asin,   ID_acos,        ID_atan,
     "sec",     ID_Unimplemented,
     "csc",     ID_Unimplemented,
     "cot",     ID_Unimplemented,
     "sec⁻¹",   ID_Unimplemented,
     "csc⁻¹",   ID_Unimplemented,
     "cot⁻¹",   ID_Unimplemented);

MENU(BasesMenu,
// ----------------------------------------------------------------------------
//   Operations on based numbers
// ----------------------------------------------------------------------------
     "#",       ID_SelfInsert,
     ID_And,
     ID_Or,
     ID_Xor,
     ID_Not,

     Base::menu_label, ID_Base,
     "Bin",     ID_Bin,
     "Oct",     ID_Oct,
     "Dec",     ID_Dec,
     "Hex",     ID_Hex,

     stws::menu_label,  ID_stws,
     ID_NAnd,
     ID_NOr,
     ID_Implies,
     ID_Excludes,

     "ShL",     ID_Unimplemented,
     "ShR",     ID_Unimplemented,
     "AShR",    ID_Unimplemented,
     "RoL",     ID_Unimplemented,
     "RoR",     ID_Unimplemented,

     "2Comp",   ID_Unimplemented,
     "1Comp",   ID_Unimplemented,
     "Unsgnd",  ID_Unimplemented);


MENU(ProbabilitiesMenu,
// ----------------------------------------------------------------------------
//   Probabilities
// ----------------------------------------------------------------------------
     "!",       ID_fact,
     "Comb",    ID_Unimplemented,
     "Perm",    ID_Unimplemented,
     "",        ID_Unimplemented,
     "Random",  ID_Unimplemented,

     ID_tgamma,
     ID_lgamma,
     ID_erf,
     ID_erfc,
     "RSeed",   ID_Unimplemented,

     // LTND: Lower Tail Normal Distribution, see HP20b
     "Normal",  ID_Unimplemented,
     "Student", ID_Unimplemented,
     "Chi²",    ID_Unimplemented,
     "F-Distr", ID_Unimplemented,
     "FFT",     ID_Unimplemented,

     "Normal⁻¹",ID_Unimplemented,
     "Studnt⁻¹",ID_Unimplemented,
     "Chi²⁻¹",  ID_Unimplemented,
     "F-Dist⁻¹",ID_Unimplemented,
     "FFT⁻¹",   ID_Unimplemented);


MENU(StatisticsMenu,
// ----------------------------------------------------------------------------
//   Statistics
// ----------------------------------------------------------------------------
     "Σ+",      ID_Unimplemented,
     "Σ-",      ID_Unimplemented,
     "Total",   ID_Unimplemented,
     "Mean",    ID_Unimplemented,
     "StdDev" , ID_Unimplemented,

     "ΣData",   ID_Unimplemented,
     "ClrΣ",    ID_Unimplemented,
     "PopSize", ID_Unimplemented,
     "Median",  ID_Unimplemented,
     "PopSDev", ID_Unimplemented,

     "Regres",  ID_Unimplemented,
     "Plot",    ID_Unimplemented,
     "MaxΣ",    ID_Unimplemented,
     "MinΣ",    ID_Unimplemented,
     "Varnce",  ID_Unimplemented,

     "Bins",    ID_Unimplemented,
     "Info",    ID_Unimplemented,
     "PopVar",  ID_Unimplemented,
     "PopSDev", ID_Unimplemented,
     "PCovar",  ID_Unimplemented,

     "BestFit", ID_Unimplemented,
     "ExpFit",  ID_Unimplemented,
     "LinFit",  ID_Unimplemented,
     "LogFit",  ID_Unimplemented,
     "PwrFit",  ID_Unimplemented);


MENU(SignalProcessingMenu,
// ----------------------------------------------------------------------------
//   Signal processing (Fast Fourier Transform)
// ----------------------------------------------------------------------------
     "FFT",     ID_Unimplemented,
     "InvFFT",  ID_Unimplemented);


MENU(ConstantsMenu,
// ----------------------------------------------------------------------------
//   Constants (to be loaded from catalog on disk)
// ----------------------------------------------------------------------------
     "π",       ID_pi,
     "e",       ID_Unimplemented,
     "i",       ID_ImaginaryUnit,
     "Avogadro",ID_Unimplemented,
     "Gravity", ID_Unimplemented,
     "∞",       ID_Unimplemented,

     "3.14151", ID_Unimplemented,
     "2.71828", ID_Unimplemented,
     "(0,1)",   ID_Unimplemented,
     "6.022e24",ID_Unimplemented,
     "6.67e-11",ID_Unimplemented,
     "∞",       ID_Unimplemented);


MENU(EquationsMenu,
// ----------------------------------------------------------------------------
//   Equations (to be loaded from file on disk)
// ----------------------------------------------------------------------------

     "Columns and Beams",       ID_Unimplemented,
     "Elastic Buckling",        ID_Unimplemented,
     "Eccentric Columns",       ID_Unimplemented,
     "Simple Deflection",       ID_Unimplemented,
     "Simple Slope",            ID_Unimplemented);


MENU(SymbolicMenu,
// ----------------------------------------------------------------------------
//   Symbolic operations
// ----------------------------------------------------------------------------
     ID_Rewrite,
     ID_Collect,
     ID_Expand,
     ID_Simplify,

     "Isolate", ID_Unimplemented,
     "Apply",   ID_Unimplemented,

     "Ex/Co",   ID_Unimplemented,
     "→Q",      ID_ToFraction,
     "→Qπ",     ID_Unimplemented,
     "↑Match",  ID_Unimplemented,
     "↓Match",  ID_Unimplemented,

     "∂",       ID_Unimplemented,
     "∫",       ID_Unimplemented,
     "∑",       ID_Unimplemented,
     "∏",       ID_Unimplemented,
     "∆",       ID_Unimplemented,
     "Taylor",  ID_Unimplemented,

     "Show",    ID_Unimplemented,
     "Quote",   ID_Unimplemented,
     "|",       ID_Unimplemented,
     "Rules",   ID_Unimplemented,
     "Simplify",ID_AutoSimplify,
     "KeepAll", ID_NoAutoSimplify);


MENU(ProgramMenu,
// ----------------------------------------------------------------------------
//   Programming menu
// ----------------------------------------------------------------------------
     "Tests",   ID_TestsMenu,
     "Compare", ID_CompareMenu,
     "Loops",   ID_LoopsMenu,
     "Bases",   ID_BasesMenu,
     "Stack",   ID_StackMenu,
     "Debug",   ID_DebugMenu,

     "Objects", ID_ObjectMenu,
     "Lists",   ID_ListMenu,
     "Flags",   ID_FlagsMenu,
                ID_Version);


MENU(DebugMenu,
// ----------------------------------------------------------------------------
//   Debugging menu
// ----------------------------------------------------------------------------
     "Over",            ID_Unimplemented,
     "Into",            ID_Unimplemented,
     "Break",           ID_Unimplemented,
     "Trace",           ID_Unimplemented,
     "Resume",          ID_Unimplemented,
     "Inspect",         ID_Unimplemented,

     "Prog",            ID_ProgramMenu);

MENU(TestsMenu,
// ----------------------------------------------------------------------------
//   Tests
// ----------------------------------------------------------------------------
     "IfThen",          ID_IfThen,
     "IfElse",          ID_IfThenElse,
     "IfErr",           ID_IfErrThen,
     "IfErrElse",       ID_IfErrThen,
     "IFT",             ID_IFT,
     "IFTE",            ID_IFTE,

     "Compare",         ID_CompareMenu,
     "Loops",           ID_LoopsMenu,
     "Prog",            ID_ProgramMenu);


MENU(CompareMenu,
// ----------------------------------------------------------------------------
//   Comparisons
// ----------------------------------------------------------------------------
     "<",       ID_TestLT,
     "=",       ID_TestEQ,
     ">",       ID_TestGT,
     "≤",       ID_TestLE,
     "≠",       ID_TestNE,
     "≥",       ID_TestGE,

     "and",     ID_And,
     "or",      ID_Or,
     "xor",     ID_Xor,
     "not",     ID_Not,
     "==",      ID_TestSame,
     "",        ID_Unimplemented,

     "true",    ID_True,
     "false",   ID_False,
     "Tests",   ID_TestsMenu,
     "Loops",   ID_LoopsMenu,
     "Prog",    ID_ProgramMenu);


MENU(FlagsMenu,
// ----------------------------------------------------------------------------
//   Operations on flags
// ----------------------------------------------------------------------------
     "FSet",    ID_Unimplemented,
     "FClr",    ID_Unimplemented,
     "FSet?",   ID_Unimplemented,
     "FClr?",   ID_Unimplemented,
     "FSet?C",  ID_Unimplemented,
     "FClr?C",  ID_Unimplemented,

     "F→Bin",   ID_Unimplemented,
     "Modes",   ID_ModesMenu,
     "Tests",   ID_TestsMenu,
     "Loops",   ID_LoopsMenu,
     "Prog",    ID_ProgramMenu);

MENU(LoopsMenu,
// ----------------------------------------------------------------------------
//   Control structures
// ----------------------------------------------------------------------------
     "Start",   ID_StartNext,
     "StStep",  ID_StartStep,
     "For",     ID_ForNext,
     "ForStep", ID_ForStep,
     "Until",   ID_DoUntil,
     "While",   ID_WhileRepeat,

     "Compare", ID_TestsMenu,
     "Prog",    ID_ProgramMenu,
     "Label",   ID_Unimplemented,
     "Goto",    ID_Unimplemented,
     "Gosub",   ID_Unimplemented,
     "Return",  ID_Unimplemented);


MENU(ListMenu,
// ----------------------------------------------------------------------------
//   Operations on list
// ----------------------------------------------------------------------------
     "→List",   ID_Unimplemented,
     "List→",   ID_Unimplemented,
     "Size",    ID_Unimplemented,
     "Map",     ID_Unimplemented,
     "Reduce",  ID_Unimplemented,
     "Filter",  ID_Unimplemented,

     "Sort",    ID_Unimplemented,
     "Reverse", ID_Unimplemented,
     "∑List",   ID_Unimplemented,
     "∏List",   ID_Unimplemented,
     "∆List",   ID_Unimplemented);


MENU(ObjectMenu,
// ----------------------------------------------------------------------------
//  Operations on objects
// ----------------------------------------------------------------------------
     "→Obj",    ID_Unimplemented,
     "Obj→",    ID_Unimplemented,
     "Bytes",   ID_Bytes,
     "Type",    ID_Type,
     "Clone",   ID_Unimplemented,
     "Size",    ID_Unimplemented,

     "→Text",   ID_ToText,
     "→List",   ID_Unimplemented,
     "→Prog",   ID_Unimplemented,
     "→Array",  ID_Unimplemented,
     "→Num",    ID_ToDecimal,
     "→Frac",   ID_ToFraction,

     "→Tag",    ID_ToTag,
     "Tag→",    ID_FromTag,
     "DTag",    ID_dtag,
     "→Graph",  ID_Unimplemented);


MENU(UnitsConversionsMenu,
// ----------------------------------------------------------------------------
//   Menu managing units and unit conversions
// ----------------------------------------------------------------------------
     "Convert", ID_Unimplemented,
     "Base",    ID_Unimplemented, // Base unit
     "Value",   ID_Unimplemented,
     "Factor",  ID_Unimplemented,
     "→Unit",   ID_Unimplemented,

     "m (-3)",  ID_Unimplemented,
     "c (-2)",  ID_Unimplemented,
     "k (+3)",  ID_Unimplemented,
     "M (+6)",  ID_Unimplemented,
     "G (+9)",  ID_Unimplemented,

     "µ (-6)",  ID_Unimplemented,
     "n (-9)",  ID_Unimplemented,
     "p (-12)", ID_Unimplemented,
     "T (+12)", ID_Unimplemented,
     "P (+15)", ID_Unimplemented,

     "f (-15)", ID_Unimplemented,
     "d (-1)",  ID_Unimplemented,
     "da (+1)", ID_Unimplemented,
     "h (+2)",  ID_Unimplemented,
     "E (+18)", ID_Unimplemented,

     "y (-24)", ID_Unimplemented,
     "z (-21)", ID_Unimplemented,
     "a (-18)", ID_Unimplemented,
     "Z (+21)", ID_Unimplemented,
     "Y (+24)", ID_Unimplemented,

     "Ki",      ID_Unimplemented,
     "Mi",      ID_Unimplemented,
     "Gi",      ID_Unimplemented,
     "Ti",      ID_Unimplemented,
     "Pi",      ID_Unimplemented,

     "Ei",      ID_Unimplemented,
     "Zi",      ID_Unimplemented,
     "Yi",      ID_Unimplemented);


MENU(UnitsMenu,
// ----------------------------------------------------------------------------
//   Menu managing units and unit conversions
// ----------------------------------------------------------------------------
     "Length",  ID_UnitsMenu,
     "Area",    ID_UnitsMenu,
     "Volume",  ID_UnitsMenu,
     "Time",    ID_UnitsMenu,
     "Speed",   ID_UnitsMenu,

     "Mass",    ID_UnitsMenu,
     "Force",   ID_UnitsMenu,
     "Energy",  ID_UnitsMenu,
     "Power",   ID_UnitsMenu,
     "Press",   ID_UnitsMenu,

     "Temp",    ID_UnitsMenu,
     "Elec",    ID_UnitsMenu,
     "Angle",   ID_AnglesMenu,
     "Light",   ID_UnitsMenu,
     "Rad",     ID_UnitsMenu,

     "Visc",    ID_UnitsMenu,
     "User",    ID_UnitsMenu,
     "Csts",    ID_ConstantsMenu,
     "",        ID_Unimplemented,
     "Convert", ID_UnitsConversionsMenu);


MENU(StackMenu,
// ----------------------------------------------------------------------------
//   Operations on the stack
// ----------------------------------------------------------------------------
     "Dup",     ID_Dup,
     "Drop",    ID_Drop,
     "Swap",    ID_Swap,
     "Pick",    ID_Pick,
     "Roll",    ID_Roll,
     "Depth",   ID_Depth,

     "Dup2",    ID_Dup2,
     "Drop2",   ID_Drop2,
     "Over",    ID_Over,
     "Rot",     ID_Rot,
     "RoolDn",  ID_RollD,
     "LastArg", ID_LastArg,

     ID_DupN,
     ID_DropN,
     "ClearStk",ID_Unimplemented,
     "FillStk", ID_Unimplemented,
     "LastX",   ID_LastX,
     "LastStk", ID_Undo);

MENU(EditMenu,
// ----------------------------------------------------------------------------
//   Operations in the editor
// ----------------------------------------------------------------------------
     "Select",  ID_EditorSelect,
     "←Word",   ID_EditorWordLeft,
     "Word→",   ID_EditorWordRight,
     "Cut",     ID_EditorCut,
     "Copy",    ID_EditorCopy,
     "Paste",   ID_EditorPaste,

     "Csr⇄Sel", ID_EditorFlip,
     "|←",      ID_EditorBegin,
     "→|",      ID_EditorEnd,
     "Search",  ID_EditorSearch,
     "Replace", ID_EditorReplace,
     "Clear",   ID_EditorClear);

MENU(IntegrationMenu,
// ----------------------------------------------------------------------------
//   Symbolic and numerical integration
// ----------------------------------------------------------------------------
     "∫",       ID_Integrate,
     "Num ∫",   ID_Integrate,
     "Symb ∫",  ID_Unimplemented,
     "Prim",    ID_Unimplemented,

     "Eq",      ID_Unimplemented,
     "Indep",   ID_Unimplemented);

MENU(DifferentiationMenu,
// ----------------------------------------------------------------------------
//   Symbolic and numerical differentiation
// ----------------------------------------------------------------------------
     "Num",     ID_Unimplemented,
     "Symb",    ID_Unimplemented,
     "Diff",    ID_Unimplemented,

     "Eq",      ID_Unimplemented,
     "Indep",   ID_Unimplemented);

MENU(SolverMenu,
// ----------------------------------------------------------------------------
//   The solver menu / application
// ----------------------------------------------------------------------------
     "Eq",      ID_Unimplemented,
     "Indep",   ID_Unimplemented,
     "Root",    ID_Root,
     "MultiR",  ID_Unimplemented,
     "PolyR",   ID_Unimplemented,

     "Num",     ID_NumericalSolverMenu,
     "Diff",    ID_DifferentialSolverMenu,
     "Symb",    ID_SymbolicSolverMenu,
     "Poly",    ID_PolynomialSolverMenu,
     "Linear",  ID_LinearSolverMenu,

     "Multi",   ID_MultiSolverMenu,
     "Finance", ID_FinanceSolverMenu,
     "Plot",    ID_PlotMenu,
     "L.R.",    ID_StatisticsMenu,
     "Eqns",    ID_EquationsMenu,


     "Parms",   ID_Unimplemented,
     "Auto",    ID_Unimplemented);

MENU(NumericalSolverMenu,
// ----------------------------------------------------------------------------
//  Menu for numerical equation solving
// ----------------------------------------------------------------------------
     "Eq",      ID_Unimplemented,
     "Indep",   ID_Unimplemented,
     "Root",    ID_Unimplemented,

     ID_SolverMenu);

MENU(DifferentialSolverMenu,
// ----------------------------------------------------------------------------
//   Menu for differential equation solving
// ----------------------------------------------------------------------------
     "Eq",      ID_Unimplemented,
     "Indep",   ID_Unimplemented,
     "Root",    ID_Unimplemented,

     ID_SolverMenu);


MENU(SymbolicSolverMenu,
// ----------------------------------------------------------------------------
//   Menu for symbolic equation solving
// ----------------------------------------------------------------------------
     "Eq",      ID_Unimplemented,
     "Indep",   ID_Unimplemented,
     "Root",    ID_Unimplemented,

     ID_SolverMenu);

MENU(PolynomialSolverMenu,
// ----------------------------------------------------------------------------
//   Menu for polynom solving
// ----------------------------------------------------------------------------
     "Eq",      ID_Unimplemented,
     "Indep",   ID_Unimplemented,
     "Root",    ID_Unimplemented,

     ID_SolverMenu);

MENU(LinearSolverMenu,
// ----------------------------------------------------------------------------
//   Menu for linear system solving
// ----------------------------------------------------------------------------
     "Eq",      ID_Unimplemented,
     "Indep",   ID_Unimplemented,
     "Root",    ID_Unimplemented,

     ID_SolverMenu);

MENU(FinanceSolverMenu,
// ----------------------------------------------------------------------------
//   Menu for finance time value of money solving
// ----------------------------------------------------------------------------
     "TVMR",    ID_Unimplemented,
     "Amort",   ID_Unimplemented,
     "Begin",   ID_Unimplemented,

     ID_SolverMenu);

MENU(MultiSolverMenu,
// ----------------------------------------------------------------------------
//   Menu for linear system solving
// ----------------------------------------------------------------------------
     "Eqs",     ID_Unimplemented,
     "Indeps",  ID_Unimplemented,
     "MRoot",   ID_Unimplemented,

     ID_SolverMenu);

MENU(PowersMenu,
// ----------------------------------------------------------------------------
//   Menu with the common powers
// ----------------------------------------------------------------------------
     "Square",  ID_sq,
     "Cube",    ID_cubed,
     ID_pow,
     ID_sqrt,
     ID_cbrt,
     "xroot",   ID_Unimplemented);

MENU(FractionsMenu,
// ----------------------------------------------------------------------------
//   Operations on fractions
// ----------------------------------------------------------------------------
     "%",       ID_Unimplemented,
     "%Chg",    ID_Unimplemented,
     "%Total",  ID_Unimplemented,
     "→Frac",   ID_ToFraction,
     "Frac→",   ID_Unimplemented,
     "→Num",    ID_ToDecimal,

     "→HMS",    ID_Unimplemented,
     "HMS→",    ID_Unimplemented,
     ToFractionIterations::menu_label,  ID_ToFractionIterations,
     ToFractionDigits::menu_label,      ID_ToFractionDigits
);

MENU(PlotMenu,
// ----------------------------------------------------------------------------
//   Plot and drawing menu
// ----------------------------------------------------------------------------
     "Plot",    ID_Unimplemented,
     "Clear",   ID_Unimplemented,
     "Axes",    ID_Unimplemented,
     "Auto",    ID_Unimplemented);

MENU(ClearThingsMenu,
// ----------------------------------------------------------------------------
//  Clearing various things
// ----------------------------------------------------------------------------
     "Stack",   ID_Unimplemented,
     "Dir",     ID_Unimplemented,
     "Stats",   ID_Unimplemented,
     "Mem",     ID_Unimplemented,
     "Error",   ID_err0,
     "LCD",     ID_ClLCD);

MENU(CharsMenu,
// ----------------------------------------------------------------------------
//   Will be dynamic
// ----------------------------------------------------------------------------
     "→",       ID_SelfInsert,
     "∂",       ID_SelfInsert,
     "∫",       ID_SelfInsert,
     "∑",       ID_SelfInsert,
     "∏",       ID_SelfInsert,
     "∆",       ID_SelfInsert);

MENU(ModesMenu,
// ----------------------------------------------------------------------------
//   Mode settings
// ----------------------------------------------------------------------------
     "Deg",     ID_Deg,
     "Rad",     ID_Rad,
     "n×π",     ID_PiRadians,
     "Rect",    ID_Unimplemented,
     "Polar",   ID_Unimplemented,
     "Spheric", ID_Unimplemented,

     ID_Grad,
     "Angles",  ID_AnglesMenu,
     "Math",    ID_MathModesMenu,
     "Display", ID_DisplayModesMenu,
     "Seps",    ID_SeparatorModesMenu,
     "UI",      ID_UserInterfaceModesMenu,

     "Modes",   ID_Modes,
     "Reset",   ID_ResetModes);

MENU(DisplayModesMenu,
// ----------------------------------------------------------------------------
//   Mode setting for numbers
// ----------------------------------------------------------------------------
     Std::menu_label,   ID_Std,
     Fix::menu_label,   ID_Fix,
     Sci::menu_label,   ID_Sci,
     Eng::menu_label,   ID_Eng,
     Sig::menu_label,   ID_Sig,
     Precision::menu_label, ID_Precision,

     MantissaSpacing::menu_label, ID_MantissaSpacing,
     FractionSpacing::menu_label, ID_FractionSpacing,
     BasedSpacing::menu_label,    ID_BasedSpacing,
     StandardExponent::menu_label, ID_StandardExponent,
     MinimumSignificantDigits::menu_label, ID_MinimumSignificantDigits,
     "Seps",            ID_SeparatorModesMenu,

     "cmd",             ID_LowerCase,
     "CMD",             ID_UpperCase,
     "Cmd",             ID_Capitalized,
     "Command",         ID_LongForm,
     "UI",              ID_UserInterfaceModesMenu,
     "Math",            ID_MathModesMenu);

MENU(SeparatorModesMenu,
// ----------------------------------------------------------------------------
//   Separators
// ----------------------------------------------------------------------------
     "1 000",   ID_NumberSpaces,
     Settings.decimal_mark == '.' ? "1,000." : "1.000,",  ID_NumberDotOrComma,
     "1'000",   ID_NumberTicks,
     "1_000",   ID_NumberUnderscore,
     "2.3",     ID_DecimalDot,
     "2,3",     ID_DecimalComma,

     "#1 000",  ID_BasedSpaces,
     Settings.decimal_mark == '.' ? "#1,000" : "#1.000",  ID_BasedDotOrComma,
     "#1'000",  ID_BasedTicks,
     "#1_000",  ID_BasedUnderscore,
     "Disp",    ID_DisplayModesMenu,
     "Modes",   ID_ModesMenu,

     "1.2x10³²", ID_FancyExponent,
     "1.2E32", ID_ClassicExponent,
     "1.0→1.", ID_TrailingDecimal,
     "1.0→1",  ID_NoTrailingDecimal);

MENU(UserInterfaceModesMenu,
// ----------------------------------------------------------------------------
//   Mode setting for numbers
// ----------------------------------------------------------------------------
     "GrStk",                                   ID_GraphicsStackDisplay,
     "TxtStk",                                  ID_TextStackDisplay,
     ResultFontSize::menu_label,                ID_ResultFontSize,
     StackFontSize::menu_label,                 ID_StackFontSize,
     EditorFontSize::menu_label,                ID_EditorFontSize,
     EditorMultilineFontSize::menu_label,       ID_EditorMultilineFontSize,

     "3-lines",                                 ID_ThreeRowsMenus,
     "1-line",                                  ID_SingleRowMenus,
     "Flat",                                    ID_FlatMenus,
     "Round",                                   ID_RoundedMenus,
     "Square",                                  ID_SquareMenus,
     "Modes",                                   ID_ModesMenu,

     CursorBlinkRate::menu_label,               ID_CursorBlinkRate);

MENU(MathModesMenu,
// ----------------------------------------------------------------------------
//   Mode setting for numbers
// ----------------------------------------------------------------------------
     "Num",                     ID_NumericResults,
     "Sym",                     ID_SymbolicResults,
     "Simplify",                ID_AutoSimplify,
     "KeepAll",                 ID_NoAutoSimplify,
     MaxBigNumBits::menu_label, ID_MaxBigNumBits,
     MaxRewrites::menu_label,   ID_MaxRewrites,

     "iℂ",                      ID_Unimplemented,
     "Auto ℂ",                  ID_Unimplemented,
     "Modes",                   ID_ModesMenu,
     "Display",                 ID_DisplayModesMenu,
     "UI",                      ID_UserInterfaceModesMenu,
     "Modes",                   ID_Modes);

MENU(PrintingMenu,
// ----------------------------------------------------------------------------
//   Printing operations
// ----------------------------------------------------------------------------
     "Print",   ID_Unimplemented,
     "Screen",  ID_Unimplemented,
     "Disk",    ID_Unimplemented,
     "IR",      ID_Unimplemented);

MENU(IOMenu,
// ----------------------------------------------------------------------------
//   I/O operations
// ----------------------------------------------------------------------------
     "Save",    ID_Unimplemented,
     "Load",    ID_Unimplemented,
     "Print",   ID_Unimplemented);

MENU(FilesMenu,
// ----------------------------------------------------------------------------
//   Files and disk operations
// ----------------------------------------------------------------------------
     "Open",    ID_Unimplemented,
     "Close",   ID_Unimplemented,
     "Read",    ID_Unimplemented,
     "Write",   ID_Unimplemented,
     "Seek",    ID_Unimplemented,
     "Dir",     ID_Unimplemented);

MENU(GraphicsMenu,
// ----------------------------------------------------------------------------
//   Graphics operations
// ----------------------------------------------------------------------------
     "Disp",    ID_Disp,
     "Line",    ID_Line,
     "Rect",    ID_Rect,
     "Rounded", ID_RRect,
     "Ellipse", ID_Ellipse,
     "Circle",  ID_Circle,

     "DispXY",  ID_DispXY,
     "Pict",    ID_Pict,
     "GOr",     ID_GOr,
     "GXor",    ID_GXor,
     "GXor",    ID_And,
     "ClLCD",   ID_ClLCD,

     "Clip",    ID_Clip,
     "Current", ID_CurrentClip);

MENU(MemMenu,
// ----------------------------------------------------------------------------
//   Memory operations
// ----------------------------------------------------------------------------
     "Home",    ID_home,
     "Path",    ID_path,
     "CrDir",   ID_crdir,
     "UpDir",   ID_updir,
     "Current", ID_CurrentDirectory,
     "Purge",   ID_Purge,

     "GC",      ID_GarbageCollect,
     "Avail",   ID_Mem,
     "Free",    ID_FreeMemory,
     "System",  ID_SystemMemory,
     "Recall",  ID_Rcl,
     "PgAll",   ID_PurgeAll);


MENU(LibsMenu,
// ----------------------------------------------------------------------------
//   Library operations
// ----------------------------------------------------------------------------
     "Attach",  ID_Unimplemented,
     "Detach",  ID_Unimplemented);


MENU(TimeMenu,
// ----------------------------------------------------------------------------
//   Time operations
// ----------------------------------------------------------------------------
     "Date",    ID_Unimplemented,
     "∆Date",   ID_Unimplemented,
     "Time",    ID_Unimplemented,
     "∆Time",   ID_Unimplemented,
     "Ticks",   ID_Ticks,

     "→Date",   ID_Unimplemented,
     "→Time",   ID_Unimplemented,
     "T→Str",   ID_Unimplemented,
     "ClkAdj",  ID_Unimplemented,
     "TmBench", ID_Unimplemented,

     "Date+",   ID_Unimplemented,
     "→HMS",    ID_Unimplemented,
     "HMS→",    ID_Unimplemented,
     "HMS+",    ID_Unimplemented,
     "HMS-",    ID_Unimplemented,

     "Alarm",   ID_Unimplemented,
     "Ack",     ID_Unimplemented,
     "→Alarm",  ID_Unimplemented,
     "Alarm→",  ID_Unimplemented,
     "FindAlm", ID_Unimplemented,

     "DelAlm",  ID_Unimplemented,
     "AckAll",  ID_Unimplemented);


MENU(TextMenu,
// ----------------------------------------------------------------------------
//   Text operations
// ----------------------------------------------------------------------------
     "→Text",   ID_ToText,
     "Text→",   ID_Unimplemented,
     "Length",  ID_Unimplemented,
     "Append",  ID_add,
     "Repeat",  ID_mul);
