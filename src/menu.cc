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

#include "input.h"
#include "parser.h"
#include "renderer.h"

RECORDER(menu,          16, "RPL menu class");
RECORDER(menu_error,    16, "Errors handling menus");

OBJECT_HANDLER_BODY(menu)
// ----------------------------------------------------------------------------
//    Handle commands for menus
// ----------------------------------------------------------------------------
{
    switch(op)
    {
    case EXEC:
    case EVAL:
        Input.menu(obj);
        return OK;
    case SIZE:
        return leb128size(obj->type());
    case MENU:
        record(menu_error, "Invalid menu %u", obj->type());
        return ERROR;
    case MENU_MARKER:
        return L'◥';

    default:
        return DELEGATE(command);
    }
}


void menu::items_init(info &mi, uint nitems, uint planes)
// ----------------------------------------------------------------------------
//   Initialize the info structure
// ----------------------------------------------------------------------------
{
    uint page0 = planes * input::NUM_SOFTKEYS;
    mi.planes  = planes;
    mi.plane   = 0;
    mi.index   = 0;
    if (nitems <= page0)
    {
        mi.page = 0;
        mi.skip = 0;
        mi.pages = 1;
    }
    else
    {
        uint perpage = planes * (input::NUM_SOFTKEYS - 1);
        mi.skip = mi.page * perpage;
        mi.pages = (nitems + perpage - 1) / perpage;
    }
    Input.menus(0, nullptr, nullptr);
    Input.pages(mi.pages);
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
                    Input.menu(1 * input::NUM_SOFTKEYS - 1, "▶",
                               command::static_object(ID_MenuNextPage));
                    Input.menu(2 * input::NUM_SOFTKEYS - 1, "◀︎",
                               command::static_object(ID_MenuPreviousPage));
                }
                else if (Input.shift_plane())
                {
                    Input.menu(1 * input::NUM_SOFTKEYS - 1, "◀︎",
                               command::static_object(ID_MenuPreviousPage));
                }
                else
                {
                    Input.menu(1 * input::NUM_SOFTKEYS - 1, "▶",
                               command::static_object(ID_MenuNextPage));

                }
            }

            if ((idx + 1) % input::NUM_SOFTKEYS == 0)
            {
                mi.plane++;
                idx = mi.index++;
                if (mi.plane >= mi.planes)
                    return;
            }
        }
        if (idx < input::NUM_SOFTKEYS * mi.planes)
        {
            Input.menu(idx, label, action);
            if (action)
            {
                if (unicode mark = action->marker())
                {
                    if ((int) mark < 0)
                        Input.marker(idx, -mark, true);
                    else
                        Input.marker(idx, mark, false);
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

#define MENU(SysMenu, ...)                                           \
  OBJECT_HANDLER_BODY(SysMenu)                                       \
  /* ------------------------------------------------------------ */ \
  /*   Create a system menu                                       */ \
  /* ------------------------------------------------------------ */ \
  {                                                                  \
      if (op == MENU)                                                \
      {                                                              \
          info &mi     = *((info *) arg);                            \
          uint  nitems = count(__VA_ARGS__);                         \
          items_init(mi, nitems, 2);                                 \
          items(mi, ##__VA_ARGS__);                                  \
          return OK;                                                 \
      }                                                              \
      return DELEGATE(menu);                                         \
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
     "Math",    ID_MathMenu,
     "Program", ID_ProgramMenu,
     "Vars",    ID_VariablesMenu,
     "Plot",    ID_PlotMenu,
     "Solve",   ID_SolverMenu,
     "Eqns",    ID_EquationsMenu,

     "Last",    ID_LastThingsMenu,
     "Stack",   ID_StackMenu,
     "Modes",   ID_ModesMenu,
     "Time",    ID_TimeMenu,
     "I/O",     ID_IOMenu,
     "Chars",   ID_CharsMenu);


MENU(MathMenu,
// ----------------------------------------------------------------------------
//   Math menu, reached from the Σ+ key
// ----------------------------------------------------------------------------

     "Real",    ID_RealMenu,
     "Cmplx",   ID_ComplexMenu,
     "Vector",  ID_VectorMenu,
     "Matrix",  ID_MatrixMenu,
     "Const",   ID_ConstantsMenu,

     "Trig",    ID_CircularMenu,
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
     "List",    ID_ListMenu,
     "Frac",    ID_FractionsMenu,
     "...",     ID_MainMenu);


MENU(RealMenu,
// ----------------------------------------------------------------------------
//   Functions on real numbers
// ----------------------------------------------------------------------------
     "Min",     ID_Unimplemented,
     "Max",     ID_Unimplemented,
     "mod",     ID_mod,
     "Abs",     ID_Unimplemented,
     "→Num",    ID_Unimplemented,
     "→Frac",   ID_Unimplemented,

     "Ceil",    ID_Unimplemented,
     "Floor",   ID_Unimplemented,
     "rem",     ID_rem,
     "%",       ID_Unimplemented,
     "%Chg",    ID_Unimplemented,
     "Parts",   ID_PartsMenu);


MENU(PartsMenu,
// ----------------------------------------------------------------------------
//   Extract parts of a number
// ----------------------------------------------------------------------------
     "Sign",    ID_Unimplemented,
     "Round",   ID_Unimplemented,
     "Trunc",   ID_Unimplemented,
     "IPart",   ID_Unimplemented,
     "FPart",   ID_Unimplemented,
     "Abs",     ID_Unimplemented,

     "Mant",    ID_Unimplemented,
     "Xpon",    ID_Unimplemented,
     "Type",    ID_Unimplemented);


MENU(AnglesMenu,
// ----------------------------------------------------------------------------
//   Operations on angles
// ----------------------------------------------------------------------------
     "→Deg",    ID_Unimplemented,
     "→Rad",    ID_Unimplemented,
     "→Grad",   ID_Unimplemented,
     "Deg",     ID_Unimplemented,
     "Rad",     ID_Unimplemented,
     "Grad",    ID_Unimplemented,

     "→Angle",  ID_Unimplemented,
     "→Polar",  ID_Unimplemented,
     "→Rect",   ID_Unimplemented,
     "→HMS",    ID_Unimplemented,
     "HMS→",    ID_Unimplemented);


MENU(ComplexMenu,
// ----------------------------------------------------------------------------
//   Operation on complex numbers
// ----------------------------------------------------------------------------
     "→ℂ",      ID_Unimplemented,
     "Conj",    ID_Unimplemented,
     "Norm",    ID_Unimplemented,
     "Arg",     ID_Unimplemented,
     "Sign",    ID_Unimplemented,
     "𝒊",        ID_Unimplemented,

     "ℂ→",      ID_Unimplemented,
     "Re",      ID_Unimplemented,
     "Im",      ID_Unimplemented,
     "→Polar",  ID_Unimplemented,
     "→Rect",   ID_Unimplemented,
     "Auto ℂ",  ID_Unimplemented);


MENU(VectorMenu,
// ----------------------------------------------------------------------------
//   Operations on vectors
// ----------------------------------------------------------------------------
     "Norm",    ID_Unimplemented,
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
     "Det",     ID_Unimplemented,
     "Transp",  ID_Unimplemented,
     "Conjug",  ID_Unimplemented,
     "Redim",   ID_Unimplemented,

     "→Diag",   ID_Unimplemented,
     "Idnty",   ID_Unimplemented,
     "Resid",   ID_Unimplemented,
     "Norm",    ID_Unimplemented,
     "RowNrm",  ID_Unimplemented,
     "ColNrm",  ID_Unimplemented);


MENU(HyperbolicMenu,
// ----------------------------------------------------------------------------
//   Hyperbolic operations
// ----------------------------------------------------------------------------
     ID_sinh,   ID_cosh,        ID_tanh,
     ID_asinh,  ID_acosh,       ID_atanh,
     ID_exp,    ID_expm1,
     ID_log,    ID_log1p);

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
     "→Bin",    ID_Unimplemented,
     "→Oct",    ID_Unimplemented,
     "→Dec",    ID_Unimplemented,
     "→Hex",    ID_Unimplemented,

     "Base",    ID_Unimplemented,
     "Bin",     ID_Unimplemented,
     "Oct",     ID_Unimplemented,
     "Dec",     ID_Unimplemented,
     "Hex",     ID_Unimplemented,

     "WordSz",  ID_Unimplemented,
     "and",     ID_Unimplemented,
     "or",      ID_Unimplemented,
     "xor",     ID_Unimplemented,
     "not",     ID_Unimplemented,

     "→WSize",  ID_Unimplemented,
     "nand",    ID_Unimplemented,
     "nor",     ID_Unimplemented,
     "impl",    ID_Unimplemented,
     "excl",    ID_Unimplemented,

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
     "!",       ID_Unimplemented,
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
     "π",       ID_Unimplemented,
     "e",       ID_Unimplemented,
     "i",       ID_Unimplemented,
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
     "Collect", ID_Unimplemented,
     "Expand",  ID_Unimplemented,
     "Isolate", ID_Unimplemented,
     "Apply",   ID_Unimplemented,
     "Taylor",  ID_Unimplemented,

     "Ex/Co",   ID_Unimplemented,
     "→Q",      ID_Unimplemented,
     "→Qπ",     ID_Unimplemented,
     "↑Match",  ID_Unimplemented,
     "↓Match",  ID_Unimplemented,

     "∂",       ID_Unimplemented,
     "∫",       ID_Unimplemented,
     "∑",       ID_Unimplemented,
     "∏",       ID_Unimplemented,
     "∆",       ID_Unimplemented,

     "Show",    ID_Unimplemented,
     "Quote",   ID_Unimplemented,
     "|",       ID_Unimplemented,
     "Rules",   ID_Unimplemented,
     "Simpl",   ID_Unimplemented);


MENU(ProgramMenu,
// ----------------------------------------------------------------------------
//   Programming menu
// ----------------------------------------------------------------------------
     "Tests",   ID_TestsMenu,
     "Compare", ID_CompareMenu,
     "Loops",   ID_LoopsMenu,
     "Bases",   ID_BasesMenu,
     "Stack",   ID_StackMenu,

     "Objects", ID_ObjectMenu,
     "Lists",   ID_ListMenu,
     "Flags",   ID_FlagsMenu);


MENU(TestsMenu,
// ----------------------------------------------------------------------------
//   Tests
// ----------------------------------------------------------------------------
     "IfThen",  ID_Unimplemented,
     "IfElse",  ID_Unimplemented,
     "IfErr",   ID_Unimplemented,
     "IFTE",    ID_Unimplemented,
     "Compare", ID_CompareMenu,
     "Loops",   ID_LoopsMenu);


MENU(CompareMenu,
// ----------------------------------------------------------------------------
//   Comparisons
// ----------------------------------------------------------------------------
     "<",       ID_Unimplemented,
     "=",       ID_Unimplemented,
     ">",       ID_Unimplemented,
     "≤",       ID_Unimplemented,
     "≠",       ID_Unimplemented,
     "≥",       ID_Unimplemented,

     "and",     ID_Unimplemented,
     "or",      ID_Unimplemented,
     "xor",     ID_Unimplemented,
     "not",     ID_Unimplemented,
     "==",      ID_Unimplemented,
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
     "Start",   ID_Unimplemented,
     "StNext",  ID_Unimplemented,
     "For",     ID_Unimplemented,
     "ForStep", ID_Unimplemented,
     "Until",   ID_Unimplemented,
     "While",   ID_Unimplemented,

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
     "Bytes",   ID_Unimplemented,
     "Type",    ID_Unimplemented,
     "Clone",   ID_Unimplemented,
     "Size",    ID_Unimplemented,

     "→Str",    ID_Unimplemented,
     "→List",   ID_Unimplemented,
     "→Prog",   ID_Unimplemented,
     "→Array",  ID_Unimplemented,
     "→Num",    ID_Unimplemented,
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
     ID_Dup,
     ID_Drop,
     ID_Swap,
     "Pick",    ID_Unimplemented,
     "Roll",    ID_Unimplemented,

     "Dup2",    ID_Unimplemented,
     "Drop2",   ID_Unimplemented,
     "Over",    ID_Unimplemented,
     "Rot",     ID_Unimplemented,
     "RollDn",  ID_Unimplemented,

     "Depth",   ID_Unimplemented,
     "LastStk", ID_Unimplemented,
     "LastArg", ID_Unimplemented,
     "ClrStk",  ID_Unimplemented,
     "FillStk", ID_Unimplemented,

     "DupN",    ID_Unimplemented,
     "DropN",   ID_Unimplemented,
     "→Depth",  ID_Unimplemented,
     "4-Stk",   ID_Unimplemented,
     "8-Stk",   ID_Unimplemented);


MENU(SolverMenu,
// ----------------------------------------------------------------------------
//   The solver menu / application
// ----------------------------------------------------------------------------
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

     "Eq",      ID_Unimplemented,
     "Indep",   ID_Unimplemented,
     "Root",    ID_Unimplemented,
     "MultiR",  ID_Unimplemented,
     "PolyR",   ID_Unimplemented,

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
     "→Frac",   ID_Unimplemented,
     "Frac→",   ID_Unimplemented,
     "→Num",    ID_Unimplemented,

     "→HMS",    ID_Unimplemented,
     "HMS→",    ID_Unimplemented);

MENU(PlotMenu,
// ----------------------------------------------------------------------------
//   Plot and drawing menu
// ----------------------------------------------------------------------------
     "Plot",    ID_Unimplemented,
     "Clear",   ID_Unimplemented,
     "Axes",    ID_Unimplemented,
     "Auto",    ID_Unimplemented);


MENU(LastThingsMenu,
// ----------------------------------------------------------------------------
//   Menu with the last things
// ----------------------------------------------------------------------------
     "Arg",     ID_Unimplemented,
     "Stack",   ID_Unimplemented,
     "Menu",    ID_Unimplemented,
     "Cmd",     ID_Unimplemented,
     "Undo",    ID_Unimplemented,
     "Redo",    ID_Unimplemented);

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
     "Std",     ID_Unimplemented,
     "Fix",     ID_Unimplemented,
     "Sci",     ID_Unimplemented,
     "Eng",     ID_Unimplemented,
     "Frac",    ID_Unimplemented,

     "Auto ℂ",  ID_Unimplemented,
     "Decimal", ID_Unimplemented,
     "NumSpc",  ID_Unimplemented,
     "StkFont", ID_Unimplemented,
     "EdFont",  ID_Unimplemented,

     "Deg",     ID_Unimplemented,
     "Rad",     ID_Unimplemented,
     "Grad",    ID_Unimplemented,
     "Angles",  ID_Unimplemented,
     "DMS",     ID_Unimplemented,

     "Rect",    ID_Unimplemented,
     "Polar",   ID_Unimplemented,
     "Spheric", ID_Unimplemented,
     "iℂ",      ID_Unimplemented,
     "()",      ID_Unimplemented);


MENU(IOMenu,
// ----------------------------------------------------------------------------
//   I/O operations
// ----------------------------------------------------------------------------
     "Save",    ID_Unimplemented,
     "Load",    ID_Unimplemented,
     "Print",   ID_Unimplemented);


MENU(TimeMenu,
// ----------------------------------------------------------------------------
//   Time operations
// ----------------------------------------------------------------------------
     "Date",    ID_Unimplemented,
     "∆Date",   ID_Unimplemented,
     "Time",    ID_Unimplemented,
     "∆Time",   ID_Unimplemented,
     "Ticks",   ID_Unimplemented,

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
