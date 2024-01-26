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


void menu::items_init(info &mi, uint nitems, uint planes, uint vplanes)
// ----------------------------------------------------------------------------
//   Initialize the info structure
// ----------------------------------------------------------------------------
{
    if (Settings.MenuAppearance() == ID_FlatMenus)
    {
        planes = 1;
        vplanes = 1;
    }
    uint page0 = vplanes * ui.NUM_SOFTKEYS;
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
        uint perpage = vplanes * (ui.NUM_SOFTKEYS - 1);
        mi.skip = mi.page * perpage;
        mi.pages = (nitems + perpage - 1) / perpage;
    }
    ui.menus(0, nullptr, nullptr);
    ui.pages(mi.pages);

    // Insert next and previous keys in large menus
    if (nitems > page0)
    {
        if (planes >= 2)
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
//   Commands related to menus
//
// ============================================================================

COMMAND_BODY(ToolsMenu)
// ----------------------------------------------------------------------------
//   Contextual tool menu
// ----------------------------------------------------------------------------
{
    id menu = ID_MainMenu;

    if (rt.editing())
    {
        switch(ui.editing_mode())
        {
        case ui.DIRECT:                 menu = ID_MathMenu; break;
        case ui.TEXT:                   menu = ID_TextMenu; break;
        case ui.PROGRAM:                menu = ID_ProgramMenu; break;
        case ui.ALGEBRAIC:              menu = ID_EquationsMenu; break;
        case ui.MATRIX:                 menu = ID_MatrixMenu; break;
        case ui.BASED:                  menu = ID_BasesMenu; break;
        default:
        case ui.STACK:                  break;
        }
    }
    else if (rt.depth())
    {
        if (!rt.args(1))
            return ERROR;
        if (object_p top = rt.top())
        {
            switch(top->type())
            {
            case ID_integer:
            case ID_neg_integer:
            case ID_bignum:
            case ID_neg_bignum:
            case ID_hwfloat:
            case ID_hwdouble:
            case ID_decimal:
            case ID_neg_decimal:        menu = ID_RealMenu; break;
            case ID_fraction:
            case ID_neg_fraction:
            case ID_big_fraction:
            case ID_neg_big_fraction:   menu = ID_FractionsMenu; break;
            case ID_polar:
            case ID_rectangular:        menu = ID_ComplexMenu; break;
#if CONFIG_FIXED_BASED_OBJECTS
            case ID_hex_integer:
            case ID_dec_integer:
            case ID_oct_integer:
            case ID_bin_integer:
            case ID_hex_bignum:
            case ID_dec_bignum:
            case ID_oct_bignum:
            case ID_bin_bignum:
#endif // CONFIG_FIXED_BASED_OBJECTS
            case ID_based_integer:
            case ID_based_bignum:       menu = ID_BasesMenu; break;
            case ID_text:               menu = ID_TextMenu; break;
            case ID_expression:         menu = ID_SymbolicMenu; break;
            case ID_program:            menu = ID_DebugMenu; break;
            case ID_list:               menu = ID_ListMenu; break;
            case ID_array:              menu = ID_MatrixMenu; break;
            case ID_tag:                menu = ID_ObjectMenu; break;
            case ID_unit:               menu = ID_UnitsConversionsMenu; break;
            default:                    break;
            }
        }
    }
    else if (!rt.args(0))
    {
        return ERROR;
    }

    object_p obj = command::static_object(menu);
    return obj->evaluate();
}


COMMAND_BODY(LastMenu)
// ----------------------------------------------------------------------------
//   Go back one entry in the menu history
// ----------------------------------------------------------------------------
{
    if (!rt.args(0))
        return ERROR;
    ui.menu_pop();
    return OK;
}



// ============================================================================
//
//   Creation of a menu
//
// ============================================================================

#define MENU(SysMenu, ...)                                              \
MENU_BODY(SysMenu)                                                      \
/* ------------------------------------------------------------ */      \
/*   Create a system menu                                       */      \
/* ------------------------------------------------------------ */      \
{                                                                       \
    uint  nitems = count(__VA_ARGS__);                                  \
    items_init(mi, nitems);                                             \
    items(mi, ##__VA_ARGS__);                                           \
    return true;                                                        \
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
     "Min",     ID_Min,
     "Max",     ID_Max,
     ID_mod,
     ID_abs,
     "→Num",    ID_ToDecimal,
     "→Frac",   ID_ToFraction,

     "Ceil",    ID_Unimplemented,
     "Floor",   ID_Unimplemented,
     ID_rem,
     "%",       ID_Percent,
     "%Chg",    ID_PercentChange,
     "%Total",  ID_PercentTotal,

     "Trig",    ID_CircularMenu,
     "Hyper",   ID_HyperbolicMenu,
     "Exp/Log", ID_ExpLogMenu,
     "Prob",    ID_ProbabilitiesMenu,
     "Angles",  ID_AnglesMenu,
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
     "IntPart", ID_IntPart,
     "FrcPart", ID_FracPart,
     "Mant",    ID_Unimplemented,
     "Xpon",    ID_Unimplemented,

     "RowNorm", ID_Unimplemented,
     "ColNorm", ID_Unimplemented,
     "Dim",     ID_Size,
     "Type",    ID_Type);


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

     "→DMS",    ID_ToDMS,
     "DMS→",    ID_FromDMS,
     "DMS+",    ID_DMSAdd,
     "DMS-",    ID_DMSSub,
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

     "2+i3",    ID_ComplexIBeforeImaginary,
     "2+3i",    ID_ComplexIAfterImaginary,
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
     "[]",      ID_SelfInsert,
     "Idnty",   ID_Unimplemented,
     "Const",   ID_Unimplemented,
     "Transp",  ID_Unimplemented,
     "Redim",   ID_Unimplemented,
     "Random",  ID_Unimplemented,

     "Det",     ID_det,
     "Norm",    ID_abs,
     "Make",    ID_Unimplemented,
     "Norms",   ID_Unimplemented,
     "Factor",  ID_Unimplemented,
     "Resize",  ID_Unimplemented,

#if 0
     "RowNrm",  ID_Unimplemented,
     "ColNrm",  ID_Unimplemented,
     "CondNum", ID_Unimplemented,
     "SpecRad", ID_Unimplemented,
     "→Mat",    ID_Unimplemented,
     "Mat→",    ID_Unimplemented,

     "QR",      ID_Unimplemented);
#endif

     "Conjug",  ID_conj,
     "Size",    ID_Unimplemented,
     "→Diag",   ID_Unimplemented,
     "LU",      ID_Unimplemented,
     "Schur",   ID_Unimplemented,
     "LQ",      ID_Unimplemented);


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

     Base::label, ID_Base,
     "Bin",     ID_Bin,
     "Oct",     ID_Oct,
     "Dec",     ID_Dec,
     "Hex",     ID_Hex,

     WordSize::label,  ID_WordSize,
     ID_NAnd,
     ID_NOr,
     ID_Implies,
     ID_Excludes,

     "SL",      ID_SL,
     "SR",      ID_SR,
     "ASR",     ID_ASR,
     "RL",      ID_RL,
     "RR",      ID_RR,

     "SLB",     ID_SLB,
     "SRB",     ID_SRB,
     "ASRB",    ID_ASRB,
     "RLB",     ID_RLB,
     "RRB",     ID_RRB,

     "SLC",     ID_SLC,
     "SRC",     ID_SRC,
     "ASRC",    ID_ASRC,
     "RLC",     ID_RLC,
     "RRC",     ID_RRC,

     "#",       ID_SelfInsert,
     "R→B",     ID_RealToBinary,
     "B→R",     ID_BinaryToReal,
     Base::label, ID_Base,
     WordSize::label,  ID_WordSize,

     ID_And,
     ID_Or,
     ID_Xor,
     ID_Not,
     "neg",     ID_neg,

     "FstSet",  ID_Unimplemented,
     "LstSet",  ID_Unimplemented,
     "PopCnt",  ID_Unimplemented,
     "1-comp",  ID_OnesComplement,
     "2-comp",  ID_TwosComplement);


MENU(ProbabilitiesMenu,
// ----------------------------------------------------------------------------
//   Probabilities
// ----------------------------------------------------------------------------
     "!",       ID_fact,
     "Comb",    ID_Unimplemented,
     "Perm",    ID_Unimplemented,
     "",        ID_Unimplemented,
     "Random",  ID_Unimplemented,

     "Γ",        ID_tgamma,
     "ln(Γ)",    ID_lgamma,
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
     "Σ+",      ID_AddData,
     "Σ-",      ID_RemoveData,
     "Total",   ID_Total,
     "Mean",    ID_Average,
     "StdDev" , ID_StandardDeviation,
     "Corr",    ID_Correlation,

     "XCol",    ID_IndependentColumn,
     "YCol",    ID_DependentColumn,
     "MinΣ",    ID_MinData,
     "MaxΣ",    ID_MaxData,
     "ΣSize",   ID_DataSize,
     "Median",  ID_Median,

     "StoΣ",    ID_StoreData,
     "RclΣ",    ID_RecallData,
     "ClrΣ",    ID_ClearData,
     "Popul",   ID_PopulationMenu,
     "Regres",  ID_RegressionMenu,
     "Plot",    ID_PlotMenu);

MENU(RegressionMenu,
// ----------------------------------------------------------------------------
//   Statistics regression menu
// ----------------------------------------------------------------------------
     "BestFit", ID_BestFit,
     "ExpFit",  ID_LinearFit,
     "LinFit",  ID_ExponentialFit,
     "LogFit",  ID_PowerFit,
     "PwrFit",  ID_LogarithmicFit,
     "LR",      ID_LinearRegression,

     "XCol",    ID_IndependentColumn,
     "YCol",    ID_DependentColumn,
     "StdDev" , ID_StandardDeviation,
     "Corr",    ID_Correlation,
     "Inter",   ID_Intercept,
     "Slope",   ID_Slope,

     "ΣX",      ID_SumOfX,
     "ΣY",      ID_SumOfY,
     "ΣXY",     ID_SumOfXY,
     "ΣX²",     ID_SumOfXSquares,
     "ΣY²",     ID_SumOfYSquares,
     "ΣSize",   ID_DataSize);

MENU(PopulationMenu,
// ----------------------------------------------------------------------------
//   Statistics population menu
// ----------------------------------------------------------------------------
     "XCol",    ID_IndependentColumn,
     "YCol",    ID_DependentColumn,
     "Bins",    ID_FrequencyBins,
     "PopVar",  ID_PopulationVariance,
     "PopSDev", ID_PopulationStandardDeviation,
     "PCovar",  ID_PopulationCovariance);


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
     "Debug",           ID_Debug,
     "Step",            ID_SingleStep,
     "Over",            ID_StepOver,
     "Steps",           ID_MultipleSteps,
     "Continue",        ID_Continue,
     "Prog",            ID_ProgramMenu,
     "Halt",            ID_Halt,
     "Kill",            ID_Kill,
     "Step↑",           ID_StepOut);

MENU(TestsMenu,
// ----------------------------------------------------------------------------
//   Tests
// ----------------------------------------------------------------------------
     "<",               ID_TestLT,
     "=",               ID_TestEQ,
     ">",               ID_TestGT,
     "≤",               ID_TestLE,
     "≠",               ID_TestNE,
     "≥",               ID_TestGE,

     "IfThen",          ID_IfThen,
     "IfElse",          ID_IfThenElse,
     "IfErr",           ID_IfErrThen,
     "IfErrElse",       ID_IfErrThen,
     "IFT",             ID_IFT,
     "IFTE",            ID_IFTE,

     "Case",            ID_CaseStatement,
     "Then",            ID_CaseThen,
     "When",            ID_CaseWhen,
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
     "Set",     ID_SetFlag,
     "Clear",   ID_ClearFlag,
     "Set?",    ID_TestFlagSet,
     "Clear?",  ID_TestFlagClear,
     "Set?Clr", ID_TestFlagSetThenClear,
     "Clr?Clr", ID_TestFlagClearThenClear,

     "F→Bin",   ID_FlagsToBinary,
     "Bin→F",   ID_BinaryToFlags,
     "Tests",   ID_TestsMenu,
     "Flip",    ID_FlipFlag,
     "Set?Set", ID_TestFlagSetThenSet,
     "Clr?Set", ID_TestFlagClearThenSet,

     "Prog",    ID_ProgramMenu,
     "Loops",   ID_LoopsMenu,
     "Modes",   ID_ModesMenu
);

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
     "→List",   ID_ToList,
     "List→",   ID_FromList,
     "Size",    ID_Size,
     "Head",    ID_Head,
     "Tail",    ID_Tail,
     "Get",     ID_Get,

     "Sort",    ID_Sort,
     "QSort",   ID_QuickSort,
     "Reverse", ID_ReverseList,
     "GetI",    ID_GetI,
     "PutI",    ID_PutI,
     "Put",     ID_Put,

     "RSort",   ID_ReverseSort,
     "RQSort",  ID_ReverseQuickSort,
     "Objects", ID_ObjectMenu,
     "Matrix",  ID_MatrixMenu,
     "Vector",  ID_VectorMenu,
     "Data",    ID_DataMenu);


MENU(DataMenu,
// ----------------------------------------------------------------------------
//   Operations on data
// ----------------------------------------------------------------------------
     "→List",   ID_ToList,
     "Obj→",    ID_Explode,
     "Sort",    ID_Sort,
     "Map",     ID_Map,
     "Reduce",  ID_Reduce,
     "Filter",  ID_Filter,

     "∑",       ID_Sum,
     "∏",       ID_Product,
     "∆",       ID_Unimplemented,
     "Get",     ID_Get,
     "Put",     ID_Put,
     "Reverse", ID_ReverseList,

     "Find",    ID_Unimplemented,
     "QSort",   ID_QuickSort,
     "RSort",   ID_ReverseSort,
     "RQSort",  ID_ReverseQuickSort,
     "Objects", ID_ObjectMenu,
     "Lists",   ID_ListMenu);


MENU(ObjectMenu,
// ----------------------------------------------------------------------------
//  Operations on objects
// ----------------------------------------------------------------------------
     "Bytes",   ID_Bytes,
     "Type",    ID_Type,
     "TypeName",ID_TypeName,
     "Clone",   ID_Unimplemented,
     "Obj→",    ID_Explode,
     "→Text",   ID_ToText,

     "→List",   ID_ToList,
     "→Prog",   ID_Unimplemented,
     "→Array",  ID_Unimplemented,
     "→Num",    ID_ToDecimal,
     "→Frac",   ID_ToFraction,
     "→Graph",  ID_Unimplemented,

     "→Tag",    ID_ToTag,
     "Tag→",    ID_FromTag,
     "DTag",    ID_dtag,
     "Matrix",  ID_MatrixMenu,
     "List",    ID_ListMenu,
     "Data",    ID_DataMenu);


MENU(UnitsConversionsMenu,
// ----------------------------------------------------------------------------
//   Menu managing units and unit conversions
// ----------------------------------------------------------------------------
     "Convert", ID_Convert,
     "Base",    ID_UBase,       // Base unit
     "Value",   ID_UVal,
     "Factor",  ID_UFact,
     "→Unit",   ID_ToUnit,

     "m (-3)",  ID_ConvertToUnitPrefix,
     "c (-2)",  ID_ConvertToUnitPrefix,
     "k (+3)",  ID_ConvertToUnitPrefix,
     "M (+6)",  ID_ConvertToUnitPrefix,
     "G (+9)",  ID_ConvertToUnitPrefix,

     "µ (-6)",  ID_ConvertToUnitPrefix,
     "n (-9)",  ID_ConvertToUnitPrefix,
     "p (-12)", ID_ConvertToUnitPrefix,
     "T (+12)", ID_ConvertToUnitPrefix,
     "P (+15)", ID_ConvertToUnitPrefix,

     "f (-15)", ID_ConvertToUnitPrefix,
     "d (-1)",  ID_ConvertToUnitPrefix,
     "da (+1)", ID_ConvertToUnitPrefix,
     "h (+2)",  ID_ConvertToUnitPrefix,
     "E (+18)", ID_ConvertToUnitPrefix,

     "y (-24)", ID_ConvertToUnitPrefix,
     "z (-21)", ID_ConvertToUnitPrefix,
     "a (-18)", ID_ConvertToUnitPrefix,
     "Z (+21)", ID_ConvertToUnitPrefix,
     "Y (+24)", ID_ConvertToUnitPrefix,

     "Ki",      ID_ConvertToUnitPrefix,
     "Mi",      ID_ConvertToUnitPrefix,
     "Gi",      ID_ConvertToUnitPrefix,
     "Ti",      ID_ConvertToUnitPrefix,
     "Pi",      ID_ConvertToUnitPrefix,

     "Ei",      ID_ConvertToUnitPrefix,
     "Zi",      ID_ConvertToUnitPrefix,
     "Yi",      ID_ConvertToUnitPrefix,
     "Ri",      ID_ConvertToUnitPrefix,
     "Qi",      ID_ConvertToUnitPrefix
);


MENU(StackMenu,
// ----------------------------------------------------------------------------
//   Operations on the stack
// ----------------------------------------------------------------------------
     "Rot",     ID_Rot,
     "Over",    ID_Over,
     "Depth",   ID_Depth,
     "Pick",    ID_Pick,
     "Roll",    ID_Roll,
     "RollDn",  ID_RollD,

     "Dup",     ID_Dup,
     "Drop",    ID_Drop,
     "Dup2",    ID_Dup2,
     "Drop2",   ID_Drop2,
     ID_DupN,
     ID_DropN,

     "Swap",    ID_Swap,
     "LastArg", ID_LastArg,
     "ClearStk",ID_ClearStack,
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
     "Search",  ID_EditorSearch,
     "Cut",     ID_EditorCut,
     "Paste",   ID_EditorPaste,

     "Csr⇄Sel", ID_EditorFlip,
     "|←",      ID_EditorBegin,
     "→|",      ID_EditorEnd,
     "Replace", ID_EditorReplace,
     "Copy",    ID_EditorCopy,
     "Clear",   ID_EditorClear);

MENU(IntegrationMenu,
// ----------------------------------------------------------------------------
//   Symbolic and numerical integration
// ----------------------------------------------------------------------------
     "∫",       ID_Integrate,
     "Num ∫",   ID_Integrate,
     "Symb ∫",  ID_Unimplemented,
     "Prim",    ID_Unimplemented,

     "Eq",      ID_Equation,
     "Indep",   ID_Unimplemented);

MENU(DifferentiationMenu,
// ----------------------------------------------------------------------------
//   Symbolic and numerical differentiation
// ----------------------------------------------------------------------------
     "Num",     ID_Unimplemented,
     "Symb",    ID_Unimplemented,
     "Diff",    ID_Unimplemented,

     "Eq",      ID_Equation,
     "Indep",   ID_Unimplemented);

MENU(SolverMenu,
// ----------------------------------------------------------------------------
//   The solver menu / application
// ----------------------------------------------------------------------------
     "Eq",      ID_Equation,
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
     "Eq",      ID_Equation,
     "Indep",   ID_Unimplemented,
     "Root",    ID_Unimplemented,

     ID_SolverMenu);

MENU(DifferentialSolverMenu,
// ----------------------------------------------------------------------------
//   Menu for differential equation solving
// ----------------------------------------------------------------------------
     "Eq",      ID_Equation,
     "Indep",   ID_Unimplemented,
     "Root",    ID_Unimplemented,

     ID_SolverMenu);


MENU(SymbolicSolverMenu,
// ----------------------------------------------------------------------------
//   Menu for symbolic equation solving
// ----------------------------------------------------------------------------
     "Eq",      ID_Equation,
     "Indep",   ID_Unimplemented,
     "Root",    ID_Unimplemented,

     ID_SolverMenu);

MENU(PolynomialSolverMenu,
// ----------------------------------------------------------------------------
//   Menu for polynom solving
// ----------------------------------------------------------------------------
     "Eq",      ID_Equation,
     "Indep",   ID_Unimplemented,
     "Root",    ID_Unimplemented,

     ID_SolverMenu);

MENU(LinearSolverMenu,
// ----------------------------------------------------------------------------
//   Menu for linear system solving
// ----------------------------------------------------------------------------
     "Eq",      ID_Equation,
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
     "/",       ID_SelfInsert,
     "%",       ID_Percent,
     "→Num",    ID_ToDecimal,
     "→Frac",   ID_ToFraction,
     "→DMS",    ID_ToDMS,
     "DMS→",    ID_FromDMS,

     "%Total",  ID_PercentTotal,
     "%Chg",    ID_PercentChange,
     "DMS+",    ID_DMSAdd,
     "DMS-",    ID_DMSSub,
     "→HMS",    ID_ToHMS,
     "HMS→",    ID_FromHMS,

     "Frac→",   ID_Explode,
     "Cycle",   ID_Cycle,
     FractionIterations::label,         ID_FractionIterations,
     FractionDigits::label,             ID_FractionDigits,
     "1 1/2",   ID_MixedFractions,
     "3/2",     ID_ImproperFractions
);


MENU(PlotMenu,
// ----------------------------------------------------------------------------
//   Plot and drawing menu
// ----------------------------------------------------------------------------
     "Function",ID_Function,
     "Polar",   ID_Polar,
     "Param",   ID_Parametric,
     "Scatter", ID_Scatter,
     "Bar",     ID_Bar,
     "Axes",    ID_Drax,

     "Foregnd", ID_Foreground,
     "LineWdth",ID_LineWidth,
     "Lines",   ID_CurveFilling,
     "Points",  ID_NoCurveFilling,
     "Axes",    ID_DrawPlotAxes,
     "No Axes", ID_NoPlotAxes,

     "Backgnd", ID_Background,
     "Clear",   ID_ClLCD);

MENU(ClearThingsMenu,
// ----------------------------------------------------------------------------
//  Clearing various things
// ----------------------------------------------------------------------------
     "Stack",   ID_ClearStack,
     "Dir",     ID_Unimplemented,
     "Stats",   ID_ClearData,
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

     "Beep",    ID_BeepOn,
     "Silent",  ID_BeepOff,
     "Flash",   ID_SilentBeepOn,
     "NoFlash", ID_SilentBeepOff,
     "Modes",   ID_Modes,
     "Reset",   ID_ResetModes);

MENU(DisplayModesMenu,
// ----------------------------------------------------------------------------
//   Mode setting for numbers
// ----------------------------------------------------------------------------
     "Std",             ID_Std,
     Fix::label,        ID_Fix,
     Sci::label,        ID_Sci,
     Eng::label,        ID_Eng,
     Sig::label,        ID_Sig,
     Precision::label,  ID_Precision,

     MantissaSpacing::label, ID_MantissaSpacing,
     FractionSpacing::label, ID_FractionSpacing,
     BasedSpacing::label,    ID_BasedSpacing,
     StandardExponent::label, ID_StandardExponent,
     MinimumSignificantDigits::label, ID_MinimumSignificantDigits,
     "Seps",            ID_SeparatorModesMenu,

     "1 1/2",           ID_MixedFractions,
     "3/2",             ID_ImproperFractions,
     "1/3",             ID_BigFractions,
     "¹/₃",             ID_SmallFractions,
     "UI",              ID_UserInterfaceModesMenu,
     "Math",            ID_MathModesMenu);

MENU(SeparatorModesMenu,
// ----------------------------------------------------------------------------
//   Separators
// ----------------------------------------------------------------------------
     "1 000",           ID_NumberSpaces,
     Settings.DecimalComma() ? "1.000," : "1,000.",  ID_NumberDotOrComma,
     "1'000",           ID_NumberTicks,
     "1_000",           ID_NumberUnderscore,
     "2.3",             ID_DecimalDot,
     "2,3",             ID_DecimalComma,

     "#1 000",          ID_BasedSpaces,
     Settings.DecimalComma() ? "#1.000" : "#1,000",  ID_BasedDotOrComma,
     "#1'000",          ID_BasedTicks,
     "#1_000",          ID_BasedUnderscore,
     "Disp",            ID_DisplayModesMenu,
     "Modes",           ID_ModesMenu,

     "1.2x10³²",        ID_FancyExponent,
     "1.2E32",          ID_ClassicExponent,
     "1.0→1.",          ID_TrailingDecimal,
     "1.0→1",           ID_NoTrailingDecimal);

MENU(UserInterfaceModesMenu,
// ----------------------------------------------------------------------------
//   Mode setting for numbers
// ----------------------------------------------------------------------------
     "GrStk",                                   ID_GraphicStackDisplay,
     "TxtStk",                                  ID_TextStackDisplay,
     ResultFont::label,                         ID_ResultFont,
     StackFont::label,                          ID_StackFont,
     EditorFont::label,                         ID_EditorFont,
     MultilineEditorFont::label,                ID_MultilineEditorFont,

     "3-lines",                                 ID_ThreeRowsMenus,
     "1-line",                                  ID_SingleRowMenus,
     "Flat",                                    ID_FlatMenus,
     "Round",                                   ID_RoundedMenus,
     "Square",                                  ID_SquareMenus,
     CursorBlinkRate::label,                    ID_CursorBlinkRate,

     "cmd",                                     ID_LowerCase,
     "CMD",                                     ID_UpperCase,
     "Cmd",                                     ID_Capitalized,
     "Command",                                 ID_LongForm,
     "Show Units",                              ID_ShowBuiltinUnits,
     "Hide Units",                              ID_HideBuiltinUnits);

MENU(MathModesMenu,
// ----------------------------------------------------------------------------
//   Mode setting for numbers
// ----------------------------------------------------------------------------
     "Num",                                     ID_NumericalResults,
     "Sym",                                     ID_SymbolicResults,
     "Simplify",                                ID_AutoSimplify,
     "KeepAll",                                 ID_NoAutoSimplify,
     "iℂ",                                      ID_Unimplemented,
     "Auto ℂ",                                  ID_Unimplemented,

     "0^0=1",                                   ID_ZeroPowerZeroIsOne,
     "0^0=?",                                   ID_ZeroPowerZeroIsUndefined,
     MaxNumberBits::label,                      ID_MaxNumberBits,
     MaxRewrites::label,                        ID_MaxRewrites,
     FractionIterations::label,                 ID_FractionIterations,
     FractionDigits::label,                     ID_FractionDigits,

     "HW flt",                                  ID_HardwareFloatingPoint,
     "SW flt",                                  ID_SoftwareFloatingPoint,
     "Display",                                 ID_DisplayModesMenu,
     "Disp",                                    ID_DisplayModesMenu,
     "UI",                                      ID_UserInterfaceModesMenu,
     "Modes",                                   ID_ModesMenu);

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

     "LnWidth", ID_LineWidth,
     "Foregnd", ID_Foreground,
     "Bckgnd",  ID_Background,
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
     "→HMS",    ID_ToHMS,
     "HMS→",    ID_FromHMS,
     "HMS+",    ID_HMSAdd,
     "HMS-",    ID_HMSSub,

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
     "→Text",           ID_ToText,
     "Text→",           ID_Compile,
     "Length",          ID_Size,
     "Append",          ID_add,
     "Repeat",          ID_mul,
     "C→Code",          ID_CharToUnicode,

     "T→Code",          ID_TextToUnicode,
     "Code→T",          ID_UnicodeToText);
