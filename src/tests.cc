// ****************************************************************************
//  tests.cc                                                      DB48X project
// ****************************************************************************
//
//   File Description:
//
//     Tests for the runtime
//
//     The tests are run by actually sending keystrokes and observing the
//     calculator's state
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

#include "tests.h"

#include "dmcp.h"
#include "recorder.h"
#include "settings.h"
#include "sim-dmcp.h"
#include "stack.h"
#include "types.h"
#include "user_interface.h"

#include <regex.h>
#include <stdio.h>

extern bool run_tests;
volatile uint keysync_sent = 0;
volatile uint keysync_done = 0;

RECORDER_DECLARE(errors);

uint tests::default_wait_time  = 500;
uint tests::image_wait_time = 500;
uint tests::key_delay_time = 2;
uint tests::refresh_delay_time = 50;

#define TEST_CATEGORY(name, enabled, descr)                     \
    RECORDER_TWEAK_DEFINE(est_##name, enabled, "Test " descr);  \
    static inline bool check_##name(tests &t)                   \
    {                                                           \
        bool result = RECORDER_TWEAK(est_##name);               \
        if (!result)                                            \
            t.begin("Skipping " #name ": " descr, true);        \
        else                                                    \
            t.begin(#name ": " descr);                          \
        return result;                                          \
    }

#define TESTS(name, descr)      TEST_CATEGORY(name, true,  descr)
#define EXTRA(name, descr)      TEST_CATEGORY(name, false, descr)

#define BEGIN(name)     do { if (!check_##name(*this)) return; } while(0)

TESTS(defaults,         "Reset settings to defaults");
TESTS(shifts,           "Shift logic");
TESTS(keyboard,         "Keyboard entry");
TESTS(types,            "Data types");
TESTS(editor,           "Editor operations");
TESTS(stack,            "Stack operations");
TESTS(arithmetic,       "Arithmetic operations");
TESTS(globals,          "Global variables");
TESTS(locals,           "Local variables");
TESTS(for_loops,        "For loops");
TESTS(conditionals,     "Conditionals");
TESTS(logical,          "Logical operations");
TESTS(styles,           "Commands display formats");
TESTS(iformat,          "Integer display formats");
TESTS(fformat,          "Fraction display formats");
TESTS(dformat,          "Decimal display formats");
TESTS(ifunctions,       "Integer functions");
TESTS(dfunctions,       "Decimal functions");
TESTS(float,            "Hardware-accelerated 7-digit (float)")
TESTS(double,           "Hardware-accelerated 16-digit (double)")
TESTS(highp,            "High-precision computations (60 digits)")
TESTS(trigoptim,        "Special trigonometry optimzations");
TESTS(trigunits,        "Trigonometric units");
TESTS(dfrac,            "Simple conversion to decimal and back");
TESTS(ctypes,           "Complex types");
TESTS(carith,           "Complex arithmetic");
TESTS(cfunctions,       "Complex functions");
TESTS(units,            "Units and conversions");
TESTS(lists,            "List operations");
TESTS(sorting,          "Sorting operations");
TESTS(text,             "Text operations");
TESTS(vectors,          "Vectors");
TESTS(matrices,         "Matrices");
TESTS(solver,           "Solver");
TESTS(integrate,        "Numerical integration");
TESTS(simplify,         "Auto-simplification of expressions");
TESTS(rewrites,         "Equation rewrite engine");
TESTS(expand,           "Expand");
TESTS(tagged,           "Tagged objects");
TESTS(catalog,          "Catalog of commands");
TESTS(cycle,            "Cycle command for quick conversions");
TESTS(rotate,           "Shift and rotate instructions");
TESTS(flags,            "User flags");
TESTS(regressions,      "Regression checks");
TESTS(plotting,         "Plotting, graphing and charting");
TESTS(graphics,         "Graphic commands");
TESTS(help,             "On-line help");
TESTS(gstack,           "Graphic stack rendering")
TESTS(hms,              "HMS and DMS operations");
TESTS(date,             "Date operations");
TESTS(infinity,         "Infinity and undefined operations");
TESTS(overflow,         "Overflow and underflow");
TESTS(insert,           "Insertion of variables, units and constants");
TESTS(characters,       "Character menu and catalog");

EXTRA(plotfns,          "Plot all functions");
EXTRA(sysflags,         "Enable/disable every RPL flag");
EXTRA(settings,         "Recall and activate every RPL setting");
EXTRA(commands,         "Parse every single RPL command");


void tests::run(bool onlyCurrent)
// ----------------------------------------------------------------------------
//   Run all test categories
// ----------------------------------------------------------------------------
{
    tindex = sindex = cindex = count = 0;
    failures.clear();

    auto tracing           = RECORDER_TRACE(errors);
    RECORDER_TRACE(errors) = false;

    // Reset to known settings stateg
    reset_settings();

    if (onlyCurrent)
    {
        // Test the current thing
        numerical_integration_testing();
    }
    else
    {
        shift_logic();
        keyboard_entry();
        data_types();
        editor_operations();
        stack_operations();
        arithmetic();
        global_variables();
        local_variables();
        for_loops();
        conditionals();
        logical_operations();
        command_display_formats();
        integer_display_formats();
        fraction_display_formats();
        decimal_display_formats();
        integer_numerical_functions();
        decimal_numerical_functions();
        float_numerical_functions();
        double_numerical_functions();
        high_precision_numerical_functions();
        exact_trig_cases();
        trig_units();
        fraction_decimal_conversions();
        complex_types();
        complex_arithmetic();
        complex_functions();
        units_and_conversions();
        list_functions();
        sorting_functions();
        vector_functions();
        matrix_functions();
        solver_testing();
        numerical_integration_testing();
        text_functions();
        auto_simplification();
        rewrite_engine();
        expand_collect_simplify();
        tagged_objects();
        catalog_test();
        cycle_test();
        shift_and_rotate();
        flags_functions();
        flags_by_name();
        settings_by_name();
        parsing_commands_by_name();
        plotting();
        plotting_all_functions();
        graphic_commands();
        hms_dms_operations();
        date_operations();
        infinity_and_undefined();
        overflow_and_underflow();
        online_help();
        graphic_stack_rendering();
        insertion_of_variables_constants_and_units();
        character_menu();
        regression_checks();
    }
    summary();

    RECORDER_TRACE(errors) = tracing;

    if (run_tests)
        exit(failures.size() ? 1 : 0);
}


void tests::reset_settings()
// ----------------------------------------------------------------------------
//   Use settings that make the results predictable on screen
// ----------------------------------------------------------------------------
{
    // Reset to default test settings
    BEGIN(defaults);
    Settings = settings();

    // Check that we have actually reset the settings
    step("Select Modes menu")
        .test("ModesMenu", ENTER).noerror();
    step("Checking output modes")
        .test("Modes", ENTER)
        .expect("« ModesMenu »");

    // Check that we can change a setting
    step("Selecting FIX 3")
        .test(CLEAR, SHIFT, O, 3, F2, "1.23456", ENTER)
        .expect("1.235");
    step("Checking Modes for FIX")
        .test("Modes", ENTER)
        .expect("« 3 FixedDisplay 3 DisplayDigits DisplayModesMenu »");
    step("Reseting with command")
        .test("ResetModes", ENTER)
        .noerror()
        .test("Modes", ENTER)
        .expect("« DisplayModesMenu »");
}


void tests::shift_logic()
// ----------------------------------------------------------------------------
//   Test all keys and check we have the correct output
// ----------------------------------------------------------------------------
{
    BEGIN(shifts);

    step("Shift state must be cleared at start")
        .shift(false)
        .xshift(false)
        .alpha(false)
        .lower(false);

    step("Shift basic cycle")
        .test(SHIFT)
        .shift(true)
        .xshift(false)
        .alpha(false)
        .lower(false);
    step("Shift-Shift is Right Shift")
        .test(SHIFT)
        .shift(false)
        .xshift(true)
        .alpha(false)
        .lower(false);
    step("Third shift clears all shifts")
        .test(SHIFT)
        .shift(false)
        .xshift(false)
        .alpha(false)
        .lower(false);

    step("Shift second cycle")
        .test(SHIFT)
        .shift(true)
        .xshift(false)
        .alpha(false)
        .lower(false);
    step("Shift second cycle: Shift-Shift is Right Shift")
        .test(SHIFT)
        .shift(false)
        .xshift(true)
        .alpha(false)
        .lower(false);
    step("Shift second cycle: Third shift clears all shifts")
        .test(SHIFT)
        .shift(false)
        .xshift(false)
        .alpha(false)
        .lower(false);

    step("Long-press shift is Alpha")
        .test(SHIFT, false)
        .wait(600)
        .test(RELEASE)
        .shift(false)
        .xshift(false)
        .alpha(true);
    step("Long-press shift clears Alpha")
        .test(SHIFT, false)
        .wait(600)
        .test(RELEASE)
        .shift(false)
        .xshift(false)
        .alpha(false);

    step("Typing alpha")
        .test(LONGPRESS, SHIFT, A)
        .shift(false)
        .alpha(true)
        .lower(false)
        .editor("A");
    step("Selecting lowercase with Shift-ENTER")
        .test(SHIFT, ENTER)
        .alpha(true)
        .lower(true);
}


void tests::keyboard_entry()
// ----------------------------------------------------------------------------
//   Test all keys and check we have the correct output
// ----------------------------------------------------------------------------
{
    BEGIN(keyboard);

    step("Uppercase entry");
    cstring entry = "ABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789";
    test(CLEAR, entry).editor(entry);

    step("Lowercase entry");
    cstring lowercase = "abcdefghijklmnopqrstuvwxyz0123456789";
    test(CLEAR, lowercase).editor(lowercase);

    step("Special characters");
    cstring special = "X+-*/!? #_";
    test(CLEAR, special).editor(special);

    step("Separators");
    cstring seps = "\"Hello [A] (B) {C} 'Test' D";
    test(CLEAR, seps).editor(seps);

    step("Separators with auto-spacing");
    cstring seps2     = "{}()[]";
    cstring seps2auto = "{ } ( ) []";
    test(CLEAR, seps2).editor(seps2auto);

    step("Key repeat");
    test(CLEAR, LONGPRESS, SHIFT, LONGPRESS, A)
        .wait(1000)
        .test(RELEASE)
        .check(ui.cursor > 4);

    step("Space key during data entry inserts space")
        .test(CLEAR, SHIFT, RUNSTOP,
              KEY7, SPACE, ALPHA, A, SPACE, B,
              NOSHIFT, ADD, ADD)
        .editor("«7 A B + + »");
    step("Space key in immediate mode evaluates")
        .test(ENTER).expect("« 7 A B + + »")
        .test(SPACE).expect("'7+(A+B)'");
    step("F key inserts equation")
        .test(CLEAR, F).editor("''")
        .test(KEY1).editor("'1'");
    step("Space key in expresion inserts = sign")
        .test(SPACE).editor("'1='")
        .test(KEY2).editor("'1=2'")
        .test(ADD).editor("'1=2+'")
        .test(KEY3).editor("'1=2+3'");
    step("F key in equation inserts parentheses")
        .test(MUL).editor("'1=2+3·'")
        .test(F).editor("'1=2+3· ()'");
    step("Automatic insertion of parentheses after functions")
        .test(D).editor("'1=2+3· (exp())'")
        .test(KEY0).editor("'1=2+3· (exp(0))'");
    step("Space key in parentheses insert semi-colon")
        .test(SPACE).editor("'1=2+3· (exp(0;))'")
        .test(KEY7).editor("'1=2+3· (exp(0;7))'");

    step("STO key while entering equation (bug #390)")
        .test(CLEAR, EXIT, KEY1, KEY2, F,
              ALPHA, A, B, C, NOSHIFT, G).noerror()
        .test(F, ALPHA, A, B, C, ENTER, SPACE).expect("12")
        .test("'ABC'", ENTER, RSHIFT, G, F6).noerror();

    step("Inserting a colon in text editor inserts tag delimiters")
        .test(CLEAR, ALPHA, KEY0).editor("::");
    step("Inserting a colon in text inserts a single colon")
        .test(CLEAR, RSHIFT, ENTER, KEY0).editor("\":\"");
}


void tests::data_types()
// ----------------------------------------------------------------------------
//   Check the basic data types
// ----------------------------------------------------------------------------
{
    BEGIN(types);

    step("Positive integer");
    test(CLEAR, "1", ENTER).type(object::ID_integer).expect("1");
    step("Negative integer");
    test(CLEAR, "1", CHS, ENTER).type(object::ID_neg_integer).expect("-1");

#if CONFIG_FIXED_BASED_OBJECTS
    step("Binary based integer");
    test(CLEAR, "#10010101b", ENTER)
        .type(object::ID_bin_integer)
        .expect("#1001 0101₂");
    test(CLEAR, "#101b", ENTER).type(object::ID_bin_integer).expect("#101₂");

    step("Decimal based integer");
    test(CLEAR, "#12345d", ENTER)
        .type(object::ID_dec_integer)
        .expect("#1 2345₁₀");
    test(CLEAR, "#123d", ENTER).type(object::ID_dec_integer).expect("#123₁₀");

    step("Octal based integer");
    test(CLEAR, "#12345o", ENTER)
        .type(object::ID_oct_integer)
        .expect("#1 2345₈");
    test(CLEAR, "#123o", ENTER).type(object::ID_oct_integer).expect("#123₈");

    step("Hexadecimal based integer");
    test(CLEAR, "#1234ABCDh", ENTER)
        .type(object::ID_hex_integer)
        .type(object::ID_hex_integer)
        .expect("#1234 ABCD₁₆");
    test(CLEAR, "#DEADBEEFh", ENTER)
        .type(object::ID_hex_integer)
        .expect("#DEAD BEEF₁₆");
#endif // CONFIG_FIXED_BASED_OBJECTS

    step("Arbitrary base input");
    test(CLEAR, "8#777", ENTER).type(object::ID_based_integer).expect("#1FF₁₆");
    test(CLEAR, "2#10000#ABCDE", ENTER)
        .type(object::ID_based_integer)
        .expect("#A BCDE₁₆");

    step("Do not parse #7D as a decimal (#371)")
        .test(CLEAR, "#7D", ENTER).expect("#7D₁₆");

    step("Symbols");
    cstring symbol = "ABC123Z";
    test(CLEAR, symbol, ENTER).type(object::ID_expression).expect("'ABC123Z'");

    step("Text");
    cstring string = "\"Hello World\"";
    test(CLEAR, string, ENTER).type(object::ID_text).expect(string);

    step("Text containing quotes")
        .test(CLEAR, RSHIFT, ENTER,
              SHIFT, SHIFT, ENTER, DOWN,
              ALPHA, H, LOWERCASE, E, L, L, O,
              SHIFT, SHIFT, ENTER, DOWN, ENTER)
        .type(object::ID_text).expect("\"\"\"Hello\"\"\"")
        .test("1 DISP", ENTER).image("quoted-text");

    step("List");
    cstring list = "{ A 1 3 }";
    test(CLEAR, list, ENTER).type(object::ID_list).expect(list);

    step("Program");
    cstring prgm = "« 1 + sin »";
    test(CLEAR, SHIFT, RUNSTOP, 1, ADD, "sin", ENTER)
        .type(object::ID_program)
        .expect(prgm);

    step("Equation");
    cstring eqn = "'X+1'";
    test(CLEAR, XEQ, "X", ENTER, KEY1, ADD)
        .type(object::ID_expression)
        .expect(eqn);
    cstring eqn2 = "'sin(X+1)'";
    test(SIN)
        .type(object::ID_expression)
        .expect(eqn2);
    test(DOWN)
        .editor(eqn2);
    test(ENTER, 1, ADD).
        type(object::ID_expression).expect("'sin(X+1)+1'");

    step("Equation parsing and simplification");
    test(CLEAR, "'(((A))+(B))-(C+D)'", ENTER)
        .type(object::ID_expression)
        .expect("'A+B-(C+D)'");
    step("Equation fancy rendering");
    test(CLEAR, XEQ, "X", ENTER, INV,
         XEQ, "Y", ENTER, SHIFT, SQRT, XEQ, "Z", ENTER,
         "CUBED", ENTER, ADD, ADD, WAIT(100))
        .type(object::ID_expression)
        .expect("'X⁻¹+(Y²+Z³)'");
    step("Equation fancy parsing from editor");
    test(DOWN, SPACE, SPACE, SPACE,
         RSHIFT, DOWN, SHIFT, F3, " 1 +", ENTER)
        .type(object::ID_expression).expect("'X⁻¹+(Y²+Z³)+1'");

    step("Fractions");
    test(CLEAR, "1/3", ENTER).type(object::ID_fraction).expect("¹/₃");
    test(CLEAR, "-80/60", ENTER).type(object::ID_neg_fraction).expect("-1 ¹/₃");
    test(CLEAR, "20/60", ENTER).type(object::ID_fraction).expect("¹/₃");

    step("Large integers");
    cstring b = "123456789012345678901234567890123456789012345678901234567890";
    cstring mb =
        "-123 456 789 012 345 678 901 234 567 890"
        " 123 456 789 012 345 678 901 234 567 890";
    test(CLEAR, b, ENTER).type(object::ID_bignum).expect(mb+1);
    test(DOWN, CHS, ENTER).type(object::ID_neg_bignum).expect(mb);
    test(CHS).type(object::ID_bignum).expect(mb + 1);
    test(DOWN, CHS, ENTER).type(object::ID_neg_bignum).expect(mb);

    step("Large fractions");
    cstring bf =
        "123456789012345678901234567890123456789012345678901234567890/"
        "123456789012345678901234567890123456789012345678901234567891";
    cstring mbf =
        "-¹²³ ⁴⁵⁶ ⁷⁸⁹ ⁰¹² ³⁴⁵ ⁶⁷⁸ ⁹⁰¹ ²³⁴ ⁵⁶⁷ ⁸⁹⁰ ¹²³ ⁴⁵⁶ ⁷⁸⁹ ⁰¹² ³⁴⁵ "
        "⁶⁷⁸ ⁹⁰¹ ²³⁴ ⁵⁶⁷ ⁸⁹⁰/"
        "₁₂₃ ₄₅₆ ₇₈₉ ₀₁₂ ₃₄₅ ₆₇₈ ₉₀₁ ₂₃₄ ₅₆₇ ₈₉₀ ₁₂₃ ₄₅₆ ₇₈₉ ₀₁₂ ₃₄₅ "
        "₆₇₈ ₉₀₁ ₂₃₄ ₅₆₇ ₈₉₁";
    test(CLEAR, bf, ENTER).type(object::ID_big_fraction).expect(mbf+1);
    test(DOWN, CHS, ENTER).type(object::ID_neg_big_fraction).expect(mbf);
    test(CHS).type(object::ID_big_fraction).expect(mbf+1);
    test(CHS).type(object::ID_neg_big_fraction).expect(mbf);
    test(DOWN, CHS, ENTER).type(object::ID_big_fraction).expect(mbf+1);

    step("Graphic objects")
        .test(CLEAR,
              "GROB 9 15 "
              "E300140015001C001400E3008000C110AA00940090004100220014102800",
              ENTER)
        .type(object::ID_grob);

    clear();

    step ("Bytes command");
    test(CLEAR, "12", ENTER, "bytes", ENTER)
        .expect("2")
        .test(BSP)
        .match("#C....");
    test(CLEAR, "129", ENTER, "bytes", ENTER)
        .expect("3")
        .test(BSP)
        .match("#1 81....");

    step("Type command (direct mode)");
    test(CLEAR, "DetailedTypes", ENTER).noerror();
    test(CLEAR, "12 type", ENTER)
        .type(object::ID_neg_integer)
        .expect(~int(object::ID_integer));
    test(CLEAR, "'ABC*3' type", ENTER)
        .type(object::ID_neg_integer)
        .expect(~int(object::ID_expression));

    step("Type command (compatible mode)");
    test(CLEAR, "CompatibleTypes", ENTER).noerror();
    test(CLEAR, "12 type", ENTER)
        .type(object::ID_integer)
        .expect(28);
    test(CLEAR, "'ABC*3' type", ENTER)
        .type(object::ID_integer)
        .expect(9);

    step("TypeName command");
    test(CLEAR, "12 typename", ENTER)
        .type(object::ID_text)
        .expect("\"integer\"");
    test(CLEAR, "'ABC*3' typename", ENTER)
        .type(object::ID_text)
        .expect("\"expression\"");
}


void tests::editor_operations()
// ----------------------------------------------------------------------------
//   Check text editor operations
// ----------------------------------------------------------------------------
{
    BEGIN(editor);

    step("Edit an object")
        .test(CLEAR, "12", ENTER).expect("12")
        .test(DOWN).editor("12");
    step("Inserting text")
        .test("A").editor("A12");
    step("Moving cursor right")
        .test(DOWN, DOWN, "B").editor("A12B");
    step("Moving cursor left")
        .test(UP, UP, "C").editor("A1C2B");
    step("Entering command line")
        .test(ENTER).expect("'A1C2B'");
    step("Entering another entry")
        .test("1 2 3 4", ENTER).expect("4");
    step("Editor history")
        .test(RSHIFT, UP)
        .editor("1 2 3 4")
        .test(RSHIFT, UP)
        .editor("A1C2B");
    step("Editor menu")
        .test(RSHIFT, DOWN);
    step("Selection")
        .test(F1, DOWN, DOWN).editor("A1C2B");
    step("Cut")
        .test(F5).editor("C2B");
    step("Paste")
        .test(F6).editor("A1C2B")
        .test(DOWN, F6).editor("A1CA12B");
    step("Select backwards")
        .test(F1).editor("A1CA12B");
    step("Move cursor word left")
        .test(F2, "X").editor("XA1CA12B");
    step("Move cursor word right")
        .test(F3, "Y").editor("XA1CA12BY");
    step("Swap cursor and selection")
        .test(SHIFT, F1, RUNSTOP, "M").editor("XA1CA1 M2BY");
    step("Copy")
        .test(SHIFT, F5, F2, F6).editor("XA1CA12BY M2BY");
    step("Select to clear selection")
        .test(F1);
    step("Search")
        .test(F4, A, ENTER, N).editor("XAN1CA12BY M2BY");
    step("Search again")
        .test(F1, F4, B, Y, F4, ENTER, SHIFT, F1, Q).editor("XAN1CA12BY M2QBY");
    step("Replace")
        .test(SHIFT, F5, F1, F4, A, SHIFT, F4).editor("XBYN1CA12BY M2QBY");
    step("Second replace")
        .test(SHIFT, F4).editor("XBYN1CBY12BY M2QBY");
    step("Third replace")
        .test(SHIFT, F4).editor("XBYN1CBY12BY M2QBY");
    step("End of search, same editor")
        .test(ENTER).editor("XBYN1CBY12BY M2QBY");
    step("End of editing, empty editor")
        .test(ENTER).editor("");
    step("History")
        .test(RSHIFT, UP).editor("XBYN1CBY12BY M2QBY");
    step("History level 2")
        .test(RSHIFT, UP).editor("1 2 3 4");
    step("Exiting old history")
        .test(EXIT).editor("");
    step("Check 8-level history")
        .test("A", ENTER, "B", ENTER, "C", ENTER, "D", ENTER,
              "E", ENTER, "F", ENTER, "G", ENTER, "H", ENTER,
              RSHIFT, UP).editor("H")
        .test(RSHIFT, UP).editor("G")
        .test(RSHIFT, UP).editor("F")
        .test(RSHIFT, UP).editor("E")
        .test(RSHIFT, UP).editor("D")
        .test(RSHIFT, UP).editor("C")
        .test(RSHIFT, UP).editor("B")
        .test(RSHIFT, UP).editor("A")
        .test(RSHIFT, UP).editor("H");
    step("EXIT key still saves editor contents")
        .test(CLEAR, "ABCD").editor("ABCD")
        .test(EXIT).editor("").noerror()
        .test(RSHIFT, UP).editor("ABCD");
    step("End of editor")
        .test(CLEAR);
}


void tests::stack_operations()
// ----------------------------------------------------------------------------
//   Test stack operations
// ----------------------------------------------------------------------------
{
    BEGIN(stack);

    step("Multi-line stack rendering")
        .test(CLEAR, "[[1 2][3 4]]", ENTER)
        .noerror().expect("[[ 1 2 ]\n  [ 3 4 ]]")
        .test("SingleLineResult", ENTER)
        .noerror().expect("[[ 1 2 ][ 3 4 ]]")
        .test("MultiLineResult", ENTER)
        .noerror().expect("[[ 1 2 ]\n  [ 3 4 ]]");
    step("Multi-line stack rendering does not impact editing")
        .test(NOSHIFT, DOWN)
        .editor("[[ 1 2 ]\n  [ 3 4 ]]")
        .test(ENTER, "SingleLineResult", ENTER, DOWN)
        .editor("[[ 1 2 ]\n  [ 3 4 ]]")
        .test(ENTER, "MultiLineResult", ENTER, DOWN)
        .editor("[[ 1 2 ]\n  [ 3 4 ]]")
        .test(ENTER).noerror();

    step("Dup with ENTER")
        .test(CLEAR, "12", ENTER, ENTER, ADD).expect("24");
    step("Drop with Backspace")
        .test(CLEAR, "12 34", ENTER).noerror().expect("34")
        .test(BSP).noerror().expect("12")
        .test(BSP).noerror()
        .test(BSP).error("Too few arguments");

    step("Dup in program")
        .test(CLEAR, "13 Dup +", ENTER).expect("26");
    step("Dup2")
        .test(CLEAR, "13 25 Dup2 * + *", ENTER).expect("4 550");
    step("Over")
        .test(CLEAR, "13 25 Over / +", ENTER).expect("14 ¹²/₁₃");
    step("Rot")
        .test(CLEAR, "13 17 25 Rot / +", ENTER).expect("18 ¹²/₁₃");
    step("Over in stack menu")
        .test(CLEAR, I, "13 25", F2, DIV, ADD).expect("14 ¹²/₁₃");
    step("Rot in stack menu")
        .test(CLEAR, "13 17 25", F1, DIV, ADD).expect("18 ¹²/₁₃");
    step("Depth in stack menu")
        .test(CLEAR, "13 17 25", F3).expect("3");
    step("Pick in stack menu")
        .test(CLEAR, "13 17 25 2", F4).expect("17");
    step("Roll in stack menu")
        .test(CLEAR, "13 17 25 42 21 372 3", F5).expect("42")
        .test(BSP).expect("372")
        .test(BSP).expect("21")
        .test(BSP).expect("25")
        .test(BSP).expect("17")
        .test(BSP).expect("13")
        .test(BSP).noerror()
        .test(BSP).error("Too few arguments");
    step("RollDn in stack menu")
        .test(CLEAR, "13 17 25 42 21 372 4", F6).expect("21")
        .test(BSP).expect("42")
        .test(BSP).expect("25")
        .test(BSP).expect("372")
        .test(BSP).expect("17")
        .test(BSP).expect("13")
        .test(BSP).noerror()
        .test(BSP).error("Too few arguments");
    step("DropN in stack menu")
        .test(CLEAR, "13 17 25 42 21 372 4", SHIFT, F6).expect("17")
        .test(BSP).expect("13")
        .test(BSP).noerror()
        .test(BSP).error("Too few arguments");
    step("DupN in stack menu")
        .test(CLEAR, "13 17 25 42 21 372 4", SHIFT, F5).expect("372")
        .test(BSP).expect("21")
        .test(BSP).expect("42")
        .test(BSP).expect("25")
        .test(BSP).expect("372")
        .test(BSP).expect("21")
        .test(BSP).expect("42")
        .test(BSP).expect("25")
        .test(BSP).expect("17")
        .test(BSP).expect("13")
        .test(BSP).noerror()
        .test(BSP).error("Too few arguments");
    step("Drop2 in stack menu")
        .test(CLEAR, "13 17 25 42 21 372 4", SHIFT, F4).expect("21")
        .test(BSP).expect("42")
        .test(BSP).expect("25")
        .test(BSP).expect("17")
        .test(BSP).expect("13")
        .test(BSP).noerror()
        .test(BSP).error("Too few arguments");
    step("Dup2 in stack menu")
        .test(CLEAR, "13 17 25 42", SHIFT, F3).expect("42")
        .test(BSP).expect("25")
        .test(BSP).expect("42")
        .test(BSP).expect("25")
        .test(BSP).expect("17")
        .test(BSP).expect("13")
        .test(BSP).noerror()
        .test(BSP).error("Too few arguments");
    step("Simple stack commands from menu")
        .test(CLEAR, SHIFT, RUNSTOP,
              F1, F2, F3, F4, F5, F6,
              SHIFT, F1, SHIFT, F2, SHIFT, F3,
              SHIFT, F4, SHIFT, F5, SHIFT, F6,
              RSHIFT, F1, RSHIFT, F2, RSHIFT, F3,
              RSHIFT, F5, RSHIFT, F6,
              ENTER)
        .expect("« Rot Over Depth Pick Roll RollDown "
                "Duplicate Drop Duplicate2 Drop2 DuplicateN DropN "
                "Swap LastArguments Clear LastX »").test(BSP).noerror();

    step("LastArg")
        .test(CLEAR, "1 2").shifts(false, false, false, false)
        .test(ADD).expect("3")
        .test(SHIFT, M).expect("2")
        .test(BSP).expect("1")
        .test(BSP).expect("3")
        .test(BSP).noerror()
        .test(BSP).error("Too few arguments");
    step("Undo")
        .test(CLEAR, "1 2").shifts(false, false, false, false)
        .test(ADD).expect("3")
        .test(RSHIFT, M).expect("2")
        .test(BSP).expect("1")
        .test(BSP).noerror()
        .test(BSP).error("Too few arguments");
    step("LastX")
        .test(CLEAR, "1 2").shifts(false, false, false, false)
        .test(ADD).expect("3")
        .test(RSHIFT, F5).expect("2")
        .test(BSP).expect("3")
        .test(BSP).noerror()
        .test(BSP).error("Too few arguments");
    step("ClearStk")
        .test(CLEAR, "1 2 3 4", ENTER)
        .test(RSHIFT, F3).noerror()
        .test(BSP).error("Too few arguments");
}


void tests::arithmetic()
// ----------------------------------------------------------------------------
//   Tests for basic arithmetic operations
// ----------------------------------------------------------------------------
{
    BEGIN(arithmetic);

    step("Integer addition");
    test(CLEAR, 1, ENTER, 1, ADD).type(object::ID_integer).expect("2");
    test(1, ADD).type(object::ID_integer).expect("3");
    test(-1, ADD).type(object::ID_integer).expect("2");
    test(-1, ADD).type(object::ID_integer).expect("1");
    test(-1, ADD).type(object::ID_integer).expect("0");
    test(-1, ADD).type(object::ID_neg_integer).expect("-1");
    test(-1, ADD).type(object::ID_neg_integer).expect("-2");
    test(-1, ADD).type(object::ID_neg_integer).expect("-3");
    test(1, ADD).type(object::ID_neg_integer).expect("-2");
    test(1, ADD).type(object::ID_neg_integer).expect("-1");
    test(1, ADD).type(object::ID_integer).expect("0");

    step("Integer addition overflow");
    test(CLEAR, (1ULL << 63) - 2ULL, ENTER, 1, ADD)
        .type(object::ID_integer)
        .expect("9 223 372 036 854 775 807");
    test(CLEAR, (1ULL << 63) - 3ULL, CHS, ENTER, -2, ADD)
        .type(object::ID_neg_integer)
        .expect("-9 223 372 036 854 775 807");

    test(CLEAR, ~0ULL, ENTER, 1, ADD)
        .type(object::ID_bignum)
        .expect("18 446 744 073 709 551 616");
    test(CLEAR, ~0ULL, CHS, ENTER, -2, ADD)
        .type(object::ID_neg_bignum)
        .expect("-18 446 744 073 709 551 617");

    step("Adding ten small integers at random");
    srand48(sys_current_ms());
    Settings.MantissaSpacing(0);
    for (int i = 0; i < 10; i++)
    {
        large x = (lrand48() & 0xFFFFFF) - 0x800000;
        large y = (lrand48() & 0xFFFFFF) - 0x800000;
        test(CLEAR, x, ENTER, y, ADD)
            .explain("Computing ", x, " + ", y, ", ")
            .expect(x + y);
    }
    Settings.MantissaSpacing(3);

    step("Integer subtraction");
    test(CLEAR, 1, ENTER, 1, SUB).type(object::ID_integer).expect("0");
    test(1, SUB).type(object::ID_neg_integer).expect("-1");
    test(-1, SUB).type(object::ID_integer).expect("0");
    test(-1, SUB).type(object::ID_integer).expect("1");
    test(-1, SUB).type(object::ID_integer).expect("2");
    test(1, SUB).type(object::ID_integer).expect("1");
    test(1, SUB).type(object::ID_integer).expect("0");
    test(3, SUB).type(object::ID_neg_integer).expect("-3");
    test(-1, SUB).type(object::ID_neg_integer).expect("-2");
    test(1, SUB).type(object::ID_neg_integer).expect("-3");
    test(-3, SUB).type(object::ID_integer).expect("0");

    step("Integer subtraction overflow");
    test(CLEAR, 0xFFFFFFFFFFFFFFFFull, CHS, ENTER, 1, SUB)
        .type(object::ID_neg_bignum)
        .expect("-18 446 744 073 709 551 616");
    test(CLEAR, -3, ENTER, 0xFFFFFFFFFFFFFFFFull, SUB)
        .type(object::ID_neg_bignum)
        .expect("-18 446 744 073 709 551 618");

    step("Subtracting ten small integers at random");
    Settings.MantissaSpacing(0);
    for (int i = 0; i < 10; i++)
    {
        large x = (lrand48() & 0xFFFFFF) - 0x800000;
        large y = (lrand48() & 0xFFFFFF) - 0x800000;
        test(CLEAR, x, ENTER, y, SUB)
            .explain("Computing ", x, " - ", y, ", ")
            .expect(x - y);
    }
    Settings.MantissaSpacing(3);

    step("Integer multiplication");
    test(CLEAR, 3, ENTER, 7, MUL).type(object::ID_integer).expect("21");
    test(3, MUL).type(object::ID_integer).expect("63");
    test(-3, MUL).type(object::ID_neg_integer).expect("-189");
    test(2, MUL).type(object::ID_neg_integer).expect("-378");
    test(-7, MUL).type(object::ID_integer).expect("2 646");

    step("Multiplying ten small integers at random");
    Settings.MantissaSpacing(0);
    for (int i = 0; i < 10; i++)
    {
        large x = (lrand48() & 0xFFFFFF) - 0x800000;
        large y = (lrand48() & 0xFFFFFF) - 0x800000;
        test(CLEAR, x, ENTER, y, MUL)
            .explain("Computing ", x, " * ", y, ", ")
            .expect(x * y);
    }
    Settings.MantissaSpacing(3);

    step("Integer division");
    test(CLEAR, 210, ENTER, 2, DIV).type(object::ID_integer).expect("105");
    test(5, DIV).type(object::ID_integer).expect("21");
    test(-3, DIV).type(object::ID_neg_integer).expect("-7");
    test(-7, DIV).type(object::ID_integer).expect("1");

    step("Dividing ten small integers at random");
    Settings.MantissaSpacing(0);
    for (int i = 0; i < 10; i++)
    {
        large x = (lrand48() & 0x3FFF) - 0x4000;
        large y = (lrand48() & 0x3FFF) - 0x4000;
        test(CLEAR, x * y, ENTER, y, DIV)
            .explain("Computing ", x * y, " / ", y, ", ")
            .expect(x);
    }
    Settings.MantissaSpacing(3);

    step("Division with fractional output");
    test(CLEAR, 1, ENTER, 3, DIV).expect("¹/₃");
    test(CLEAR, 2, ENTER, 5, DIV).expect("²/₅");

    step("Manual computation of 100!");
    test(CLEAR, 1, ENTER);
    for (uint i = 1; i <= 100; i++)
        test(i, MUL, NOKEYS, WAIT(20));
    expect( "93 326 215 443 944 152 681 699 238 856 266 700 490 715 968 264 "
            "381 621 468 592 963 895 217 599 993 229 915 608 941 463 976 156 "
            "518 286 253 697 920 827 223 758 251 185 210 916 864 000 000 000 "
            "000 000 000 000 000");
    step("Manual division by all factors of 100!");
    for (uint i = 1; i <= 100; i++)
        test(i * 997 % 101, DIV, NOKEYS, WAIT(20));
    expect(1);

    step("Manual computation of 997/100!");
    test(CLEAR, 997, ENTER);
    for (uint i = 1; i <= 100; i++)
        test(i * 997 % 101, DIV, NOKEYS, WAIT(20));
    expect("⁹⁹⁷/"
           "₉₃ ₃₂₆ ₂₁₅ ₄₄₃ ₉₄₄ ₁₅₂ ₆₈₁ ₆₉₉ ₂₃₈ ₈₅₆ ₂₆₆ ₇₀₀ ₄₉₀ ₇₁₅ ₉₆₈ "
           "₂₆₄ ₃₈₁ ₆₂₁ ₄₆₈ ₅₉₂ ₉₆₃ ₈₉₅ ₂₁₇ ₅₉₉ ₉₉₃ ₂₂₉ ₉₁₅ ₆₀₈ ₉₄₁ ₄₆₃ "
           "₉₇₆ ₁₅₆ ₅₁₈ ₂₈₆ ₂₅₃ ₆₉₇ ₉₂₀ ₈₂₇ ₂₂₃ ₇₅₈ ₂₅₁ ₁₈₅ ₂₁₀ ₉₁₆ ₈₆₄ "
           "₀₀₀ ₀₀₀ ₀₀₀ ₀₀₀ ₀₀₀ ₀₀₀ ₀₀₀ ₀₀₀");

    step("Computation of 2^256 (bug #460)")
        .test(CLEAR, 2, ENTER, 256, SHIFT, B)
        .expect("115 792 089 237 316 195 423 570 985 008 687 907 853 269 984 "
                "665 640 564 039 457 584 007 913 129 639 936");
    step("Sign of modulo and remainder");
    test(CLEAR, " 7  3 MOD", ENTER).expect(1);
    test(CLEAR, " 7 -3 MOD", ENTER).expect(1);
    test(CLEAR, "-7  3 MOD", ENTER).expect(2);
    test(CLEAR, "-7 -3 MOD", ENTER).expect(2);
    test(CLEAR, " 7  3 REM", ENTER).expect(1);
    test(CLEAR, " 7 -3 REM", ENTER).expect(1);
    test(CLEAR, "-7  3 REM", ENTER).expect(-1);
    test(CLEAR, "-7 -3 REM", ENTER).expect(-1);

    step("Fraction modulo and remainder");
    test(CLEAR, " 7/2  3 REM", ENTER).expect("¹/₂");
    test(CLEAR, " 7/2 -3 REM", ENTER).expect("¹/₂");
    test(CLEAR, "-7/2  3 REM", ENTER).expect("-¹/₂");
    test(CLEAR, "-7/2 -3 REM", ENTER).expect("-¹/₂");
    test(CLEAR, " 7/2  3 REM", ENTER).expect("¹/₂");
    test(CLEAR, " 7/2 -3 REM", ENTER).expect("¹/₂");
    test(CLEAR, "-7/2  3 REM", ENTER).expect("-¹/₂");
    test(CLEAR, "-7/2 -3 REM", ENTER).expect("-¹/₂");

    step("Modulo of negative value");
    test(CLEAR, "-360 360 MOD", ENTER).expect("0");
    test(CLEAR, "1/3 -1/3 MOD", ENTER).expect("0");
    test(CLEAR, "360 -360 MOD", ENTER).expect("0");
    test(CLEAR, "-1/3 1/3 MOD", ENTER).expect("0");

    step("Power");
    test(CLEAR, "2 3 ^", ENTER).expect("8");
    test(CLEAR, "-2 3 ^", ENTER).expect("-8");
    step("Negative power");
    test(CLEAR, "2 -3 ^", ENTER).expect("¹/₈");
    test(CLEAR, "-2 -3 ^", ENTER).expect("-¹/₈");

    step("Special case of 0^0")
        .test(CLEAR, "0 0 ^", ENTER).noerror().expect("1")
        .test(CLEAR,
              "ZeroPowerZeroIsUndefined", ENTER,
              "0 0 ^", ENTER).error("Undefined operation")
        .test(CLEAR,
              "ZeroPowerZeroIsOne", ENTER,
              "0 0 ^", ENTER).noerror().expect("1");

    step("xroot");
    test(CLEAR, "8 3 xroot", ENTER).expect("2.");
    test(CLEAR, "-8 3 xroot", ENTER).expect("-2.");
}


void tests::global_variables()
// ----------------------------------------------------------------------------
//   Tests for access to global variables
// ----------------------------------------------------------------------------
{
    BEGIN(globals);

    step("Store in global variable");
    test(CLEAR, 12345, ENTER).expect("12 345");
    test(XEQ, "A", ENTER).expect("'A'");
    test(STO).noerror();
    step("Recall global variable");
    test(CLEAR, 1, ENTER, XEQ, "A", ENTER).expect("'A'");
    test("RCL", ENTER).noerror().expect("12 345");

    step("Store in long-name global variable");
    test(CLEAR, "\"Hello World\"", ENTER, XEQ, "SomeLongVariable", ENTER, STO)
        .noerror();
    step("Recall global variable");
    test(CLEAR, XEQ, "SomeLongVariable", ENTER, "recall", ENTER)
        .noerror()
        .expect("\"Hello World\"");

    step("Recall non-existent variable");
    test(CLEAR, XEQ, "DOESNOTEXIST", ENTER, "RCL", ENTER)
        .error("Undefined name")
        .clear();

    step("Store and recall invalid variable object");
    test(CLEAR, 5678, ENTER, 1234, ENTER,
         "STO", ENTER).error("Invalid name").clear();
    test(CLEAR, 1234, ENTER,
         "RCL", ENTER).error("Invalid name").clear();

    step("Store and recall to EQ");
    test(CLEAR, "'X+Y' 'eq' STO", ENTER).noerror();
    test(CLEAR, "'EQ' RCL", ENTER).expect("'X+Y'");
    test(CLEAR, "'equation' RCL", ENTER).expect("'X+Y'");
    test(CLEAR, "'Equation' PURGE", ENTER).noerror();

    step("Store and recall to ΣData");
    test(CLEAR, "[1 2 3] 'ΣData' STO", ENTER).noerror();
    test(CLEAR, "'ΣDat' RCL", ENTER).expect("[ 1 2 3 ]");
    test(CLEAR, "'StatsData' RCL", ENTER).expect("[ 1 2 3 ]");
    test(CLEAR, "'ΣData' PURGE", ENTER).noerror();

    step("Store and recall to StatsParameters");
    test(CLEAR, "{0} 'ΣParameters' STO", ENTER).noerror();
    test(CLEAR, "'ΣPar' RCL", ENTER).expect("{ 0 }");
    test(CLEAR, "'StatsParameters' RCL", ENTER).expect("{ 0 }");
    test(CLEAR, "'ΣPar' purge", ENTER).noerror();

    step("Store and recall to PlotParameters");
    test(CLEAR, "{1} 'PPAR' STO", ENTER).noerror();
    test(CLEAR, "'PlotParameters' RCL", ENTER).expect("{ 1 }");
    test(CLEAR, "'ppar' RCL", ENTER).expect("{ 1 }");
    test(CLEAR, "'PPAR' purge", ENTER).noerror();

    step("Numbered store and recall should fail by default");
    test(CLEAR, 5678, ENTER, 1234, ENTER, "STO", ENTER).error("Invalid name");
    test(CLEAR, 1234, ENTER, "RCL", ENTER).error("Invalid name");
    test(CLEAR, 1234, ENTER, "Purge", ENTER).error("Invalid name");

    step("Enable NumberedVariables");
    test(CLEAR, "NumberedVariables", ENTER).noerror();
    test(CLEAR, 5678, ENTER, 1234, ENTER, "STO", ENTER).noerror();
    test(CLEAR, 1234, ENTER, "RCL", ENTER).noerror().expect("5 678");
    test(CLEAR, 1234, ENTER, "Purge", ENTER).noerror();

    step("Disable NumberedVariables");
    test(CLEAR, "NoNumberedVariables", ENTER).noerror();
    test(CLEAR, 5678, ENTER, 1234, ENTER, "STO", ENTER).error("Invalid name");
    test(CLEAR, 1234, ENTER, "RCL", ENTER).error("Invalid name");
    test(CLEAR, 1234, ENTER, "Purge", ENTER).error("Invalid name");

    step("Store program in global variable");
    test(CLEAR, "« 1 + »", ENTER, XEQ, "INCR", ENTER, STO).noerror();
    step("Evaluate global variable");
    test(CLEAR, "A INCR", ENTER).expect("12 346");

    step("Purge global variable");
    test(CLEAR, XEQ, "A", ENTER, "PURGE", ENTER).noerror();
    test(CLEAR, XEQ, "INCR", ENTER, "PURGE", ENTER).noerror();
    test(CLEAR, XEQ, "SomeLongVariable", ENTER, "PURGE", ENTER).noerror();

    test(CLEAR, XEQ, "A", ENTER, "RCL", ENTER).error("Undefined name").clear();
    test(CLEAR, XEQ, "INCR", ENTER, "RCL", ENTER)
        .error("Undefined name")
        .clear();
    test(CLEAR, XEQ, "SomeLongVariable", ENTER, "RCL", ENTER)
        .error("Undefined name")
        .clear();

    step("Go to top-level")
        .test(CLEAR, "Home", ENTER).noerror();
    step("Clear 'DirTest'")
        .test(CLEAR, "'DirTest' pgdir", ENTER);
    step("Create directory")
        .test(CLEAR, "'DirTest' crdir", ENTER).noerror();
    step("Enter directory")
        .test(CLEAR, "DirTest", ENTER).noerror();
    step("Path function")
        .test(CLEAR, "PATH", ENTER).expect("{ HomeDirectory DirTest }");
    step("Updir function")
        .test(CLEAR, "UpDir path", ENTER).expect("{ HomeDirectory }");
    step("Enter directory again")
        .test(CLEAR, "DirTest path", ENTER).expect("{ HomeDirectory DirTest }");
    step("Current directory content")
        .test(CLEAR, "CurrentDirectory", ENTER).want("Directory { }");
    step("Store in subdirectory")
        .test(CLEAR, "242 'Foo' STO", ENTER).noerror();
    step("Recall from subdirectory")
        .test(CLEAR, "Foo", ENTER).expect("242");
    step("Recursive directory")
        .test(CLEAR, "'DirTest2' crdir", ENTER).noerror();
    step("Entering sub-subdirectory")
        .test(CLEAR, "DirTest2", ENTER).noerror();
    step("Path in sub-subdirectory")
        .test(CLEAR, "path", ENTER).expect("{ HomeDirectory DirTest DirTest2 }");
    step("Find variable from level above")
        .test(CLEAR, "Foo", ENTER).expect("242");
    step("Create local variable")
        .test(CLEAR, "\"Hello\" 'Foo' sto", ENTER).noerror();
    step("Local variable hides variable above")
        .test(CLEAR, "Foo", ENTER).expect("\"Hello\"");
    step("Updir shows shadowed variable again")
        .test(CLEAR, "Updir Foo", ENTER).expect("242");
    step("Two independent variables with the same name")
        .test(CLEAR, "DirTest2 Foo", ENTER).expect("\"Hello\"");
    step("Cleanup")
        .test(CLEAR, "'Foo' Purge", ENTER).noerror();

    step("Save to file as text")
        .test(CLEAR, "1.42 \"Hello.txt\"", NOSHIFT, G).noerror();
    step("Restore from file as text")
        .test(CLEAR, "\"Hello.txt\" RCL", ENTER).noerror().expect("\"1.42\"");
    step("Save to file as source")
        .test(CLEAR, "1.42 \"Hello.48s\"", NOSHIFT, G).noerror();
    step("Restore from file as source")
        .test(CLEAR, "\"Hello.48s\" RCL", ENTER).noerror().expect("1.42");
    step("Save to file as binary")
        .test(CLEAR, "1.42 \"Hello.48b\"", NOSHIFT, G).noerror();
    step("Restore from file as text")
        .test(CLEAR, "\"Hello.48b\" RCL", ENTER).noerror().expect("1.42");
}


void tests::local_variables()
// ----------------------------------------------------------------------------
//   Tests for access to local variables
// ----------------------------------------------------------------------------
{
    BEGIN(locals);

    step("Creating a local block");
    cstring source = "« → A B C « A B + A B - × B C + B C - × ÷ » »";
    test(CLEAR, source, ENTER).type(object::ID_program).want(source);
    test(XEQ, "LocTest", ENTER, STO).noerror();

    step("Calling a local block with numerical values");
    test(CLEAR, 1, ENTER, 2, ENTER, 3, ENTER, "LocTest", ENTER).expect("³/₅");

    step("Calling a local block with symbolic values");
    test(CLEAR,
         XEQ, "X", ENTER,
         XEQ, "Y", ENTER,
         XEQ, "Z", ENTER,
         "LocTest", ENTER)
        .expect("'(X+Y)·(X-Y)÷((Y+Z)·(Y-Z))'");

    step("Cleanup");
    test(CLEAR, XEQ, "LocTest", ENTER, "PurgeAll", ENTER).noerror();
}


void tests::for_loops()
// ----------------------------------------------------------------------------
//   Test simple for loops
// ----------------------------------------------------------------------------
{
    BEGIN(for_loops);

    step("Simple 1..10");
    cstring pgm  = "« 0 1 10 FOR i i SQ + NEXT »";
    cstring pgmo = "« 0 1 10 for i i x² + next »";
    test(CLEAR, pgm, ENTER).noerror().type(object::ID_program).want(pgmo);
    test(RUNSTOP).noerror().type(object::ID_integer).expect(385);

    step("Algebraic 1..10");
    pgm  = "« 'X' 1 5 FOR i i SQ + NEXT »";
    pgmo = "« 'X' 1 5 for i i x² + next »";
    test(CLEAR, pgm, ENTER).noerror().type(object::ID_program).want(pgmo);
    test(RUNSTOP).noerror().type(object::ID_expression).expect("'X+1+4+9+16+25'");

    step("Stepping by 2");
    pgm  = "« 0 1 10 FOR i i SQ + 2 STEP »";
    pgmo = "« 0 1 10 for i i x² + 2 step »";
    test(CLEAR, pgm, ENTER).noerror().type(object::ID_program).want(pgmo);
    test(RUNSTOP).noerror().type(object::ID_integer).expect(165);

    step("Stepping by i");
    pgm  = "« 'X' 1 100 FOR i i SQ + i step »";
    pgmo = "« 'X' 1 100 for i i x² + i step »";
    test(CLEAR, pgm, ENTER).noerror().type(object::ID_program).want(pgmo);
    test(RUNSTOP)
        .noerror()
        .type(object::ID_expression)
        .expect("'X+1+4+16+64+256+1 024+4 096'");

    step("Negative stepping");
    pgm  = "« 0 10 1 FOR i i SQ + -1 STEP »";
    pgmo = "« 0 10 1 for i i x² + -1 step »";
    test(CLEAR, pgm, ENTER).noerror().type(object::ID_program).want(pgmo);
    test(RUNSTOP).noerror().type(object::ID_integer).expect(385);

    step("Negative stepping algebraic");
    pgm  = "« 'X' 10 1 FOR i i SQ + -1 step »";
    pgmo = "« 'X' 10 1 for i i x² + -1 step »";
    test(CLEAR, pgm, ENTER).noerror().type(object::ID_program).want(pgmo);
    test(RUNSTOP)
        .noerror()
        .type(object::ID_expression)
        .expect("'X+100+81+64+49+36+25+16+9+4+1'");

    step("Fractional");
    pgm  = "« 'X' 0.1 0.9 FOR i i SQ + 0.1 step »";
    pgmo = "« 'X' 0.1 0.9 for i i x² + 0.1 step »";
    test(CLEAR, pgm, ENTER).noerror().type(object::ID_program).want(pgmo);
    test(RUNSTOP)
        .noerror()
        .type(object::ID_expression)
        .expect("'X+0.01+0.04+0.09+0.16+0.25+0.36+0.49+0.64+0.81'");

    step("Fractional down");
    pgm  = "« 'X' 0.9 0.1 FOR i i SQ + -0.1 step »";
    pgmo = "« 'X' 0.9 0.1 for i i x² + -0.1 step »";
    test(CLEAR, pgm, ENTER).noerror().type(object::ID_program).want(pgmo);
    test(RUNSTOP)
        .noerror()
        .type(object::ID_expression)
        .expect("'X+0.81+0.64+0.49+0.36+0.25+0.16+0.09+0.04+0.01'");

    step("Execute at least once");
    pgm  = "« 'X' 10 1 FOR i i SQ + NEXT »";
    pgmo = "« 'X' 10 1 for i i x² + next »";
    test(CLEAR, pgm, ENTER).noerror().type(object::ID_program).want(pgmo);
    test(RUNSTOP).noerror().type(object::ID_expression).expect("'X+100'");
}


void tests::conditionals()
// ----------------------------------------------------------------------------
//   Test conditionals
// ----------------------------------------------------------------------------
{
    BEGIN(conditionals);

    step("If-Then (true)");
    test(CLEAR, "PASS if 0 0 > then FAIL end", ENTER)
        .expect("'PASS'");
    step("If-Then (false)");
    test(CLEAR, "FAIL if 1 0 > then PASS end", ENTER)
        .expect("'PASS'");
    step("If-Then-Else (true)");
    test(CLEAR, "if 1 0 > then PASS else FAIL end", ENTER)
        .expect("'PASS'");
    step("If-Then-Else (false)");
    test(CLEAR, "if 1 0 = then FAIL else PASS end", ENTER)
        .expect("'PASS'");

    step("IFT command (true)");
    test(CLEAR, "FAIL true PASS IFT", ENTER)
        .expect("'PASS'");
    step("IFT command (false)");
    test(CLEAR, "PASS 0 FAIL IFT", ENTER)
        .expect("'PASS'");
    step("IFTE command (true)");
    test(CLEAR, "true PASS FAIL IFTE", ENTER)
        .expect("'PASS'");
    step("IFTE command (false)");
    test(CLEAR, "0 FAIL PASS IFTE", ENTER)
        .expect("'PASS'");

    step("IfErr-Then (true)");
    test(CLEAR, "FAIL iferr 1 0 / drop then PASS end", ENTER)
        .expect("'PASS'");
    step("IfErr-Then (false)");
    test(CLEAR, "PASS iferr 1 0 + drop then FAIL end", ENTER)
        .expect("'PASS'");
    step("IfErr-Then-Else (true)");
    test(CLEAR, "iferr 1 0 / drop then PASS ELSE FAIL end", ENTER)
        .expect("'PASS'");
    step("IfErr-Then-Else (false)");
    test(CLEAR, "IFERR 1 0 + drop then FAIL ELSE PASS end", ENTER)
        .expect("'PASS'");

    step("IfErr reading error message");
    test(CLEAR, "iferr 1 0 / drop then errm end", ENTER)
        .expect("\"Divide by zero\"");
    step("IfErr reading error number");
    test(CLEAR, "iferr 1 0 / drop then errn end", ENTER)
        .type(object::ID_based_integer)
        .expect("#A₁₆");        // May change if you update errors.tbl

    step("DoErr with built-in message");
    test(CLEAR, "3 DoErr", ENTER)
        .error("Too few arguments");
    step("DoErr with custom message");
    test(CLEAR, "\"You lose!\" doerr \"You lose worse!\"", ENTER)
        .error("You lose!");
    step("errm for custom error message");
    test(BSP, "errm", ENTER)
        .expect("\"You lose!\"");
    step("errn for custom error message");
    test("errn", ENTER)
        .expect("#7 0000₁₆");

    step("Getting message after iferr");
    test(CLEAR, "« FAILA iferr 1 0 / then FAILB end errm »",
         ENTER, RUNSTOP)
        .expect("\"Divide by zero\"");

    step("err0 clearing message");
    test(CLEAR, "« FAILA iferr 1 0 / then FAILB end err0 errm errn »",
         ENTER, RUNSTOP)
        .expect("#0₁₆")
        .test(BSP)
        .expect("\"\"");
}


void tests::logical_operations()
// ----------------------------------------------------------------------------
//   Perform logical operations on small and big integers
// ----------------------------------------------------------------------------
{
    BEGIN(logical);

#if CONFIG_FIXED_BASED_OBJECTS
    step("Binary number");
    cstring binary  = "#10001b";
    cstring binaryf = "#1 0001₂";
    test(CLEAR, binary, ENTER).type(object::ID_bin_integer).expect(binaryf);

    step("Octal number");
    cstring octal  = "#1777o";
    cstring octalf = "#1777₈";
    test(CLEAR, octal, ENTER).type(object::ID_oct_integer).expect(octalf);

    step("Decimal number");
    cstring decimal  = "#12345d";
    cstring decimalf = "#1 2345₁₀";
    test(CLEAR, decimal, ENTER).type(object::ID_dec_integer).expect(decimalf);

    step("Hexadecimal number");
    cstring hexa  = "#135AFh";
    cstring hexaf = "#1 35AF₁₆";
    test(CLEAR, hexa, ENTER).type(object::ID_hex_integer).expect(hexaf);
#endif // CONFIG_FIXED_BASED_OBJECTS

    step("Based number (default base)");
    cstring based  = "#1234A";
    cstring basedf = "#1 234A₁₆";
    test(CLEAR, based, ENTER).type(object::ID_based_integer).expect(basedf);

    step("Based number (arbitrary base)");
    cstring abased  = "17#1234AG";
    cstring abasedf = "#18 75A4₁₆";
    test(CLEAR, abased, ENTER).type(object::ID_based_integer).expect(abasedf);

    step("Display in arbitrary base");
    test("17 base", ENTER).expect("#12 34AG₁₇");
    test("3 base", ENTER).expect("#10 0001 0221 2122₃");
    test("36 base", ENTER).expect("#YCV8₃₆");
    test("16 base", ENTER).expect("#18 75A4₁₆");

    step("Range for bases");
    test("1 base", ENTER).error("Argument outside domain");
    test(CLEAR, "37 base", ENTER).error("Argument outside domain");
    test(CLEAR, "0.3 base", ENTER).error("Argument outside domain");
    test(CLEAR);

    step("Default word size");
    test("RCWS", ENTER).expect("64");
    step("Set word size to 16");
    test(CLEAR, "16 STWS", ENTER).noerror();

    step("Binary not");
    test(CLEAR, "#12 not", ENTER).expect("#FFED₁₆");
    test("not", ENTER).expect("#12₁₆");

    step("Binary or");
    test(CLEAR, "#123 #A23 or", ENTER).expect("#B23₁₆");

    step("Binary xor");
    test(CLEAR, "#12 #A23 xor", ENTER).expect("#A31₁₆");

    step("Binary and");
    test(CLEAR, "#72 #A23 and", ENTER).expect("#22₁₆");

    step("Binary nand");
    test(CLEAR, "#72 #A23 nand", ENTER).expect("#FFDD₁₆");

    step("Binary nor");
    test(CLEAR, "#72 #A23 nor", ENTER).expect("#F58C₁₆");

    step("Binary implies");
    test(CLEAR, "#72 #A23 implies", ENTER).expect("#FFAF₁₆");

    step("Binary excludes");
    test(CLEAR, "#72 #A23 excludes", ENTER).expect("#50₁₆");

    step("Set word size to 32");
    test(CLEAR, "32 STWS", ENTER).noerror();
    test(CLEAR, "#12 not", ENTER).expect("#FFFF FFED₁₆");
    test("not", ENTER).expect("#12₁₆");

    step("Set word size to 30");
    test(CLEAR, "30 STWS", ENTER).noerror();
    test(CLEAR, "#142 not", ENTER).expect("#3FFF FEBD₁₆");
    test("not", ENTER).expect("#142₁₆");

    step("Set word size to 48");
    test(CLEAR, "48 STWS", ENTER).noerror();
    test(CLEAR, "#233 not", ENTER).expect("#FFFF FFFF FDCC₁₆");
    test("not", ENTER).expect("#233₁₆");

    step("Set word size to 64");
    test(CLEAR, "64 STWS", ENTER).noerror();
    test(CLEAR, "#64123 not", ENTER).expect("#FFFF FFFF FFF9 BEDC₁₆");
    test("not", ENTER).expect("#6 4123₁₆");

    step("Set word size to 128");
    test(CLEAR, "128 STWS", ENTER).noerror();
    test(CLEAR, "#12 not", ENTER)
        .expect("#FFFF FFFF FFFF FFFF FFFF FFFF FFFF FFED₁₆");
    test("dup not", ENTER).expect("#12₁₆");
    test("xor not", ENTER).expect("#0₁₆");

    step("Set word size to 623");
    test(CLEAR, "623 STWS", ENTER).noerror();
    test(CLEAR, "#12 not", ENTER)
        .expect("#7FFF FFFF FFFF FFFF FFFF FFFF FFFF FFFF FFFF FFFF FFFF FFFF "
                "FFFF FFFF FFFF FFFF FFFF FFFF FFFF FFFF FFFF FFFF FFFF FFFF "
                "FFFF FFFF FFFF FFFF FFFF FFFF FFFF FFFF FFFF FFFF FFFF FFFF "
                "FFFF FFFF FFED₁₆");
    test("dup not", ENTER).expect("#12₁₆");
    test("xor not", ENTER).expect("#0₁₆");

    step("Check that arithmetic truncates to small word size (#624)")
        .test("15 STWS", ENTER).noerror()
        .test("#0 #4 -", ENTER).expect("#7FFC₁₆")
        .test("#321 *", ENTER).expect("#737C₁₆")
        .test("#27 /", ENTER).expect("#2F6₁₆")
        .test("13 STWS", ENTER).noerror()
        .test("#0 #6 -", ENTER).expect("#1FFA₁₆")
        .test("#321 *", ENTER).expect("#D3A₁₆")
        .test("#27 /", ENTER).expect("#56₁₆");

    step("Reset word size to default")
        .test(CLEAR, "64 WordSize", ENTER).noerror();
}


void tests::command_display_formats()
// ----------------------------------------------------------------------------
//   Check the various display formats for commands
// ----------------------------------------------------------------------------
{
    BEGIN(styles);

    step("Commands");
    // There is a trap in this command line
    cstring prgm =
        "«"
        "  1 1.0"
        "+ - * / ^ "
        "sin cos tan asin acos atan "
        "LowerCase PurgeAll Precision "
        "start step next start step for i next for i step "
        "while repeat end do until end » ";

    test(CLEAR, prgm, ENTER).noerror();
    step("Lower case");
    test("lowercase", ENTER)
        .want("« 1 1. + - * / ^ sin cos tan asin acos atan "
              "lowercase purgeall precision "
              "start  step next start  step for i  next for i  step "
              "while  repeat  end do  until  end »");

    step("Upper case");
    test("UPPERCASE", ENTER)
        .want("« 1 1. + - * / ^ SIN COS TAN ASIN ACOS ATAN "
              "LOWERCASE PURGEALL PRECISION "
              "START  STEP next START  STEP FOR i  NEXT FOR i  STEP "
              "WHILE  REPEAT  END DO  UNTIL  END »");

    step("Capitalized");
    test("Capitalized", ENTER)
        .want("« 1 1. + - * / ^ Sin Cos Tan Asin Acos Atan "
              "LowerCase PurgeAll Precision "
              "Start  Step next Start  Step For i  Next For i  Step "
              "While  Repeat  End Do  Until  End »");

    step("Long form");
    test("LongForm", ENTER)
        .want("« 1 1. + - × ÷ ↑ sin cos tan sin⁻¹ cos⁻¹ tan⁻¹ "
              "LowerCaseCommands PurgeAll Precision "
              "start  step next start  step for i  next for i  step "
              "while  repeat  end do  until  end »");
}


void tests::integer_display_formats()
// ----------------------------------------------------------------------------
//   Check the various display formats for integer values
// ----------------------------------------------------------------------------
{
    BEGIN(iformat);

    step("Reset settings to defaults");
    test(CLEAR)
        .test("3 MantissaSpacing", ENTER)       .noerror()
        .test("5 FractionSpacing", ENTER)       .noerror()
        .test("4 BasedSpacing", ENTER)          .noerror()
        .test("NumberSpaces", ENTER)            .noerror()
        .test("BasedSpaces", ENTER)             .noerror();

    step("Default integer rendering");
    test(CLEAR, 1, ENTER)
        .type(object::ID_integer)
        .expect("1");
    test(CLEAR, 12, ENTER)
        .type(object::ID_integer)
        .expect("12");
    test(CLEAR, 123, ENTER)
        .type(object::ID_integer)
        .expect("123");
    test(CLEAR, 1234, ENTER)
        .type(object::ID_integer)
        .expect("1 234");
    test(CLEAR, 12345, ENTER)
        .type(object::ID_integer)
        .expect("12 345");
    test(CLEAR, 123456789, ENTER)
        .type(object::ID_integer)
        .expect("123 456 789");

    step("No spacing");
    test("0 MantissaSpacing", ENTER)
        .expect("123456789");

    step("Four spacing");
    test("4 MantissaSpacing", ENTER)
        .expect("1 2345 6789");

    step("Five spacing");
    test("5 MantissaSpacing", ENTER)
        .expect("1234 56789");

    step("Three spacing");
    test("3 MantissaSpacing 5 FractionSpacing", ENTER)
        .expect("123 456 789");

    step("Comma spacing");
    test("NumberDotOrComma", ENTER)
        .expect("123,456,789");

    step("Dot spacing");
    test("DecimalComma", ENTER)
        .expect("123.456.789");

    step("Ticks spacing");
    test("DecimalDot", ENTER)
        .expect("123,456,789");
    test("NumberTicks", ENTER)
        .expect("123’456’789");

    step("Underscore spacing");
    test("NumberUnderscore", ENTER)
        .expect("123_456_789");

    step("Space spacing");
    test("NumberSpaces", ENTER)
        .expect("123 456 789");

    step("Big integer rendering");
    test(CLEAR, "123456789012345678901234567890", ENTER)
        .type(object::ID_bignum)
        .expect("123 456 789 012 345 678 901 234 567 890");

    step("Entering numbers with spacing");
    test(CLEAR, "FancyExponent", ENTER).noerror();

    test(CLEAR, "1").editor("1");
    test(CHS).editor("-1");
    test(CHS).editor("1");
    test("2").editor("12");
    test("3").editor("123");
    test("4").editor("1 234");
    test("5").editor("12 345");
    test(CHS).editor("-12 345");
    test(EEX).editor("-12 345⁳");
    test("34").editor("-12 345⁳34");
    test(CHS).editor("-12 345⁳-34");
    test(" ").editor("-12 345⁳-34 ");
    test("12345.45678901234").editor("-12 345⁳-34 12 345.45678 90123 4");
    test(ENTER).noerror();

    step("Based number rendering");
    test(CLEAR, "#1234ABCDEFh", ENTER)
#if CONFIG_FIXED_BASED_OBJECTS
        .type(object::ID_hex_integer)
#endif // CONFIG_FIXED_BASED_OBJECTS
        .expect("#12 34AB CDEF₁₆");

    step("Two spacing");
    test("2 BasedSpacing", ENTER)
        .expect("#12 34 AB CD EF₁₆");

    step("Three spacing");
    test("3 BasedSpacing", ENTER)
        .expect("#1 234 ABC DEF₁₆");

    step("Four spacing");
    test("4 BasedSpacing", ENTER)
        .expect("#12 34AB CDEF₁₆");

    step("Comma spacing");
    test("BasedDotOrComma", ENTER)
        .expect("#12,34AB,CDEF₁₆");

    step("Dot spacing");
    test("DecimalComma", ENTER)
        .expect("#12.34AB.CDEF₁₆");

    step("Ticks spacing");
    test("DecimalDot", ENTER)
        .expect("#12,34AB,CDEF₁₆");
    test("BasedTicks", ENTER)
        .expect("#12’34AB’CDEF₁₆");

    step("Underscore spacing");
    test("BasedUnderscore", ENTER)
        .expect("#12_34AB_CDEF₁₆");

    step("Space spacing");
    test("BasedSpaces", ENTER)
        .expect("#12 34AB CDEF₁₆");
}


void tests::fraction_display_formats()
// ----------------------------------------------------------------------------
//   Check the various display formats for fraction values
// ----------------------------------------------------------------------------
{
    BEGIN(fformat);

    step("Default format for small fractions (1/3)")
        .test(CLEAR, 1, ENTER, 3, DIV)
        .type(object::ID_fraction).expect("¹/₃");
    step("Big fraction format")
        .test("BigFractions", ENTER).expect("1/3");
    step("Mixed big fraction")
        .test("MixedFractions", ENTER).expect("1/3");
    step("Small fractions")
        .test("SmallFractions", ENTER).expect("¹/₃");
    step("Improper fractions")
        .test("ImproperFractions", ENTER).expect("¹/₃");

    step("Default format for medium fractions (355/113)")
        .test(CLEAR, 355, ENTER, 113, DIV)
        .type(object::ID_fraction).expect("³⁵⁵/₁₁₃");
    step("Big fraction format")
        .test("BigFractions", ENTER).expect("355/113");
    step("Mixed big fraction")
        .test("MixedFractions", ENTER).expect("3 16/113");
    step("Small fractions")
        .test("SmallFractions", ENTER).expect("3 ¹⁶/₁₁₃");
    step("Improper fractions")
        .test("ImproperFractions", ENTER).expect("³⁵⁵/₁₁₃");

    step("Default format for large fractions (1000000000/99999999)")
        .test(CLEAR, 1000000000, ENTER, 99999999, DIV)
        .type(object::ID_fraction).expect("¹ ⁰⁰⁰ ⁰⁰⁰ ⁰⁰⁰/₉₉ ₉₉₉ ₉₉₉");
    step("Big fraction format")
        .test("BigFractions", ENTER).expect("1 000 000 000/99 999 999");
    step("Mixed big fraction")
        .test("MixedFractions", ENTER).expect("10 10/99 999 999");
    step("Small fractions")
        .test("SmallFractions", ENTER).expect("10 ¹⁰/₉₉ ₉₉₉ ₉₉₉");
    step("Improper fractions")
        .test("ImproperFractions", ENTER).expect("¹ ⁰⁰⁰ ⁰⁰⁰ ⁰⁰⁰/₉₉ ₉₉₉ ₉₉₉");
    step("Back to mixed fractions")
        .test("MixedFractions", ENTER).expect("10 ¹⁰/₉₉ ₉₉₉ ₉₉₉");
}


void tests::decimal_display_formats()
// ----------------------------------------------------------------------------
//   Check the various display formats for decimal values
// ----------------------------------------------------------------------------
{
    BEGIN(dformat);

    step("Standard mode");
    test(CLEAR, "STD", ENTER).noerror();

    step("Small number");
    test(CLEAR, "1.03", ENTER)
        .type(object::ID_decimal)
        .expect("1.03");

    step("Zero with dot is an error")
        .test(CLEAR, ".", ENTER).error("Syntax error").test(EXIT);
    step("Zero as 0. is accepted")
        .test(CLEAR, "0.").editor("0.")
        .test(ENTER).type(object::ID_decimal).expect("0.");

    // Regression test for bug #726
    step("Showing 0.2");
    test(CLEAR, "0.2", ENTER).type(object::ID_decimal).expect("0.2");
    step("Showing 0.2 with NoTrailingDecimal (bug #726)");
    test("NoTrailingDecimal", ENTER).type(object::ID_decimal).expect("0.2");
    step("Showing 0.2 with TrailingDecimal (bug #726)");
    test("TrailingDecimal", ENTER).type(object::ID_decimal).expect("0.2");

    step("Negative");
    test(CLEAR, "0.3", CHS, ENTER)
        .type(object::ID_neg_decimal)
        .expect("-0.3");

    step("Scientific entry");
    test(CLEAR, "1", EEX, "2", ENTER)
        .type(object::ID_decimal)
        .expect("100.");

    step("Scientific entry with negative exponent");
    test(CLEAR, "1", EEX, "2", CHS, ENTER)
        .type(object::ID_decimal)
        .expect("0.01");

    step("Negative entry with negative exponent");
    test(CLEAR, "1", CHS, EEX, "2", CHS, ENTER)
        .type(object::ID_neg_decimal)
        .expect("-0.01");

    step("Non-scientific display");
    test(CLEAR, "0.245", ENTER)
        .type(object::ID_decimal)
        .expect("0.245");
    test(CLEAR, "0.0003", CHS, ENTER)
        .type(object::ID_neg_decimal)
        .expect("-0.0003");
    test(CLEAR, "123.456", ENTER)
        .type(object::ID_decimal)
        .expect("123.456");

    step("Formerly selection of decimal64");
    test(CLEAR, "1.2345678", ENTER)
        .type(object::ID_decimal)
        .expect("1.23456 78");

    step("Formerly selection of decimal64 based on exponent");
    test(CLEAR, "1.23", EEX, 100, ENTER)
        .type(object::ID_decimal)
        .expect("1.23⁳¹⁰⁰");

    step("Formerly selection of decimal128");
    test(CLEAR, "1.2345678901234567890123", ENTER)
        .type(object::ID_decimal)
        .expect("1.23456 78901 2");
    step("Selection of decimal128 based on exponent");
    test(CLEAR, "1.23", EEX, 400, ENTER)
        .type(object::ID_decimal)
        .expect("1.23⁳⁴⁰⁰");

    step("Automatic switching to scientific display");
    test(CLEAR, "1000000000000.", ENTER)
        .expect("1.⁳¹²");
    test(CLEAR, "0.00000000000025", ENTER)
        .expect("2.5⁳⁻¹³");

    step("FIX 4 mode");
    test(CLEAR, "4 FIX", ENTER).noerror();
    test(CLEAR, "1.01", ENTER).expect("1.0100")
        .test(CHS).expect("-1.0100");
    test(CLEAR, "1.0123", ENTER).expect("1.0123");
    test(CLEAR, "10.12345", ENTER).expect("10.1235");
    test(CLEAR, "101.29995", ENTER).expect("101.3000");
    test(CLEAR, "1999.99999", ENTER).expect("2 000.0000");
    test(CLEAR, "19999999999999.", ENTER).expect("2.0000⁳¹³");
    test(CLEAR, "0.00000000001999999", ENTER).expect("2.0000⁳⁻¹¹")
        .test(CHS).expect("-2.0000⁳⁻¹¹");

    step("FIX 24 mode");
    test(CLEAR, "24 FIX", ENTER).noerror();
    test(CLEAR, "1.01", ENTER).expect("1.01000 00000 00000 00000 0000");
    test(CLEAR, "1.0123 log", ENTER)
        .expect("0.01222 49696 22568 97092 2453");

    step("SCI 3 mode");
    test(CLEAR, "3 Sci", ENTER).noerror();
    test(CLEAR, "1.01", ENTER).expect("1.010⁳⁰")
        .test(CHS).expect("-1.010⁳⁰");
    test(CLEAR, "1.0123", ENTER).expect("1.012⁳⁰");
    test(CLEAR, "10.12345", ENTER).expect("1.012⁳¹");
    test(CLEAR, "101.2543", ENTER).expect("1.013⁳²");
    test(CLEAR, "1999.999", ENTER).expect("2.000⁳³");
    test(CLEAR, "19999999999999.", ENTER).expect("2.000⁳¹³");
    test(CLEAR, "0.00000000001999999", ENTER).expect("2.000⁳⁻¹¹")
        .test(CHS).expect("-2.000⁳⁻¹¹");

    step("ENG 3 mode");
    test(CLEAR, "3 eng", ENTER).noerror();
    test(CLEAR, "1.01", ENTER).expect("1.010⁳⁰")
        .test(CHS).expect("-1.010⁳⁰");
    test(CLEAR, "1.0123", ENTER).expect("1.012⁳⁰");
    test(CLEAR, "10.12345", ENTER).expect("10.12⁳⁰");
    test(CLEAR, "101.2543", ENTER).expect("101.3⁳⁰");
    test(CLEAR, "1999.999", ENTER).expect("2.000⁳³");
    test(CLEAR, "19999999999999.", ENTER).expect("20.00⁳¹²");
    test(CLEAR, "0.00000000001999999", ENTER).expect("20.00⁳⁻¹²")
        .test(CHS).expect("-20.00⁳⁻¹²");

    step("SIG 3 mode");
    test(CLEAR, "3 sig", ENTER).noerror();
    test(CLEAR, "1.01", ENTER).expect("1.01")
        .test(CHS).expect("-1.01");
    test(CLEAR, "1.0123", ENTER).expect("1.01");
    test(CLEAR, "10.12345", ENTER).expect("10.1");
    test(CLEAR, "101.2543", ENTER).expect("101.");
    test(CLEAR, "1999.999", ENTER).expect("2 000.");
    test(CLEAR, "19999999999999.", ENTER).expect("2.⁳¹³");
    test(CLEAR, "0.00000000001999999", ENTER).expect("2.⁳⁻¹¹")
        .test(CHS).expect("-2.⁳⁻¹¹");

    step("SCI 5 mode");
    test(CLEAR, "5 Sci", ENTER).noerror();
    test(CLEAR, "1.01", ENTER).expect("1.01000⁳⁰")
        .test(CHS).expect("-1.01000⁳⁰");
    test(CLEAR, "1.0123", ENTER).expect("1.01230⁳⁰");
    test(CLEAR, "10.12345", ENTER).expect("1.01235⁳¹");
    test(CLEAR, "101.2543", ENTER).expect("1.01254⁳²");
    test(CLEAR, "1999.999", ENTER).expect("2.00000⁳³");
    test(CLEAR, "19999999999999.", ENTER).expect("2.00000⁳¹³");
    test(CLEAR, "0.00000000001999999", ENTER).expect("2.00000⁳⁻¹¹")
        .test(CHS).expect("-2.00000⁳⁻¹¹");

    step("ENG 5 mode");
    test(CLEAR, "5 eng", ENTER).noerror();
    test(CLEAR, "1.01", ENTER).expect("1.01000⁳⁰")
        .test(CHS).expect("-1.01000⁳⁰");
    test(CLEAR, "1.0123", ENTER).expect("1.01230⁳⁰");
    test(CLEAR, "10.12345", ENTER).expect("10.1235⁳⁰");
    test(CLEAR, "101.2543", ENTER).expect("101.254⁳⁰");
    test(CLEAR, "1999.999", ENTER).expect("2.00000⁳³");
    test(CLEAR, "19999999999999.", ENTER).expect("20.0000⁳¹²");
    test(CLEAR, "0.00000000001999999", ENTER).expect("20.0000⁳⁻¹²")
        .test(CHS).expect("-20.0000⁳⁻¹²");

    step("SIG 5 mode");
    test(CLEAR, "5 sig", ENTER).noerror();
    test(CLEAR, "1.01", ENTER).expect("1.01")
        .test(CHS).expect("-1.01");
    test(CLEAR, "1.0123", ENTER).expect("1.0123");
    test(CLEAR, "10.12345", ENTER).expect("10.123");
    test(CLEAR, "101.2543", ENTER).expect("101.25");
    test(CLEAR, "1999.999", ENTER).expect("2 000.");
    test(CLEAR, "19999999999999.", ENTER).expect("2.⁳¹³");
    test(CLEAR, "0.00000000001999999", ENTER).expect("2.⁳⁻¹¹")
        .test(CHS).expect("-2.⁳⁻¹¹");

    step("SCI 13 mode");
    test(CLEAR, "13 Sci", ENTER).noerror();
    test(CLEAR, "1.01", ENTER).expect("1.01000 00000 000⁳⁰")
        .test(CHS).expect("-1.01000 00000 000⁳⁰");
    test(CLEAR, "1.0123", ENTER).expect("1.01230 00000 000⁳⁰");
    test(CLEAR, "10.12345", ENTER).expect("1.01234 50000 000⁳¹");
    test(CLEAR, "101.2543", ENTER).expect("1.01254 30000 000⁳²");
    test(CLEAR, "1999.999", ENTER).expect("1.99999 90000 000⁳³");
    test(CLEAR, "19999999999999.", ENTER).expect("1.99999 99999 999⁳¹³");
    test(CLEAR, "0.00000000001999999", ENTER).expect("1.99999 90000 000⁳⁻¹¹")
        .test(CHS).expect("-1.99999 90000 000⁳⁻¹¹");

    step("ENG 13 mode");
    test(CLEAR, "13 eng", ENTER).noerror();
    test(CLEAR, "1.01", ENTER).expect("1.01000 00000 000⁳⁰")
        .test(CHS).expect("-1.01000 00000 000⁳⁰");
    test(CLEAR, "1.0123", ENTER).expect("1.01230 00000 000⁳⁰");
    test(CLEAR, "10.12345", ENTER).expect("10.12345 00000 00⁳⁰");
    test(CLEAR, "101.2543", ENTER).expect("101.25430 00000 0⁳⁰");
    test(CLEAR, "1999.999", ENTER).expect("1.99999 90000 000⁳³");
    test(CLEAR, "19999999999999.", ENTER).expect("19.99999 99999 99⁳¹²");
    test(CLEAR, "0.00000000001999999", ENTER).expect("19.99999 00000 00⁳⁻¹²")
        .test(CHS).expect("-19.99999 00000 00⁳⁻¹²");

    step("SIG 13 mode");
    test(CLEAR, "13 sig", ENTER).noerror();
    test(CLEAR, "1.01", ENTER).expect("1.01")
        .test(CHS).expect("-1.01");
    test(CLEAR, "1.0123", ENTER).expect("1.0123");
    test(CLEAR, "10.12345", ENTER).expect("10.12345");
    test(CLEAR, "101.2543", ENTER).expect("101.2543");
    test(CLEAR, "1999.999", ENTER).expect("1 999.999");
    test(CLEAR, "19999999999999.", ENTER).expect("2.⁳¹³");
    test(CLEAR, "0.00000000001999999", ENTER).expect("1.99999 9⁳⁻¹¹")
        .test(CHS).expect("-1.99999 9⁳⁻¹¹");

    step("FIX 4 in HP48-compatible mode")
        .test(CLEAR, "4", LSHIFT, O, F2, "0", LSHIFT, F5).noerror()
        .test("0.635", ENTER).expect("0.6350")
        .test("10", DIV).expect("0.0635")
        .test("10", DIV).expect("0.0064")
        .test("10", DIV).expect("0.0006")
        .test("10", DIV).expect("0.0001")
        .test("10", DIV).expect("6.3500⁳⁻⁶")
        .test("10", DIV).expect("6.3500⁳⁻⁷");

    step("FIX 4 showing 2 significant digits")
        .test(CLEAR, "2", LSHIFT, F5).noerror()
        .test("0.635", ENTER).expect("0.6350")
        .test("10", DIV).expect("0.0635")
        .test("10", DIV).expect("0.0064")
        .test("10", DIV).expect("6.3500⁳⁻⁴")
        .test("10", DIV).expect("6.3500⁳⁻⁵")
        .test("10", DIV).expect("6.3500⁳⁻⁶")
        .test("10", DIV).expect("6.3500⁳⁻⁷");

    step("FIX 4 showing 12 significant digits")
        .test(CLEAR, "12", LSHIFT, F5).noerror()
        .test("0.635", ENTER).expect("0.6350")
        .test("10", DIV).expect("0.0635")
        .test("10", DIV).expect("6.3500⁳⁻³")
        .test("10", DIV).expect("6.3500⁳⁻⁴")
        .test("10", DIV).expect("6.3500⁳⁻⁵")
        .test("10", DIV).expect("6.3500⁳⁻⁶")
        .test("10", DIV).expect("6.3500⁳⁻⁷");

    step("FIX 4 in old HP style (showing 0.0000)")
        .test(CLEAR, "-1", LSHIFT, F5).noerror()
        .test("0.635", ENTER).expect("0.6350")
        .test("10", DIV).expect("0.0635")
        .test("10", DIV).expect("0.0064")
        .test("10", DIV).expect("0.0006")
        .test("10", DIV).expect("0.0001")
        .test("10", DIV).expect("0.0000")
        .test("10", DIV).expect("0.0000");

    step("Reset defaults");
    test(CLEAR, LSHIFT, O, F1, KEY3, LSHIFT, F5).noerror();

    step("Test display of 5000.");
    test(CLEAR, "5000.", ENTER)        .expect("5 000.");
    test(CLEAR, "50000.", ENTER)       .expect("50 000.");
    test(CLEAR, "500000.", ENTER)      .expect("500 000.");
    test(CLEAR, "5000000.", ENTER)     .expect("5 000 000.");
}


void tests::integer_numerical_functions()
// ----------------------------------------------------------------------------
//   Test integer numerical functions
// ----------------------------------------------------------------------------
{
    BEGIN(ifunctions);

    step("neg")
        .test(CLEAR, "3 neg", ENTER).expect("-3")
        .test("negate", ENTER).expect("3");
    step("inv")
        .test(CLEAR, "3 inv", ENTER).expect("¹/₃")
        .test("inv", ENTER).expect("3")
        .test(CLEAR, "-3 inv", ENTER).expect("-¹/₃")
        .test("inv", ENTER).expect("-3");
    step("sq (square)")
        .test(CLEAR, "-3 sq", ENTER).expect("9")
        .test("sq", ENTER).expect("81");
    step("cubed")
        .test(CLEAR, "3 cubed", ENTER).expect("27")
        .test("cubed", ENTER).expect("19 683")
        .test(CLEAR, "-3 cubed", ENTER).expect("-27")
        .test("cubed", ENTER).expect("-19 683");
    step("abs")
        .test(CLEAR, "-3 abs", ENTER).expect("3")
        .test("abs", ENTER, 1, ADD).expect("4");
    step("norm").test("-5 norm", ENTER).expect("5");
}


void tests::decimal_numerical_functions()
// ----------------------------------------------------------------------------
//   Test decimal numerical functions
// ----------------------------------------------------------------------------
{
    BEGIN(dfunctions);

    step("Select 34-digit precision to match Intel Decimal 128");
    test(CLEAR, "34 PRECISION 64 SIG", ENTER).noerror();

    step("Addition")
        .test(CLEAR, "1.23 2.34", NOSHIFT, ADD).expect("3.57")
        .test(CLEAR, "1.23 -2.34", NOSHIFT, ADD).expect("-1.11")
        .test(CLEAR, "-1.23 2.34", NOSHIFT, ADD).expect("1.11")
        .test(CLEAR, "-1.23 -2.34", NOSHIFT, ADD).expect("-3.57")
        .test(CLEAR, "1.234 SIN 2.34", NOSHIFT, ADD).expect("2.36153 56979 61861 56851 62100 48334 91721")
        .test(CLEAR, "1.23 COS -2.34", NOSHIFT, ADD).expect("-1.34023 04189 97834 80530 72456 24377 86853")
        .test(CLEAR, "-1.23 TAN 2.34", NOSHIFT, ADD).expect("2.31852 91517 78239 80211 40912 32514 08406")
        .test(CLEAR, "-1.23 TANH -2.34", NOSHIFT, ADD).expect("-3.18257 93256 58929 54289 07208 91501 6509");
    step("Subtraction")
        .test(CLEAR, "1.23 2.34", NOSHIFT, SUB).expect("-1.11")
        .test(CLEAR, "1.23 -2.34", NOSHIFT, SUB).expect("3.57")
        .test(CLEAR, "-1.23 2.34", NOSHIFT, SUB).expect("-3.57")
        .test(CLEAR, "-1.23 -2.34", NOSHIFT, SUB).expect("1.11")
        .test(CLEAR, "1.234 SIN 2.34", NOSHIFT, SUB).expect("-2.31846 43020 38138 43148 37899 51665 08279")
        .test(CLEAR, "1.23 COS -2.34", NOSHIFT, SUB).expect("3.33976 95810 02165 19469 27543 75622 13147")
        .test(CLEAR, "-1.23 TAN 2.34", NOSHIFT, SUB).expect("-2.36147 08482 21760 19788 59087 67485 91594")
        .test(CLEAR, "-1.23 TANH -2.34", NOSHIFT, SUB).expect("1.49742 06743 41070 45710 92791 08498 3491");
    step("Multiplication")
        .test(CLEAR, "1.23 2.34", NOSHIFT, MUL).expect("2.8782")
        .test(CLEAR, "1.23 -2.34", NOSHIFT, MUL).expect("-2.8782")
        .test(CLEAR, "-1.23 2.34", NOSHIFT, MUL).expect("-2.8782")
        .test(CLEAR, "-1.23 -2.34", NOSHIFT, MUL).expect("2.8782")
        .test(CLEAR, "1.234 SIN 2.34", NOSHIFT, MUL).expect("0.05039 35332 30756 07032 79315 13103 70629 5")
        .test(CLEAR, "1.23 COS -2.34", NOSHIFT, MUL).expect("-2.33946 08195 45066 55558 10452 38955 78766")
        .test(CLEAR, "-1.23 TAN 2.34", NOSHIFT, MUL).expect("-0.05024 17848 38918 86305 30265 15917 04330 3")
        .test(CLEAR, "-1.23 TANH -2.34", NOSHIFT, MUL).expect("1.97163 56220 41895 13036 42868 86113 86312");
    step("Division")
        .test(CLEAR, "1.23 2.34", NOSHIFT, DIV).expect("0.52564 10256 41025 64102 56410 25641 02564 1")
        .test(CLEAR, "1.23 -2.34", NOSHIFT, DIV).expect("-0.52564 10256 41025 64102 56410 25641 02564 1")
        .test(CLEAR, "-1.23 2.34", NOSHIFT, DIV).expect("-0.52564 10256 41025 64102 56410 25641 02564 1")
        .test(CLEAR, "-1.23 -2.34", NOSHIFT, DIV).expect("0.52564 10256 41025 64102 56410 25641 02564 1")
        .test(CLEAR, "1.234 SIN 2.34", NOSHIFT, DIV).expect("0.00920 32897 27291 26859 66709 60826 88770 081")
        .test(CLEAR, "1.23 COS -2.34", NOSHIFT, DIV).expect("-0.42725 19576 93232 98918 49377 67359 88524 7")
        .test(CLEAR, "-1.23 TAN 2.34", NOSHIFT, DIV).expect("-0.00917 55761 63145 38371 19268 23711 92988 948")
        .test(CLEAR, "-1.23 TANH -2.34", NOSHIFT, DIV).expect("0.36007 66348 96978 43713 27867 05769 93628 4");
    step("Power")
        .test(CLEAR, "1.23 2.34", LSHIFT, B).expect("1.62322 21516 85370 76170 21776 74374 04099")
        .test(CLEAR, "1.23 -2.34", LSHIFT, B).expect("0.61605 86207 88111 35803 50956 46724 98593")
        .test(CLEAR, "-1.23 23", LSHIFT, B).expect("-116.90082 15014 43291 74653 48578 88750 679")
        .test(CLEAR, "-1.23 -2.34", LSHIFT, B).error("Argument outside domain")
        .test(CLEAR, "-1.23 23", LSHIFT, B).expect("-116.90082 15014 43291 74653 48578 88750 679")
        .test(CLEAR, "-1.23 -2.34", LSHIFT, B).error("Argument outside domain")
        .test(CLEAR, "1.234 SIN 2.34", LSHIFT, B).expect("0.00012 57743 10956 55759 81666 83961 25288 114")
        .test(CLEAR, "1.23 COS -2.34", LSHIFT, B).expect("1.00053 93880 00606 36152 22273 75863 57849")
        .test(CLEAR, "-1.23 TAN 23", LSHIFT, B).expect("-4.29073 45139 05064 31475 52781 67797 518⁳⁻³⁹")
        .test(CLEAR, "-1.23 TAN 2.34", LSHIFT, B).error("Argument outside domain")
        .test(CLEAR, "-1.23 TAN -23", LSHIFT, B).expect("-2.33060 32959 14210 32416 06485 39037 41948⁳³⁸")
        .test(CLEAR, "-1.23 TANH -2.34", LSHIFT, B).error("Argument outside domain");

    step("Square root of 2")
        .test(CLEAR, "2 sqrt", ENTER)
        .expect("1.41421 35623 73095 04880 16887 24209 698");
    step("Square root of 3")
        .test(CLEAR, "3 sqrt", ENTER)
        .expect("1.73205 08075 68877 29352 74463 41505 872");
    step("Square root of 4")
        .test(CLEAR, "4 sqrt", ENTER)
        .expect("2.");
    step("Cube root of 2")
        .test(CLEAR, "2 cbrt", ENTER)
        .expect("1.25992 10498 94873 16476 72106 07278 228");
    step("Cube root of 3")
        .test(CLEAR, "3 cbrt", ENTER)
        .expect("1.44224 95703 07408 38232 16383 10780 11");
    step("Cube root of 27")
        .test(CLEAR, "27 cbrt", ENTER)
        .expect("3.");

    step("neg")
        .test(CLEAR, "3.21 neg", ENTER).expect("-3.21")
        .test("negate", ENTER).expect("3.21");
    step("inv")
        .test(CLEAR, "3.21 inv", ENTER)
        .expect("0.31152 64797 50778 81619 93769 47040 49844 2")
        .test("inv", ENTER).expect("3.21");
    step("sq (square)")
        .test(CLEAR, "-3.21 sq", ENTER).expect("10.3041")
        .test("sq", ENTER).expect("106.17447 681");
    step("cubed")
        .test(CLEAR, "3.21 cubed", ENTER).expect("33.07616 1")
        .test("cubed", ENTER).expect("36 186.39267 80659 01161 281")
        .test(CLEAR, "-3 cubed", ENTER).expect("-27")
        .test("cubed", ENTER).expect("-19 683");
    step("abs")
        .test(CLEAR, "-3.21 abs", ENTER).expect("3.21")
        .test("abs", ENTER, 1, ADD).expect("4.21");

    step("Setting radians mode");
    test(CLEAR, "RAD", ENTER).noerror();

#define TFNA(name, arg)                                         \
    step(#name).test(CLEAR, #arg " " #name, ENTER)
#define TFN(name)  TFNA(name, 0.321)

    TFN(sqrt).expect("0.56656 86189 68611 77992 54734 04696 769");
    TFN(sin).expect("0.31551 56385 92727 11130 65931 11143 46369 9");
    TFN(cos).expect("0.94892 03769 56583 01754 39451 32826 92553 3");
    TFN(tan).expect("0.33249 95924 36471 87510 87087 30102 73793 5");
    TFN(asin).expect("0.32678 51765 31495 46326 91997 64519 59826 7 r");
    TFN(acos).expect("1.24401 11502 63401 15596 21219 27120 15339 r");
    TFN(atan).expect("0.31060 97928 13889 91760 67000 51446 83602 7 r");
    TFN(sinh).expect("0.32654 11649 51806 35701 22065 63885 73434");
    TFN(cosh).expect("1.05196 44159 41947 53843 52241 43605 67798");
    TFN(tanh).expect("0.31041 08466 05886 02148 50502 09383 09588 5");
    TFN(asinh).expect("0.31572 82658 29379 61791 08945 47102 06968 7");
    TFNA(acosh, 1.321).expect("0.78123 02051 96252 61474 22171 61603 43493");
    TFN(atanh).expect("0.33276 15884 81814 59580 17641 70508 75108 5");
    TFN(log1p).expect("0.27838 90255 40188 26677 16283 42111 55095 2");
    TFN(lnp1).expect("0.27838 90255 40188 26677 16283 42111 55095 2");
    TFN(expm1).expect("0.37850 55808 93753 89544 74307 07491 41232 1");
    TFN(log).expect("-1.13631 41558 52121 18735 43303 10107 28989");
    TFN(log10).expect("-0.49349 49675 95127 92187 04308 57283 44906");
    TFN(exp).expect("1.37850 55808 93753 89544 74307 07491 41232");
    TFN(exp10).expect("2.09411 24558 50892 67051 98819 85846 25421");
    TFN(exp2).expect("1.24919 61256 53376 70052 14667 82085 80659");
    TFN(erf).expect("0.35014 42208 20023 82355 16032 45050 23913");
    TFN(erfc).expect("0.64985 57791 79976 17644 83967 54949 76087");
    TFN(tgamma).expect("2.78663 45408 45472 36795 07642 12781 773");
    TFN(lgamma).expect("1.02483 46099 57313 19869 10927 53834 887");
    TFN(gamma).expect("2.78663 45408 45472 36795 07642 12781 773");
    TFN(cbrt).expect("0.68470 21277 57224 16184 09277 32646 815");
    TFN(norm).expect("0.321");
#undef TFN
#undef TFNA

    step("pow")
        ,test(CLEAR, "3.21 1.23 pow", ENTER)
        .expect("4.19760 13402 69557 03133 41557 04388 7116")
        .test(CLEAR, "1.23 2.31").shifts(true,false,false,false).test(B)
        .expect("1.61317 24907 55543 84434 14148 92337 98556");

    step("hypot")
        .test(CLEAR, "3.21 1.23 hypot", ENTER)
        .expect("3.43758 63625 51492 31996 16557 32945 235");

    step("atan2 pos / pos quadrant")
        .test(CLEAR, "3.21 1.23 atan2", ENTER)
        .expect("1.20487 56251 52809 23400 86691 05495 30674");
    step("atan2 pos / neg quadrant")
        .test(CLEAR, "3.21 -1.23 atan2", ENTER)
        .expect("1.93671 70284 36984 00445 39742 77784 19614");
    step("atan2 neg / pos quadrant")
        .test(CLEAR, "-3.21 1.23 atan2", ENTER)
        .expect("-1.20487 56251 52809 23400 86691 05495 30674");
    step("atan2 neg / neg quadrant")
        .test(CLEAR, "-3.21 -1.23 atan2", ENTER)
        .expect("-1.93671 70284 36984 00445 39742 77784 19614");

    step("Restore default 24-digit precision");
    test(CLEAR, "24 PRECISION 12 SIG", ENTER).noerror();

    step("→Frac should work for integers")
        .test(CLEAR, "0 →Frac", ENTER).noerror().expect("0")
        .test(CLEAR, "1 →Frac", ENTER).noerror().expect("1")
        .test(CLEAR, "-123 →Frac", ENTER).noerror().expect("-123");
}


void tests::float_numerical_functions()
// ----------------------------------------------------------------------------
//   Test hardware-accelerated numerical functions
// ----------------------------------------------------------------------------
{
    BEGIN(float);

    step("Select float acceleration")
        .test(CLEAR, "7 PRECISION 10 SIG HardFP", ENTER).noerror();
    step("Binary representation does not align with decimal")
        .test(CLEAR, "1.2", ENTER).noerror().expect("1.20000 0048");
    step("Select 6-digit precision for output stability")
        .test("6 SIG", ENTER)
        .noerror();

    step("Addition")
        .test(CLEAR, "1.23 2.34", NOSHIFT, ADD).expect("3.57")
        .test(CLEAR, "1.23 -2.34", NOSHIFT, ADD).expect("-1.11")
        .test(CLEAR, "-1.23 2.34", NOSHIFT, ADD).expect("1.11")
        .test(CLEAR, "-1.23 -2.34", NOSHIFT, ADD).expect("-3.57")
        .test(CLEAR, "1.234 SIN 2.34", NOSHIFT, ADD).expect("3.28382")
        .test(CLEAR, "1.23 COS -2.34", NOSHIFT, ADD).expect("-2.00576")
        .test(CLEAR, "-1.23 TAN 2.34", NOSHIFT, ADD).expect("-0.47981 6")
        .test(CLEAR, "-1.23 TANH -2.34", NOSHIFT, ADD).expect("-3.18258");
    step("Subtraction")
        .test(CLEAR, "1.23 2.34", NOSHIFT, SUB).expect("-1.11")
        .test(CLEAR, "1.23 -2.34", NOSHIFT, SUB).expect("3.57")
        .test(CLEAR, "-1.23 2.34", NOSHIFT, SUB).expect("-3.57")
        .test(CLEAR, "-1.23 -2.34", NOSHIFT, SUB).expect("1.11")
        .test(CLEAR, "1.234 SIN 2.34", NOSHIFT, SUB).expect("-1.39618")
        .test(CLEAR, "1.23 COS -2.34", NOSHIFT, SUB).expect("2.67424")
        .test(CLEAR, "-1.23 TAN 2.34", NOSHIFT, SUB).expect("-5.15982")
        .test(CLEAR, "-1.23 TANH -2.34", NOSHIFT, SUB).expect("1.49742");
    step("Multiplication")
        .test(CLEAR, "1.23 2.34", NOSHIFT, MUL).expect("2.8782")
        .test(CLEAR, "1.23 -2.34", NOSHIFT, MUL).expect("-2.8782")
        .test(CLEAR, "-1.23 2.34", NOSHIFT, MUL).expect("-2.8782")
        .test(CLEAR, "-1.23 -2.34", NOSHIFT, MUL).expect("2.8782")
        .test(CLEAR, "1.234 SIN 2.34", NOSHIFT, MUL).expect("2.20853")
        .test(CLEAR, "1.23 COS -2.34", NOSHIFT, MUL).expect("-0.78211 6")
        .test(CLEAR, "-1.23 TAN 2.34", NOSHIFT, MUL).expect("-6.59837")
        .test(CLEAR, "-1.23 TANH -2.34", NOSHIFT, MUL).expect("1.97164");
    step("Division")
        .test(CLEAR, "1.23 2.34", NOSHIFT, DIV).expect("0.52564 1")
        .test(CLEAR, "1.23 -2.34", NOSHIFT, DIV).expect("-0.52564 1")
        .test(CLEAR, "-1.23 2.34", NOSHIFT, DIV).expect("-0.52564 1")
        .test(CLEAR, "-1.23 -2.34", NOSHIFT, DIV).expect("0.52564 1")
        .test(CLEAR, "1.234 SIN 2.34", NOSHIFT, DIV).expect("0.40334 1")
        .test(CLEAR, "1.23 COS -2.34", NOSHIFT, DIV).expect("-0.14283 7")
        .test(CLEAR, "-1.23 TAN 2.34", NOSHIFT, DIV).expect("-1.20505")
        .test(CLEAR, "-1.23 TANH -2.34", NOSHIFT, DIV).expect("0.36007 7");
    step("Power")
        .test(CLEAR, "1.23 2.34", LSHIFT, B).expect("1.62322")
        .test(CLEAR, "1.23 -2.34", LSHIFT, B).expect("0.61605 9")
        .test(CLEAR, "-1.23 23", LSHIFT, B).expect("-116.901")
        .test(CLEAR, "-1.23 -2.34", LSHIFT, B).error("Argument outside domain")
        .test(CLEAR, "-1.23 23", LSHIFT, B).expect("-116.901")
        .test(CLEAR, "-1.23 -2.34", LSHIFT, B).error("Argument outside domain")
        .test(CLEAR, "1.234 SIN 2.34", LSHIFT, B).expect("0.87345 1")
        .test(CLEAR, "1.23 COS -2.34", LSHIFT, B).expect("12.993")
        .test(CLEAR, "-1.23 TAN 23", LSHIFT, B).expect("-2.26505⁳¹⁰")
        .test(CLEAR, "-1.23 TAN 2.34", LSHIFT, B).error("Argument outside domain")
        .test(CLEAR, "-1.23 TAN -23", LSHIFT, B).expect("-4.41492⁳⁻¹¹")
        .test(CLEAR, "-1.23 TANH -2.34", LSHIFT, B).error("Argument outside domain");

    step("Square root of 2")
        .test(CLEAR, "2 sqrt", ENTER)
        .expect("1.41421");
    step("Square root of 3")
        .test(CLEAR, "3 sqrt", ENTER)
        .expect("1.73205");
    step("Square root of 4")
        .test(CLEAR, "4 sqrt", ENTER)
        .expect("2.");
    step("Cube root of 2")
        .test(CLEAR, "2 cbrt", ENTER)
        .expect("1.25992");
    step("Cube root of 3")
        .test(CLEAR, "3 cbrt", ENTER)
        .expect("1.44225");
    step("Cube root of 27")
        .test(CLEAR, "27 cbrt", ENTER)
        .expect("3.");

    step("neg")
        .test(CLEAR, "3.21 neg", ENTER).expect("-3.21")
        .test("negate", ENTER).expect("3.21");
    step("inv")
        .test(CLEAR, "3.21 inv", ENTER)
        .expect("0.31152 6")
        .test("inv", ENTER).expect("3.21");
    step("sq (square)")
        .test(CLEAR, "-3.21 sq", ENTER).expect("10.3041")
        .test("sq", ENTER).expect("106.174");
    step("cubed")
        .test(CLEAR, "3.21 cubed", ENTER).expect("33.0762")
        .test("cubed", ENTER).expect("36 186.4")
        .test(CLEAR, "-3 cubed", ENTER).expect("-27")
        .test("cubed", ENTER).expect("-19 683");
    step("abs")
        .test(CLEAR, "-3.21 abs", ENTER).expect("3.21")
        .test("abs", ENTER, 1, ADD).expect("4.21");

    step("Setting radians mode");
    test(CLEAR, "RAD", ENTER).noerror();

#define TFNA(name, arg)         step(#name).test(CLEAR, #arg " " #name, ENTER)
#define TFN(name)               TFNA(name, 0.321)

    TFN(sqrt).expect("0.56656 9");
    TFN(sin).expect("0.31551 6");
    TFN(cos).expect("0.94892");
    TFN(tan).expect("0.3325");
    TFN(asin).expect("0.32678 5 r");
    TFN(acos).expect("1.24401 r");
    TFN(atan).expect("0.31061 r");
    TFN(sinh).expect("0.32654 1");
    TFN(cosh).expect("1.05196");
    TFN(tanh).expect("0.31041 1");
    TFN(asinh).expect("0.31572 8");
    TFNA(acosh, 1.321).expect("0.78123");
    TFN(atanh).expect("0.33276 2");
    TFN(log1p).expect("0.27838 9");
    TFN(lnp1).expect("0.27838 9");
    TFN(expm1).expect("0.37850 6");
    TFN(log).expect("-1.13631");
    TFN(log10).expect("-0.49349 5");
    TFN(exp).expect("1.37851");
    TFN(exp10).expect("2.09411");
    TFN(exp2).expect("1.2492");
    TFN(erf).expect("0.35014 4");
    TFN(erfc).expect("0.64985 6");
    TFN(tgamma).expect("2.78663");
    TFN(lgamma).expect("1.02483");
    TFN(gamma).expect("2.78663");
    TFN(cbrt).expect("0.68470 2");
    TFN(norm).expect("0.321");
#undef TFN
#undef TFNA

    step("pow")
        ,test(CLEAR, "3.21 1.23 pow", ENTER)
        .expect("4.1976")
        .test(CLEAR, "1.23 2.31").shifts(true,false,false,false).test(B)
        .expect("1.61317");

    step("hypot")
        .test(CLEAR, "3.21 1.23 hypot", ENTER)
        .expect("3.43759");

    step("atan2 pos / pos quadrant")
        .test(CLEAR, "3.21 1.23 atan2", ENTER)
        .expect("1.20488");
    step("atan2 pos / neg quadrant")
        .test(CLEAR, "3.21 -1.23 atan2", ENTER)
        .expect("1.93672");
    step("atan2 neg / pos quadrant")
        .test(CLEAR, "-3.21 1.23 atan2", ENTER)
        .expect("-1.20488");
    step("atan2 neg / neg quadrant")
        .test(CLEAR, "-3.21 -1.23 atan2", ENTER)
        .expect("-1.93672");

    step("Restore default 24-digit precision");
    test(CLEAR, "24 PRECISION 12 SIG SoftFP", ENTER).noerror();
}


void tests::double_numerical_functions()
// ----------------------------------------------------------------------------
//   Test hardware-accelerated numerical functions
// ----------------------------------------------------------------------------
{
    BEGIN(double);

    step("Select double acceleration")
        .test(CLEAR, "16 PRECISION 24 SIG HardFP", ENTER).noerror();
    step("Binary representation does not align with decimal")
        .test(CLEAR, "1.2", ENTER).noerror().expect("1.19999 99999 99999 96");
    step("Select 15-digit precision for output stability")
        .test("15 SIG", ENTER).noerror();

    step("Addition")
        .test(CLEAR, "1.23 2.34", NOSHIFT, ADD).expect("3.57")
        .test(CLEAR, "1.23 -2.34", NOSHIFT, ADD).expect("-1.11")
        .test(CLEAR, "-1.23 2.34", NOSHIFT, ADD).expect("1.11")
        .test(CLEAR, "-1.23 -2.34", NOSHIFT, ADD).expect("-3.57")
        .test(CLEAR, "1.234 SIN 2.34", NOSHIFT, ADD).expect("3.28381 82093 7463")
        .test(CLEAR, "1.23 COS -2.34", NOSHIFT, ADD).expect("-2.00576 22728 755")
        .test(CLEAR, "-1.23 TAN 2.34", NOSHIFT, ADD).expect("-0.47981 57342 68152")
        .test(CLEAR, "-1.23 TANH -2.34", NOSHIFT, ADD).expect("-3.18257 93256 5893");
    step("Subtraction")
        .test(CLEAR, "1.23 2.34", NOSHIFT, SUB).expect("-1.11")
        .test(CLEAR, "1.23 -2.34", NOSHIFT, SUB).expect("3.57")
        .test(CLEAR, "-1.23 2.34", NOSHIFT, SUB).expect("-3.57")
        .test(CLEAR, "-1.23 -2.34", NOSHIFT, SUB).expect("1.11")
        .test(CLEAR, "1.234 SIN 2.34", NOSHIFT, SUB).expect("-1.39618 17906 2537")
        .test(CLEAR, "1.23 COS -2.34", NOSHIFT, SUB).expect("2.67423 77271 245")
        .test(CLEAR, "-1.23 TAN 2.34", NOSHIFT, SUB).expect("-5.15981 57342 6815")
        .test(CLEAR, "-1.23 TANH -2.34", NOSHIFT, SUB).expect("1.49742 06743 4107");
    step("Multiplication")
        .test(CLEAR, "1.23 2.34", NOSHIFT, MUL).expect("2.8782")
        .test(CLEAR, "1.23 -2.34", NOSHIFT, MUL).expect("-2.8782")
        .test(CLEAR, "-1.23 2.34", NOSHIFT, MUL).expect("-2.8782")
        .test(CLEAR, "-1.23 -2.34", NOSHIFT, MUL).expect("2.8782")
        .test(CLEAR, "1.234 SIN 2.34", NOSHIFT, MUL).expect("2.20853 46099 3664")
        .test(CLEAR, "1.23 COS -2.34", NOSHIFT, MUL).expect("-0.78211 62814 71336")
        .test(CLEAR, "-1.23 TAN 2.34", NOSHIFT, MUL).expect("-6.59836 88181 8747")
        .test(CLEAR, "-1.23 TANH -2.34", NOSHIFT, MUL).expect("1.97163 56220 419");
    step("Division")
        .test(CLEAR, "1.23 2.34", NOSHIFT, DIV).expect("0.52564 10256 41026")
        .test(CLEAR, "1.23 -2.34", NOSHIFT, DIV).expect("-0.52564 10256 41026")
        .test(CLEAR, "-1.23 2.34", NOSHIFT, DIV).expect("-0.52564 10256 41026")
        .test(CLEAR, "-1.23 -2.34", NOSHIFT, DIV).expect("0.52564 10256 41026")
        .test(CLEAR, "1.234 SIN 2.34", NOSHIFT, DIV).expect("0.40334 11151 17365")
        .test(CLEAR, "1.23 COS -2.34", NOSHIFT, DIV).expect("-0.14283 66355 23292")
        .test(CLEAR, "-1.23 TAN 2.34", NOSHIFT, DIV).expect("-1.20504 94590 8895")
        .test(CLEAR, "-1.23 TANH -2.34", NOSHIFT, DIV).expect("0.36007 66348 96978");
    step("Power")
        .test(CLEAR, "1.23 2.34", LSHIFT, B).expect("1.62322 21516 8537")
        .test(CLEAR, "1.23 -2.34", LSHIFT, B).expect("0.61605 86207 88111")
        .test(CLEAR, "-1.23 23", LSHIFT, B).expect("-116.90082 15014 43")
        .test(CLEAR, "-1.23 -2.34", LSHIFT, B).error("Argument outside domain")
        .test(CLEAR, "-1.23 23", LSHIFT, B).expect("-116.90082 15014 43")
        .test(CLEAR, "-1.23 -2.34", LSHIFT, B).error("Argument outside domain")
        .test(CLEAR, "1.234 SIN 2.34", LSHIFT, B).expect("0.87345 13971 11437")
        .test(CLEAR, "1.23 COS -2.34", LSHIFT, B).expect("12.99302 28339 821")
        .test(CLEAR, "-1.23 TAN 23", LSHIFT, B).expect("-2.26504 47100 3673⁳¹⁰")
        .test(CLEAR, "-1.23 TAN 2.34", LSHIFT, B).error("Argument outside domain")
        .test(CLEAR, "-1.23 TAN -23", LSHIFT, B).expect("-4.41492 38890 0254⁳⁻¹¹")
        .test(CLEAR, "-1.23 TANH -2.34", LSHIFT, B).error("Argument outside domain");

    step("Square root of 2")
        .test(CLEAR, "2 sqrt", ENTER)
        .expect("1.41421 35623 731");
    step("Square root of 3")
        .test(CLEAR, "3 sqrt", ENTER)
        .expect("1.73205 08075 6888");
    step("Square root of 4")
        .test(CLEAR, "4 sqrt", ENTER)
        .expect("2.");
    step("Cube root of 2")
        .test(CLEAR, "2 cbrt", ENTER)
        .expect("1.25992 10498 9487");
    step("Cube root of 3")
        .test(CLEAR, "3 cbrt", ENTER)
        .expect("1.44224 95703 0741");
    step("Cube root of 27")
        .test(CLEAR, "27 cbrt", ENTER)
        .expect("3.");

    step("neg")
        .test(CLEAR, "3.21 neg", ENTER).expect("-3.21")
        .test("negate", ENTER).expect("3.21");
    step("inv")
        .test(CLEAR, "3.21 inv", ENTER)
        .expect("0.31152 64797 50779")
        .test("inv", ENTER).expect("3.21");
    step("sq (square)")
        .test(CLEAR, "-3.21 sq", ENTER).expect("10.3041")
        .test("sq", ENTER).expect("106.17447 681");
    step("cubed")
        .test(CLEAR, "3.21 cubed", ENTER).expect("33.07616 1")
        .test("cubed", ENTER).expect("36 186.39267 80659")
        .test(CLEAR, "-3 cubed", ENTER).expect("-27")
        .test("cubed", ENTER).expect("-19 683");
    step("abs")
        .test(CLEAR, "-3.21 abs", ENTER).expect("3.21")
        .test("abs", ENTER, 1, ADD).expect("4.21");

    step("Setting radians mode");
    test(CLEAR, "RAD", ENTER).noerror();

#define TFNA(name, arg)         step(#name).test(CLEAR, #arg " " #name, ENTER)
#define TFN(name)               TFNA(name, 0.321)

    TFN(sqrt).expect("0.56656 86189 68612");
    TFN(sin).expect("0.31551 56385 92727");
    TFN(cos).expect("0.94892 03769 56583");
    TFN(tan).expect("0.33249 95924 36472");
    TFN(asin).expect("0.32678 51765 31495 r");
    TFN(acos).expect("1.24401 11502 634 r");
    TFN(atan).expect("0.31060 97928 1389 r");
    TFN(sinh).expect("0.32654 11649 51806");
    TFN(cosh).expect("1.05196 44159 4195");
    TFN(tanh).expect("0.31041 08466 05886");
    TFN(asinh).expect("0.31572 82658 2938");
    TFNA(acosh, 1.321).expect("0.78123 02051 96253");
    TFN(atanh).expect("0.33276 15884 81815");
    TFN(log1p).expect("0.27838 90255 40188");
    TFN(lnp1).expect("0.27838 90255 40188");
    TFN(expm1).expect("0.37850 55808 93754");
    TFN(log).expect("-1.13631 41558 5212");
    TFN(log10).expect("-0.49349 49675 95128");
    TFN(exp).expect("1.37850 55808 9375");
    TFN(exp10).expect("2.09411 24558 5089");
    TFN(exp2).expect("1.24919 61256 5338");
    TFN(erf).expect("0.35014 42208 20024");
    TFN(erfc).expect("0.64985 57791 79976");
    TFN(tgamma).expect("2.78663 45408 4547");
    TFN(lgamma).expect("1.02483 46099 5731");
    TFN(gamma).expect("2.78663 45408 4547");
    TFN(cbrt).expect("0.68470 21277 57224");
    TFN(norm).expect("0.321");
#undef TFN
#undef TFNA

    step("pow")
        ,test(CLEAR, "3.21 1.23 pow", ENTER)
        .expect("4.19760 13402 6956")
        .test(CLEAR, "1.23 2.31").shifts(true,false,false,false).test(B)
        .expect("1.61317 24907 5554");

    step("hypot")
        .test(CLEAR, "3.21 1.23 hypot", ENTER)
        .expect("3.43758 63625 5149");

    step("atan2 pos / pos quadrant")
        .test(CLEAR, "3.21 1.23 atan2", ENTER)
        .expect("1.20487 56251 5281");
    step("atan2 pos / neg quadrant")
        .test(CLEAR, "3.21 -1.23 atan2", ENTER)
        .expect("1.93671 70284 3698");
    step("atan2 neg / pos quadrant")
        .test(CLEAR, "-3.21 1.23 atan2", ENTER)
        .expect("-1.20487 56251 5281");
    step("atan2 neg / neg quadrant")
        .test(CLEAR, "-3.21 -1.23 atan2", ENTER)
        .expect("-1.93671 70284 3698");

    step("Restore default 24-digit precision");
    test(CLEAR, "24 PRECISION 12 SIG SoftFP", ENTER).noerror();
}


void tests::high_precision_numerical_functions()
// ----------------------------------------------------------------------------
//   Test high-precision numerical functions
// ----------------------------------------------------------------------------
{
    BEGIN(highp);

    step("Select 120-digit precision");
    test(CLEAR, "120 PRECISION 119 SIG", ENTER).noerror();

    step("Addition")
        .test(CLEAR, "1.23 2.34", NOSHIFT, ADD).expect("3.57")
        .test(CLEAR, "1.23 -2.34", NOSHIFT, ADD).expect("-1.11")
        .test(CLEAR, "-1.23 2.34", NOSHIFT, ADD).expect("1.11")
        .test(CLEAR, "-1.23 -2.34", NOSHIFT, ADD).expect("-3.57")
        .test(CLEAR, "1.234 SIN 2.34", NOSHIFT, ADD).expect("3.28381 82093 74633 70486 17510 06156 82758 95172 14272 07657 60747 22091 17818 71399 90696 80994 83012 59886 50556 27858 44350 79955 18738 767")
        .test(CLEAR, "1.23 COS -2.34", NOSHIFT, ADD).expect("-2.00576 22728 75497 40176 04527 54502 33554 62422 20360 95512 16741 09716 34981 87666 27553 75383 23279 23951 11502 06776 89604 78156 26344 971")
        .test(CLEAR, "-1.23 TAN 2.34", NOSHIFT, ADD).expect("-0.47981 57342 68151 97480 88818 34909 67267 63017 29576 63870 87847 72873 08737 86224 89502 16556 77388 45242 02685 46713 25008 91512 90180 8172")
        .test(CLEAR, "-1.23 TANH -2.34", NOSHIFT, ADD).expect("-3.18257 93256 58929 54289 07208 91501 65091 42132 21054 06082 52654 90143 67515 93012 41309 88423 04706 28583 94673 60063 58625 76729 87437 236");
    step("Subtraction")
        .test(CLEAR, "1.23 2.34", NOSHIFT, SUB).expect("-1.11")
        .test(CLEAR, "1.23 -2.34", NOSHIFT, SUB).expect("3.57")
        .test(CLEAR, "-1.23 2.34", NOSHIFT, SUB).expect("-3.57")
        .test(CLEAR, "-1.23 -2.34", NOSHIFT, SUB).expect("1.11")
        .test(CLEAR, "1.234 SIN 2.34", NOSHIFT, SUB).expect("-1.39618 17906 25366 29513 82489 93843 17241 04827 85727 92342 39252 77908 82181 28600 09303 19005 16987 40113 49443 72141 55649 20044 81261 234")
        .test(CLEAR, "1.23 COS -2.34", NOSHIFT, SUB).expect("2.67423 77271 24502 59823 95472 45497 66445 37577 79639 04487 83258 90283 65018 12333 72446 24616 76720 76048 88497 93223 10395 21843 73655 029")
        .test(CLEAR, "-1.23 TAN 2.34", NOSHIFT, SUB).expect("-5.15981 57342 68151 97480 88818 34909 67267 63017 29576 63870 87847 72873 08737 86224 89502 16556 77388 45242 02685 46713 25008 91512 90180 817")
        .test(CLEAR, "-1.23 TANH -2.34", NOSHIFT, SUB).expect("1.49742 06743 41070 45710 92791 08498 34908 57867 78945 93917 47345 09856 32484 06987 58690 11576 95293 71416 05326 39936 41374 23270 12562 764");
    step("Multiplication")
        .test(CLEAR, "1.23 2.34", NOSHIFT, MUL).expect("2.8782")
        .test(CLEAR, "1.23 -2.34", NOSHIFT, MUL).expect("-2.8782")
        .test(CLEAR, "-1.23 2.34", NOSHIFT, MUL).expect("-2.8782")
        .test(CLEAR, "-1.23 -2.34", NOSHIFT, MUL).expect("2.8782")
        .test(CLEAR, "1.234 SIN 2.34", NOSHIFT, MUL).expect("2.20853 46099 36642 86937 64973 54406 97655 94702 81396 65918 80148 49693 35695 79075 78230 53527 90249 48134 42301 69188 75780 87095 13848 714")
        .test(CLEAR, "1.23 COS -2.34", NOSHIFT, MUL).expect("-0.78211 62814 71336 07988 05405 54464 53482 17932 04355 36501 52825 83263 74142 40860 91524 21603 23526 57954 39085 16142 06324 81114 34352 7674")
        .test(CLEAR, "-1.23 TAN 2.34", NOSHIFT, MUL).expect("-6.59836 88181 87475 62105 27834 93688 63406 25460 47209 33457 85563 68523 02446 59766 25435 06742 85088 97866 34283 99309 00520 86140 19023 112")
        .test(CLEAR, "-1.23 TANH -2.34", NOSHIFT, MUL).expect("1.97163 56220 41895 13036 42868 86113 86313 92589 37266 50233 11212 46936 19987 27649 04665 12909 93012 70886 43536 22548 79184 29547 90603 133");
    step("Division")
        .test(CLEAR, "1.23 2.34", NOSHIFT, DIV).expect("0.52564 10256 41025 64102 56410 25641 02564 10256 41025 64102 56410 25641 02564 10256 41025 64102 56410 25641 02564 10256 41025 64102 56410 2564")
        .test(CLEAR, "1.23 -2.34", NOSHIFT, DIV).expect("-0.52564 10256 41025 64102 56410 25641 02564 10256 41025 64102 56410 25641 02564 10256 41025 64102 56410 25641 02564 10256 41025 64102 56410 2564")
        .test(CLEAR, "-1.23 2.34", NOSHIFT, DIV).expect("-0.52564 10256 41025 64102 56410 25641 02564 10256 41025 64102 56410 25641 02564 10256 41025 64102 56410 25641 02564 10256 41025 64102 56410 2564")
        .test(CLEAR, "-1.23 -2.34", NOSHIFT, DIV).expect("0.52564 10256 41025 64102 56410 25641 02564 10256 41025 64102 56410 25641 02564 10256 41025 64102 56410 25641 02564 10256 41025 64102 56410 2564")
        .test(CLEAR, "1.234 SIN 2.34", NOSHIFT, DIV).expect("0.40334 11151 17364 83113 75004 29981 55025 19304 33449 60537 43909 06876 57187 48461 49870 43160 18381 45250 64340 28999 33483 24767 17409 7293")
        .test(CLEAR, "1.23 COS -2.34", NOSHIFT, DIV).expect("-0.14283 66355 23291 70864 93791 64742 59164 69050 34033 77986 25324 31745 14965 00997 31814 63511 43897 76089 26708 51804 74527 87112 70792 7473")
        .test(CLEAR, "-1.23 TAN 2.34", NOSHIFT, DIV).expect("-1.20504 94590 88953 83538 84110 40559 68917 79067 22041 29859 34977 66185 08007 63343 97223 14767 85208 74035 05421 13980 02140 56202 09478 982")
        .test(CLEAR, "-1.23 TANH -2.34", NOSHIFT, DIV).expect("0.36007 66348 96978 43713 27867 05769 93628 81253 08142 76103 64382 43651 14323 04706 15944 39497 02865 93411 94304 95753 66934 08858 92067 195");
    step("Power")
        .test(CLEAR, "1.23 2.34", LSHIFT, B).expect("1.62322 21516 85370 76170 21776 74374 04103 27090 58024 62880 50736 29360 27592 07917 75146 99083 57726 38100 05735 87359 05132 61280 29729 273")
        .test(CLEAR, "1.23 -2.34", LSHIFT, B).expect("0.61605 86207 88111 35803 50956 46724 98591 90279 99659 77958 49978 01436 78988 97209 72893 73693 48233 61309 17629 97957 78283 38559 84827 6569")
        .test(CLEAR, "-1.23 23", LSHIFT, B).expect("-116.90082 15014 43291 74653 48578 88750 68007 69541 15726 7")
        .test(CLEAR, "-1.23 -2.34", LSHIFT, B).error("Argument outside domain")
        .test(CLEAR, "-1.23 23", LSHIFT, B).expect("-116.90082 15014 43291 74653 48578 88750 68007 69541 15726 7")
        .test(CLEAR, "-1.23 -2.34", LSHIFT, B).error("Argument outside domain")
        .test(CLEAR, "1.234 SIN 2.34", LSHIFT, B).expect("0.87345 13971 11436 95155 06870 44540 70174 27291 82925 84673 60872 62775 48945 10990 94126 48813 44383 61846 88450 45997 75145 12827 34289 0582")
        .test(CLEAR, "1.23 COS -2.34", LSHIFT, B).expect("12.99302 28339 82056 39426 87501 27880 37045 92536 16587 57403 56215 08880 50350 81194 61226 34205 49843 15463 66527 28429 54768 38033 10733 33")
        .test(CLEAR, "-1.23 TAN 23", LSHIFT, B).expect("-2.26504 47100 36734 53632 11380 88267 73995 83095 30275 90565 69960 79911 60281 89036 12608 17378 72500 95112 47589 25610 99723 61528 46412 821⁳¹⁰")
        .test(CLEAR, "-1.23 TAN 2.34", LSHIFT, B).error("Argument outside domain")
        .test(CLEAR, "-1.23 TAN -23", LSHIFT, B).expect("-4.41492 38890 02535 32657 39183 33114 42610 79161 90457 07890 27869 50941 95017 26203 95996 17209 38898 89303 26193 59642 46151 77992 62440 313⁳⁻¹¹")
        .test(CLEAR, "-1.23 TANH -2.34", LSHIFT, B).error("Argument outside domain");

    step("Square root of 2")
        .test(CLEAR, "2 sqrt", ENTER)
        .expect("1.41421 35623 73095 04880 16887 24209 69807 85696 71875 37694 80731 76679 73799 07324 78462 10703 88503 87534 32764 15727 35013 84623 09122 97");
    step("Square root of 3")
        .test(CLEAR, "3 sqrt", ENTER)
        .expect("1.73205 08075 68877 29352 74463 41505 87236 69428 05253 81038 06280 55806 97945 19330 16908 80003 70811 46186 75724 85756 75626 14141 54067 03");
    step("Square root of 4")
        .test(CLEAR, "4 sqrt", ENTER)
        .expect("2.");
    step("Cube root of 2")
        .test(CLEAR, "2 cbrt", ENTER)
        .expect("1.25992 10498 94873 16476 72106 07278 22835 05702 51464 70150 79800 81975 11215 52996 76513 95948 37293 96562 43625 50941 54310 25603 56156 653");
    step("Cube root of 3")
        .test(CLEAR, "3 cbrt", ENTER)
        .expect("1.44224 95703 07408 38232 16383 10780 10958 83918 69253 49935 05775 46416 19454 16875 96829 99733 98547 55479 70564 52566 86835 08085 44895 5");
    step("Cube root of 27")
        .test(CLEAR, "27 cbrt", ENTER)
        .expect("3.");

    step("neg")
        .test(CLEAR, "3.21 neg", ENTER).expect("-3.21")
        .test("negate", ENTER).expect("3.21");
    step("inv")
        .test(CLEAR, "3.21 inv", ENTER)
        .expect("0.31152 64797 50778 81619 93769 47040 49844 23676 01246 10591 90031 15264 79750 77881 61993 76947 04049 84423 67601 24610 59190 03115 26479 7508")
        .test("inv", ENTER).expect("3.21");
    step("sq (square)")
        .test(CLEAR, "-3.21 sq", ENTER).expect("10.3041")
        .test("sq", ENTER).expect("106.17447 681");
    step("cubed")
        .test(CLEAR, "3.21 cubed", ENTER).expect("33.07616 1")
        .test("cubed", ENTER).expect("36 186.39267 80659 01161 281")
        .test(CLEAR, "-3 cubed", ENTER).expect("-27")
        .test("cubed", ENTER).expect("-19 683");
    step("abs")
        .test(CLEAR, "-3.21 abs", ENTER).expect("3.21")
        .test("abs", ENTER, 1, ADD).expect("4.21");

    step("Setting radians mode");
    test(CLEAR, "RAD", ENTER).noerror();

#define TFNA(name, arg)         step(#name).test(CLEAR, #arg " " #name, ENTER)
#define TFN(name)               TFNA(name, 0.321)

    TFN(sqrt).expect("0.56656 86189 68611 77992 54734 04696 76902 95391 98874 84029 02431 74015 07100 23314 25810 89388 23378 74831 09026 25322 95207 15522 13334 6095");
    TFN(sin).expect("0.31551 56385 92727 11130 65931 11143 46372 42059 02807 32616 09042 60788 57395 26113 45495 81136 00515 46916 99088 86060 26856 68677 57717 8756");
    TFN(cos).expect("0.94892 03769 56583 01754 39451 32826 92551 54763 03148 22817 38878 87425 10454 37289 66657 74827 69303 30686 52796 72622 06704 70515 68539 2249");
    TFN(tan).expect("0.33249 95924 36471 87510 87087 30102 73796 83946 23980 80503 83311 21021 12491 95974 29552 25917 15859 83641 11747 44032 23741 83994 87487 0292");
    TFN(asin).expect("0.32678 51765 31495 46326 91997 64519 59826 36182 58080 21574 39673 71903 92028 08817 69439 28409 80968 96776 17859 18932 19725 72513 95078 4427 r");
    TFN(acos).expect("1.24401 11502 63401 15596 21219 27120 15317 84803 26619 47180 89431 15568 37587 30264 33703 82040 12171 20636 49246 66407 71348 31811 71283 483 r");
    TFN(atan).expect("0.31060 97928 13889 91760 67000 51446 83602 81125 07025 77281 14539 44776 64690 76612 68860 40731 31597 84656 31883 84021 79831 76697 34106 3622 r");
    TFN(sinh).expect("0.32654 11649 51806 35701 22065 63885 73434 59869 32810 98627 21625 46131 20539 70600 10083 27315 63713 66136 47461 26495 76415 60697 57676 2937");
    TFN(cosh).expect("1.05196 44159 41947 53843 52241 43605 67798 60702 39830 04737 76342 59201 97569 28172 48173 45468 64605 47110 19220 77704 23747 11369 53013 732");
    TFN(tanh).expect("0.31041 08466 05886 02148 50502 09383 09588 97683 04936 20954 99090 64143 22194 05034 27301 10326 36239 07947 98201 74192 58627 58374 14428 5902");
    TFN(asinh).expect("0.31572 82658 29379 61791 08945 47102 06380 00526 27320 40054 59952 39850 65785 93616 95975 70753 88242 69995 19084 50283 99306 71224 23629 0976");
    TFNA(acosh, 1.321).expect("0.78123 02051 96252 61474 22171 61603 43488 77028 85612 70883 33986 53192 83139 13864 10921 83081 88302 58903 47353 53634 04169 89742 02815 2861");
    TFN(atanh).expect("0.33276 15884 81814 59580 17641 70508 75106 43974 10006 34850 01665 72697 61781 57932 14419 67812 59706 77324 50200 63307 05966 90651 74209 3103");
    TFN(log1p).expect("0.27838 90255 40188 26677 16283 42111 55094 94375 15179 05132 39494 81036 05142 66257 54337 55520 43633 04277 35736 38433 06042 83576 22139 6357");
    TFN(lnp1).expect("0.27838 90255 40188 26677 16283 42111 55094 94375 15179 05132 39494 81036 05142 66257 54337 55520 43633 04277 35736 38433 06042 83576 22139 6357");
    TFN(expm1).expect("0.37850 55808 93753 89544 74307 07491 41233 20571 72641 03364 97968 05333 18108 98772 58256 72784 28319 13246 66682 04200 00162 72067 10690 0254");
    TFN(log).expect("-1.13631 41558 52121 18735 43303 10107 28991 65926 67631 93216 19228 05172 65001 85061 66283 45581 72770 57156 95345 21563 26917 04911 30388 597");
    TFN(log10).expect("-0.49349 49675 95127 92187 04308 57283 44904 46730 54244 17528 47831 88472 35123 39989 07607 74010 64305 99151 74781 24152 01829 22941 99221 5486");
    TFN(exp).expect("1.37850 55808 93753 89544 74307 07491 41233 20571 72641 03364 97968 05333 18108 98772 58256 72784 28319 13246 66682 04200 00162 72067 10690 025");
    TFN(exp10).expect("2.09411 24558 50892 67051 98819 85846 25435 50121 44808 82328 80597 04327 54118 26943 97658 88916 82284 18499 99928 85620 51265 40190 16492 154");
    TFN(exp2).expect("1.24919 61256 53376 70052 14667 82085 80659 83711 96789 11078 50872 03968 89639 54927 57400 23696 00219 70718 47302 80643 90803 89872 28867 485");
    TFN(erf).expect("0.35014 42208 20023 82355 16032 45050 23912 83120 71924 29072 35684 90423 15676 68631 26483 67740 59618 93127 36786 06239 23468 00013 58887 2181");
    TFN(erfc).expect("0.64985 57791 79976 17644 83967 54949 76087 16879 28075 70927 64315 09576 84323 31368 73516 32259 40381 06872 63213 93760 76531 99986 41112 7819");
    TFN(tgamma).expect("2.78663 45408 45472 36795 07642 12781 77275 03497 82995 16602 55760 07828 51424 44941 90542 89306 12905 33223 77665 62678 93736 34160 48127 165", 20000);
    TFN(lgamma).wait(500).expect("1.02483 46099 57313 19869 10927 53834 88666 18028 66769 43209 08437 87004 46327 04911 25770 09539 00530 12325 23947 42518 21539 89107 12509 699");
    TFN(gamma).wait(500).expect("2.78663 45408 45472 36795 07642 12781 77275 03497 82995 16602 55760 07828 51424 44941 90542 89306 12905 33223 77665 62678 93736 34160 48127 165");
    TFN(cbrt).expect("0.68470 21277 57224 16184 09277 32646 81496 28057 14749 53139 45950 35873 52977 73009 35191 71304 84396 28932 73625 07589 02266 77954 73690 2353");
    TFN(norm).expect("0.321");
#undef TFN
#undef TFNA

    step("pow")
        ,test(CLEAR, "3.21 1.23 pow", ENTER)
        .expect("4.19760 13402 69557 03133 41557 04388 71185 62403 13482 15741 54975 76397 39514 93831 64438 34447 96787 36431 56648 68643 95471 93476 15863 225")
        .test(CLEAR, "1.23 2.31").shifts(true,false,false,false).test(B)
        .expect("1.61317 24907 55543 84434 14148 92337 98559 17006 64245 18957 27180 28125 67872 74870 17458 75459 57723 53996 95111 93456 40634 86700 09601 018");

    step("hypot")
        .test(CLEAR, "3.21 1.23 hypot", ENTER)
        .expect("3.43758 63625 51492 31996 16557 32945 23541 88726 55087 78271 21507 69382 98782 20308 03280 97137 37583 47164 32055 25578 11148 26146 57350 441");

    step("atan2 pos / pos quadrant")
        .test(CLEAR, "3.21 1.23 atan2", ENTER)
        .expect("1.20487 56251 52809 23400 86691 05495 30674 32743 54426 68497 01001 78719 37086 47165 61508 05592 53255 02332 28917 23139 67613 92267 03142 769");
    step("atan2 pos / neg quadrant")
        .test(CLEAR, "3.21 -1.23 atan2", ENTER)
        .expect("1.93671 70284 36984 00445 39742 77784 19614 09228 14972 69013 57207 96225 22144 30998 44778 15307 33025 32493 05294 47540 14534 16384 29680 297");
    step("atan2 neg / pos quadrant")
        .test(CLEAR, "-3.21 1.23 atan2", ENTER)
        .expect("-1.20487 56251 52809 23400 86691 05495 30674 32743 54426 68497 01001 78719 37086 47165 61508 05592 53255 02332 28917 23139 67613 92267 03142 769");
    step("atan2 neg / neg quadrant")
        .test(CLEAR, "-3.21 -1.23 atan2", ENTER)
        .expect("-1.93671 70284 36984 00445 39742 77784 19614 09228 14972 69013 57207 96225 22144 30998 44778 15307 33025 32493 05294 47540 14534 16384 29680 297");

    step("Restore default 24-digit precision");
    test(CLEAR, "24 PRECISION 12 SIG", ENTER).noerror();
}


void tests::exact_trig_cases()
// ----------------------------------------------------------------------------
//   Special trig cases that are handled accurately for polar representation
// ----------------------------------------------------------------------------
{
    BEGIN(trigoptim);

    cstring unit_names[] = { "Grads", "Degrees", "PiRadians" };
    int circle[] = { 400, 360, 2 };

    step("Switch to big fractions")
        .test("BigFractions", ENTER).noerror();

    for (uint unit = 0; unit < 3; unit++)
    {
        step(unit_names[unit]);
        test(CLEAR, unit_names[unit], ENTER).noerror();

        int base = ((lrand48() & 0xFF) - 0x80) * 360;
        char buf[80];
        snprintf(buf, sizeof(buf),
                 "Selecting base %d degrees for %s angles",
                 base, unit_names[unit]);
        step(buf);
        test(CLEAR, base, ENTER, 360, " mod", ENTER).expect("0");
        test(CLEAR, base, ENTER, circle[unit], MUL, 360, DIV,
             circle[unit], " mod", ENTER).expect("0");

        step("sin(0) = 0")
            .test(base + 0, ENTER, circle[unit], MUL, 360, DIV, SIN)
            .expect("0");
        step("cos(0) = 1")
            .test(base + 0, ENTER, circle[unit], MUL, 360, DIV, COS)
            .expect("1");
        step("tan(0) = 0")
            .test(base + 0, ENTER, circle[unit], MUL, 360, DIV, TAN)
            .expect("0");

        step("sin(30) = 1/2")
            .test(base + 30, ENTER, circle[unit], MUL, 360, DIV, SIN)
            .expect("1/2");
        step("tan(45) = 1")
            .test(base + 45, ENTER, circle[unit], MUL, 360, DIV, TAN)
            .expect("1");
        step("cos(60) = 1/2")
            .test(base + 60, ENTER, circle[unit], MUL, 360, DIV, COS)
            .expect("1/2");

        step("sin(90) = 1")
            .test(base + 90, ENTER, circle[unit], MUL, 360, DIV, SIN)
            .expect("1");
        step("cos(90) = 0")
            .test(base + 90, ENTER, circle[unit], MUL, 360, DIV, COS)
            .expect("0");

        step("cos(120) = -1/2")
            .test(base + 120, ENTER, circle[unit], MUL, 360, DIV, COS)
            .expect("-1/2");
        step("tan(135) = -1")
            .test(base + 135, ENTER, circle[unit], MUL, 360, DIV, TAN)
            .expect("-1");
        step("sin(150) = 1/2")
            .test(base + 150, ENTER, circle[unit], MUL, 360, DIV, SIN)
            .expect("1/2");

        step("sin(180) = 0")
            .test(base + 180, ENTER, circle[unit], MUL, 360, DIV, SIN)
            .expect("0");
        step("cos(180) = -1")
            .test(base + 180, ENTER, circle[unit], MUL, 360, DIV, COS)
            .expect("-1");
        step("tan(180) = 0")
            .test(base + 180, ENTER, circle[unit], MUL, 360, DIV, TAN)
            .expect("0");

        step("sin(210) = -1/2")
            .test(base + 210, ENTER, circle[unit], MUL, 360, DIV, SIN)
            .expect("-1/2");
        step("tan(225) = 1")
            .test(base + 225, ENTER, circle[unit], MUL, 360, DIV, TAN)
            .expect("1");
        step("cos(240) = -1/2")
            .test(base + 240, ENTER, circle[unit], MUL, 360, DIV, COS)
            .expect("-1/2");

        step("sin(270) = -1")
            .test(base + 270, ENTER, circle[unit], MUL, 360, DIV, SIN)
            .expect("-1");
        step("cos(270) = 0")
            .test(base + 270, ENTER, circle[unit], MUL, 360, DIV, COS)
            .expect("0");

        step("cos(300) = 1/2")
            .test(base + 300, ENTER, circle[unit], MUL, 360, DIV, COS)
            .expect("1/2");
        step("tan(315) = -1")
            .test(base + 315, ENTER, circle[unit], MUL, 360, DIV, TAN)
            .expect("-1");
        step("sin(330) = -1/2")
            .test(base + 330, ENTER, circle[unit], MUL, 360, DIV, SIN)
            .expect("-1/2");
    }

    step("Cleaning up")
        .test(CLEAR, "SmallFractions DEG", ENTER).noerror();
}


void tests::fraction_decimal_conversions()
// ----------------------------------------------------------------------------
//   Exercise the conversion from decimal to fraction and back
// ----------------------------------------------------------------------------
{
    cstring cases[] =
        {
            // Easy exact cases (decimal)
            "1/2",          "0.5",
            "1/4",          "0.25",
            "5/4",          "1.25",
            "-5/4",         "-1.25",

            // More tricky fractions
            "1/3",          "0.33333 33333 33",
            "-1/7",         "-0.14285 71428 57",
            "22/7",         "3.14285 71428 6",
            "37/213",       "0.17370 89201 88",
        };

    BEGIN(dfrac);

    step("Selecting big mixed fraction mode")
        .test(CLEAR, "BigFractions ImproperFractions", ENTER).noerror();

    for (uint c = 0; c < sizeof(cases) / sizeof(*cases); c += 2)
    {
        step(cases[c]);
        test(CLEAR, cases[c], ENTER).expect(cases[c]);
        test("→Num", ENTER).expect(cases[c+1]);
        test("→Q", ENTER).expect(cases[c]);
    }

    step("Alternate spellings");
    test(CLEAR, "1/4 →Decimal", ENTER).expect("0.25");
    test(CLEAR, "1/5 ToDecimal", ENTER).expect("0.2");
    test(CLEAR, "0.25 →Frac", ENTER).expect("1/4");
    test(CLEAR, "0.2 ToFraction", ENTER).expect("1/5");

    step("Complex numbers");
    test(CLEAR, "1-2ⅈ 4", ENTER, DIV).expect("1/4-1/2ⅈ");
    test("→Num", ENTER).expect("0.25-0.5ⅈ");
    test("→Q", ENTER).expect("1/4-1/2ⅈ");

    step("Vectors");
    test(CLEAR, "[1-2ⅈ 3] 4", ENTER, DIV).expect("[ 1/4-1/2ⅈ 3/4 ]");
    test("→Num", ENTER).expect("[ 0.25-0.5ⅈ 0.75 ]");
    test("→Q", ENTER).expect("[ 1/4-1/2ⅈ 3/4 ]");

    step("Expressions");
    test(CLEAR, "355 113 /",
         LSHIFT, I, F1, F1, "-", ENTER) .expect("'355/113-π'");
    test("→Num", ENTER).expect("0.00000 02667 64");

    step("Restoring small fraction mode")
        .test(CLEAR, "SmallFractions MixedFractions", ENTER).noerror();
}


void tests::trig_units()
// ----------------------------------------------------------------------------
//   Check trigonometric units
// ----------------------------------------------------------------------------
{
    BEGIN(trigunits);

    step("Select degrees mode")
        .test(CLEAR, LSHIFT, N, F1).noerror();
    step("Disable trig units mode")
        .test("NoAngleUnits", ENTER).noerror();
    step("Check that arc-sin produces numerical value")

        .test(CLEAR, "0.2", LSHIFT, J)
        .noerror()
        .type(object::ID_decimal)
        .expect("11.53695 90328");
    step("Check that arc-sin numerical value depends on angle mode")
        .test(CLEAR, LSHIFT, N, F2)
        .test("0.2", LSHIFT, J)
        .noerror()
        .type(object::ID_decimal)
        .expect("0.20135 79207 9");

    step("Enable trig units mode")
        .test("SetAngleUnits", ENTER).noerror();
    step("Select degrees mode")
        .test(CLEAR, LSHIFT, N, F1).noerror();
    step("Check that arc-sin produces unit value with degrees")

        .test("0.2", LSHIFT, J)
        .noerror()
        .type(object::ID_unit)
        .expect("11.53695 90328 °");
    step("Check that arc-sin produces radians unit")
        .test(F2)
        .test("0.2", LSHIFT, J)
        .noerror()
        .type(object::ID_unit)
        .expect("0.20135 79207 9 r");
    step("Check that arc-sin produces pi-radians unit")
        .test(F3)
        .test("0.2", LSHIFT, J)
        .noerror()
        .type(object::ID_unit)
        .expect("0.06409 42168 49 πr");
    step("Check that arc-sin produces grads unit")
        .test(LSHIFT, F1)
        .test("0.2", LSHIFT, J)
        .noerror()
        .type(object::ID_unit)
        .expect("12.81884 33698 grad");

    step("Check that grad value is respected in degrees")
        .test(F1, J)
        .expect("0.2")
        .test(BSP);
    step("Check that pi-radians value is respected in grads")
        .test(SHIFT, F1, J)
        .expect("0.2")
        .test(BSP);
    step("Check that radians value is respected in degrees")
        .test(F1, J)
        .expect("0.2")
        .test(BSP);
    step("Check that degrees value is respected in degrees")
        .test(F1, J)
        .expect("0.2")
        .test(BSP);

    step ("Numerical conversion from degrees to radians")
        .test(CLEAR, "1.2 R→D", ENTER).noerror().expect("68.75493 54157");
    step ("Symbolic conversion from degrees to radians")
        .test(CLEAR, "'X' R→D", ENTER).noerror().expect("'57.29577 95131·X'");
    step ("Numerical conversion from radians to degrees")
        .test(CLEAR, "1.2 D→R", ENTER).noerror().expect("0.02094 39510 24");
    step ("Symbolic conversion from radians to degrees")
        .test(CLEAR, "'X' D→R", ENTER).noerror().expect("'0.01745 32925 2·X'");

    step("Select degrees mode")
        .test(CLEAR, LSHIFT, N, LSHIFT, F2, F1).noerror();
    step("Numerical conversion to degrees in degrees mode")
        .test("1.2", LSHIFT, F1).expect("1.2 °");
    step("Numerical conversion to radians in degrees mode")
        .test("1.2", LSHIFT, F2).expect("0.02094 39510 24 r");
    step("Numerical conversion to grad in degrees mode")
        .test("1.2", LSHIFT, F3).expect("1.33333 33333 3 grad");
    step("Numerical conversion to pi-radians in degrees mode")
        .test("1.2", LSHIFT, F4).expect("0.00666 66666 67 πr");

    step("Select radians mode")
        .test(CLEAR, LSHIFT, N, LSHIFT, F2, F2).noerror();
    step("Numerical conversion to degrees in radians mode")
        .test("1.2", LSHIFT, F1).expect("68.75493 54157 °");
    step("Numerical conversion to radians in radians mode")
        .test("1.2", LSHIFT, F2).expect("1.2 r");
    step("Numerical conversion to grad in radians mode")
        .test("1.2", LSHIFT, F3).expect("76.39437 26841 grad");
    step("Numerical conversion to pi-radians in radians mode")
        .test("1.2", LSHIFT, F4).expect("0.38197 18634 21 πr");

    step("Select grads mode")
        .test(CLEAR, LSHIFT, N, LSHIFT, F2, F3).noerror();
    step("Numerical conversion to degrees in grads mode")
        .test("1.2", LSHIFT, F1).expect("1.08 °");
    step("Numerical conversion to radians in grads mode")
        .test("1.2", LSHIFT, F2).expect("0.01884 95559 22 r");
    step("Numerical conversion to grad in grads mode")
        .test("1.2", LSHIFT, F3).expect("1.2 grad");
    step("Numerical conversion to pi-radians in grads mode")
        .test("1.2", LSHIFT, F4).expect("0.006 πr");

    step("Select pi-radians mode")
        .test(CLEAR, LSHIFT, N, LSHIFT, F2, F4).noerror();
    step("Numerical conversion to degrees in pi-radians mode")
        .test("1.2", LSHIFT, F1).expect("216. °");
    step("Numerical conversion to radians in pi-radians mode")
        .test("1.2", LSHIFT, F2).expect("3.76991 11843 1 r");
    step("Numerical conversion to grad in pi-radians mode")
        .test("1.2", LSHIFT, F3).expect("240. grad");
    step("Numerical conversion to pi-radians in pi-radians mode")
        .test("1.2", LSHIFT, F4).expect("1.2 πr");

    step("Selecting degrees")
        .test(CLEAR, LSHIFT, N, LSHIFT, F2, F1).noerror();
    step("Creating a degrees value")
        .test(CLEAR, "1/2", LSHIFT, F1).expect("¹/₂ °");
    step("Converting to grad")
        .test(LSHIFT, F3).expect("⁵/₉ grad");
    step("Converting to pi-radians")
        .test(LSHIFT, F4).expect("¹/₃₆₀ πr");
    step("Converting to degrees")
        .test(LSHIFT, F1).expect("¹/₂ °");
    step("Converting to radians")
        .test(LSHIFT, F2).expect("0.00872 66462 6 r");
    step("Converting to degrees")
        .test(LSHIFT, F1).expect("0.5 °");
}


void tests::complex_types()
// ----------------------------------------------------------------------------
//   Complex data typess
// ----------------------------------------------------------------------------
{
    BEGIN(ctypes);

    step("Select degrees for the angle");
    test(CLEAR, "DEG", ENTER).noerror();

    step("Integer rectangular form");
    test(CLEAR, "0ⅈ0", ENTER)
        .type(object::ID_rectangular).expect("0+0ⅈ");
    test(CLEAR, "1ⅈ2", ENTER)
        .type(object::ID_rectangular).expect("1+2ⅈ");
    test(CLEAR, "3+ⅈ4", ENTER)
        .type(object::ID_rectangular).expect("3+4ⅈ")
        .test(DOWN, ENTER)
        .type(object::ID_rectangular).expect("3+4ⅈ");
    test("ComplexIBeforeImaginary", ENTER)
        .type(object::ID_rectangular).expect("3+ⅈ4");
    test("ComplexIAfterImaginary", ENTER)
        .type(object::ID_rectangular).expect("3+4ⅈ");

    step("Behaviour of CHS on command-line");
    test(CLEAR, "4+ⅈ5", CHS, ENTER)
        .type(object::ID_rectangular).expect("4-5ⅈ");
    test(CLEAR, "5", CHS, "ⅈ6", CHS, ENTER)
        .type(object::ID_rectangular).expect("-5-6ⅈ");
    test(CLEAR, "6+7ⅈ", ENTER)
        .type(object::ID_rectangular).expect("6+7ⅈ");
    test(CLEAR, "7-8ⅈ", ENTER)
        .type(object::ID_rectangular).expect("7-8ⅈ");

    step("Integer polar form");
    test(CLEAR, "0∡0", ENTER)
        .type(object::ID_polar).expect("0∡0°")
        .test(DOWN, ENTER)
        .type(object::ID_polar).expect("0∡0°");
    test(CLEAR, "1∡90", ENTER)
        .type(object::ID_polar).expect("1∡90°")
        .test(DOWN, ENTER)
        .type(object::ID_polar).expect("1∡90°");
    test(CLEAR, "1∡-90", ENTER)
        .type(object::ID_polar).expect("1∡-90°")
        .test(DOWN, ENTER)
        .type(object::ID_polar).expect("1∡-90°");
    test(CLEAR, "-1∡0", ENTER)
        .type(object::ID_polar).expect("1∡180°");

    step("Decimal rectangular form");
    test(CLEAR, "0.1ⅈ2.3", ENTER)
        .type(object::ID_rectangular).expect("0.1+2.3ⅈ");
    test(CLEAR, "0.1ⅈ2.3", CHS, ENTER)
        .type(object::ID_rectangular).expect("0.1-2.3ⅈ");

    step("Decimal polar form");
    test(CLEAR, "0.1∡2.3", ENTER)
        .type(object::ID_polar).expect("0.1∡2.3°");
    test(CLEAR, "0.1∡2.3", CHS, ENTER)
        .type(object::ID_polar).expect("0.1∡-2.3°");

    step("Symbolic rectangular form");
    test(CLEAR, "aⅈb", ENTER)
        .type(object::ID_rectangular).expect("a+bⅈ");
    test(CLEAR, "c+dⅈ", ENTER)
        .type(object::ID_rectangular).expect("c+dⅈ");

    step("Symbolic polar form");
    test(CLEAR, "a∡b", ENTER)
        .type(object::ID_polar).expect("a∡b");
    test(CLEAR, "c∡d", ENTER)
        .type(object::ID_polar).expect("c∡d");

    step("Polar angle conversions");
    test(CLEAR, "1∡90", ENTER).expect("1∡90°");
    test("GRAD", ENTER).expect("1∡100ℊ");
    test("PiRadians", ENTER).expect("1∡¹/₂ℼ");
    test("RAD", ENTER).expect("1∡1.57079 63267 9ʳ");

    step("Convert real to rectangular");
    test(CLEAR, "1 2", LSHIFT, G, F3)
        .type(object::ID_rectangular)
        .expect("1+2ⅈ");
    test(CLEAR, "1.2 3.4", LSHIFT, G, F3)
        .type(object::ID_rectangular)
        .expect("1.2+3.4ⅈ");

    step("Convert rectangular to real");
    test(CLEAR, "1ⅈ2", LSHIFT, G, F4)
        .type(object::ID_tag)
        .expect("im:2")
        .test(NOSHIFT, BSP)
        .type(object::ID_tag)
        .expect("re:1");
    test(CLEAR, "1.2ⅈ3.4", LSHIFT, G, F4)
        .type(object::ID_tag)
        .expect("im:3.4")
        .test(NOSHIFT, BSP)
        .type(object::ID_tag)
        .expect("re:1.2");

    step("Convert real to rectangular and back (strip tags)");
    test(CLEAR, "1 2", LSHIFT, G, F3)
        .type(object::ID_rectangular)
        .expect("1+2ⅈ")
        .test(F4).expect("im:2")
        .test(F3).expect("1+2ⅈ");
    test(CLEAR, "1.2 3.4", LSHIFT, G, F3)
        .type(object::ID_rectangular)
        .expect("1.2+3.4ⅈ")
        .test(F4).expect("im:3.4")
        .test(F3).expect("1.2+3.4ⅈ");

    step("Convert real to polar");
    test(CLEAR, LSHIFT, N, F1).noerror();
    test(CLEAR, "1 2", LSHIFT, G, RSHIFT, F3)
        .type(object::ID_polar)
        .expect("1∡2°");
    test(CLEAR, "1.2 3.4", LSHIFT, G, RSHIFT, F3)
        .type(object::ID_polar)
        .expect("1.2∡3.4°");

    step("Convert polar to real");
    test(CLEAR, "1∡2", LSHIFT, G, RSHIFT, F4)
        .type(object::ID_tag)
        .expect("arg:2 °")
        .test(NOSHIFT, BSP)
        .type(object::ID_tag)
        .expect("mod:1");
    test(CLEAR, "1.2∡3.4", LSHIFT, G, RSHIFT, F4)
        .type(object::ID_tag)
        .expect("arg:3.4 °")
        .test(NOSHIFT, BSP)
        .type(object::ID_tag)
        .expect("mod:1.2");

    step("Convert real to polar and back (add units, strip tags)");
    test(CLEAR, LSHIFT, N, F1).noerror();
    test(CLEAR, "1 2", LSHIFT, G, RSHIFT, F3)
        .type(object::ID_polar)
        .expect("1∡2°")
        .test(RSHIFT, F4).expect("arg:2 °")
        .test("RAD", ENTER).expect("arg:2 °")
        .test(RSHIFT, F3).expect("1∡0.03490 65850 4ʳ")
        .test("DEG", ENTER).expect("1∡2°");
    test(CLEAR, "1.2 3.4", LSHIFT, G, RSHIFT, F3)
        .type(object::ID_polar)
        .expect("1.2∡3.4°")
        .test(RSHIFT, F4).expect("arg:3.4 °")
        .test("RAD", ENTER).expect("arg:3.4 °")
        .test(RSHIFT, F3).expect("1.2∡0.05934 11945 68ʳ")
        .test("DEG", ENTER).expect("1.2∡3.4°");
}


void tests::complex_arithmetic()
// ----------------------------------------------------------------------------
//   Complex arithmetic operations
// ----------------------------------------------------------------------------
{
    BEGIN(carith);

    step("Use degrees");
    test("DEG", ENTER).noerror();

    step("Addition");
    test(CLEAR, "1ⅈ2", ENTER, "3+ⅈ4", ENTER, ADD)
        .type(object::ID_rectangular).expect("4+6ⅈ");
    step("Subtraction");
    test("1-2ⅈ", SUB)
        .type(object::ID_rectangular).expect("3+8ⅈ");
    step("Multiplication");
    test("7+8ⅈ", MUL)
        .type(object::ID_rectangular).expect("-43+80ⅈ");
    step("Division");
    test("7+8ⅈ", DIV)
        .type(object::ID_rectangular).expect("3+8ⅈ");
    test("2+3ⅈ", DIV)
        .type(object::ID_rectangular).expect("2 ⁴/₁₃+⁷/₁₃ⅈ");
    test("2+3ⅈ", MUL)
        .type(object::ID_rectangular).expect("3+8ⅈ");
    step("Power");
    test("5", SHIFT, B)
        .type(object::ID_rectangular).expect("44 403-10 072ⅈ");

    step("Symbolic addition");
    test(CLEAR, "a+bⅈ", ENTER, "c+dⅈ", ADD)
        .expect("'a+c'+'b+d'ⅈ");
    step("Symbolic subtraction");
    test(CLEAR, "a+bⅈ", ENTER, "c+dⅈ", SUB)
        .expect("'a-c'+'b-d'ⅈ");
    step("Symbolic multiplication");
    test(CLEAR, "a+bⅈ", ENTER, "c+dⅈ", MUL)
        .expect("'a·c-b·d'+'a·d+b·c'ⅈ");
    step("Symbolic division");
    test(CLEAR, "a+bⅈ", ENTER, "c+dⅈ", DIV)
        .expect("'(a·c+b·d)÷(c²+d²)'+'(b·c-a·d)÷(c²+d²)'ⅈ");

    step("Addition in aligned polar form");
    test(CLEAR, "1∡2", ENTER, "3∡2", ENTER, ADD)
        .expect("4∡2°");
    step("Subtraction in aligned polar form");
    test("1∡2", SUB)
        .expect("3∡2°");
    test("5∡2", SUB)
        .expect("2∡-178°");
    step("Addition in polar form");
    test(CLEAR, "1∡2", ENTER, "3∡4", ENTER, ADD)
        .expect("3.99208 29778+0.24416 89179 35ⅈ");
    step("Subtraction");
    test("1∡2", SUB)
        .expect("2.99269 21507 8+0.20926 94212 32ⅈ");
    step("Multiplication");
    test("7∡8", MUL)
        .expect("21.∡12.°");
    step("Division");
    test("7∡8", DIV)
        .expect("3.∡4.°");
    test("2∡3", DIV)
        .expect("1.5∡1.°");
    test("2∡3", MUL)
        .expect("3.∡4.°");
    step("Power");
    test("5", SHIFT, B)
        .expect("243.∡20.°");

    step("Symbolic addition aligned");
    test(CLEAR, "a∡b", ENTER, "c∡b", ENTER, ADD)
        .expect("'a+c'∡b");
    step("Symbolic addition");
    test(CLEAR, "a∡b", ENTER, "c∡d", ENTER, ADD)
        .expect("'a·cos b+c·cos d'+'a·sin b+c·sin d'ⅈ");
    step("Symbolic substraction aligned");
    test(CLEAR, "a∡b", ENTER, "c∡b", ENTER, SUB)
        .expect("'a-c'∡b");
    step("Symbolic subtraction");
    test(CLEAR, "a∡b", ENTER, "c∡d", ENTER, SUB)
        .expect("'a·cos b-c·cos d'+'a·sin b-c·sin d'ⅈ");
    step("Symbolic multiplication");
    test(CLEAR, "a∡b", ENTER, "c∡d", ENTER, MUL)
        .expect("'a·c'∡'b+d'");
    step("Symbolic division");
    test(CLEAR, "a∡b", ENTER, "c∡d", ENTER, DIV)
        .expect("'a÷c'∡'b-d'");

    step("Precedence of complex numbers during rendering");
    test(CLEAR, "'2+3ⅈ' '3∡4' *", ENTER)
        .expect("'(2+3ⅈ)·(3∡4°)'");
    test(CLEAR, "'2+3ⅈ' '3∡4' +", ENTER)
        .expect("'(2+3ⅈ)+(3∡4°)'");
    test(CLEAR, "'2+3ⅈ' '3∡4' -", ENTER)
        .expect("'(2+3ⅈ)-(3∡4°)'");

    step("Do not promote symbols to complex");
    test(CLEAR, "2+3ⅈ 'A' +", ENTER)
        .expect("'(2+3ⅈ)+A'");
}


void tests::complex_functions()
// ----------------------------------------------------------------------------
//   Complex functions
// ----------------------------------------------------------------------------
{
    BEGIN(cfunctions);

    step("Select 34-digit precision to match Intel Decimal 128");
    test(CLEAR, "34 PRECISION 20 SIG", ENTER).noerror();

    step("Using radians");
    test(CLEAR, "RAD", ENTER).noerror();

    step("Square root (optimized negative case)");
    test(CLEAR, "-1ⅈ0", ENTER, SQRT).expect("0+1.ⅈ");
    test(CLEAR, "-4ⅈ0", ENTER, SQRT).expect("0+2.ⅈ");

    step("Square root (optimized positive case)");
    test(CLEAR, "1ⅈ0", ENTER, SQRT).expect("1.+0ⅈ");
    test(CLEAR, "4ⅈ0", ENTER, SQRT).expect("2.+0ⅈ");

    step("Square root (disable optimization for symbols)");
    test(CLEAR, "aⅈ0", ENTER, SQRT).expect("'√((a⊿0+a)÷2)'+'√((a⊿0-a)÷2)'ⅈ");

    step("Square");
    test(CLEAR, "1+2ⅈ", ENTER, SHIFT, SQRT).expect("-3+4ⅈ");

    step("Square root");
    test(SQRT).expect("1.+2.ⅈ");

    step("Negate");
    test(CLEAR, "1+2ⅈ", ENTER, CHS).expect("-1-2ⅈ");
    test(CHS).expect("1+2ⅈ");

    step("Invert");
    test(CLEAR, "3+7ⅈ", ENTER, INV).expect("³/₅₈-⁷/₅₈ⅈ");
    test("58", MUL).expect("3-7ⅈ");
    test(INV).expect("³/₅₈+⁷/₅₈ⅈ");

    step("Symbolic sqrt");
    test(CLEAR, "aⅈb", ENTER, SQRT)
        .expect("'√((a⊿b+a)÷2)'+'sign (√((a⊿b-a)÷2))·√((a⊿b-a)÷2)'ⅈ");

    step("Symbolic sqrt in polar form");
    test(CLEAR, "a∡b", ENTER, SQRT)
        .expect("'√ a'∡'b÷2'");

    step("Cubed");
    test(CLEAR, "3+7ⅈ", ENTER, "cubed", ENTER)
        .expect("-414-154ⅈ");
    step("Cube root");
    test("cbrt", ENTER)
        .expect("7.61577 31058 63908 2857∡-0.92849 05618 83382 29639ʳ");

    step("Logarithm");
    test(CLEAR, "12+14ⅈ", ENTER, LN)
        .expect("2.91447 28088 05103 5368+0.86217 00546 67226 34884ⅈ");
    step("Exponential");
    test("exp", ENTER)
        .expect("18.43908 89145 85774 62∡0.86217 00546 67226 34884ʳ");

    step("Power");
    test(CLEAR, "3+7ⅈ", ENTER, "2-3ⅈ", ENTER, SHIFT, B)
        .expect("1 916.30979 15541 96293 8∡2.52432 98723 79583 8639ʳ");

    step("Sine");
    test(CLEAR, "4+2ⅈ", ENTER, SIN)
        .expect("-2.84723 90868 48827 8827-2.37067 41693 52001 6145ⅈ");

    step("Cosine");
    test(CLEAR, "3+11ⅈ", ENTER, COS)
        .expect("-29 637.47552 74860 62145-4 224.71967 95347 02126ⅈ");

    step("Tangent");
    test(CLEAR, "2+1ⅈ", ENTER, TAN)
        .expect("-0.24345 82011 85725 2527+1.16673 62572 40919 8818ⅈ");

    step("Arc sine");
    test(CLEAR, "3+5ⅈ", ENTER, SHIFT, SIN)
        .expect("0.53399 90695 94168 61164+2.45983 15216 23434 5129ⅈ");

    step("Arc cosine");
    test(CLEAR, "7+11ⅈ", ENTER, SHIFT, COS)
        .expect("1.00539 67973 35154 2326-3.26167 13063 80062 6275ⅈ");

    step("Arc tangent");
    test(CLEAR, "9.+2ⅈ", ENTER, SHIFT, TAN)
        .expect("1.46524 96601 83523 3458+0.02327 26057 66502 98838ⅈ");

    step("Hyperbolic sine");
    test(CLEAR, "4+2ⅈ", ENTER, "SINH", ENTER)
        .expect("-11.35661 27112 18172 906+24.83130 58489 46379 372ⅈ");

    step("Hyperbolic cosine");
    test(CLEAR, "3+11ⅈ", ENTER, "COSH", ENTER)
        .expect("0.04433 60889 10782 41416-10.06756 33986 40475 46ⅈ");

    step("Hyperbolic tangent");
    test(CLEAR, "2+8ⅈ", ENTER, "TANH", ENTER)
        .expect("1.03564 79469 63237 6354-0.01092 58843 35752 53196ⅈ");

    step("Hyperbolic arc sine");
    test(CLEAR, "3+5ⅈ", ENTER, SHIFT, "ASINH", ENTER)
        .expect("2.45291 37425 02811 7695+1.02382 17465 11782 9101ⅈ");

    step("Hyperbolic arc cosine");
    test(CLEAR, "7+11ⅈ", ENTER, SHIFT, "ACOSH", ENTER)
        .expect("3.26167 13063 80062 6275+1.00539 67973 35154 2326ⅈ");

    step("Hyperbolic arc tangent");
    test(CLEAR, "9.+2ⅈ", ENTER, SHIFT, "ATANH", ENTER)
        .expect("0.10622 07984 91316 49131+1.54700 47751 56404 9213ⅈ");

    step("Real to complex");
    test(CLEAR, "1 2 R→C", ENTER)
        .type(object::ID_rectangular).expect("1+2ⅈ");
    step("Symbolic real to complex");
    test(CLEAR, "a b R→C", ENTER)
        .type(object::ID_rectangular).expect("'a'+'b'ⅈ");

    step("Complex to real");
    test(CLEAR, "1+2ⅈ C→R", ENTER)
        .expect("im:2").test(BSP).expect("re:1");
    step("Symbolic complex to real");
    test(CLEAR, "a+bⅈ C→R", ENTER)
        .expect("im:b").test(BSP).expect("re:a");

    step("Re function");
    test(CLEAR, "33+22ⅈ Re", ENTER).expect("33");
    step("Symbolic Re function");
    test(CLEAR, "a+bⅈ Re", ENTER).expect("a");
    step("Re function on integers");
    test(CLEAR, "31 Re", ENTER).expect("31");
    step("Re function on decimal");
    test(CLEAR, "31.234 Re", ENTER).expect("31.234");

    step("Im function");
    test(CLEAR, "33+22ⅈ Im", ENTER).expect("22");
    step("Symbolic Im function");
    test(CLEAR, "a+bⅈ Im", ENTER).expect("b");
    step("Im function on integers");
    test(CLEAR, "31 Im", ENTER).expect("0");
    step("Im function on decimal");
    test(CLEAR, "31.234 Im", ENTER).expect("0");

    step("Complex modulus");
    test(CLEAR, "3+4ⅈ abs", ENTER).expect("5.");
    step("Symbolic complex modulus");
    test(CLEAR, "a+bⅈ abs", ENTER).expect("'a⊿b'");
    step("Norm alias");
    test(CLEAR, "3+4ⅈ norm", ENTER).expect("5.");
    test(CLEAR, "a+bⅈ norm", ENTER).expect("'a⊿b'");
    step("Modulus alias");
    test(CLEAR, "3+4ⅈ modulus", ENTER).expect("5.");
    test(CLEAR, "a+bⅈ modulus", ENTER).expect("'a⊿b'");

    step("Complex argument");
    test(CLEAR, "1+1ⅈ arg", ENTER).expect("0.78539 81633 97448 30962");
    step("Symbolic complex argument");
    test(CLEAR, "a+bⅈ arg", ENTER).expect("'b∠a'");
    step("Complex argument on integers");
    test(CLEAR, "31 arg", ENTER).expect("0");
    step("Complex argument on decimals");
    test(CLEAR, "31.234 arg", ENTER).expect("0");

    step("Complex conjugate");
    test(CLEAR, "3+4ⅈ conj", ENTER).expect("3-4ⅈ");
    step("Symbolic complex conjugate");
    test(CLEAR, "a+bⅈ conj", ENTER).expect("a+'-b'ⅈ");
    step("Complex conjugate on integers");
    test(CLEAR, "31 conj", ENTER).expect("31");
    step("Complex conjugate on decimals");
    test(CLEAR, "31.234 conj", ENTER).expect("31.234");

    step("Restore default 24-digit precision");
    test(CLEAR, "24 PRECISION 12 SIG", ENTER).noerror();
}


void tests::units_and_conversions()
// ----------------------------------------------------------------------------
//   Unit types and data conversions
// ----------------------------------------------------------------------------
{
    BEGIN(units);

    step("Entering unit from command-line")
        .test(CLEAR, "1_kg", ENTER)
        .type(object::ID_unit)
        .expect("1 kg");
    step("Unit symbol from unit menu")
        .test(CLEAR, SHIFT, KEY5, KEY1, F1, LOWERCASE, M, S, ENTER)
        .type(object::ID_unit)
        .expect("1 ms");
    step("Unit symbol division from unit menu")
        .test(CLEAR, SHIFT, KEY5, KEY1, F1, LOWERCASE, M, SHIFT, DIV, S, ENTER)
        .type(object::ID_unit)
        .expect("1 m/s");
    step("Unit symbol multiplication from unit menu")
        .test(CLEAR, SHIFT, KEY5, KEY1, F1, LOWERCASE, M, SHIFT, MUL, S, ENTER)
        .type(object::ID_unit)
        .expect("1 m·s");
    step("Insert unit with soft key")
        .test(CLEAR, SHIFT, KEY5, KEY1, F2, F1)
        .type(object::ID_unit)
        .expect("1 in");
    step("Convert integer unit with soft key")
        .test(SHIFT, F2)
        .type(object::ID_unit)
        .expect("25 ²/₅ mm");
    step("Convert decimal unit with soft key")
        .test(CLEAR, KEY2, DOT, F1, SHIFT, F2)
        .type(object::ID_unit)
        .expect("50.8 mm");
    step("Do not apply simplifications for unit conversions")
        .test(CLEAR, KEY1, DOT, F1, SHIFT, F2)
        .type(object::ID_unit)
        .expect("25.4 mm");
    step("Multiply by unit using softkey")
        .test(CLEAR, SHIFT, KEY5, KEY1, F2, F1, F2)
        .type(object::ID_unit)
        .expect("1 in·mm");
    step("Divide by unit using softkey")
        .test(CLEAR, SHIFT, KEY5, KEY1, F2, F1, RSHIFT, F2)
        .type(object::ID_unit)
        .expect("1 in/mm");
    step("Conversion across compound units")
        .test(CLEAR, SHIFT, KEY5, KEY1, F2, F3)
        .type(object::ID_unit).expect("1 km/h")
        .test(SHIFT, F4).type(object::ID_unit).expect("¹⁵ ⁶²⁵/₂₅ ₁₄₆ mph")
        .test(SHIFT, F3).type(object::ID_unit).expect("1 km/h");
    step("Conversion to base units")
        .test(ENTER, RSHIFT, KEY5, F2)
        .type(object::ID_unit).expect("⁵/₁₈ m/s");
    step("Extract value from unit object")
        .test(ENTER, F3)
        .expect("⁵/₁₈");
    step("Split unit object")
        .test(BSP, RSHIFT, N, F5).expect("'m÷s'")
        .test(BSP).expect("⁵/₁₈");
    step("Convert operation")
        .test(CLEAR, KEY1, SHIFT, KEY5, F2, F3)
        .type(object::ID_unit).expect("1 km/h")
        .test(KEY1, F1, SHIFT, KEY5, SHIFT, F1, RSHIFT, F2)
        .type(object::ID_unit).expect("1 in/min")
        .test(RSHIFT, KEY5, F1) // Convert
        .type(object::ID_unit).expect("656 ⁶⁴/₃₈₁ in/min");
    step("Convert to unit")
        .test(CLEAR, KEY3, KEY7, ENTER).expect("37")
        .test(SHIFT, KEY5, KEY4, KEY2, F2, F3).expect("42 km/h")
        .test(RSHIFT, KEY5, F5).expect("37 km/h");
    step("Factoring out a unit")
        .test(CLEAR, KEY3, SHIFT, KEY5, SHIFT, F6, F2).expect("3 kW")
        .test(KEY1, SHIFT, KEY5, SHIFT, F4, F1).expect("1 N")
        .test(RSHIFT, KEY5, F4).expect("3 000 N·m/s");
    step("Orders of magnitude")
        .test(CLEAR, KEY3, SHIFT, KEY5, SHIFT, F6, F2).expect("3 kW")
        .test(RSHIFT, KEY5, SHIFT, F2).expect("300 000 cW")
        .test(SHIFT, F3).expect("3 kW")
        .test(SHIFT, F4).expect("³/₁ ₀₀₀ MW");
    step("Unit simplification (same unit)")
        .test(CLEAR, KEY3, SHIFT, KEY5, SHIFT, F6, F2).expect("3 kW")
        .test(SHIFT, KEY5, SHIFT, F4, F1).expect("3 kW·N")
        .test(SHIFT, KEY5, SHIFT, F6, RSHIFT, F2).expect("3 N");
    step("Arithmetic on units")
        .test(CLEAR, KEY3, KEY7, SHIFT, KEY5, F2, F4).expect("37 mph")
        .test(SHIFT, KEY5, KEY4, KEY2, F2, F3).expect("42 km/h")
        .test(ADD).expect("101 ⁸ ⁵²⁷/₁₅ ₆₂₅ km/h");
    step("Arithmetic on units (decimal)")
        .test(CLEAR, KEY3, KEY7, DOT, SHIFT, KEY5, F2, F4).expect("37. mph")
        .test(SHIFT, KEY5, KEY4, KEY2, F2, F3).expect("42 km/h")
        .test(ADD).expect("101.54572 8 km/h");
    step("Unit parsing on command line")
        .test(CLEAR, "12_km/s^2", ENTER).expect("12 km/s↑2");
    step("Parsing degrees as a unit")
        .test(CLEAR, "DEG", ENTER).noerror()
        .test("1∡90", ENTER).expect("1∡90°")
        .test(DOWN).editor("1∡90°")
        .test(DOWN, DOWN, BSP, DOWN, DOWN, "_").editor("190_°")
        .test(ENTER).expect("190 °");

    step("No auto-simplification for unit addition")
        .test(CLEAR, "1_s", ENTER, "0", NOSHIFT, ADD)
        .error("Inconsistent units");
    step("No auto-simplification for unit subtraction")
        .test(CLEAR, "1_s", ENTER, ENTER, SUB)
        .noerror()
        .expect("0 s");
    step("No auto-simplification for unit multiplication")
        .test(CLEAR, "1_s", ENTER, "1", NOSHIFT, MUL)
        .noerror()
        .expect("1 s");
    step("No auto-simplification for unit division")
        .test(CLEAR, "1_s", ENTER, "1", NOSHIFT, DIV)
        .noerror()
        .expect("1 s");
}

void tests::list_functions()
// ----------------------------------------------------------------------------
//   Some operations on lists
// ----------------------------------------------------------------------------
{
    BEGIN(lists);

    step("Integer index");
    test(CLEAR, "{ A B C }", ENTER, "2 GET", ENTER)
        .expect("B");
    step("Real index");
    test(CLEAR, "{ A B C }", ENTER, "2.3 GET", ENTER)
        .expect("B");
    step("Bad index type");
    test(CLEAR, "{ A B C }", ENTER, "\"A\" GET", ENTER)
        .error("Bad argument type");
    step("Out-of-range index");
    test(CLEAR, "{ A B C }", ENTER, "5 GET", ENTER)
        .error("Index out of range");
    step("Empty list index");
    test(CLEAR, "{ A B C }", ENTER, "{} GET", ENTER)
        .expect("{ A B C }");
    step("Single element list index");
    test(CLEAR, "{ A B C }", ENTER, "{2} GET", ENTER)
        .expect("B");
    step("List index nested");
    test(CLEAR, "{ A {D E F} C }", ENTER, "{2 3} GET", ENTER)
        .expect("F");
    step("List index, too many items");
    test(CLEAR, "{ A B C }", ENTER, "{2 3} GET", ENTER)
        .error("Bad argument type");
    step("Character from array");
    test(CLEAR, "\"Hello World\"", ENTER, "2 GET", ENTER)
        .expect("\"e\"");
    step("Deep nesting");
    test(CLEAR, "{ A { D E { 1 2 \"Hello World\" } F } 2 3 }", ENTER,
         "{ 2 3 3 5 } GET", ENTER)
        .expect("\"o\"");

    step("Incrementing integer index")
        .test(CLEAR,
              "{ A B C }", ENTER, "2 ")
        .test("GETI", ENTER).expect("B").test(BSP)
        .test("GETI", ENTER).expect("C").test(BSP)
        .test("GETI", ENTER).expect("A").test(BSP);

    step("Incrementing decimal index")
        .test(CLEAR,
              "{ A B C }", ENTER, "2.3 ")
        .test("GETI", ENTER).expect("B").test(BSP)
        .test("GETI", ENTER).expect("C").test(BSP)
        .test("GETI", ENTER).expect("A").test(BSP);
    step("Bad index type for GETI")
        .test(CLEAR, "{ A B C }", ENTER, "\"A\" GETI", ENTER)
        .error("Bad argument type");
    step("Out-of-range index for GETI")
        .test(CLEAR, "{ A B C }", ENTER, "5 GETI", ENTER)
        .error("Index out of range");
    step("Empty list index for GETI")
        .test(CLEAR, "{ A B C }", ENTER, "{} GETI", ENTER)
        .error("Bad argument value");
    step("Single element list index for GETI")
        .test(CLEAR, "{ A B C }", ENTER, "{2} ")
        .test("GETI", ENTER).expect("B").test(BSP).expect("{ 3 }")
        .test("GETI", ENTER).expect("C").test(BSP).expect("{ 1 }")
        .test("GETI", ENTER).expect("A").test(BSP).expect("{ 2 }");
    step("List index nested for GETI")
        .test(CLEAR, "{ A {D E F} C }", ENTER, "{2 3} ")
        .test("GETI", ENTER).expect("F").test(BSP).expect("{ 3 1 }")
        .test("GETI", ENTER).error("Bad argument type");
    step("List index, too many items for GETI")
        .test(CLEAR, "{ A B C }", ENTER, "{2 3} GETI", ENTER)
        .error("Bad argument type");
    step("Character from array using GETI")
        .test(CLEAR, "\"Hello\"", ENTER, "2 ")
        .test("GETI", ENTER).expect("\"e\"").test(BSP).expect("3")
        .test("GETI", ENTER).expect("\"l\"").test(BSP).expect("4")
        .test("GETI", ENTER).expect("\"l\"").test(BSP).expect("5")
        .test("GETI", ENTER).expect("\"o\"").test(BSP).expect("1")
        .test("GETI", ENTER).expect("\"H\"").test(BSP).expect("2")
        .test("GETI", ENTER).expect("\"e\"").test(BSP).expect("3");
    step("Deep nesting for GETI");
    test(CLEAR, "{ A { D E { 1 2 \"Hello World\" } F } 2 3 }", ENTER,
         "{ 2 3 3 5 } GETI", ENTER)
        .expect("\"o\"").test(BSP).expect("{ 2 3 3 6 }");

    step("Array indexing");
    test(CLEAR, "[ A [ D E [ 1 2 \"Hello World\" ] F ] 2 3 ]", ENTER,
         "[ 2 3 3 5 ] GET", ENTER)
        .expect("\"o\"");

    step("Concatenation of lists");
    test(CLEAR, "{ A B C D } { F G H I } +", ENTER)
        .expect("{ A B C D F G H I }");
    step("Concatenation of item to list");
    test(CLEAR, "{ A B C D } 2.3 +", ENTER)
        .expect("{ A B C D 2.3 }");
    test(CLEAR, "2.5 { A B C D } +", ENTER)
        .expect("{ 2.5 A B C D }");

    step("Concatenation of list and text");
    test(CLEAR, "{ } \"Hello\" +", ENTER)
        .expect("{ \"Hello\" }");

    step("Repetition of a list");
    test(CLEAR, "{ A B C D } 3 *", ENTER)
        .expect("{ A B C D A B C D A B C D }");
    test(CLEAR, "3 { A B C D } *", ENTER)
        .expect("{ A B C D A B C D A B C D }");

    step("Applying a function to a  list");
    test(CLEAR, "{ A B C } sin", ENTER)
        .expect("{ 'sin A' 'sin B' 'sin C' }");
}


void tests::sorting_functions()
// ----------------------------------------------------------------------------
//   Sorting operations
// ----------------------------------------------------------------------------
{
    BEGIN(sorting);

    step("Value sort (SORT)")
        .test(CLEAR, "{ 7 2.5 3 9.2 \"DEF\" 8.4 \"ABC\" } SORT", ENTER)
        .expect("{ \"ABC\" \"DEF\" 2.5 3 7 8.4 9.2 }");
    step("Reverse list (REVLIST)")
         .test("revlist", ENTER)
         .expect("{ 9.2 8.4 7 3 2.5 \"DEF\" \"ABC\" }");
    step("Memory sort (QUICKSORT)")
        .test("QUICKSORT", ENTER)
        .expect("{ \"ABC\" \"DEF\" 3 7 2.5 8.4 9.2 }");
    step("Reverse memory sort (ReverseQuickSort)")
        .test("reverseQuickSort", ENTER)
        .expect("{ 9.2 8.4 2.5 7 3 \"DEF\" \"ABC\" }");
    step("Reverse sort (ReverseSort)")
        .test("ReverseSort", ENTER)
        .expect("{ 9.2 8.4 7 3 2.5 \"DEF\" \"ABC\" }");
    step("Min function (integer)")
        .test(CLEAR, "1 2 MIN", ENTER).expect("1");
    step("Max function (integer)")
        .test(CLEAR, "1 2 MAX", ENTER).expect("2");
    step("Min function (decimal)")
        .test(CLEAR, "1.23 4.56 MIN", ENTER).expect("1.23");
    step("Max function (decimal)")
        .test(CLEAR, "1.23 4.56 MAX", ENTER).expect("4.56");
    step("Min function (fraction)")
        .test(CLEAR, "1/23 4/56 MIN", ENTER).expect("¹/₂₃");
    step("Max function (fraction)")
        .test(CLEAR, "1/23 4/56 MAX", ENTER).expect("¹/₁₄");
    step("Min function (mixed numbers)")
        .test(CLEAR, "1/23 4.56 MIN", ENTER).expect("¹/₂₃");
    step("Max function (mixed numbers)")
        .test(CLEAR, "1/23 4.56 MAX", ENTER).expect("4.56");
    step("Min function (text)")
        .test(CLEAR, "\"ABC\" \"DEF\" MIN", ENTER).expect("\"ABC\"");
    step("Max function (text)")
        .test(CLEAR, "\"ABC\" \"DEF\" MAX", ENTER).expect("\"DEF\"");
    step("Min function (mixed types)")
        .test(CLEAR, "1 \"DEF\" MAX", ENTER).error("Bad argument type");
    step("Max function (mixed types)")
        .test(CLEAR, "1 \"DEF\" MAX", ENTER).error("Bad argument type");
}


void tests::text_functions()
// ----------------------------------------------------------------------------
//   Some operations on text
// ----------------------------------------------------------------------------
{
    BEGIN(text);

    step("Concatenation of text");
    test(CLEAR, "\"Hello \" \"World\" +", ENTER)
        .expect("\"Hello World\"");
    step("Concatenation of text and object");
    test(CLEAR, "\"Hello \" 2.3 +", ENTER)
        .expect("\"Hello 2.3\"");
    step("Concatenation of object and text");
    test(CLEAR, "2.3 \"Hello \" +", ENTER)
        .expect("\"2.3Hello \"");

    step("Repeating text");
    test(CLEAR, "\"AbC\" 3 *", ENTER)
        .expect("\"AbCAbCAbC\"");
    test(CLEAR, "3 \"AbC\" *", ENTER)
        .expect("\"AbCAbCAbC\"");

    step("Character generation with CHR")
        .test(CLEAR, "64 CHR", ENTER).type(object::ID_text).expect("\"@\"");
    step("Codepoint generation with NUM")
        .test(CLEAR,"\"a\" NUM", ENTER).type(object::ID_integer).expect(97);
    step("Codepoint generation with NUM, multiple characters")
        .test(CLEAR,"\"ba\" NUM", ENTER).type(object::ID_integer).expect(98);

    step("Convert object to text")
        .test(CLEAR, RSHIFT, KEY4, "1.42", F1)
        .type(object::ID_text).expect("\"1.42\"");
    step("Convert object from text")
        .test(CLEAR, RSHIFT, KEY4, "\"1.42 2.43 +\"", F2)
        .type(object::ID_decimal).expect("3.85");
    step("Size of single object")
        .test(CLEAR, "3.85", F3)
        .type(object::ID_integer).expect("1");
    step("Length of null text")
        .test(ENTER, RSHIFT, ENTER, ENTER, F3)
        .type(object::ID_integer).expect("0");
    step("Length of text")
        .test(CLEAR, RSHIFT, KEY4, "\"1.42 2.43 +\"", F3)
        .type(object::ID_integer).expect("11")
        .test(SHIFT, M, ADD, ENTER, ADD, F3)
        .type(object::ID_integer).expect("26");

    step("Conversion of text to code")
        .test(CLEAR, RSHIFT, ENTER, "Hello", NOSHIFT, RSHIFT, KEY4, SHIFT, F1)
        .type(object::ID_list).expect("{ 72 101 108 108 111 }");
    step("Conversion of code to text")
        .test(CLEAR, RSHIFT, RUNSTOP,
              232, SPACE, 233, SPACE, 234, SPACE, 235, SPACE,
              960, SPACE, 8730, SPACE, 8747, ENTER,
              RSHIFT, KEY4, SHIFT, F2)
        .type(object::ID_text).expect("\"èéêëπ√∫\"");


    step("Ensure we can parse integer numbers with separators in them")
        .test(CLEAR, "100000", ENTER).expect("100 000")
        .test(RSHIFT, ENTER, NOSHIFT, ENTER).expect("\"\"")
        .test(NOSHIFT, ADD).expect("\"100 000\"")
        .test(NOSHIFT, A, F2).expect("100 000");
    step("Ensure we can parse decimal numbers with separators in them")
        .test(CLEAR, "100000.123456123456", ENTER).expect("100 000.12345 6")
        .test(RSHIFT, ENTER, NOSHIFT, ENTER).expect("\"\"")
        .test(NOSHIFT, ADD).expect("\"100 000.12345 61234 56\"")
        .test(NOSHIFT, A, F2).expect("100 000.12345 6");
    step("Ensure we can parse base numbers with separators in them")
        .test(CLEAR, "16#ABCD1234", ENTER).expect("#ABCD 1234₁₆")
        .test(RSHIFT, ENTER, NOSHIFT, ENTER).expect("\"\"")
        .test(NOSHIFT, ADD).expect("\"#ABCD 1234\"")
        .test(NOSHIFT, A, F2).expect("#ABCD 1234₁₆");
}


void tests::vector_functions()
// ----------------------------------------------------------------------------
//   Test operations on vectors
// ----------------------------------------------------------------------------
{
    BEGIN(vectors);

    step("Data entry in numeric form");
    test(CLEAR, "[  1  2  3  ]", ENTER)
        .type(object::ID_array).expect("[ 1 2 3 ]");
    test(CLEAR, "[  1.5  2.300  3.02  ]", ENTER)
        .type(object::ID_array).expect("[ 1.5 2.3 3.02 ]");

    step("Symbolic vector");
    test(CLEAR, "[a b c]", ENTER)
        .expect("[ a b c ]");

    step("Non-homogneous data types");
    test(CLEAR, "[  \"ABC\"  'X' 3/2  ]", ENTER)
        .type(object::ID_array).expect("[ \"ABC\" 'X' 1 ¹/₂ ]");

    step("Addition");
    test(CLEAR, "[1 2 3][4 5 6] +", ENTER)
        .expect("[ 5 7 9 ]");
    test(CLEAR, "[a b c][d e f] +", ENTER)
        .expect("[ 'a+d' 'b+e' 'c+f' ]");

    step("Subtraction");
    test(CLEAR, "[1 2 3 4][4 5 2 1] -", ENTER)
        .expect("[ -3 -3 1 3 ]");
    test(CLEAR, "[a b c][d e f] -", ENTER)
        .expect("[ 'a-d' 'b-e' 'c-f' ]");

    step("Multiplication (extension)");
    test(CLEAR, "[1 2  3 4 6][4 5 2 1 3] *", ENTER)
        .expect("[ 4 10 6 4 18 ]");
    test(CLEAR, "[a b c][d e f] *", ENTER)
        .expect("[ 'a·d' 'b·e' 'c·f' ]");

    step("Division (extension)");
    test(CLEAR, "[1 2  3 4 6][4 5 2 1 3] /", ENTER)
        .expect("[ ¹/₄ ²/₅ 1 ¹/₂ 4 2 ]");
    test(CLEAR, "[a b c][d e f] /", ENTER)
        .expect("[ 'a÷d' 'b÷e' 'c÷f' ]");

    step("Addition of constant (extension)");
    test(CLEAR, "[1 2 3] 3 +", ENTER)
        .expect("[ 4 5 6 ]");
    test(CLEAR, "[a b c] x +", ENTER)
        .expect("[ 'a+x' 'b+x' 'c+x' ]");

    step("Subtraction of constant (extension)");
    test(CLEAR, "[1 2 3 4] 3 -", ENTER)
        .expect("[ -2 -1 0 1 ]");
    test(CLEAR, "[a b c] x -", ENTER)
        .expect("[ 'a-x' 'b-x' 'c-x' ]");
    test(CLEAR, "x [a b c] -", ENTER)
        .expect("[ 'x-a' 'x-b' 'x-c' ]");

    step("Multiplication by constant (extension)");
    test(CLEAR, "[a b c] x *", ENTER)
        .expect("[ 'a·x' 'b·x' 'c·x' ]");
    test(CLEAR, "x [a b c] *", ENTER)
        .expect("[ 'x·a' 'x·b' 'x·c' ]");

    step("Division by constant (extension)");
    test(CLEAR, "[a b c] x /", ENTER)
        .expect("[ 'a÷x' 'b÷x' 'c÷x' ]");
    test(CLEAR, "x [a b c] /", ENTER)
        .expect("[ 'x÷a' 'x÷b' 'x÷c' ]");

    step("Invalid dimension for binary operations");
    test(CLEAR, "[1 2 3][1 2] +", ENTER)
        .error("Invalid dimension");
    test(CLEAR, "[1 2 3][1 2] -", ENTER)
        .error("Invalid dimension");
    test(CLEAR, "[1 2 3][1 2] *", ENTER)
        .error("Invalid dimension");
    test(CLEAR, "[1 2 3][1 2] /", ENTER)
        .error("Invalid dimension");

    step("Component-wise inversion of a vector");
    test(CLEAR, "[1 2 3] INV", ENTER)
        .expect("[ 1 ¹/₂ ¹/₃ ]");

    step("Fröbenius norm");
    test(CLEAR, "[1 2 3] ABS", ENTER)
        .expect("3.74165 73867 7");
    test(CLEAR, "[1 2 3] NORM", ENTER)
        .expect("3.74165 73867 7");

    step("Component-wise application of functions");
    test(CLEAR, "[a b c] SIN", ENTER)
        .expect("[ 'sin a' 'sin b' 'sin c' ]");
}


void tests::matrix_functions()
// ----------------------------------------------------------------------------
//   Test operations on vectors
// ----------------------------------------------------------------------------
{
    BEGIN(matrices);

    step("Data entry in numeric form");
    test(CLEAR, "[  [1  2  3][4 5 6]  ]", ENTER)
        .type(object::ID_array).want("[[ 1 2 3 ] [ 4 5 6 ]]");

    step("Non-rectangular matrices");
    test(CLEAR, "[  [ 1.5  2.300 ] [ 3.02 ]]", ENTER)
        .type(object::ID_array).want("[[ 1.5 2.3 ] [ 3.02 ]]");

    step("Symbolic matrix");
    test(CLEAR, "[[a b] [c d]]", ENTER)
        .want("[[ a b ] [ c d ]]");

    step("Non-homogneous data types");
    test(CLEAR, "[  [ \"ABC\"  'X' ] 3/2  [ 4 [5] [6 7]]]", ENTER)
        .type(object::ID_array)
        .want("[[ \"ABC\" 'X' ] 1 ¹/₂ [ 4 [ 5 ] [ 6 7 ] ] ]");

    step("Addition");
    test(CLEAR, "[[1 2] [3 4]] [[5 6][7 8]] +", ENTER)
        .want("[[ 6 8 ] [ 10 12 ]]");
    test(CLEAR, "[[a b][c d]] [[e f][g h]] +", ENTER)
        .want("[[ 'a+e' 'b+f' ] [ 'c+g' 'd+h' ]]");

    step("Subtraction");
    test(CLEAR, "[[1 2] [3 4]] [[5 6][7 8]] -", ENTER)
        .want("[[ -4 -4 ] [ -4 -4 ]]");
    test(CLEAR, "[[a b][c d]] [[e f][g h]] -", ENTER)
        .want("[[ 'a-e' 'b-f' ] [ 'c-g' 'd-h' ]]");

    step("Multiplication (square)");
    test(CLEAR, "[[1 2] [3 4]] [[5 6][7 8]] *", ENTER)
        .want("[[ 19 22 ] [ 43 50 ]]");
    test(CLEAR, "[[a b][c d]] [[e f][g h]] *", ENTER)
        .want("[[ 'a·e+b·g' 'a·f+b·h' ] [ 'c·e+d·g' 'c·f+d·h' ]]");

    step("Multiplication (non-square)");
    test(CLEAR, "[[1 2 3] [4 5 6]] [[5 6][7 8][9 10]] *", ENTER)
        .want("[[ 46 52 ] [ 109 124 ]]");
    test(CLEAR, "[[a b c d][e f g h]] [[x][y][z][t]] *", ENTER)
        .want("[[ 'a·x+b·y+c·z+d·t' ] [ 'e·x+f·y+g·z+h·t' ]]");
    test(CLEAR, "[[a b c d][e f g h]] [x y z t] *", ENTER)
        .want("[ 'a·x+b·y+c·z+d·t' 'e·x+f·y+g·z+h·t' ]");

    step("Division");
    test(CLEAR,
         "[[5 12 1968][17 2 1969][30 3 1993]] "
         "[[16 5 1995][21 5 1999][28 5 2009]] /", ENTER)
        .want("[[ 3 ¹/₁₁ -4 ⁸/₁₁ -3 ¹⁰/₁₁ ] [ 335 ⁷/₁₀ -1 342 ⁷/₁₀ -1 643 ³/₁₀ ] [ -¹⁹/₂₂ 3 ⁹/₂₂ 5 ³/₂₂ ]]");
    step("Division (symbolic)");
    test(CLEAR, "[[a b][c d]][[e f][g h]] /", ENTER)
        .want("[[ '(e⁻¹-f÷e·((-g)÷(e·h-g·f)))·a+(-(f÷e·(e÷(e·h-g·f))))·c' '(e⁻¹-f÷e·((-g)÷(e·h-g·f)))·b+(-(f÷e·(e÷(e·h-g·f))))·d' ] [ '(-g)÷(e·h-g·f)·a+e÷(e·h-g·f)·c' '(-g)÷(e·h-g·f)·b+e÷(e·h-g·f)·d' ]]");

    step("Addition of constant (extension)");
    test(CLEAR, "[[1 2] [3 4]] 3 +", ENTER)
        .want("[[ 4 5 ] [ 6 7 ]]");
    test(CLEAR, "[[a b] [c d]] x +", ENTER)
        .want("[[ 'a+x' 'b+x' ] [ 'c+x' 'd+x' ]]");

    step("Subtraction of constant (extension)");
    test(CLEAR, "[[1 2] [3 4]] 3 -", ENTER)
        .want("[[ -2 -1 ] [ 0 1 ]]");
    test(CLEAR, "[[a b] [c d]] x -", ENTER)
        .want("[[ 'a-x' 'b-x' ] [ 'c-x' 'd-x' ]]");

    step("Multiplication by constant (extension)");
    test(CLEAR, "[[a b] [c d]] x *", ENTER)
        .want("[[ 'a·x' 'b·x' ] [ 'c·x' 'd·x' ]]");
    test(CLEAR, "x [[a b] [c d]] *", ENTER)
        .want("[[ 'x·a' 'x·b' ] [ 'x·c' 'x·d' ]]");

    step("Division by constant (extension)");
    test(CLEAR, "[[a b] [c d]] x /", ENTER)
        .want("[[ 'a÷x' 'b÷x' ] [ 'c÷x' 'd÷x' ]]");
    test(CLEAR, "x [[a b] [c d]] /", ENTER)
        .want("[[ 'x÷a' 'x÷b' ] [ 'x÷c' 'x÷d' ]]");

    step("Invalid dimension for binary operations");
    test(CLEAR, "[[1 2] [3 4]][1 2] +", ENTER)
        .error("Bad argument type");
    test(CLEAR, "[[1 2] [3 4]][[1 2][3 4][5 6]] +", ENTER)
        .error("Invalid dimension");
    test(CLEAR, "[[1 2] [3 4]][1 2] +", ENTER)
        .error("Bad argument type");
    test(CLEAR, "[[1 2] [3 4]][[1 2][3 4][5 6]] -", ENTER)
        .error("Invalid dimension");
    test(CLEAR, "[[1 2] [3 4]][1 2] +", ENTER)
        .error("Bad argument type");
    test(CLEAR, "[[1 2] [3 4]][[1 2][3 4][5 6]] -", ENTER)
        .error("Invalid dimension");
    test(CLEAR, "[[1 2] [3 4]][1 2] +", ENTER)
        .error("Bad argument type");
    test(CLEAR, "[[1 2] [3 4]][[1 2][3 4][5 6]] *", ENTER)
        .error("Invalid dimension");
    test(CLEAR, "[[1 2] [3 4]][1 2 3] *", ENTER)
        .error("Invalid dimension");
    test(CLEAR, "[[1 2] [3 4]][[1 2][3 4][5 6]] /", ENTER)
        .error("Invalid dimension");
    test(CLEAR, "[[1 2] [3 4]][1 2] /", ENTER)
        .error("Bad argument type");

    step("Inversion of a definite matrix");
    test(CLEAR, "[[1 2 3][4 5 6][7 8 19]] INV", ENTER)
        .want("[[ -1 ¹⁷/₃₀ ⁷/₁₅ ¹/₁₀ ]"
              " [ 1 ²/₁₅ ¹/₁₅ -¹/₅ ]"
              " [ ¹/₁₀ -¹/₅ ¹/₁₀ ]]");
    test(CLEAR, "[[a b][c d]] INV", ENTER)
        .want("[[ 'a⁻¹-b÷a·((-c)÷(a·d-c·b))' '-(b÷a·(a÷(a·d-c·b)))' ] [ '(-c)÷(a·d-c·b)' 'a÷(a·d-c·b)' ]]");

    step("Invert with zero determinant");       // HP48 gets this one wrong
    test(CLEAR, "[[1 2 3][4 5 6][7 8 9]] INV", ENTER)
        .error("Divide by zero");

    step("Determinant");                        // HP48 gets this one wrong
    test(CLEAR, "[[1 2 3][4 5 6][7 8 9]] DET", ENTER)
        .want("0");
    test(CLEAR, "[[1 2 3][4 5 6][7 8 19]] DET", ENTER)
        .want("-30");

    step("Froebenius norm");
    test(CLEAR, "[[1 2] [3 4]] ABS", ENTER)
        .want("5.47722 55750 5");
    test(CLEAR, "[[1 2] [3 4]] NORM", ENTER)
        .want("5.47722 55750 5");

    step("Component-wise application of functions");
    test(CLEAR, "[[a b] [c d]] SIN", ENTER)
        .want("[[ 'sin a' 'sin b' ] [ 'sin c' 'sin d' ]]");
}


void tests::solver_testing()
// ----------------------------------------------------------------------------
//   Test that the solver works as expected
// ----------------------------------------------------------------------------
{
    BEGIN(solver);

    step("Solver with expression")
        .test(CLEAR, "'X+3' 'X' 0 ROOT", ENTER)
        .noerror().expect("X:-3.");
    step("Solver with equation")
        .test(CLEAR, "'sq(x)=3' 'X' 0 ROOT", ENTER)
        .noerror().expect("X:1.73205 08075 7");
    step("Solver without solution")
        .test(CLEAR, "'sq(x)+3=0' 'X' 0 ROOT", ENTER)
        .error("No solution?");
}


void tests::numerical_integration_testing()
// ----------------------------------------------------------------------------
//   Test that the numerica integartion function works as expected
// ----------------------------------------------------------------------------
{
    BEGIN(integrate);

    step("Integrate with expression")
        .test(CLEAR, "1 2 '1/X' 'X' INTEGRATE", ENTER)
        .noerror().expect("0.69314 71805 6")
        .test(KEY2, E, SUB).expect("3.00876⁳⁻¹⁹");
    step("Integration through menu")
        .test(CLEAR, 2, ENTER).expect("2")
        .test(3, ENTER).expect("3")
        .test("'sq(Z)+Z'", ENTER).expect("'Z²+Z'")
        .test(F, ALPHA, Z, ENTER).expect("'Z'")
        .test(SHIFT, KEY8, F2).expect("8.83333 33333 3", 350);
    step("Integration with decimals")
        .test(CLEAR, "2.", ENTER).expect("2.")
        .test("3.", ENTER).expect("3.")
        .test("'sq(Z)+Z'", ENTER).expect("'Z²+Z'")
        .test(F, ALPHA, Z, ENTER).expect("'Z'")
        .test(SHIFT, KEY8, F2).expect("8.83333 33333 3", 350);
}


void tests::auto_simplification()
// ----------------------------------------------------------------------------
//   Check auto-simplification rules for arithmetic
// ----------------------------------------------------------------------------
{
    BEGIN(simplify);

    step("Enable auto simplification");
    test(CLEAR, "AutoSimplify", ENTER).noerror();

    step("X + 0 = X");
    test(CLEAR, "X 0 +", ENTER).expect("'X'");

    step("0 + X = X");
    test(CLEAR, "0 X +", ENTER).expect("'X'");

    step("X - 0 = X");
    test(CLEAR, "X 0 -", ENTER).expect("'X'");

    step("0 - X = -X");
    test(CLEAR, "0 X -", ENTER).expect("'-X'");

    step("X - X = 0");
    test(CLEAR, "X X -", ENTER).expect("0");

    step("0 * X = 0");
    test(CLEAR, "0 X *", ENTER).expect("0");

    step("X * 0 = 0");
    test(CLEAR, "X 0 *", ENTER).expect("0");

    step("1 * X = X");
    test(CLEAR, "1 X *", ENTER).expect("'X'");

    step("X * 1 = X");
    test(CLEAR, "X 1 *", ENTER).expect("'X'");

    step("X * X = sq(X)");
    test(CLEAR, "X sin 1 * X 0 + sin *", ENTER).expect("'(sin X)²'");

    step("0 / X = -");
    test(CLEAR, "0 X /", ENTER).expect("0");

    step("X / 1 = X");
    test(CLEAR, "X 1 /", ENTER).expect("'X'");

    step("1 / X = inv(X)");
    test(CLEAR, "1 X sin /", ENTER).expect("'(sin X)⁻¹'");

    step("X / X = 1");
    test(CLEAR, "X cos 1 * X 0 + cos /", ENTER).expect("1");

    step("1.0 == 1");
    test(CLEAR, "1.0000 X * ", ENTER).expect("'X'");

    step("0.0 == 0 (but preserves types)");
    test(CLEAR, "0.0000 X * ", ENTER).expect("0.");

    step("i*i == -1");
    test(CLEAR, "ⅈ", ENTER, ENTER, MUL).expect("-1");

    step("i*i == -1 (symbolic constant)");
    test(CLEAR, LSHIFT, I, F1, F3, ENTER, MUL).expect("-1");

    step("Simplification of rectangular real-only results");
    test(CLEAR, "0ⅈ3 0ⅈ5", ENTER, MUL).expect("-15");
    test(CLEAR, "0ⅈ3 0-ⅈ5", ENTER, MUL).expect("15");

    step("Simplification of polar real-only results");
    test(CLEAR, "2∡90 3∡90", ENTER, MUL).expect("-6");
    test(CLEAR, "2∡90 3∡-90", ENTER, MUL).expect("6");

    step("Applies when building a matrix");
    test(CLEAR, "[[3 0 2][2 0 -2][ 0 1 1 ]] [x y z] *", ENTER)
        .expect("[ '3·x+2·z' '2·x+-2·z' 'y+z' ]");

    step("Does not reduce matrices");
    test(CLEAR, "[a b c] 0 *", ENTER).expect("[ 0 0 0 ]");

    step("Does not apply to text");
    test(CLEAR, "\"Hello\" 0 +", ENTER)
        .expect("\"Hello0\"");

    step("Does not apply to lists");
    test(CLEAR, "{ 1 2 3 } 0 +", ENTER)
        .expect("{ 1 2 3 0 }");

    step("Disable auto simplification");
    test(CLEAR, "NoAutoSimplify", ENTER).noerror();

    step("When disabled, get the complicated expression");
    test(CLEAR, "[[3 0 2][2 0 -2][ 0 1 1 ]] [x y z] *", ENTER)
        .expect("[ '3·x+0·y+2·z' '2·x+0·y+-2·z' '0·x+1·y+1·z' ]");

    step("Re-enable auto simplification");
    test(CLEAR, "AutoSimplify", ENTER).noerror();
}


void tests::rewrite_engine()
// ----------------------------------------------------------------------------
//   Equation rewrite engine
// ----------------------------------------------------------------------------
{
    BEGIN(rewrites);

    step("Single replacement");
    test(CLEAR, "'A+B' 'X+Y' 'Y-sin X' rewrite", ENTER)
        .expect("'B-sin A'");

    step("In-depth replacement");
    test(CLEAR, " 'A*(B+C)' 'X+Y' 'Y-sin X' rewrite", ENTER)
        .expect("'A·(C-sin B)'");

    step("Variable matching");
    test(CLEAR, "'A*(B+C)' 'X+X' 'X-sin X' rewrite", ENTER)
        .expect("'A·(B+C)'");
    test(CLEAR, "'A*(B+(B))' 'X+X' 'X-sin X' rewrite", ENTER)
        .expect("'A·(B-sin B)'");

    step("Constant folding");
    test(CLEAR, "'A+B+0' 'X+0' 'X' rewrite", ENTER)
        .expect("'A+B'");
    step("Multiple substitutions");
    test(CLEAR, "'A+B+C' 'X+Y' 'Y-X' rewrite", ENTER)
        .expect("'C-(B-A)'");

    step("Deep substitution");
    test(CLEAR, "'tan(A-B)+3' 'A-B' '-B+A' rewrite", ENTER)
        .expect("'tan(-B+A)+3'");
    step("Deep substitution with multiple changes");
    test(CLEAR, "'5+tan(A-B)+(3-sin(C+D-A))' 'A-B' '-B+A' rewrite", ENTER)
        .expect("'5+tan(-B+A)+(-sin(-A+(C+D))+3)'");

    step("Matching integers");
    test(CLEAR, "'(A+B)^3' 'X^N' 'X*X^(N-1)' rewrite", ENTER)
        .expect("'(A+B)·(A+B)²'");

    step("Matching unique terms");
    test(CLEAR, "'(A+B+A)' 'X+U+X' '2*X+U' rewrite", ENTER)
        .expect("'2·A+B'");
    test(CLEAR, "'(A+A+A)' 'X+U+X' '2*X+U' rewrite", ENTER)
        .expect("'A+A+A'");
}


void tests::expand_collect_simplify()
// ----------------------------------------------------------------------------
//   Equation rewrite engine
// ----------------------------------------------------------------------------
{
    BEGIN(expand);

    step("Single add, right");
    test(CLEAR, "'(A+B)*C' expand ", ENTER)
        .expect("'A·C+B·C'");
    step("Single add, left");
    test(CLEAR, "'2*(A+B)' expand ", ENTER)
        .expect("'2·A+2·B'");

    step("Multiple adds");
    test(CLEAR, "'3*(A+B+C)' expand ", ENTER)
        .expect("'3·A+3·B+3·C'");

    step("Single sub, right");
    test(CLEAR, "'(A-B)*C' expand ", ENTER)
        .expect("'A·C-B·C'");
    step("Single sub, left");
    test(CLEAR, "'2*(A-B)' expand ", ENTER)
        .expect("'2·A-2·B'");

    step("Multiple subs");
    test(CLEAR, "'3*(A-B-C)' expand ", ENTER)
        .expect("'3·A-3·B-3·C'");

    step("Expand and collect a power");
    test(CLEAR, "'(A+B)^3' expand ", ENTER)
        .expect("'A·A·A+A·A·B+A·A·B+A·B·B+A·A·B+A·B·B+A·B·B+B·B·B'");
    test("collect ", ENTER)
        .expect("'2·(B↑2·A)+(A↑3+A↑2·(2·B)+B↑2·A+A↑2·B)+B↑3'");
    // .expect("'(A+B)³'");
}


void tests::tagged_objects()
// ----------------------------------------------------------------------------
//   Some very basic testing of tagged objects
// ----------------------------------------------------------------------------
{
    BEGIN(tagged);

    step("Parsing tagged integer");
    test(CLEAR, ":ABC:123", ENTER)
        .type(object::ID_tag)
        .expect("ABC :123");
    step("Parsing tagged fraction");
    test(CLEAR, ":Label:123/456", ENTER)
        .type(object::ID_tag)
        .expect("Label :⁴¹/₁₅₂");
    step("Parsing nested label");
    test(CLEAR, ":Nested::Label:123.456", ENTER)
        .type(object::ID_tag)
        .expect("Nested :Label :123.456");

    step("Arithmetic");
    test(CLEAR, ":First:1 :Second:2 +", ENTER)
        .expect("3");
    test(CLEAR, "5 :Second:2 -", ENTER)
        .expect("3");
    test(CLEAR, ":First:3/2 2 *", ENTER)
        .expect("3");

    step("Functions");
    test(CLEAR, ":First:1 ABS", ENTER)
        .expect("1");
    test(CLEAR, ":First:0 SIN", ENTER)
        .expect("0");

    step("ToTag");
    test(CLEAR, "125 \"Hello\" ToTag", ENTER)
        .expect("Hello:125");
    test(CLEAR, "125 127 ToTag", ENTER)
        .type(object::ID_tag)
        .expect("127:125");

    step("FromTag");
    test(CLEAR, ":Hello:123 FromTag", ENTER)
        .type(object::ID_text)
        .expect("\"Hello \"")
        .test("Drop", ENTER)
        .expect("123");

    step("DeleteTag");
    test(CLEAR, ":Hello:123 DeleteTag", ENTER)
        .expect("123");

    step("Tagged unit")
        .test(CLEAR, ":ABC:1_kg", ENTER)
        .expect("ABC :1 kg");
    step("Tagged unit (without space)")
        .test(CLEAR, ALPHA, KEY0, A, B, C, NOSHIFT, DOWN,
              KEY1, SHIFT, KEY5, F1,
              LOWERCASE, K, G,
              ENTER)
        .expect("ABC:1 kg");
    step("Tagged complex (without space)")
        .test(CLEAR, ALPHA, KEY0,
              A, B, C, NOSHIFT, DOWN,
              KEY1, SHIFT, G, F1, KEY2, KEY3,
              ENTER)
        .expect("ABC:1+23ⅈ")
        .test(RSHIFT, N, RSHIFT, F2) // TAG->
        .expect("\"ABC\"")
        .test(BSP)
        .expect("1+23ⅈ");
}


void tests::catalog_test()
// ----------------------------------------------------------------------------
//   Test the catalog features
// ----------------------------------------------------------------------------
{
    BEGIN(catalog);

    step("Entering commands through the catalog")
        .test(CLEAR, RSHIFT, RUNSTOP).editor("{}")
        .test(ALPHA, A).editor("{A}")
        .test(ADD).editor("{A}")
        .test(F1).editor("{ %Change }");
    step("Finding functions from inside")
        .test(B).editor("{ %Change B}")
        .test(F1).editor("{ %Change abs }");
    step("Finding functions with middle characters")
        .test(B, U).editor("{ %Change abs BU}")
        .test(F1).editor("{ %Change abs Debug }");
    step("Catalog with nothing entered")
        .test(F6, F3).editor("{ %Change abs Debug cosh⁻¹ }");

    step("Test the default menu")
        .test(CLEAR, EXIT, A, RSHIFT, RUNSTOP).editor("{}")
        .test(F1).editor("{ Help }");
    step("Test catalog as a menu")
        .test(SHIFT, ADD, F1).editor("{ Help x! }")
        .test(ENTER).expect("{ Help x! }");
}


void tests::cycle_test()
// ----------------------------------------------------------------------------
//   Test the Cycle feature
// ----------------------------------------------------------------------------
{
    BEGIN(cycle);

    step("Using the EEX key to enter powers of 10")
        .test(CLEAR, KEY1, O, KEY3, KEY2).editor("1⁳32")
        .test(ENTER).expect("1.⁳³²");
    step("Convert decimal to integer")
        .test(O).expect("100 000 000 000 000 000 000 000 000 000 000");
    step("Convert integer to decimal")
        .test(ENTER, KEY2, KEY0, KEY0, DIV, SUB)
        .test(O).expect("9.95⁳³¹");
    step("Convert decimal to fraction")
        .test(CLEAR, KEY1, DOT, KEY2, ENTER).expect("1.2")
        .test(O).expect("1 ¹/₅");
    step("Convert fraction to decimal")
        .test(B).expect("⁵/₆")
        .test(O).expect("0.83333 33333 33");
    step("Convert decimal to fraction with rounding")
        .test(O).expect("⁵/₆");
    step("Convert decimal to fraction with multiple digits")
        .test(CLEAR, "1.325", ENTER, O).expect("1 ¹³/₄₀");
    step("Convert rectangular to polar")
        .test(CLEAR, "DEG", ENTER,
              "10", SHIFT, G, F1, "10", ENTER).expect("10+10ⅈ")
        .test(O).expect("14.14213 56237∡45°");
    step("Convert polar to rectangular")
        .test(O).expect("10.+10.ⅈ");
    step("Convert based integer bases")
        .test(CLEAR, "#123", ENTER).expect("#123₁₆")
        .test(O).expect("#123₁₆")
        .test(O).expect("#291₁₀")
        .test(O).expect("#443₈")
        .test(O).expect("#1 0010 0011₂")
        .test(O).expect("#123₁₆")
        .test(O).expect("#123₁₆");
    step("Convert list to array")
        .test(CLEAR, "{ 1 2 3 }", ENTER).expect("{ 1 2 3 }")
        .test(O).expect("[ 1 2 3 ]");
    step("Convert array to program")
        .test(O).expect("« 1 2 3 »");
    step("Convert program to list")
        .test(O).expect("{ 1 2 3 }");
    step("Delete tag")
        .test(CLEAR, ":ABC:1.25", ENTER).expect("ABC :1.25")
        .test(O).expect("1.25");
    step("Cycle unit orders of magnitude up (as fractions)")
        .test(CLEAR, "1_kN", ENTER).expect("1 kN")
        .test(O).expect("¹/₁ ₀₀₀ MN")
        .test(O).expect("¹/₁ ₀₀₀ ₀₀₀ GN");
    step("Cycle unit orders of magnitude down (as decimal)")
        .test(O).expect("0.00000 1 GN")
        .test(O).expect("0.001 MN")
        .test(O).expect("1. kN")
        .test(O).expect("10. hN")
        .test(O).expect("100. daN")
        .test(O).expect("1 000. N")
        .test(O).expect("10 000. dN")
        .test(O).expect("100 000. cN")
        .test(O).expect("1 000 000. mN")
        .test(O).expect("1.⁳⁹ µN");
    step("Cycle unit orders of magnitude up (as integers)")
        .test(O).expect("1 000 000 000 µN")
        .test(O).expect("1 000 000 mN")
        .test(O).expect("100 000 cN")
        .test(O).expect("10 000 dN")
        .test(O).expect("1 000 N")
        .test(O).expect("100 daN")
        .test(O).expect("10 hN")
        .test(O).expect("1 kN");
    step("Cycle unit orders of magnitude up (as fractions)")
        .test(O).expect("¹/₁ ₀₀₀ MN")
        .test(O).expect("¹/₁ ₀₀₀ ₀₀₀ GN");
    step("Cycle unit orders of magnitude up (back to decimal)")
        .test(O).expect("0.00000 1 GN")
        .test(O).expect("0.001 MN")
        .test(O).expect("1. kN");

    step("Cycle angle units")
        .test(CLEAR, "1.2.3", ENTER).expect("1°02′03″");
    step("Cycle from DMS to fractional pi-radians")
        .test(O).expect("¹ ²⁴¹/₂₁₆ ₀₀₀ πr");
    step("Cycle from fractional pi-radians to fractional degrees")
        .test(O).expect("1 ⁴¹/₁ ₂₀₀ °");
    step("Cycle from fractional degrees to fractional grad")
        .test(O).expect("1 ¹⁶¹/₁ ₀₈₀ grad");
    step("Cycle from fractional grad to decimal radians")
        .test(O).expect("0.01804 96133 48 r");
    step("Cycle from decimal radians to decimal grad")
        .test(O).expect("1.14907 40740 7 grad");
    step("Cycle from decimal grad to decimal degrees")
        .test(O).expect("1.03416 66666 7 °");
    step("Cycle from decimal degrees to decimal pi-radians")
        .test(O).expect("0.00574 53703 7 πr");
    step("Cycle to decimal DMS")
        .test(O).expect("1°02′02″1");
    step("Cycle back to fractional DMS")
        .test(O).expect("1°02′03″");
    step("Check that DMS produced the original pi-radians fraction")
        .test(O).expect("¹ ²⁴¹/₂₁₆ ₀₀₀ πr");
    step("Check that DMS produced the original degrees fraction")
        .test(O).expect("1 ⁴¹/₁ ₂₀₀ °");
}


void tests::shift_and_rotate()
// ----------------------------------------------------------------------------
//    Test shift and rotate instructions
// ----------------------------------------------------------------------------
{
    BEGIN(rotate);

    step("Default word size should be 64")
        .test(CLEAR, "RCWS", ENTER).noerror().expect("64");

    step("Shift left")
        .test(CLEAR, "#123A", LSHIFT, KEY4, F6)
        .test(F1).expect("#2474₁₆")
        .test(F1).expect("#48E8₁₆")
        .test(F1).expect("#91D0₁₆")
        .test(F1).expect("#1 23A0₁₆")
        .test(F1).expect("#2 4740₁₆")
        .test(F1).expect("#4 8E80₁₆")
        .test(F1).expect("#9 1D00₁₆")
        .test(F1).expect("#12 3A00₁₆");
    step("Shift right")
        .test(F2).expect("#9 1D00₁₆")
        .test(F2).expect("#4 8E80₁₆")
        .test(F2).expect("#2 4740₁₆")
        .test(F2).expect("#1 23A0₁₆")
        .test(F2).expect("#91D0₁₆")
        .test(F2).expect("#48E8₁₆")
        .test(F2).expect("#2474₁₆")
        .test(F2).expect("#123A₁₆")
        .test(F2).expect("#91D₁₆")
        .test(F2).expect("#48E₁₆")
        .test(F2).expect("#247₁₆")
        .test(F2).expect("#123₁₆");
    step("Rotate left")
        .test(F4).expect("#246₁₆")
        .test(F4).expect("#48C₁₆")
        .test(F4).expect("#918₁₆")
        .test(F4).expect("#1230₁₆");
    step("Rotate byte left")
        .test(LSHIFT, F4).expect("#12 3000₁₆")
        .test(LSHIFT, F4).expect("#1230 0000₁₆")
        .test(LSHIFT, F4).expect("#12 3000 0000₁₆")
        .test(LSHIFT, F4).expect("#1230 0000 0000₁₆")
        .test(LSHIFT, F4).expect("#12 3000 0000 0000₁₆")
        .test(LSHIFT, F4).expect("#1230 0000 0000 0000₁₆")
        .test(LSHIFT, F4).expect("#3000 0000 0000 0012₁₆")
        .test(LSHIFT, F4).expect("#1230₁₆")
        .test(LSHIFT, F4).expect("#12 3000₁₆")
        .test(LSHIFT, F4).expect("#1230 0000₁₆")
        .test(LSHIFT, F4).expect("#12 3000 0000₁₆")
        .test(LSHIFT, F4).expect("#1230 0000 0000₁₆")
        .test(LSHIFT, F4).expect("#12 3000 0000 0000₁₆")
        .test(LSHIFT, F4).expect("#1230 0000 0000 0000₁₆");
    step("Rotate left with bit rotating")
        .test(F4).expect("#2460 0000 0000 0000₁₆")
        .test(F4).expect("#48C0 0000 0000 0000₁₆")
        .test(F4).expect("#9180 0000 0000 0000₁₆")
        .test(F4).expect("#2300 0000 0000 0001₁₆")
        .test(F4).expect("#4600 0000 0000 0002₁₆")
        .test(F4).expect("#8C00 0000 0000 0004₁₆");
    step("Rotate right")
        .test(F5).expect("#4600 0000 0000 0002₁₆")
        .test(F5).expect("#2300 0000 0000 0001₁₆")
        .test(F5).expect("#9180 0000 0000 0000₁₆")
        .test(F5).expect("#48C0 0000 0000 0000₁₆")
        .test(F5).expect("#2460 0000 0000 0000₁₆")
        .test(F5).expect("#1230 0000 0000 0000₁₆")
        .test(F5).expect("#918 0000 0000 0000₁₆")
        .test(F5).expect("#48C 0000 0000 0000₁₆")
        .test(F5).expect("#246 0000 0000 0000₁₆")
        .test(F5).expect("#123 0000 0000 0000₁₆")
        .test(F5).expect("#91 8000 0000 0000₁₆")
        .test(F5).expect("#48 C000 0000 0000₁₆");
    step("Rotate right byte")
        .test(LSHIFT, F5).expect("#48C0 0000 0000₁₆")
        .test(LSHIFT, F5).expect("#48 C000 0000₁₆")
        .test(LSHIFT, F5).expect("#48C0 0000₁₆")
        .test(LSHIFT, F5).expect("#48 C000₁₆")
        .test(LSHIFT, F5).expect("#48C0₁₆")
        .test(LSHIFT, F5).expect("#C000 0000 0000 0048₁₆");
    step("Arithmetic shift right byte")
        .test(LSHIFT, F3).expect("#FFC0 0000 0000 0000₁₆")
        .test(LSHIFT, F3).expect("#FFFF C000 0000 0000₁₆")
        .test(LSHIFT, F3).expect("#FFFF FFC0 0000 0000₁₆")
        .test(LSHIFT, F3).expect("#FFFF FFFF C000 0000₁₆")
        .test(LSHIFT, F3).expect("#FFFF FFFF FFC0 0000₁₆");
    step("Arithmetic shift right")
        .test(F3).expect("#FFFF FFFF FFE0 0000₁₆")
        .test(F3).expect("#FFFF FFFF FFF0 0000₁₆")
        .test(F3).expect("#FFFF FFFF FFF8 0000₁₆")
        .test(F3).expect("#FFFF FFFF FFFC 0000₁₆")
        .test(F3).expect("#FFFF FFFF FFFE 0000₁₆");
    step("Shift left byte")
        .test(LSHIFT, F1).expect("#FFFF FFFF FE00 0000₁₆")
        .test(LSHIFT, F1).expect("#FFFF FFFE 0000 0000₁₆")
        .test(LSHIFT, F1).expect("#FFFF FE00 0000 0000₁₆")
        .test(LSHIFT, F1).expect("#FFFE 0000 0000 0000₁₆");
    step("Shift right byte")
        .test(LSHIFT, F2).expect("#FF FE00 0000 0000₁₆")
        .test(LSHIFT, F2).expect("#FFFE 0000 0000₁₆")
        .test(LSHIFT, F2).expect("#FF FE00 0000₁₆")
        .test(LSHIFT, F2).expect("#FFFE 0000₁₆");

    step("32-bit test")
        .test(CLEAR, "32 STWS", ENTER, EXIT).noerror();
    step("Shift left")
        .test(CLEAR, "#123A", LSHIFT, KEY4, F6)
        .test(F1).expect("#2474₁₆")
        .test(F1).expect("#48E8₁₆")
        .test(F1).expect("#91D0₁₆")
        .test(F1).expect("#1 23A0₁₆")
        .test(F1).expect("#2 4740₁₆")
        .test(F1).expect("#4 8E80₁₆")
        .test(F1).expect("#9 1D00₁₆")
        .test(F1).expect("#12 3A00₁₆");
    step("Shift right")
        .test(F2).expect("#9 1D00₁₆")
        .test(F2).expect("#4 8E80₁₆")
        .test(F2).expect("#2 4740₁₆")
        .test(F2).expect("#1 23A0₁₆")
        .test(F2).expect("#91D0₁₆")
        .test(F2).expect("#48E8₁₆")
        .test(F2).expect("#2474₁₆")
        .test(F2).expect("#123A₁₆")
        .test(F2).expect("#91D₁₆")
        .test(F2).expect("#48E₁₆")
        .test(F2).expect("#247₁₆")
        .test(F2).expect("#123₁₆");
    step("Rotate left")
        .test(F4).expect("#246₁₆")
        .test(F4).expect("#48C₁₆")
        .test(F4).expect("#918₁₆")
        .test(F4).expect("#1230₁₆");
    step("Rotate byte left")
        .test(LSHIFT, F4).expect("#12 3000₁₆")
        .test(LSHIFT, F4).expect("#1230 0000₁₆")
        .test(LSHIFT, F4).expect("#3000 0012₁₆")
        .test(LSHIFT, F4).expect("#1230₁₆")
        .test(LSHIFT, F4).expect("#12 3000₁₆")
        .test(LSHIFT, F4).expect("#1230 0000₁₆")
        .test(LSHIFT, F4).expect("#3000 0012₁₆");
    step("Rotate left with bit rotating")
        .test(F4).expect("#6000 0024₁₆")
        .test(F4).expect("#C000 0048₁₆")
        .test(F4).expect("#8000 0091₁₆")
        .test(F4).expect("#123₁₆")
        .test(F4).expect("#246₁₆")
        .test(F4).expect("#48C₁₆");
    step("Rotate right")
        .test(F5).expect("#246₁₆")
        .test(F5).expect("#123₁₆")
        .test(F5).expect("#8000 0091₁₆")
        .test(F5).expect("#C000 0048₁₆")
        .test(F5).expect("#6000 0024₁₆")
        .test(F5).expect("#3000 0012₁₆")
        .test(F5).expect("#1800 0009₁₆")
        .test(F5).expect("#8C00 0004₁₆")
        .test(F5).expect("#4600 0002₁₆")
        .test(F5).expect("#2300 0001₁₆")
        .test(F5).expect("#9180 0000₁₆")
        .test(F5).expect("#48C0 0000₁₆");
    step("Rotate right byte")
        .test(LSHIFT, F5).expect("#48 C000₁₆")
        .test(LSHIFT, F5).expect("#48C0₁₆")
        .test(LSHIFT, F5).expect("#C000 0048₁₆");
    step("Arithmetic shift right byte")
        .test(LSHIFT, F3).expect("#FFC0 0000₁₆")
        .test(LSHIFT, F3).expect("#FFFF C000₁₆");
    step("Arithmetic shift right")
        .test(F3).expect("#FFFF E000₁₆")
        .test(F3).expect("#FFFF F000₁₆")
        .test(F3).expect("#FFFF F800₁₆")
        .test(F3).expect("#FFFF FC00₁₆")
        .test(F3).expect("#FFFF FE00₁₆");
    step("Shift left byte")
        .test(LSHIFT, F1).expect("#FFFE 0000₁₆")
        .test(LSHIFT, F1).expect("#FE00 0000₁₆")
        .test(LSHIFT, F1).expect("#0₁₆")
        .test(LSHIFT, M).expect("#FE00 0000₁₆");
    step("Shift right byte")
        .test(LSHIFT, F2).expect("#FE 0000₁₆")
        .test(LSHIFT, F2).expect("#FE00₁₆")
        .test(LSHIFT, F2).expect("#FE₁₆")
        .test(LSHIFT, F2).expect("#0₁₆");

    step("128-bit test")
        .test(CLEAR, "128 STWS", ENTER, EXIT).noerror();
    step("Shift left")
        .test(CLEAR, "#123A", LSHIFT, KEY4, F6)
        .test(F1).expect("#2474₁₆")
        .test(F1).expect("#48E8₁₆")
        .test(F1).expect("#91D0₁₆")
        .test(F1).expect("#1 23A0₁₆")
        .test(F1).expect("#2 4740₁₆")
        .test(F1).expect("#4 8E80₁₆")
        .test(F1).expect("#9 1D00₁₆")
        .test(F1).expect("#12 3A00₁₆");
    step("Shift right")
        .test(F2).expect("#9 1D00₁₆")
        .test(F2).expect("#4 8E80₁₆")
        .test(F2).expect("#2 4740₁₆")
        .test(F2).expect("#1 23A0₁₆")
        .test(F2).expect("#91D0₁₆")
        .test(F2).expect("#48E8₁₆")
        .test(F2).expect("#2474₁₆")
        .test(F2).expect("#123A₁₆")
        .test(F2).expect("#91D₁₆")
        .test(F2).expect("#48E₁₆")
        .test(F2).expect("#247₁₆")
        .test(F2).expect("#123₁₆");
    step("Rotate left")
        .test(F4).expect("#246₁₆")
        .test(F4).expect("#48C₁₆")
        .test(F4).expect("#918₁₆")
        .test(F4).expect("#1230₁₆");
    step("Rotate byte left")
        .test(LSHIFT, F4).expect("#12 3000₁₆")
        .test(LSHIFT, F4).expect("#1230 0000₁₆")
        .test(LSHIFT, F4).expect("#12 3000 0000₁₆")
        .test(LSHIFT, F4).expect("#1230 0000 0000₁₆")
        .test(LSHIFT, F4).expect("#12 3000 0000 0000₁₆")
        .test(LSHIFT, F4).expect("#1230 0000 0000 0000₁₆")
        .test(LSHIFT, F4).expect("#12 3000 0000 0000 0000₁₆")
        .test(LSHIFT, F4).expect("#1230 0000 0000 0000 0000₁₆")
        .test(LSHIFT, F4).expect("#12 3000 0000 0000 0000 0000₁₆")
        .test(LSHIFT, F4).expect("#1230 0000 0000 0000 0000 0000₁₆")
        .test(LSHIFT, F4).expect("#12 3000 0000 0000 0000 0000 0000₁₆")
        .test(LSHIFT, F4).expect("#1230 0000 0000 0000 0000 0000 0000₁₆")
        .test(LSHIFT, F4).expect("#12 3000 0000 0000 0000 0000 0000 0000₁₆")
        .test(LSHIFT, F4).expect("#1230 0000 0000 0000 0000 0000 0000 0000₁₆")
        .test(LSHIFT, F4).expect("#3000 0000 0000 0000 0000 0000 0000 0012₁₆");
    step("Rotate left with bit rotating")
        .test(F4).expect("#6000 0000 0000 0000 0000 0000 0000 0024₁₆")
        .test(F4).expect("#C000 0000 0000 0000 0000 0000 0000 0048₁₆")
        .test(F4).expect("#8000 0000 0000 0000 0000 0000 0000 0091₁₆")
        .test(F4).expect("#123₁₆")
        .test(F4).expect("#246₁₆")
        .test(F4).expect("#48C₁₆");
    step("Rotate right")
        .test(F5).expect("#246₁₆")
        .test(F5).expect("#123₁₆")
        .test(F5).expect("#8000 0000 0000 0000 0000 0000 0000 0091₁₆")
        .test(F5).expect("#C000 0000 0000 0000 0000 0000 0000 0048₁₆")
        .test(F5).expect("#6000 0000 0000 0000 0000 0000 0000 0024₁₆")
        .test(F5).expect("#3000 0000 0000 0000 0000 0000 0000 0012₁₆")
        .test(F5).expect("#1800 0000 0000 0000 0000 0000 0000 0009₁₆")
        .test(F5).expect("#8C00 0000 0000 0000 0000 0000 0000 0004₁₆")
        .test(F5).expect("#4600 0000 0000 0000 0000 0000 0000 0002₁₆")
        .test(F5).expect("#2300 0000 0000 0000 0000 0000 0000 0001₁₆")
        .test(F5).expect("#9180 0000 0000 0000 0000 0000 0000 0000₁₆");
    step("Rotate right byte")
        .test(LSHIFT, F5).expect("#91 8000 0000 0000 0000 0000 0000 0000₁₆")
        .test(LSHIFT, F5).expect("#9180 0000 0000 0000 0000 0000 0000₁₆")
        .test(LSHIFT, F5).expect("#91 8000 0000 0000 0000 0000 0000₁₆")
        .test(LSHIFT, F5).expect("#9180 0000 0000 0000 0000 0000₁₆")
        .test(LSHIFT, F5).expect("#91 8000 0000 0000 0000 0000₁₆")
        .test(LSHIFT, F5).expect("#9180 0000 0000 0000 0000₁₆")
        .test(LSHIFT, F5).expect("#91 8000 0000 0000 0000₁₆")
        .test(LSHIFT, F5).expect("#9180 0000 0000 0000₁₆")
        .test(LSHIFT, F5).expect("#91 8000 0000 0000₁₆")
        .test(LSHIFT, F5).expect("#9180 0000 0000₁₆")
        .test(LSHIFT, F5).expect("#91 8000 0000₁₆")
        .test(LSHIFT, F5).expect("#9180 0000₁₆")
        .test(LSHIFT, F5).expect("#91 8000₁₆")
        .test(LSHIFT, F5).expect("#9180₁₆")
        .test(LSHIFT, F5).expect("#8000 0000 0000 0000 0000 0000 0000 0091₁₆");
    step("Arithmetic shift right byte")
        .test(LSHIFT, F3).expect("#FF80 0000 0000 0000 0000 0000 0000 0000₁₆")
        .test(LSHIFT, F3).expect("#FFFF 8000 0000 0000 0000 0000 0000 0000₁₆")
        .test(LSHIFT, F3).expect("#FFFF FF80 0000 0000 0000 0000 0000 0000₁₆")
        .test(LSHIFT, F3).expect("#FFFF FFFF 8000 0000 0000 0000 0000 0000₁₆")
        .test(LSHIFT, F3).expect("#FFFF FFFF FF80 0000 0000 0000 0000 0000₁₆");
    step("Arithmetic shift right")
        .test(F3).expect("#FFFF FFFF FFC0 0000 0000 0000 0000 0000₁₆")
        .test(F3).expect("#FFFF FFFF FFE0 0000 0000 0000 0000 0000₁₆")
        .test(F3).expect("#FFFF FFFF FFF0 0000 0000 0000 0000 0000₁₆")
        .test(F3).expect("#FFFF FFFF FFF8 0000 0000 0000 0000 0000₁₆")
        .test(F3).expect("#FFFF FFFF FFFC 0000 0000 0000 0000 0000₁₆");
    step("Shift left byte")
        .test(LSHIFT, F1).expect("#FFFF FFFF FC00 0000 0000 0000 0000 0000₁₆")
        .test(LSHIFT, F1).expect("#FFFF FFFC 0000 0000 0000 0000 0000 0000₁₆")
        .test(LSHIFT, F1).expect("#FFFF FC00 0000 0000 0000 0000 0000 0000₁₆")
        .test(LSHIFT, F1).expect("#FFFC 0000 0000 0000 0000 0000 0000 0000₁₆");
    step("Shift right byte")
        .test(LSHIFT, F2).expect("#FF FC00 0000 0000 0000 0000 0000 0000₁₆")
        .test(LSHIFT, F2).expect("#FFFC 0000 0000 0000 0000 0000 0000₁₆")
        .test(LSHIFT, F2).expect("#FF FC00 0000 0000 0000 0000 0000₁₆")
        .test(LSHIFT, F2).expect("#FFFC 0000 0000 0000 0000 0000₁₆");

    step("16-bit test")
        .test(CLEAR, "16 STWS", ENTER, EXIT).noerror();
    step("Shift left")
        .test(CLEAR, "#123A", LSHIFT, KEY4, F6)
        .test(F1).expect("#2474₁₆")
        .test(F1).expect("#48E8₁₆")
        .test(F1).expect("#91D0₁₆")
        .test(F1).expect("#23A0₁₆")
        .test(F1).expect("#4740₁₆")
        .test(F1).expect("#8E80₁₆")
        .test(F1).expect("#1D00₁₆")
        .test(F1).expect("#3A00₁₆");
    step("Shift right")
        .test(F2).expect("#1D00₁₆")
        .test(F2).expect("#E80₁₆")
        .test(F2).expect("#740₁₆")
        .test(F2).expect("#3A0₁₆")
        .test(F2).expect("#1D0₁₆");
    step("Rotate left")
        .test(F4).expect("#3A0₁₆")
        .test(F4).expect("#740₁₆")
        .test(F4).expect("#E80₁₆")
        .test(F4).expect("#1D00₁₆")
        .test(F4).expect("#3A00₁₆")
        .test(F4).expect("#7400₁₆")
        .test(F4).expect("#E800₁₆")
        .test(F4).expect("#D001₁₆");
    step("Rotate byte left")
        .test(LSHIFT, F4).expect("#1D0₁₆")
        .test(LSHIFT, F4).expect("#D001₁₆")
        .test(LSHIFT, F4).expect("#1D0₁₆")
        .test(LSHIFT, F4).expect("#D001₁₆");
    step("Rotate left with bit rotating")
        .test(F4).expect("#A003₁₆")
        .test(F4).expect("#4007₁₆")
        .test(F4).expect("#800E₁₆")
        .test(F4).expect("#1D₁₆");
    step("Rotate right")
        .test(F5).expect("#800E₁₆")
        .test(F5).expect("#4007₁₆")
        .test(F5).expect("#A003₁₆")
        .test(F5).expect("#D001₁₆")
        .test(F5).expect("#E800₁₆")
        .test(F5).expect("#7400₁₆")
        .test(F5).expect("#3A00₁₆")
        .test(F5).expect("#1D00₁₆")
        .test(F5).expect("#E80₁₆")
        .test(F5).expect("#740₁₆")
        .test(F5).expect("#3A0₁₆")
        .test(F5).expect("#1D0₁₆");
    step("Rotate right byte")
        .test(LSHIFT, F5).expect("#D001₁₆")
        .test(LSHIFT, F5).expect("#1D0₁₆")
        .test(LSHIFT, F5).expect("#D001₁₆");
    step("Arithmetic shift right byte")
        .test(LSHIFT, F3).expect("#FFD0₁₆")
        .test(LSHIFT, F3).expect("#FFFF₁₆")
        .test(LSHIFT, F3).expect("#FFFF₁₆");
    step("Shift left byte")
        .test(LSHIFT, F1).expect("#FF00₁₆")
        .test(LSHIFT, F1).expect("#0₁₆")
        .test(LSHIFT, M).expect("#FF00₁₆");
    step("Arithmetic shift right")
        .test(F3).expect("#FF80₁₆")
        .test(F3).expect("#FFC0₁₆")
        .test(F3).expect("#FFE0₁₆")
        .test(F3).expect("#FFF0₁₆");
    step("Shift right byte")
        .test(LSHIFT, F2).expect("#FF₁₆")
        .test(LSHIFT, F2).expect("#0₁₆");

    step("13-bit test")
        .test(CLEAR, "13 STWS", ENTER, EXIT).noerror();
    step("Shift left")
        .test(CLEAR, "#123A", LSHIFT, KEY4, F6)
        .test(F1).expect("#474₁₆")
        .test(F1).expect("#8E8₁₆")
        .test(F1).expect("#11D0₁₆")
        .test(F1).expect("#3A0₁₆")
        .test(F1).expect("#740₁₆")
        .test(F1).expect("#E80₁₆")
        .test(F1).expect("#1D00₁₆");
    step("Shift right")
        .test(F2).expect("#E80₁₆")
        .test(F2).expect("#740₁₆")
        .test(F2).expect("#3A0₁₆")
        .test(F2).expect("#1D0₁₆")
        .test(F2).expect("#E8₁₆");
    step("Rotate left")
        .test(F4).expect("#1D0₁₆")
        .test(F4).expect("#3A0₁₆")
        .test(F4).expect("#740₁₆")
        .test(F4).expect("#E80₁₆")
        .test(F4).expect("#1D00₁₆")
        .test(F4).expect("#1A01₁₆")
        .test(F4).expect("#1403₁₆")
        .test(F4).expect("#807₁₆")
        .test(F4).expect("#100E₁₆")
        .test(F4).expect("#1D₁₆");
    step("Rotate byte left")
        .test(LSHIFT, F4).expect("#1D00₁₆")
        .test(LSHIFT, F4).expect("#E8₁₆")
        .test(LSHIFT, F4).expect("#807₁₆")
        .test(LSHIFT, F4).expect("#740₁₆")
        .test(LSHIFT, F4).expect("#3A₁₆")
        .test(LSHIFT, F4).expect("#1A01₁₆")
        .test(LSHIFT, F4).expect("#1D0₁₆");
    step("Rotate left with bit rotating")
        .test(F4).expect("#3A0₁₆")
        .test(F4).expect("#740₁₆")
        .test(F4).expect("#E80₁₆")
        .test(F4).expect("#1D00₁₆")
        .test(F4).expect("#1A01₁₆")
        .test(F4).expect("#1403₁₆");
    step("Rotate right")
        .test(F5).expect("#1A01₁₆")
        .test(F5).expect("#1D00₁₆")
        .test(F5).expect("#E80₁₆")
        .test(F5).expect("#740₁₆")
        .test(F5).expect("#3A0₁₆")
        .test(F5).expect("#1D0₁₆")
        .test(F5).expect("#E8₁₆")
        .test(F5).expect("#74₁₆")
        .test(F5).expect("#3A₁₆")
        .test(F5).expect("#1D₁₆")
        .test(F5).expect("#100E₁₆")
        .test(F5).expect("#807₁₆");
    step("Rotate right byte")
        .test(LSHIFT, F5).expect("#E8₁₆")
        .test(LSHIFT, F5).expect("#1D00₁₆")
        .test(LSHIFT, F5).expect("#1D₁₆")
        .test(LSHIFT, F5).expect("#3A0₁₆")
        .test(LSHIFT, F5).expect("#1403₁₆")
        .test(LSHIFT, F5).expect("#74₁₆")
        .test(LSHIFT, F5).expect("#E80₁₆")
        .test(LSHIFT, F5).expect("#100E₁₆");
    step("Arithmetic shift right")
        .test(F3).expect("#1807₁₆")
        .test(F3).expect("#1C03₁₆")
        .test(F3).expect("#1E01₁₆")
        .test(F3).expect("#1F00₁₆")
        .test(F3).expect("#1F80₁₆");
    step("Arithmetic shift right byte")
        .test(LSHIFT, F3).expect("#1FFF₁₆")
        .test(LSHIFT, F3).expect("#1FFF₁₆")
        .test(LSHIFT, F3).expect("#1FFF₁₆");
    step("Shift left byte")
        .test(LSHIFT, F1).expect("#1F00₁₆")
        .test(LSHIFT, F1).expect("#0₁₆")
        .test(RSHIFT, M).expect("#1F00₁₆");
    step("Shift right byte")
        .test(LSHIFT, F2).expect("#1F₁₆")
        .test(LSHIFT, F2).expect("#0₁₆")
        .test(LSHIFT, F2).expect("#0₁₆")
        .test(RSHIFT, M).expect("#0₁₆");

    step("72-bit test")
        .test(CLEAR, "72 STWS", ENTER, EXIT).noerror();
    step("Shift left")
        .test(CLEAR, "#123A", LSHIFT, KEY4, F6)
        .test(F1).expect("#2474₁₆")
        .test(F1).expect("#48E8₁₆")
        .test(F1).expect("#91D0₁₆")
        .test(F1).expect("#1 23A0₁₆")
        .test(F1).expect("#2 4740₁₆")
        .test(F1).expect("#4 8E80₁₆")
        .test(F1).expect("#9 1D00₁₆")
        .test(F1).expect("#12 3A00₁₆");
    step("Shift right")
        .test(F2).expect("#9 1D00₁₆")
        .test(F2).expect("#4 8E80₁₆")
        .test(F2).expect("#2 4740₁₆")
        .test(F2).expect("#1 23A0₁₆")
        .test(F2).expect("#91D0₁₆")
        .test(F2).expect("#48E8₁₆")
        .test(F2).expect("#2474₁₆")
        .test(F2).expect("#123A₁₆")
        .test(F2).expect("#91D₁₆")
        .test(F2).expect("#48E₁₆")
        .test(F2).expect("#247₁₆")
        .test(F2).expect("#123₁₆");
    step("Rotate left")
        .test(F4).expect("#246₁₆")
        .test(F4).expect("#48C₁₆")
        .test(F4).expect("#918₁₆")
        .test(F4).expect("#1230₁₆");
    step("Rotate byte left")
        .test(LSHIFT, F4).expect("#12 3000₁₆")
        .test(LSHIFT, F4).expect("#1230 0000₁₆")
        .test(LSHIFT, F4).expect("#12 3000 0000₁₆")
        .test(LSHIFT, F4).expect("#1230 0000 0000₁₆")
        .test(LSHIFT, F4).expect("#12 3000 0000 0000₁₆")
        .test(LSHIFT, F4).expect("#1230 0000 0000 0000₁₆")
        .test(LSHIFT, F4).expect("#12 3000 0000 0000 0000₁₆")
        .test(LSHIFT, F4).expect("#30 0000 0000 0000 0012₁₆")
        .test(LSHIFT, F4).expect("#1230₁₆")
        .test(LSHIFT, F4).expect("#12 3000₁₆")
        .test(LSHIFT, F4).expect("#1230 0000₁₆")
        .test(LSHIFT, F4).expect("#12 3000 0000₁₆")
        .test(LSHIFT, F4).expect("#1230 0000 0000₁₆")
        .test(LSHIFT, F4).expect("#12 3000 0000 0000₁₆")
        .test(LSHIFT, F4).expect("#1230 0000 0000 0000₁₆")
        .test(LSHIFT, F4).expect("#12 3000 0000 0000 0000₁₆");
    step("Rotate left with bit rotating")
        .test(F4).expect("#24 6000 0000 0000 0000₁₆")
        .test(F4).expect("#48 C000 0000 0000 0000₁₆")
        .test(F4).expect("#91 8000 0000 0000 0000₁₆")
        .test(F4).expect("#23 0000 0000 0000 0001₁₆")
        .test(F4).expect("#46 0000 0000 0000 0002₁₆")
        .test(F4).expect("#8C 0000 0000 0000 0004₁₆");
    step("Rotate right")
        .test(F5).expect("#46 0000 0000 0000 0002₁₆")
        .test(F5).expect("#23 0000 0000 0000 0001₁₆")
        .test(F5).expect("#91 8000 0000 0000 0000₁₆")
        .test(F5).expect("#48 C000 0000 0000 0000₁₆")
        .test(F5).expect("#24 6000 0000 0000 0000₁₆")
        .test(F5).expect("#12 3000 0000 0000 0000₁₆")
        .test(F5).expect("#9 1800 0000 0000 0000₁₆")
        .test(F5).expect("#4 8C00 0000 0000 0000₁₆")
        .test(F5).expect("#2 4600 0000 0000 0000₁₆")
        .test(F5).expect("#1 2300 0000 0000 0000₁₆")
        .test(F5).expect("#9180 0000 0000 0000₁₆")
        .test(F5).expect("#48C0 0000 0000 0000₁₆");
    step("Rotate right byte")
        .test(LSHIFT, F5).expect("#48 C000 0000 0000₁₆")
        .test(LSHIFT, F5).expect("#48C0 0000 0000₁₆")
        .test(LSHIFT, F5).expect("#48 C000 0000₁₆")
        .test(LSHIFT, F5).expect("#48C0 0000₁₆")
        .test(LSHIFT, F5).expect("#48 C000₁₆")
        .test(LSHIFT, F5).expect("#48C0₁₆")
        .test(LSHIFT, F5).expect("#C0 0000 0000 0000 0048₁₆");
    step("Arithmetic shift right byte")
        .test(LSHIFT, F3).expect("#FF C000 0000 0000 0000₁₆")
        .test(LSHIFT, F3).expect("#FF FFC0 0000 0000 0000₁₆")
        .test(LSHIFT, F3).expect("#FF FFFF C000 0000 0000₁₆")
        .test(LSHIFT, F3).expect("#FF FFFF FFC0 0000 0000₁₆")
        .test(LSHIFT, F3).expect("#FF FFFF FFFF C000 0000₁₆");
    step("Arithmetic shift right")
        .test(F3).expect("#FF FFFF FFFF E000 0000₁₆")
        .test(F3).expect("#FF FFFF FFFF F000 0000₁₆")
        .test(F3).expect("#FF FFFF FFFF F800 0000₁₆")
        .test(F3).expect("#FF FFFF FFFF FC00 0000₁₆")
        .test(F3).expect("#FF FFFF FFFF FE00 0000₁₆");
    step("Shift left byte")
        .test(LSHIFT, F1).expect("#FF FFFF FFFE 0000 0000₁₆")
        .test(LSHIFT, F1).expect("#FF FFFF FE00 0000 0000₁₆")
        .test(LSHIFT, F1).expect("#FF FFFE 0000 0000 0000₁₆")
        .test(LSHIFT, F1).expect("#FF FE00 0000 0000 0000₁₆");
    step("Shift right byte")
        .test(LSHIFT, F2).expect("#FFFE 0000 0000 0000₁₆")
        .test(LSHIFT, F2).expect("#FF FE00 0000 0000₁₆")
        .test(LSHIFT, F2).expect("#FFFE 0000 0000₁₆")
        .test(LSHIFT, F2).expect("#FF FE00 0000₁₆");

}


void tests::flags_functions()
// ----------------------------------------------------------------------------
//    Check the user flag functions
// ----------------------------------------------------------------------------
{
    BEGIN(flags);

    const uint nflags = 11;

    step("Check that flags are initially clear");
    for (uint f = 0; f < nflags; f++)
        test(CLEAR, (f * 23) % 128, " FS?", ENTER).noerror().expect("False");

    step("Setting random flags");
    large fset = lrand48() & ((1<<nflags)-1);
    for (uint f = 0; f < 13; f++)
        test(CLEAR,
             (f * 23) % 128, (fset & (1<<f)) ? " SF" : " CF", ENTER).noerror();

    step("Getting flags value")
        .test(CLEAR, LSHIFT, KEY6, LSHIFT, F1).noerror()
        .type(object::ID_based_bignum);
    step("Clearing flag values from menu")
        .test("#0", LSHIFT, F2).noerror();
    step("Check that flags are initially clear");
    for (uint f = 0; f < nflags; f++)
        if (fset & (1 << f))
            test((f * 23) % 128, LSHIFT, F5).expect("False").test(BSP);
        else
            test((f * 23) % 128, LSHIFT, F6).expect("True").test(BSP);
    step("Restore values of flags from binary")
        .test(LSHIFT, KEY6, LSHIFT, F2).noerror();

    step("Check that flags were set as expected");
    for (uint f = 0; f < nflags; f++)
        test(CLEAR, (f * 23) % 128, " FS?", ENTER)
            .expect((fset & (1<<f)) ? "True" : "False");
    step("Check that flags were clear as expected");
    for (uint f = 0; f < nflags; f++)
        test(CLEAR, (f * 23) % 128, " FC?", ENTER)
            .expect((fset & (1<<f)) ? "False" : "True");
    step("Check that flags were set and set them");
    for (uint f = 0; f < nflags; f++)
        test(CLEAR, (f * 23) % 128, " FS?C", ENTER)
            .expect((fset & (1<<f)) ? "True" : "False");
    step("Check that flags were set them");
    for (uint f = 0; f < nflags; f++)
        test(CLEAR, (f * 23) % 128, " FC?", ENTER).expect("True");

    step("Setting random flags (inverse pattern) using menu")
        .test(CLEAR, LSHIFT, KEY6);
    for (uint f = 0; f < 13; f++)
        test(CLEAR,
             (f * 23) % 128, (fset & (1<<f)) ? F2 : F1).noerror();
    step("Check that flags were clear and clear them");
    for (uint f = 0; f < nflags; f++)
        test(CLEAR, (f * 23) % 128, F6, ENTER)
            .expect((fset & (1<<f)) ? "True" : "False");
    step("Check that flags were all clear");
    for (uint f = 0; f < nflags; f++)
        test(CLEAR, (f * 23) % 128, " FC?", ENTER).expect("True");
    step("Clear flags with menus");
    for (uint f = 0; f < 13; f++)
        test(CLEAR, (f * 23) % 128, F2).noerror();
    step("Check that flags are still all clear");
    for (uint f = 0; f < nflags; f++)
        test(CLEAR, (f * 23) % 128, " FC?", ENTER).expect("True");

    step("Flipping the bits to revert to original pattern using menu")
        .test(CLEAR, LSHIFT, KEY6);
    for (uint f = 0; f < 13; f++)
        if (fset & (1<<f))
            test(CLEAR, (f * 23) % 128, LSHIFT, F4).noerror();
    step("Check that required flags were flipped using FC?");
    for (uint f = 0; f < nflags; f++)
        test(CLEAR, (f * 23) % 128, " FC?", ENTER)
            .expect((fset & (1<<f)) ? "False" : "True");
    step("Check that required flags were flipped using FS?C");
    for (uint f = 0; f < nflags; f++)
        test(CLEAR, (f * 23) % 128, " FS?C", ENTER)
            .expect((fset & (1<<f)) ? "True" : "False");

    step("Check that flags are all clear at end");
    for (uint f = 0; f < nflags; f++)
        test(CLEAR, (f * 23) % 128, " FC?", ENTER).expect("True");
}


void tests::flags_by_name()
// ----------------------------------------------------------------------------
//   Set and clear all flags by name
// ----------------------------------------------------------------------------
{
    BEGIN(sysflags);

#define ID(id)
#define FLAG(Enable, Disable)                           \
    step("Setting flag " #Enable)                       \
        .test(#Enable, ENTER)                           \
        .noerror();                                     \
    step("Clearing flag " #Disable " (default)")        \
        .test(#Disable, ENTER)                          \
        .noerror();
#define SETTING(Name, Low, High, Init)          \
    step("Setting " #Name " to default " #Init) \
        .noerror();
#include "ids.tbl"
}


void tests::settings_by_name()
// ----------------------------------------------------------------------------
//   Set and clear all settings by name
// ----------------------------------------------------------------------------
{
    BEGIN(settings);

#define ID(id)
#define FLAG(Enable, Disable)
#define SETTING(Name, Low, High, Init)                  \
    step("Getting " #Name " current value")             \
        .test("'" #Name "' RCL", ENTER)                 \
        .noerror();                                       \
    step("Setting " #Name " to its current value")      \
        .test("" #Name "", ENTER)                       \
        .noerror();
#include "ids.tbl"
}


void tests::parsing_commands_by_name()
// ----------------------------------------------------------------------------
//   Set and clear all settings by name
// ----------------------------------------------------------------------------
{
    BEGIN(commands);

#define SPECIAL(ty, ref, name, rname)                                   \
    (object::ID_##ty == object::ID_##ref && strcmp(name, rname) == 0)

#define ALIAS(ty, name)                                                 \
    if (object::is_command(object::ID_##ty))                            \
    {                                                                   \
        if (name)                                                       \
        {                                                               \
            step("Parsing " #name " for " #ty);                         \
            if (SPECIAL(ty, inv,                name, "x⁻¹")      ||    \
                SPECIAL(ty, sq,                 name, "x²")       ||    \
                SPECIAL(ty, cubed,              name, "x³")       ||    \
                SPECIAL(ty, cbrt,               name, "∛")        ||    \
                SPECIAL(ty, hypot,              name, "⊿")        ||    \
                SPECIAL(ty, atan2,              name, "∠")        ||    \
                SPECIAL(ty, asin,               name, "sin⁻¹")    ||    \
                SPECIAL(ty, acos,               name, "cos⁻¹")    ||    \
                SPECIAL(ty, atan,               name, "tan⁻¹")    ||    \
                SPECIAL(ty, asinh,              name, "sinh⁻¹")   ||    \
                SPECIAL(ty, acosh,              name, "cosh⁻¹")   ||    \
                SPECIAL(ty, atanh,              name, "tanh⁻¹")   ||    \
                SPECIAL(ty, RealToRectangular,  name, "ℝ→ℂ")      ||    \
                SPECIAL(ty, RectangularToReal,  name, "ℂ→ℝ")      ||    \
                SPECIAL(ty, RealToPolar,        name, "ℝ→Polarℂ") ||    \
                SPECIAL(ty, PolarToReal,        name, "Polarℂ→ℝ") ||    \
                SPECIAL(ty, ToRectangular,      name, "→Rectℂ")   ||    \
                SPECIAL(ty, SumOfXSquares,      name, "ΣX²")      ||    \
                SPECIAL(ty, SumOfYSquares,      name, "ΣY²")      ||    \
                false)                                                  \
                                                                        \
            {                                                           \
                test(CLEAR, "{ " #ty " }", ENTER, DOWN,                 \
                     ENTER, "1 GET", ENTER)                             \
                    .type(object::ID_##ty);                             \
            }                                                           \
            else                                                        \
            {                                                           \
                test(CLEAR, "{ ", (cstring) name, " } 1 GET", ENTER)    \
                    .type(object::ID_##ty);                             \
            }                                                           \
        }                                                               \
    }
#define ID(ty)                  ALIAS(ty, #ty)
#define NAMED(ty, name)         ALIAS(ty, name) ALIAS(ty, #ty)
#include "ids.tbl"
}


void tests::hms_dms_operations()
// ----------------------------------------------------------------------------
//   Test HMS and DMS operations
// ----------------------------------------------------------------------------
{
    BEGIN(hms);

    step("HMS data type")
        .test(CLEAR, "1.5_hms", ENTER).expect("1:30:00");
    step("DMS data type")
        .test(CLEAR, "1.7550_dms", ENTER).expect("1°45′18″");
    step("Creating DMS using fractions menu")
        .test(CLEAR, "1.2345", LSHIFT, H)
        .test(F6).expect("1 ¹⁹/₄₈")
        .test(F5).expect("1°23′45″");
    step("Creating DMS by adding zero")
        .test(CLEAR, "1.4241 0", LSHIFT, H)
        .test(LSHIFT, F3).expect("1°42′41″");
    step("Creating DMS by subtracting one")
        .test(CLEAR, "1.4241 1", LSHIFT, H)
        .test(LSHIFT, F4).expect("0°42′41″");
    step("HMS addition")
        .test(CLEAR, "1.4241 1.2333 HMS+", ENTER).expect("3:06:14");
    step("DMS addition")
        .test(CLEAR, "1.4241 1.2333 DMS+", ENTER).expect("3°06′14″");
    step("DMS addition through menu")
        .test(CLEAR, "1.4241 1.2333", LSHIFT, H, LSHIFT, F3).expect("3°06′14″");
    step("HMS subtraction")
        .test(CLEAR, "1.4241 1.2333 HMS-", ENTER).expect("0:19:08");
    step("DMS subtraction")
        .test(CLEAR, "1.4241 1.2333 DMS-", ENTER).expect("0°19′08″");
    step("DMS subtraction through menu")
        .test(CLEAR, "1.4241 1.2333", LSHIFT, H, LSHIFT, F4).expect("0°19′08″");
    step("DMS multiplication")
        .test(CLEAR, "1.2345", LSHIFT, H)
        .test(F6).expect("1 ¹⁹/₄₈")
        .test(F5).expect("1°23′45″")
        .test(2, MUL).expect("2°47′30″");
    step("DMS division")
        .test(2, DIV).expect("1°23′45″")
        .test(3, DIV).expect("0°27′55″")
        .test(5, DIV).expect("0°05′35″")
        .test(12, DIV).expect("0°00′27″¹¹/₁₂");

    step("Entering integral DMS using two dots")
        .test(CLEAR)
        .test(1, DOT).editor("1.")
        .test(DOT).editor("1°_dms")
        .test(ENTER).expect("1°00′00″");
    step("Entering DMS degree/minutes values using two dots")
        .test(CLEAR)
        .test(1, DOT).editor("1.")
        .test(2, DOT).editor("1°2′_dms")
        .test(ENTER).expect("1°02′00″");
    step("Entering DMS degree/minutes/seconds values using two dots")
        .test(CLEAR)
        .test(1, DOT).editor("1.")
        .test(2, DOT).editor("1°2′_dms")
        .test(3).editor("1°2′3_dms")
        .test(ENTER).expect("1°02′03″");
    step("Entering degrees/minutes/seconds using three dots")
        .test(CLEAR)
        .test(1, DOT).editor("1.")
        .test(2,DOT).editor("1°2′_dms")
        .test(35,DOT).editor("1°2′35″_dms")
        .test(ENTER).expect("1°02′35″");
    step("Entering degrees/minutes/seconds/fraction using four dots")
        .test(CLEAR)
        .test(1, DOT).editor("1.")
        .test(2,DOT).editor("1°2′_dms")
        .test(35,DOT).editor("1°2′35″_dms")
        .test(42,DOT).editor("1°2′35″42/_dms")
        .test(100).editor("1°2′35″42/100_dms")
        .test(ENTER).expect("1°02′35″²¹/₅₀");
    step("Cancelling DMS with third dot")
        .test(CLEAR)
        .test(1, DOT).editor("1.")
        .test(DOT).editor("1°_dms")
        .test(DOT).editor("1.")
        .test(ENTER).expect("1.");
    step("DMS disabled in text")
        .test(CLEAR, RSHIFT, ENTER,"1", NOSHIFT, DOT).editor("\"1.\"")
        .test(DOT).editor("\"1..\"")
        .test(DOT).editor("\"1...\"")
        .test(ENTER).expect("\"1...\"");

    step("Converting DMS to HMS")
        .test(CLEAR)
        .test(1, DOT, 2, DOT, 3, ENTER).expect("1°02′03″")
        .test(LSHIFT, H, LSHIFT, F5).expect("1:02:03")
        .test(F5).expect("1°02′03″")
        .test(F5).noerror().expect("1°02′03″")
        .test(LSHIFT, F5).noerror().expect("1:02:03")
        .test(LSHIFT, F5).noerror().expect("1:02:03");

    step("Converting angles to DMS")
        .test(CLEAR, "0.5", LSHIFT, J, A, RSHIFT, F1).expect("30°00′00″");
}


void tests::date_operations()
// ----------------------------------------------------------------------------
//   Test date-related operations
// ----------------------------------------------------------------------------
{
    BEGIN(date);

    step("Displaying a date")
        .test(CLEAR, "19681205_date", ENTER)
        .expect("Thu 5/Dec/1968");
    step("Displaying a date with a time")
        .test(CLEAR, "19690217.035501_date", ENTER)
        .expect("Mon 17/Feb/1969, 3:55:01");
    step("Displaying a date with a fractional time")
        .test(CLEAR, "19690217.03550197_date", ENTER)
        .expect("Mon 17/Feb/1969, 3:55:01.97");
    step("Displaying invalid date and time")
        .test(CLEAR, "999999999.99999999_date", ENTER)
        .expect("Sat 99/99/99999, 99:99:99.99");

    step("Difference between two dates using DDays")
        .test(CLEAR, "20230908", ENTER)
        .expect("20 230 908")
        .test("19681205", ENTER)
        .expect("19 681 205")
        .test("DDays", ENTER)
        .expect("20 000 d");
    step("Difference between two dates using DDays (units)")
        .test(CLEAR, "19681205_date", ENTER)
        .expect("Thu 5/Dec/1968")
        .test("20230908_date", ENTER)
        .expect("Fri 8/Sep/2023")
        .test("DDays", ENTER)
        .expect("-20 000 d");
    step("Difference between two dates using sub")
        .test(CLEAR, "19681205_date", ENTER)
        .expect("Thu 5/Dec/1968")
        .test("20230908_date", ENTER)
        .expect("Fri 8/Sep/2023")
        .test(SUB)
        .expect("-20 000 d");
    step("Adding days to a date (before)")
        .test("20240217_date", ENTER, NOSHIFT, ADD)
        .expect("Fri 16/May/1969");
    step("Adding days to a date (after)")
        .test(CLEAR, "20240217_date", ENTER)
        .expect("Sat 17/Feb/2024")
        .test("42", NOSHIFT, ADD)
        .expect("Sat 30/Mar/2024");
    step("Subtracting days to a date")
        .test("116", NOSHIFT, SUB)
        .expect("Tue 5/Dec/2023");
    step("Subtracting days to a date (with day unit)")
        .test("112_d", NOSHIFT, SUB)
        .expect("Tue 15/Aug/2023");
    step("Adding days to a date (with time unit)")
        .test("112_h", NOSHIFT, ADD)
        .expect("Sat 19/Aug/2023, 16:00:00");

    step("Runing TEVAL to time something")
        .test(CLEAR, LSHIFT, RUNSTOP,
              "0 1 10 FOR i i + 0.01 WAIT NEXT", ENTER,
              "TEVAL", ENTER, WAIT(200)).noerror()
        .match("duration:[1-3][0-9][0-9] ms");
}


void tests::online_help()
// ----------------------------------------------------------------------------
//   Check the online help system
// ----------------------------------------------------------------------------
{
    BEGIN(help);

    step("Main menu shows help as F1")
        .test(CLEAR, EXIT, A, F1).wait(100).noerror()
        .image_noheader("help");
    step("Exiting help with EXIT")
        .test(EXIT).noerror()
        .image_noheader("help-exit");
    step("Help with keyboard shortcut")
        .test(CLEAR, RSHIFT, ADD).noerror()
        .image_noheader("help");
    step("Following link with ENTER")
        .test(ENTER).noerror()
        .image_noheader("help-topic");
    step("Help with command line")
        .test(CLEAR, "help", ENTER).noerror()
        .image_noheader("help");
    step("History across invokations")
        .test(NOSHIFT, BSP).noerror()
        .image_noheader("help-topic");
    step("Help topic - Integers")
        .test(CLEAR, EXIT, "123", RSHIFT, ADD).noerror()
        .image_noheader("help-integers");
    step("Help topic - Decimal")
        .test(CLEAR, EXIT, "123.5", RSHIFT, ADD).noerror()
        .image_noheader("help-decimal");
    step("Help topic - topic")
        .test(CLEAR, EXIT, "\"authors\"",
              NOSHIFT, RSHIFT, ADD, DOWN, DOWN, DOWN, DOWN)
        .noerror()
        .image_noheader("help-authors");
    step("Returning to main screen with F1")
        .test(F1).noerror()
        .image_noheader("help");
    step("Page up and down with F2 and F3")
        .test(F3).noerror()
        .image_noheader("help-page2")
        .test(F3).noerror()
        .image_noheader("help-page3")
        .test(F2).noerror()
        .image_noheader("help-page4")
        .test(F3).noerror()
        .image_noheader("help-page5");
    step("Follow link with ENTER")
        .test(ENTER).noerror()
        .image_noheader("help-help");
    step("Back to previous topic with BSP")
        .test(BSP).noerror()
        .image_noheader("help-page6");
    step("Next link with F5")
        .test(F2, F3, F3, F5, ENTER).noerror()
        .image_noheader("help-keyboard");
    step("Back with F6")
        .test(F6).noerror()
        .image_noheader("help-page7");
    step("Previous topic with F4")
        .test(F4).noerror()
        .image_noheader("help-page8");
    step("Select topic with ENTER")
        .test(ENTER).wait(200).noerror()
        .image_noheader("help-design");
    step("Exit to normal command line")
        .test(EXIT, CLEAR, EXIT).noerror();
    step("Invoke help about SIN command with long press")
        .test(LONGPRESS, J).wait(20)
        .image_noheader("help-sin");
    step("Invoke help about COS command with long press")
        .test(EXIT, LONGPRESS, K)
        .image_noheader("help-cos");
    step("Invoke help about DEG menu command with long press")
        .test(EXIT, SHIFT, N, LONGPRESS, F1)
        .image_noheader("help-degrees");
    step("Exit and cleanup")
        .test(EXIT, CLEAR, EXIT);
}


void tests::infinity_and_undefined()
// ----------------------------------------------------------------------------
//   Check infinity and undefined operations
// ----------------------------------------------------------------------------
{
    BEGIN(infinity);

    step("Divide by zero error (integer)")
        .test(CLEAR, "1 0", ENTER, NOSHIFT, DIV)
        .error("Divide by zero");
    step("Divide by zero error (decimal)")
        .test(CLEAR, "1.0 0.0", ENTER, NOSHIFT, DIV)
        .error("Divide by zero");
    step("Divide by zero error (bignum)")
        .test(CLEAR, "2 100 ^ 0", ENTER, NOSHIFT, DIV)
        .error("Divide by zero");
    step("Divide by zero error (fractions)")
        .test(CLEAR, "1/3 0", ENTER, NOSHIFT, DIV)
        .error("Divide by zero");

    step("Setting infinity flag")
        .test(CLEAR, "'InfinityValue' FS?", ENTER).expect("False")
        .test("-22 SF", ENTER).noerror()
        .test("'InfinityValue' FS?", ENTER).expect("True");

    step("Clear infinite result flag")
        .test("-26 CF", ENTER).noerror()
        .test("'InfiniteResultIndicator' FS?", ENTER).expect("False");

    step("Divide by zero as symbolic infinity (integer)")
        .test(CLEAR, "-26 FS?", ENTER).expect("False")
        .test("1 0", ENTER, NOSHIFT, DIV)
        .expect("∞")
        .test("-26 FS?C", ENTER).expect("True");
    step("Divide by zero as symbolic infinity (decimal)")
        .test(CLEAR, "-26 FS?", ENTER).expect("False")
        .test("1.0 0.0", ENTER, NOSHIFT, DIV)
        .expect("∞")
        .test("-26 FS?C", ENTER).expect("True");
    step("Divide by zero as symbolic infinity (bignum)")
        .test(CLEAR, "-26 FS?", ENTER).expect("False")
        .test("2 100 ^ 0", ENTER, NOSHIFT, DIV)
        .expect("∞")
        .test("'InfiniteResultIndicator' FS?C", ENTER).expect("True");
    step("Divide by zero as symbolic infinity (fractions)")
        .test(CLEAR, "-26 FS?", ENTER).expect("False")
        .test("1/3 0", ENTER, NOSHIFT, DIV)
        .expect("∞")
        .test("'InfiniteResultIndicator' FS?C", ENTER).expect("True");

    step("Divide by zero as symbolic infinity (negative integer)")
        .test(CLEAR, "-26 FS?", ENTER).expect("False")
        .test("-1 0", ENTER, NOSHIFT, DIV)
        .expect("'-∞'")
        .test("-26 FS?C", ENTER).expect("True");
    step("Divide by zero as symbolic infinity (decimal)")
        .test(CLEAR, "-26 FS?", ENTER).expect("False")
        .test("-1.0 0.0", ENTER, NOSHIFT, DIV)
        .expect("'-∞'")
        .test("-26 FS?C", ENTER).expect("True");
    step("Divide by zero as symbolic infinity (bignum)")
        .test(CLEAR, "-26 FS?", ENTER).expect("False")
        .test("2 100 ^ NEG 0", ENTER, NOSHIFT, DIV)
        .expect("'-∞'")
        .test("-26 FS?C", ENTER).expect("True");
    step("Divide by zero as symbolic infinity (fractions)")
        .test(CLEAR, "-26 FS?", ENTER).expect("False")
        .test("-1/3 0", ENTER, NOSHIFT, DIV)
        .expect("'-∞'")
        .test("-26 FS?C", ENTER).expect("True");

    step("Setting numerical constants flag")
        .test(CLEAR, "'NumericalConstants' FS?", ENTER).expect("False")
        .test("-2 SF", ENTER).noerror()
        .test("'NumericalConstants' FS?", ENTER).expect("True");

    step("Divide by zero as numeric infinity (integer)")
        .test(CLEAR, "-26 FS?", ENTER).expect("False")
        .test("1 0", ENTER, NOSHIFT, DIV)
        .expect("9.99999⁳⁹⁹⁹⁹⁹⁹")
        .test("-26 FS?C", ENTER).expect("True");
    step("Divide by zero as numeric infinity (decimal)")
        .test(CLEAR, "-26 FS?", ENTER).expect("False")
        .test("1.0 0.0", ENTER, NOSHIFT, DIV)
        .expect("9.99999⁳⁹⁹⁹⁹⁹⁹")
        .test("-26 FS?C", ENTER).expect("True");
    step("Divide by zero as numeric infinity (bignum)")
        .test(CLEAR, "-26 FS?", ENTER).expect("False")
        .test("2 100 ^ 0", ENTER, NOSHIFT, DIV)
        .expect("9.99999⁳⁹⁹⁹⁹⁹⁹")
        .test("-26 FS?C", ENTER).expect("True");
    step("Divide by zero as numeric infinity (fractions)")
        .test(CLEAR, "-26 FS?", ENTER).expect("False")
        .test("1/3 0", ENTER, NOSHIFT, DIV)
        .expect("9.99999⁳⁹⁹⁹⁹⁹⁹")
        .test("-26 FS?C", ENTER).expect("True");

    step("Divide by zero as numeric infinity (negative integer)")
        .test(CLEAR, "-26 FS?", ENTER).expect("False")
        .test("-1 0", ENTER, NOSHIFT, DIV)
        .expect("-9.99999⁳⁹⁹⁹⁹⁹⁹")
        .test("-26 FS?C", ENTER).expect("True");
    step("Divide by zero as numeric infinity (decimal)")
        .test(CLEAR, "-26 FS?", ENTER).expect("False")
        .test("-1.0 0.0", ENTER, NOSHIFT, DIV)
        .expect("-9.99999⁳⁹⁹⁹⁹⁹⁹")
        .test("-26 FS?C", ENTER).expect("True");
    step("Divide by zero as numeric infinity (bignum)")
        .test(CLEAR, "-26 FS?", ENTER).expect("False")
        .test("2 100 ^ NEG 0", ENTER, NOSHIFT, DIV)
        .expect("-9.99999⁳⁹⁹⁹⁹⁹⁹")
        .test("-26 FS?C", ENTER).expect("True");
    step("Divide by zero as numeric infinity (fractions)")
        .test(CLEAR, "-26 FS?", ENTER).expect("False")
        .test("-1/3 0", ENTER, NOSHIFT, DIV)
        .expect("-9.99999⁳⁹⁹⁹⁹⁹⁹")
        .test("-26 FS?C", ENTER).expect("True");

    step("Clearing numerical constants flag")
        .test(CLEAR, "'NumericalConstants' FS?", ENTER).expect("True")
        .test("-2 CF", ENTER).noerror()
        .test("'NumericalConstants' FS?", ENTER).expect("False");
    step("Setting numerical results flag")
        .test(CLEAR, "'NumericalResults' FS?", ENTER).expect("False")
        .test("-3 SF", ENTER).noerror()
        .test("'NumericalResults' FS?", ENTER).expect("True");

    step("Divide by zero as numeric infinity (integer)")
        .test(CLEAR, "1 0", ENTER, NOSHIFT, DIV)
        .expect("9.99999⁳⁹⁹⁹⁹⁹⁹");
    step("Divide by zero as numeric infinity (decimal)")
        .test(CLEAR, "1.0 0.0", ENTER, NOSHIFT, DIV)
        .expect("9.99999⁳⁹⁹⁹⁹⁹⁹");
    step("Divide by zero as numeric infinity (bignum)")
        .test(CLEAR, "2 100 ^ 0", ENTER, NOSHIFT, DIV)
        .expect("9.99999⁳⁹⁹⁹⁹⁹⁹");
    step("Divide by zero as numeric infinity (fractions)")
        .test(CLEAR, "1/3 0", ENTER, NOSHIFT, DIV)
        .expect("9.99999⁳⁹⁹⁹⁹⁹⁹");

    step("Divide by zero as numeric infinity (negative integer)")
        .test(CLEAR, "-1 0", ENTER, NOSHIFT, DIV)
        .expect("-9.99999⁳⁹⁹⁹⁹⁹⁹");
    step("Divide by zero as numeric infinity (decimal)")
        .test(CLEAR, "-1.0 0.0", ENTER, NOSHIFT, DIV)
        .expect("-9.99999⁳⁹⁹⁹⁹⁹⁹");
    step("Divide by zero as numeric infinity (bignum)")
        .test(CLEAR, "2 100 ^ NEG 0", ENTER, NOSHIFT, DIV)
        .expect("-9.99999⁳⁹⁹⁹⁹⁹⁹");
    step("Divide by zero as numeric infinity (fractions)")
        .test(CLEAR, "-1/3 0", ENTER, NOSHIFT, DIV)
        .expect("-9.99999⁳⁹⁹⁹⁹⁹⁹");

    step("Clear numerical results flag")
        .test(CLEAR, "'NumericalResults' FS?", ENTER).expect("True")
        .test("-3 CF", ENTER).noerror()
        .test("'NumericalResults' FS?", ENTER).expect("False");

    step("Divide by zero as symbolic infinity (integer)")
        .test(CLEAR, "1 0", ENTER, NOSHIFT, DIV)
        .expect("∞");
    step("Divide by zero as symbolic infinity (decimal)")
        .test(CLEAR, "1.0 0.0", ENTER, NOSHIFT, DIV)
        .expect("∞");
    step("Divide by zero as symbolic infinity (bignum)")
        .test(CLEAR, "2 100 ^ 0", ENTER, NOSHIFT, DIV)
        .expect("∞");
    step("Divide by zero as symbolic infinity (fractions)")
        .test(CLEAR, "1/3 0", ENTER, NOSHIFT, DIV)
        .expect("∞");

    step("Clear infinity value flag")
        .test(CLEAR, "'InfinityError' FS?", ENTER).expect("False")
        .test("-22 CF", ENTER).noerror()
        .test("'InfinityError' FS?", ENTER).expect("True");

    step("Divide by zero error (integer)")
        .test(CLEAR, "1 0", ENTER, NOSHIFT, DIV)
        .error("Divide by zero");
    step("Divide by zero error (decimal)")
        .test(CLEAR, "1.0 0.0", ENTER, NOSHIFT, DIV)
        .error("Divide by zero");
    step("Divide by zero error (bignum)")
        .test(CLEAR, "2 100 ^ 0", ENTER, NOSHIFT, DIV)
        .error("Divide by zero");
    step("Divide by zero error (fractions)")
        .test(CLEAR, "1/3 0", ENTER, NOSHIFT, DIV)
        .error("Divide by zero");

    test(CLEAR);
}


void tests::overflow_and_underflow()
// ----------------------------------------------------------------------------
//   Test overflow and underflow
// ----------------------------------------------------------------------------
{
    BEGIN(overflow);

    step("Set maximum exponent to 499")
        .test(CLEAR, "499 MaximumDecimalExponent", ENTER).noerror()
        .test("'MaximumDecimalExponent' RCL", ENTER).expect("499");

    step("Check that undeflow error is not set by default")
        .test("'UnderflowError' FS?", ENTER).expect("False")
        .test("'UnderflowValue' FS?", ENTER).expect("True");

    step("Clear overflow and underflow indicators")
        .test("-23 CF -24 CF -25 CF -26 CF", ENTER).noerror();
    step("Check negative underflow indicator is clear")
        .test("'NegativeUnderflowIndicator' FS?", ENTER).expect("False");
    step("Check positive underflow indicator is clear")
        .test("'PositiveUnderflowIndicator' FS?", ENTER).expect("False");
    step("Check overflow indicator is clear")
        .test("'OverflowIndicator' FS?", ENTER).expect("False");
    step("Check infinite result indicator is clear")
        .test("'InfiniteResultindicator' FS?", ENTER).expect("False");

    step("Test numerical overflow as infinity for multiply")
        .test(CLEAR)
        .test("1E499 10 *", ENTER).expect("∞")
        .test("-23 FS?C", ENTER).expect("False")
        .test("-24 FS?C", ENTER).expect("False")
        .test("-25 FS?C", ENTER).expect("True");
    step("Test numerical overflow as infinity for exponential")
        .test(CLEAR)
        .test("1280 exp", ENTER).expect("∞")
        .test("-23 FS?C", ENTER).expect("False")
        .test("-24 FS?C", ENTER).expect("False")
        .test("-25 FS?C", ENTER).expect("True");
    step("Test numerical overflow as infinity")
        .test(CLEAR)
        .test("1E499 10 *", ENTER).expect("∞")
        .test("-23 FS?C", ENTER).expect("False")
        .test("-24 FS?C", ENTER).expect("False")
        .test("-25 FS?C", ENTER).expect("True");
    step("Test positive numerical underflow as zero")
        .test(CLEAR)
        .test("1E-499 10 /", ENTER).expect("0.")
        .test("-23 FS?C", ENTER).expect("False")
        .test("-24 FS?C", ENTER).expect("True")
        .test("-25 FS?C", ENTER).expect("False");
    step("Test negative numerical underflow as zero")
        .test(CLEAR)
        .test("-1E-499 10 /", ENTER).expect("-0.")
        .test("-23 FS?C", ENTER).expect("True")
        .test("-24 FS?C", ENTER).expect("False")
        .test("-25 FS?C", ENTER).expect("False");

    step("Set underflow as error")
        .test(CLEAR, "-20 SF", ENTER).noerror()
        .test("'UnderflowError' FS?", ENTER).expect("True");

    step("Test numerical overflow as infinity")
        .test(CLEAR)
        .test("1E499 10 *", ENTER).expect("∞")
        .test("-23 FS?C", ENTER).expect("False")
        .test("-24 FS?C", ENTER).expect("False")
        .test("-25 FS?C", ENTER).expect("True");
    step("Test positive numerical underflow as error")
        .test(CLEAR)
        .test("1E-499 10 /", ENTER).error("Positive numerical underflow")
        .test(BSP).expect("10")
        .test(BSP).expect("1.⁳⁻⁴⁹⁹")
        .test("-23 FS?C", ENTER).expect("False")
        .test("-24 FS?C", ENTER).expect("False")
        .test("-25 FS?C", ENTER).expect("False");
    step("Test negative numerical underflow as error")
        .test(CLEAR)
        .test("-1E-499 10 /", ENTER).error("Negative numerical underflow")
        .test(BSP).expect("10")
        .test(BSP).expect("-1.⁳⁻⁴⁹⁹")
        .test("-23 FS?C", ENTER).expect("False")
        .test("-24 FS?C", ENTER).expect("False")
        .test("-25 FS?C", ENTER).expect("False");

    step("Set overflow as error")
        .test(CLEAR, "-21 SF", ENTER).noerror()
        .test("'OverflowError' FS?", ENTER).expect("True");

    step("Test numerical overflow as infinity")
        .test(CLEAR)
        .test("1E499 10 *", ENTER).error("Numerical overflow")
        .test(BSP).expect("10")
        .test(BSP).expect("1.⁳⁴⁹⁹")
        .test("-23 FS?C", ENTER).expect("False")
        .test("-24 FS?C", ENTER).expect("False")
        .test("-25 FS?C", ENTER).expect("False");
    step("Test positive numerical underflow as error")
        .test(CLEAR)
        .test("1E-499 10 /", ENTER).error("Positive numerical underflow")
        .test(BSP).expect("10")
        .test(BSP).expect("1.⁳⁻⁴⁹⁹")
        .test("-23 FS?C", ENTER).expect("False")
        .test("-24 FS?C", ENTER).expect("False")
        .test("-25 FS?C", ENTER).expect("False");
    step("Test negative numerical underflow as error")
        .test(CLEAR)
        .test("-1E-499 10 /", ENTER).error("Negative numerical underflow")
        .test(BSP).expect("10")
        .test(BSP).expect("-1.⁳⁻⁴⁹⁹")
        .test("-23 FS?C", ENTER).expect("False")
        .test("-24 FS?C", ENTER).expect("False")
        .test("-25 FS?C", ENTER).expect("False");

    step("Reset modes")
        .test(CLEAR, "ResetModes", ENTER)
        .test("'MaximumDecimalExponent' RCL", ENTER)
        .expect("1 152 921 504 606 846 976");
}


void tests::graphic_stack_rendering()
// ----------------------------------------------------------------------------
//   Check the rendering of expressions in graphic mode
// ----------------------------------------------------------------------------
{
    BEGIN(gstack);

    step("Draw expression")
        .test(CLEAR, EXIT, EXIT)
        .test("1 'X' +", ENTER, B, C, E, "3 X 3", LSHIFT, B, MUL, ADD)
        .test(ALPHA, X, NOSHIFT, J, K, L, ADD)
        .image_noheader("expression");

    step("Two levels of stack")
        .test(CLEAR, EXIT, EXIT)
        .test("1 'X' +", ENTER, B, C, E, "3 X 3", LSHIFT, B, MUL, ADD)
        .test(ALPHA, X, NOSHIFT, J, K, L)
        .image_noheader("two-levels");

    step("Automatic reduction of size")
        .test(CLEAR, EXIT, EXIT)
        .test("1 'X' +", ENTER, B, C, E, "3 X 3", LSHIFT, B, MUL, ADD)
        .test(ALPHA, X, NOSHIFT, J, K, L, ADD, C, B, C, B)
        .image_noheader("reduced");

    step("Constants")
        .test(CLEAR, LSHIFT, I, F1, F1, F2, F3)
        .image_noheader("constants", 2);

    step("Vector")
        .test(CLEAR, LSHIFT, KEY9, "1 2 3", ENTER, EXIT)
        .wait(100)
        .image_noheader("vector-horizontal");
    step("Vector vertical rendering")
        .test("VerticalVectors", ENTER)
        .wait(100)
        .image_noheader("vector-vertical");
    step("Vector horizontal rendering")
        .test("HorizontalVectors", ENTER)
        .wait(100)
        .image_noheader("vector-horizontal");

    step("Matrix")
        .test(CLEAR, LSHIFT, KEY9,
              LSHIFT, KEY9, "1 2 3 4", DOWN,
              LSHIFT, KEY9, "4 5 6 7", DOWN,
              LSHIFT, KEY9, "8 9 10 11", DOWN,
              LSHIFT, KEY9, "12 13 14 18", ENTER, EXIT)
        .wait(100)
        .image_noheader("matrix");
    step("Matrix with smaller size")
        .test(13, DIV, ENTER, MUL)
        .wait(100)
        .image_noheader("matrix-smaller");

    step("Lists")
        .test(CLEAR, RSHIFT, SPACE, "1 2 \"ABC\"", ENTER, EXIT)
        .wait(100)
        .image_noheader("list-horizontal");
    step("List vertical")
        .test("VerticalLists", ENTER)
        .test(CLEAR, RSHIFT, SPACE, "1 2 \"ABC\"", ENTER, EXIT)
        .wait(100)
        .image_noheader("list-vertical");
    step("List horizontal")
        .test("HorizontalLists", ENTER)
        .test(CLEAR, RSHIFT, SPACE, "1 2 \"ABC\"", ENTER, EXIT)
        .wait(100)
        .image_noheader("list-horizontal");

}


void tests::insertion_of_variables_constants_and_units()
// ----------------------------------------------------------------------------
//   Check that we correctly insert constant and variables in programs
// ----------------------------------------------------------------------------
{
    BEGIN(insert);

    step("Select constant menu")
        .test(CLEAR, LSHIFT, I, F1).image_menus("constants-menu", 1);
    step("Insert pi")
        .test(CLEAR, F1).expect("π");
    step("Insert e")
        .test(CLEAR, F2).expect("e");
    step("Insert i")
        .test(CLEAR, F3).expect("ⅈ");
    step("Insert j")
        .test(CLEAR, F4).expect("ⅉ");
    step("Insert infinity")
        .test(CLEAR, F5).expect("∞");
    step("Insert undefined")
        .test(CLEAR, F6).expect("?");

    step("Insert pi value")
        .test(CLEAR, LSHIFT, F1).expect("3.14159 26535 9");
    step("Insert e value")
        .test(CLEAR, LSHIFT, F2).expect("2.71828 18284 6");
    step("Insert i value")
        .test(CLEAR, LSHIFT, F3).expect("0+1ⅈ");
    step("Insert j value")
        .test(CLEAR, LSHIFT, F4).expect("0+1ⅈ");
    step("Insert infinity value")
        .test(CLEAR, LSHIFT, F5).expect("9.99999⁳⁹⁹⁹⁹⁹⁹");
    step("Insert undefined value")
        .test(CLEAR, LSHIFT, F6).expect("Undefined");


    step("Begin program")
        .test(CLEAR, LSHIFT, RUNSTOP).editor("«»");
    step("Insert pi")
        .test(F1).editor("« Ⓒπ »");
    step("Insert e")
        .test(F2).editor("« Ⓒπ  Ⓒe »");
    step("Insert i")
        .test(F3).editor("« Ⓒπ  Ⓒe  Ⓒⅈ »");
    step("Insert j")
        .test(F4).editor("« Ⓒπ  Ⓒe  Ⓒⅈ  Ⓒⅉ »");
    step("Insert infinity")
        .test(F5).editor("« Ⓒπ  Ⓒe  Ⓒⅈ  Ⓒⅉ  Ⓒ∞ »");
    step("Insert undefined")
        .test(F6).editor("« Ⓒπ  Ⓒe  Ⓒⅈ  Ⓒⅉ  Ⓒ∞  Ⓒ? »");

    step("Insert pi value")
        .test(LSHIFT, F1).editor("« Ⓒπ  Ⓒe  Ⓒⅈ  Ⓒⅉ  Ⓒ∞  Ⓒ?  "
                                 "3.14159 26535 89793 23846 264 »");
    step("Insert e value")
        .test(LSHIFT, F2).editor("« Ⓒπ  Ⓒe  Ⓒⅈ  Ⓒⅉ  Ⓒ∞  Ⓒ?  "
                                 "3.14159 26535 89793 23846 264  "
                                 "2.71828 18284 59045 23536 028 »");
    step("Insert i value")
        .test(LSHIFT, F3).editor("« Ⓒπ  Ⓒe  Ⓒⅈ  Ⓒⅉ  Ⓒ∞  Ⓒ?  "
                                 "3.14159 26535 89793 23846 264  "
                                 "2.71828 18284 59045 23536 028  "
                                 "0+ⅈ1 »");
    step("Insert j value")
        .test(LSHIFT, F4).editor("« Ⓒπ  Ⓒe  Ⓒⅈ  Ⓒⅉ  Ⓒ∞  Ⓒ?  "
                                 "3.14159 26535 89793 23846 264  "
                                 "2.71828 18284 59045 23536 028  "
                                 "0+ⅈ1  "
                                 "0+ⅈ1 »");
    step("Insert infinity value")
        .test(LSHIFT, F5).editor("« Ⓒπ  Ⓒe  Ⓒⅈ  Ⓒⅉ  Ⓒ∞  Ⓒ?  "
                                 "3.14159 26535 89793 23846 264  "
                                 "2.71828 18284 59045 23536 028  "
                                 "0+ⅈ1  "
                                 "0+ⅈ1  "
                                 "9.99999⁳999999 »");
    step("Insert undefined value")
        .test(LSHIFT, F6).editor("« Ⓒπ  Ⓒe  Ⓒⅈ  Ⓒⅉ  Ⓒ∞  Ⓒ?  "
                                 "3.14159 26535 89793 23846 264  "
                                 "2.71828 18284 59045 23536 028  "
                                 "0+ⅈ1  "
                                 "0+ⅈ1  "
                                 "9.99999⁳999999  "
                                 "Undefined »");

    step("Test that constants parse")
        .test(ENTER)
        .expect("« π e ⅈ ⅉ ∞ ? "
                "3.14159 26535 9 2.71828 18284 6 0+1ⅈ 0+1ⅈ 9.99999⁳⁹⁹⁹⁹⁹⁹ "
                "Undefined »", 3000);

    step("Select library menu")
        .test(CLEAR, RSHIFT, H).noerror();
    step("Select secrets menu")
        .test(F1).noerror();
    step("Insert Dedicace")
        .test(CLEAR, F1).expect("Dedicace");
    step("Evaluate stack Dedicace")
        .test(RUNSTOP).expect("\"À tous ceux qui se souviennent de "
                              "Maubert électronique\"");
    step("Evaluate Dedicace directly")
        .test(LSHIFT, F1).expect("\"À tous ceux qui se souviennent de "
                                 "Maubert électronique\"");

    step("Begin program")
        .test(CLEAR, LSHIFT, RUNSTOP).editor("«»");
    step("Insert Dedicace")
        .test(F1).editor("« ⓁDedicace »");
    step("Insert LibraryHelp")
        .test(F2).editor("« ⓁDedicace  ⓁLibraryHelp »");

    step("Test that xlibs parse")
        .test(ENTER)
        .expect("« Dedicace LibraryHelp »");
    step("Test that xlib can be edited")
        .test(DOWN)
        .editor("« ⓁDedicace ⓁLibraryHelp »");
    step("Test that xlib edit can be entered")
        .test(ENTER)
        .expect("« Dedicace LibraryHelp »");
    step("Test that xlib name can be edited")
        .test(DOWN)
        .editor("« ⓁDedicace ⓁLibraryHelp »")
        .test(DOWN, DOWN, DOWN, DOWN, "ee")
        .editor("« ⓁDeeedicace ⓁLibraryHelp »");
    step("Test that bad xlib name is detected")
        .test(ENTER)
        .error("Invalid or unknown library entry")
        .test(EXIT);

    step("Select units menu")
        .test(CLEAR, LSHIFT, KEY5, F4).image("units-menu",
                                             0, 240-3*22, 400, 3*22);
    step("Select meter")
        .test(CLEAR, KEY1, F1).expect("1 m");
    step("Convert to yards")
        .test(LSHIFT, F2).expect("1 ¹⁰⁷/₁ ₁₄₃ yd");
    step("Select yards")
        .test(CLEAR, KEY1, F2).expect("1 yd");
    step("Convert to feet")
        .test(LSHIFT, F3).expect("3 ft");
    step("Select feet")
        .test(CLEAR, KEY1, F3).expect("1 ft");
    step("Convert to meters")
        .test(LSHIFT, F1).expect("³⁸¹/₁ ₂₅₀ m");

    step("Enter 27_m in program and evaluate it")
        .test(CLEAR, LSHIFT, RUNSTOP).editor("«»")
        .test("27", NOSHIFT, F1).editor("«27_m »")
        .test(ENTER).expect("« 27 m »")
        .test(RUNSTOP).expect("27 m");
    step("Enter 27_yd in program and evaluate it")
        .test(CLEAR, LSHIFT, RUNSTOP).editor("«»")
        .test("27", NOSHIFT, F2).editor("«27_yd »")
        .test(ENTER).expect("« 27 yd »")
        .test(RUNSTOP).expect("27 yd");
    step("Enter 27_ft in program and evaluate it")
        .test(CLEAR, LSHIFT, RUNSTOP).editor("«»")
        .test("27", NOSHIFT, F3).editor("«27_ft »")
        .test(ENTER).expect("« 27 ft »")
        .test(RUNSTOP).expect("27 ft");

    step("Enter A with unit _m in program and evaluate it")
        .test(CLEAR, LSHIFT, RUNSTOP).editor("«»")
        .test("A", NOSHIFT, F1).editor("«A 1_m * »")
        .test(ENTER).expect("« A 1 m × »")
        .test(RUNSTOP).expect("'A' m");

    step("Enter 27_m⁻¹ in program and evaluate it")
        .test(CLEAR, LSHIFT, RUNSTOP).editor("«»")
        .test("27", RSHIFT, F1).editor("«27_(m)⁻¹ »")
        .test(ENTER).expect("« 27 m⁻¹ »")
        .test(RUNSTOP).expect("27 m⁻¹");
    step("Enter 27_yd⁻¹ in program and evaluate it")
        .test(CLEAR, LSHIFT, RUNSTOP).editor("«»")
        .test("27", RSHIFT, F2).editor("«27_(yd)⁻¹ »")
        .test(ENTER).expect("« 27 yd⁻¹ »")
        .test(RUNSTOP).expect("27 yd⁻¹");
    step("Enter 27_ft⁻¹ in program and evaluate it")
        .test(CLEAR, LSHIFT, RUNSTOP).editor("«»")
        .test("27", RSHIFT, F3).editor("«27_(ft)⁻¹ »")
        .test(ENTER).expect("« 27 ft⁻¹ »")
        .test(RUNSTOP).expect("27 ft⁻¹");

    step("Select variables menu")
        .test(CLEAR, NOSHIFT, H).noerror();
    step("Create variables named Foo and Baz")
        .test(CLEAR, "1968 'Foo'", NOSHIFT, G).noerror()
        .test("42", NOSHIFT, F, "Baz", NOSHIFT, G).noerror();

    step("Check we can read the variables back")
        .test(CLEAR, F1).expect("42")
        .test(CLEAR, F2).expect("1 968");

    step("Insert evaluation code")
        .test(CLEAR, LSHIFT, RUNSTOP).editor("«»")
        .test(F1).editor("« Baz »")
        .test(F2).editor("« Baz  Foo »");
    step("Check position of insertion point")
        .test("ABC").editor("« Baz  Foo ABC»");
    step("Check we can parse resulting program")
        .test(ENTER).expect("« Baz Foo ABC »");
    step("Check evaluation of program")
        .test(RUNSTOP).expect("'ABC'")
        .test(BSP).expect("1 968")
        .test(BSP).expect("42")
        .test(BSP, BSP).error("Too few arguments");

    step("Insert recall code")
        .test(CLEAR, LSHIFT, RUNSTOP).editor("«»")
        .test(LSHIFT, F1).editor("« 'Baz' Recall »")
        .test(LSHIFT, F2).editor("« 'Baz' Recall  'Foo' Recall »");
    step("Insert store code")
        .test(RSHIFT, F1).editor("« 'Baz' Recall  'Foo' Recall  "
                                 "'Baz' Store »")
        .test(RSHIFT, F2).editor("« 'Baz' Recall  'Foo' Recall  "
                                 "'Baz' Store  'Foo' Store »");
    step("Check that it parses")
        .test(ENTER).expect("« 'Baz' Recall 'Foo' Recall "
                            "'Baz' Store 'Foo' Store »");
    step("Check evaluation")
        .test(RUNSTOP)
        .test(CLEAR, F1).expect("1 968")
        .test(CLEAR, F2).expect("42");
    step("Cleanup")
        .test(CLEAR, "'Foo' Purge 'Baz' Purge", ENTER);
}


void tests::character_menu()
// ----------------------------------------------------------------------------
//   Character menu and character catalog
// ----------------------------------------------------------------------------
{
    BEGIN(characters);

    step("Character menu")
        .test(CLEAR, RSHIFT, KEY2).noerror()
        .image_menus("char-menu", 3);

    step("RPL menu")
        .test(CLEAR, RSHIFT, KEY2).noerror()
        .test(F1, RSHIFT, ENTER)
        .test(NOSHIFT, F1, F2, F3, F4, F5, F6,
              LSHIFT, F1, LSHIFT, F2, LSHIFT, F3,
              LSHIFT, F4, LSHIFT, F5, LSHIFT, F6,
              RSHIFT, F1, RSHIFT, F2, RSHIFT, F3,
              RSHIFT, F4, RSHIFT, F5, RSHIFT, F6,
              ENTER)
        .expect("\"→«»Σ∏∆⇄{}≤≠≥ⅈ∡_∂∫|\"");
    step("Arith menu")
        .test(CLEAR, RSHIFT, KEY2).noerror()
        .test(F2, RSHIFT, ENTER)
        .test(NOSHIFT, F1, F2, F3, F4, F5, F6,
              LSHIFT, F1, LSHIFT, F2, LSHIFT, F3,
              LSHIFT, F4, LSHIFT, F5, LSHIFT, F6,
              RSHIFT, F1, RSHIFT, F2, RSHIFT, F3,
              RSHIFT, F4, RSHIFT, F5, RSHIFT, F6,
              ENTER)
        .expect("\"+-*/×÷<=>≤≠≥·%^↑\\±\"");
    step("Math menu")
        .test(CLEAR, RSHIFT, KEY2).noerror()
        .test(F3, RSHIFT, ENTER)
        .test(NOSHIFT, F1, F2, F3, F4, F5,
              LSHIFT, F1, LSHIFT, F2, LSHIFT, F3,
              LSHIFT, F4, LSHIFT, F5,
              RSHIFT, F1, RSHIFT, F2, RSHIFT, F3,
              RSHIFT, F4, RSHIFT, F5,
              ENTER)
        .expect("\"Σ∏∆∂∫πℼ′″°ⅈⅉℂℚℝ\"");
    step("French menu")
        .test(CLEAR, RSHIFT, KEY2).noerror()
        .test(F4, RSHIFT, ENTER)
        .test(NOSHIFT, F1, F2, F3, F4, F5,
              LSHIFT, F1, LSHIFT, F2, LSHIFT, F3,
              LSHIFT, F4, LSHIFT, F5,
              RSHIFT, F1, RSHIFT, F2, RSHIFT, F3,
              RSHIFT, F4, RSHIFT, F5,
              ENTER)
        .expect("\"àéèêôÀÉÈÊÔëîïûü\"");
    step("Punct menu")
        .test(CLEAR, RSHIFT, KEY2).noerror()
        .test(F5, RSHIFT, ENTER)
        .test(NOSHIFT, F1, F2, F3, F4, F5, F6,
              LSHIFT, F1, LSHIFT, F2, LSHIFT, F3,
              LSHIFT, F4, LSHIFT, F5, LSHIFT, F6, LSHIFT, F6,
              RSHIFT, F1, RSHIFT, F2, RSHIFT, F3,
              RSHIFT, F4, RSHIFT, F5, RSHIFT, F6,
              ENTER)
        .expect("\".,;:!?#$%&'\"\"¡¿`´~\\\"");
    step("Delim menu")
        .test(CLEAR, RSHIFT, KEY2).noerror()
        .test(F6, RSHIFT, ENTER)
        .test(NOSHIFT, F1, F2, F3, F4, F5, F6,
              LSHIFT, F1, LSHIFT, F2, LSHIFT, F3,
              LSHIFT, F4, LSHIFT, F4, LSHIFT, F5, LSHIFT, F6,
              RSHIFT, F1, RSHIFT, F2, RSHIFT, F3,
              ENTER)
        .expect("\"()[]{}«»'\"\"¦§¨­¯\"");

    step("Arrows menu")
        .test(CLEAR, RSHIFT, KEY2).noerror()
        .test(LSHIFT, F1, RSHIFT, ENTER)
        .test(NOSHIFT, F1, F2, F3, F4, F5,
              LSHIFT, F1, LSHIFT, F2, LSHIFT, F3,
              LSHIFT, F4, LSHIFT, F5,
              RSHIFT, F1, RSHIFT, F2, RSHIFT, F3,
              RSHIFT, F4, RSHIFT, F5,
              ENTER)
        .expect("\"←↑→↓↔↕⇄⇆↨⌂▲▼◀▬▶\"");
    step("Blocks menu")
        .test(CLEAR, RSHIFT, KEY2).noerror()
        .test(LSHIFT, F2, RSHIFT, ENTER)
        .test(NOSHIFT, F1, F2, F3, F4, F5,
              LSHIFT, F1, LSHIFT, F2, LSHIFT, F3,
              LSHIFT, F4, LSHIFT, F5,
              RSHIFT, F1, RSHIFT, F2, RSHIFT, F3,
              RSHIFT, F4, RSHIFT, F5,
              ENTER)
        .expect("\"┌┬┐─├┼┤│└┴┘▬╒╤╕\"");
    step("Bullets menu")
        .test(CLEAR, RSHIFT, KEY2).noerror()
        .test(LSHIFT, F3, RSHIFT, ENTER)
        .test(NOSHIFT, F1, F2, F3, F4, F5,
              LSHIFT, F1, LSHIFT, F2, LSHIFT, F3,
              LSHIFT, F4, LSHIFT, F5, LSHIFT, F6,
              RSHIFT, F1, RSHIFT, F2, RSHIFT, F3,
              ENTER)
        .expect("\"·∙►▶→□▪▫▬○●◊◘◙\"");
    step("Currency menu")
        .test(CLEAR, RSHIFT, KEY2).noerror()
        .test(LSHIFT, F4, RSHIFT, ENTER)
        .test(NOSHIFT, F1, F2, F3, F4, F5,
              LSHIFT, F1, LSHIFT, F2, LSHIFT, F3,
              LSHIFT, F4, LSHIFT, F5, LSHIFT, F6,
              RSHIFT, F1, RSHIFT, F2, RSHIFT, F3,
              ENTER)
        .expect("\"$€¢£¤₣₤₧₫₭₹₺₽ƒ\"");
    step("Greek menu")
        .test(CLEAR, RSHIFT, KEY2).noerror()
        .test(LSHIFT, F5, RSHIFT, ENTER)
        .test(NOSHIFT, F1, F2, F3, F4, F5,
              LSHIFT, F1, LSHIFT, F2, LSHIFT, F3,
              LSHIFT, F4, LSHIFT, F5,
              RSHIFT, F1, RSHIFT, F2, RSHIFT, F3,
              RSHIFT, F4, RSHIFT, F5,
              ENTER)
        .expect("\"αβγδεΑΒΓΔΕάΆ·Έέ\"");
    step("Europe menu")
        .test(CLEAR, RSHIFT, KEY2).noerror()
        .test(LSHIFT, F6, RSHIFT, ENTER)
        .test(NOSHIFT, F1, F2, F3, F4, F5,
              LSHIFT, F1, LSHIFT, F2, LSHIFT, F3,
              LSHIFT, F4, LSHIFT, F5,
              RSHIFT, F1, RSHIFT, F2, RSHIFT, F3,
              RSHIFT, F4, RSHIFT, F5,
              ENTER)
        .expect("\"ÀÁÂÃÄàáâãäÅÆÇåæ\"");

    step("Cyrillic menu")
        .test(CLEAR, RSHIFT, KEY2).noerror()
        .test(RSHIFT, F1, RSHIFT, ENTER)
        .test(NOSHIFT, F1, F2, F3, F4, F5,
              LSHIFT, F1, LSHIFT, F2, LSHIFT, F3,
              LSHIFT, F4, LSHIFT, F5,
              ENTER)
        .expect("\"АБВГДабвгд\"");
    step("Picto menu")
        .test(CLEAR, RSHIFT, KEY2).noerror()
        .test(RSHIFT, F2, RSHIFT, ENTER)
        .test(NOSHIFT, F1, F2, F3, F4, F5,
              LSHIFT, F1, LSHIFT, F2, LSHIFT, F3,
              LSHIFT, F4, LSHIFT, F5,
              RSHIFT, F1, RSHIFT, F2, RSHIFT, F3,
              RSHIFT, F4, RSHIFT, F5,
              ENTER)
        .expect("\"⌂№℡™⚙☺☻☼♀♂♠♣♥♦◊\"");
    step("Boxes menu")
        .test(CLEAR, RSHIFT, KEY2).noerror()
        .test(RSHIFT, F3, RSHIFT, ENTER)
        .test(NOSHIFT, F1, F2, F3, F4, F5, F6,
              LSHIFT, F1, LSHIFT, F2, LSHIFT, F3,
              LSHIFT, F4, LSHIFT, F5, LSHIFT, F6,
              RSHIFT, F1, RSHIFT, F2, RSHIFT, F3,
              RSHIFT, F4, RSHIFT, F5, RSHIFT, F6,
              ENTER)
        .expect("\"→«»Σ∏∆⇄{}≤≠≥ⅈ∡_∂∫|\"");
    step("Num-like menu")
        .test(CLEAR, RSHIFT, KEY2).noerror()
        .test(RSHIFT, F4, RSHIFT, ENTER)
        .test(NOSHIFT, F1, F2, F3, F4, F5,
              LSHIFT, F1, LSHIFT, F2, LSHIFT, F3,
              LSHIFT, F4, LSHIFT, F5,
              RSHIFT, F1, RSHIFT, F2, RSHIFT, F3,
              RSHIFT, F4, RSHIFT, F5,
              ENTER)
        .expect("\"₀₁₂₃₄⁰¹²³⁴ⅠⅡⅢⅣⅤ\"");
    step("Ltr-like menu")
        .test(CLEAR, RSHIFT, KEY2).noerror()
        .test(RSHIFT, F5, RSHIFT, ENTER)
        .test(NOSHIFT, F1, F2, F3, F4, F5,
              LSHIFT, F1, LSHIFT, F2, LSHIFT, F3,
              LSHIFT, F4, LSHIFT, F5,
              RSHIFT, F1, RSHIFT, F2, RSHIFT, F3,
              RSHIFT, F4, RSHIFT, F5,
              ENTER)
        .expect("\"$&@¢©¥ℂ℅ℊℎℏℓ№ℚℝ\"");
    step("All menu")
        .test(CLEAR, RSHIFT, KEY2).noerror()
        .test(RSHIFT, F6, RSHIFT, ENTER)
        .test(NOSHIFT, F1, F2, F3, F3, F4, F5,
              LSHIFT, F1, LSHIFT, F2, LSHIFT, F3,
              LSHIFT, F4, LSHIFT, F5,
              RSHIFT, F1, RSHIFT, F2, RSHIFT, F3,
              RSHIFT, F4, RSHIFT, F5,
              ENTER)
        .expect("\" !\"\"#$%&'()*+,-.\"");

    step("Catalog")
        .test(CLEAR, RSHIFT, ENTER, ADD, A, F4).editor("\"À\"")
        .test(F2).editor("\"A\"")
        .test(LSHIFT, F3).editor("\"a\"");
    step("Générons un peu de français")
        .test(CLEAR, RSHIFT, ENTER, ADD,
              "Ge", F5, "ne", F5, "rons un peu de franc", F4, "ais")
        .editor("\"Générons un peu de français\"")
        .test(ENTER).expect("\"Générons un peu de français\"");
}


void tests::regression_checks()
// ----------------------------------------------------------------------------
//   Checks for specific regressions
// ----------------------------------------------------------------------------
{
    BEGIN(regressions);

    Settings = settings();

    step("Bug 116: Rounding of gamma(7) and gamma(8)");
    test(CLEAR, "7 gamma", ENTER).expect("720.");
    test(CLEAR, "8 gamma", ENTER).expect("5 040.");

    step("Bug 168: pi no longer parses correctly");
    test(CLEAR, SHIFT, I, F1, F1).expect("π");
    test(DOWN).editor("Ⓒπ");
    test(ENTER).expect("π");

    step("Bug 207: parsing of cos(X+pi)");
    test(CLEAR, "'COS(X+π)'", ENTER).expect("'cos(X+π)'");

    step("Bug 238: Parsing of power");
    test(CLEAR, "'X↑3'", ENTER).expect("'X↑3'");
    test(CLEAR, "'X·X↑(N-1)'", ENTER).expect("'X·X↑(N-1)'");

    step("Bug 253: Complex cos outside domain");
    test(CLEAR, "0+30000.ⅈ sin", ENTER).expect("3.41528 61889 6⁳¹³⁰²⁸∡90°");
    test(CLEAR, "0+30000.ⅈ cos", ENTER).expect("3.41528 61889 6⁳¹³⁰²⁸∡0°");
    test(CLEAR, "0+30000.ⅈ tan", ENTER).expect("1∡90°");

    step("Bug 272: Type error on logical operations");
    test(CLEAR, "'x' #2134AF AND", ENTER).error("Bad argument type");

    step("Bug 277: 1+i should have positive arg");
    test(CLEAR, "1+1ⅈ arg", ENTER).expect("45");
    test(CLEAR, "1-1ⅈ arg", ENTER).expect("-45");
    test(CLEAR, "1 1 atan2", ENTER).expect("45");
    test(CLEAR, "1+1ⅈ ToPolar", ENTER).match("1.414.*∡45°");

    step("Bug 287: arg of negative number");
    test(CLEAR, "-35 arg", ENTER).expect("180");

    step("Bug 288: Abusive simplification of multiplication by -1");
    test(CLEAR, "-1 3 *", ENTER).expect("-3");

    step("Bug 279: 0/0 should error out");
    test(CLEAR, "0 0 /", ENTER).error("Divide by zero");

    step("Bug 695: Putting program separators in names");
    test(CLEAR,
         LSHIFT, RUNSTOP,               // «»
         ALPHA_RS, G,                   // «→»
         N,                             // «→N»
         SHIFT, RUNSTOP,                // «→N «»»
         UP, BSP, DOWN, DOWN, UP,       // «→N«»»
         N,                             // «→N«N»»
         ENTER)
        .noerror().type(object::ID_program)
        .test(RUNSTOP)
        .noerror().type(object::ID_program).expect("« N »")
        .test(BSP)
        .noerror().type(object::ID_expression).expect("'→N'");

    step("Bug 822: Fraction iteration")
        .test(CLEAR,
              LSHIFT, H,
              100, RSHIFT, F3,
              20,  RSHIFT, F4)
        .test("1968.1205", F4).expect("1 968 ²⁴¹/₂ ₀₀₀")
        .test("1968.0512", F4).expect("1 968 ³²/₆₂₅")
        .test(LSHIFT, N, RSHIFT, F4);
}


void tests::plotting()
// ----------------------------------------------------------------------------
//   Test the plotting functions
// ----------------------------------------------------------------------------
{
    BEGIN(plotting);

    step("Select radians");
    test(CLEAR, "RAD", ENTER).noerror();

    step("Function plot: Sine wave");
    test(CLEAR, "'3*sin(x)' FunctionPlot", ENTER).noerror()
        .wait(200).image("plot-sine");
    step("Function plot: Sine wave without axes");
    test(CLEAR, "NoPlotAxes '3*sin(x)' FunctionPlot", ENTER).noerror()
        .wait(200).image("plot-sine-noaxes");
    step("Function plot: Sine wave not connected no axes");
    test(CLEAR, "NoCurveFilling '3*sin(x)' FunctionPlot", ENTER).noerror()
        .wait(200).image("plot-sine-noaxes-nofill");
    step("Function plot: Sine wave with axes no fill");
    test(CLEAR, "-29 CF '3*sin(x)' FunctionPlot", ENTER).noerror()
        .wait(200).image("plot-sine-nofill");
    step("Function plot: Sine wave defaults");
    test(CLEAR, "-31 CF '3*sin(x)' FunctionPlot", ENTER).noerror()
        .wait(200).image("plot-sine");

    step("Function plot: Equation");
    test(CLEAR,
         ALPHA, X, ENTER, ENTER, J, 3, MUL, M, 21, MUL, COS, 2, MUL, ADD,
         RSHIFT, O, F1).noerror()
        .wait(200).image("plot-eq");
    step("Function plot: Program");
    test(CLEAR, SHIFT, RUNSTOP,
         I, SHIFT, F1, L, M, 41, MUL, J, MUL, ENTER, ENTER,
         RSHIFT, O, F1).wait(200).image("plot-pgm").noerror();
    step("Function plot: Disable curve filling");
    test(CLEAR, RSHIFT, UP, ENTER, "NoCurveFilling", ENTER,
         RSHIFT, O, F1).wait(200).image("plot-nofill").noerror();
    step("Function plot: Disable curve filling with flag -31");
    test(CLEAR, RSHIFT, UP, ENTER, "-31 CF", ENTER,
         RSHIFT, O, F1).wait(200).image("plot-pgm").noerror();

    step("Polar plot: Program");
    test(CLEAR, SHIFT, RUNSTOP,
         61, MUL, L, SHIFT, C, 2, ADD, ENTER,
         RSHIFT, O, F2).noerror().wait(200).image("polar-pgm");
    step("Polar plot: Program, no fill");
    test(CLEAR, "NoCurveFilling", ENTER,
         SHIFT, RUNSTOP,
         61, MUL, L, SHIFT, C, 2, ADD, ENTER,
         RSHIFT, O, F2).noerror().wait(200).image("polar-pgm-nofill");
    step("Polar plot: Program, curve filling");
    test(CLEAR, "CurveFilling", ENTER,
         SHIFT, RUNSTOP,
         61, MUL, L, SHIFT, C, 2, ADD, ENTER,
         RSHIFT, O, F2).noerror().wait(200).image("polar-pgm");
    step("Polar plot: Equation");
    test(CLEAR, F, J, 611, MUL, ALPHA, X,
         NOSHIFT, DOWN, MUL, K, 271, MUL,
         ALPHA, X, NOSHIFT, DOWN,
         ADD, KEY2, DOT, KEY5, ENTER,
         RSHIFT, O,
         ENTER, F2).noerror().wait(200).image("polar-eq");
    step("Polar plot: Zoom in X and Y");
    test(EXIT, "0.5 XSCALE 0.5 YSCALE", ENTER).noerror()
        .test(ENTER, F2).noerror().wait(200).image("polar-zoomxy");
    step("Polar plot: Zoom out Y");
    test(EXIT, "2 YSCALE", ENTER).noerror()
        .test(ENTER, F2).noerror().wait(200).image("polar-zoomy");
    step("Polar plot: Zoom out X");
    test(EXIT, "2 XSCALE", ENTER).noerror()
        .test(ENTER, F2).noerror().wait(200).image("polar-zoomx");
    step("Saving plot parameters")
        .test("PPAR", ENTER, NOSHIFT, M);
    step("Polar plot: Select min point with PMIN");
    test(EXIT, "-3-4ⅈ PMIN", ENTER).noerror()
        .test(ENTER, RSHIFT, O, F2).noerror().wait(200).image("polar-pmin");

    step("Polar plot: Select max point with PMAX");
    test(EXIT, "5+6ⅈ pmax", ENTER).noerror()
        .test(ENTER, RSHIFT, O, F2).noerror().wait(200).image("polar-pmax");
    step("Polar plot: Select X range with XRNG");
    test(EXIT, "-6 7 xrng", ENTER).noerror()
        .test(ENTER, F2).noerror().wait(200).image("polar-xrng");
    step("Polar plot: Select Y range with YRNG");
    test(EXIT, "-3 2.5 yrng", ENTER).noerror()
        .test(ENTER, F2).noerror().wait(200).image("polar-yrng");
    step("Restoring plot parameters")
        .test(NOSHIFT, M, "'PPAR'", NOSHIFT, G);

    step("Parametric plot: Program");
    test(CLEAR, SHIFT, RUNSTOP,
         "'9.5*sin(31.27*X)' eval '5.5*cos(42.42*X)' eval RealToComplex",
         ENTER, ENTER, F3)
        .noerror().wait(200).image("pplot-pgm");
    step("Parametric plot: Degrees");
    test("DEG 2 LINEWIDTH", ENTER, F3).noerror().wait(200).image("pplot-deg");
    step("Parametric plot: Equation");
    test(CLEAR,
         "3 LINEWIDTH 0.25 GRAY FOREGROUND "
         "'exp((0.17ⅈ5.27)*x+(1.5ⅈ8))' ParametricPlot", ENTER)
        .noerror().wait(200).image("pplot-eq");

    step("Bar plot");
    test(CLEAR,
         "[[ 1 -1 ][2 -2][3 -3][4 -4][5 -6][7 -8][9 -10]]", ENTER,
         33, MUL, K, 2, MUL,
         RSHIFT, O, F5).noerror().wait(200).image("barplot");

    step("Scatter plot");
    test(CLEAR,
         "[[ -5 -5][ -3 0][ -5 5][ 0 3][ 5 5][ 3 0][ 5 -5][ 0 -3][-5 -5]]",
         ENTER,
         "4 LineWidth ScatterPlot", ENTER)
        .noerror().wait(200).image("scatterplot");

     step("Reset drawing parameters");
     test(CLEAR, "1 LineWidth 0 GRAY Foreground", ENTER).noerror();
}


void tests::plotting_all_functions()
// ----------------------------------------------------------------------------
//   Plot all real functions
// ----------------------------------------------------------------------------
{
    BEGIN(plotfns);

    step("Select radians")
        .test(CLEAR, SHIFT, N, F2).noerror();

    step("Select 24-digit precision")
        .test(CLEAR, SHIFT, O, 24, F6).noerror();

    step("Purge the `PlotParameters` variable")
        .test(CLEAR, "'PPAR' purge", ENTER).noerror();

    step("Select plotting menu")
        .test(CLEAR, RSHIFT, O).noerror();

    uint dur = 300;

#define FUNCTION(name)                          \
    step("Plotting " #name);                    \
    test(CLEAR, "'" #name "(x)'", F1)           \
        .wait(dur).noerror()                      \
        .image("fnplot-" #name)

    FUNCTION(sqrt);
    FUNCTION(cbrt);

    FUNCTION(sin);
    FUNCTION(cos);
    FUNCTION(tan);
    FUNCTION(asin);
    FUNCTION(acos);
    FUNCTION(atan);

    step("Select degrees");
    test(CLEAR, SHIFT, N, F1).noerror();

    step("Reselect plotting menu");
    test(CLEAR, RSHIFT, O).noerror();

    FUNCTION(sinh);
    FUNCTION(cosh);
    FUNCTION(tanh);
    FUNCTION(asinh);
    FUNCTION(acosh);
    FUNCTION(atanh);

    FUNCTION(log1p);
    FUNCTION(expm1);
    FUNCTION(log);
    FUNCTION(log10);
    FUNCTION(log2);
    FUNCTION(exp);
    FUNCTION(exp10);
    FUNCTION(exp2);
    FUNCTION(erf);
    FUNCTION(erfc);
    FUNCTION(tgamma);
    FUNCTION(lgamma);


    FUNCTION(abs);
    FUNCTION(sign);
    FUNCTION(IntPart);
    FUNCTION(FracPart);
    FUNCTION(ceil);
    FUNCTION(floor);
    FUNCTION(inv);
    FUNCTION(neg);
    FUNCTION(sq);
    FUNCTION(cubed);
    FUNCTION(fact);

    FUNCTION(re);
    FUNCTION(im);
    FUNCTION(arg);
    FUNCTION(conj);

    FUNCTION(ToDecimal);
    FUNCTION(ToFraction);
}


void tests::graphic_commands()
// ----------------------------------------------------------------------------
//   Graphic commands
// ----------------------------------------------------------------------------
{
    BEGIN(graphics);

    step("Clear LCD");
    test(CLEAR, "ClearLCD", ENTER)
        .noerror().wait(200).image("cllcd").test(ENTER);

    step("Draw graphic objects")
        .test(CLEAR,
              "13 LineWidth { 0 0 } 5 Circle 1 LineWidth "
              "GROB 9 15 "
              "E300140015001C001400E3008000C110AA00940090004100220014102800 "
              "2 25 for i "
              "PICT OVER "
              "2.321 ⅈ * i * exp 4.44 0.08 i * + * Swap "
              "GXor "
              "PICT OVER "
              "1.123 ⅈ * i * exp 4.33 0.08 i * + * Swap "
              "GAnd "
              "PICT OVER "
              "4.12 ⅈ * i * exp 4.22 0.08 i * + * Swap "
              "GOr "
              "next", ENTER)
        .wait(200).noerror().image("walkman").test(EXIT);

    step("Displaying text, compatibility mode");
    test(CLEAR,
         "\"Hello World\" 1 DISP "
         "\"Compatibility mode\" 2 DISP", ENTER)
        .noerror().wait(200).image("text-compat").test(ENTER);

    step("Displaying text, fractional row");
    test(CLEAR,
         "\"Gutentag\" 1.5 DrawText "
         "\"Fractional row\" 3.8 DrawText", ENTER)
        .noerror().wait(200).image("text-frac").test(ENTER);

    step("Displaying text, pixel row");
    test(CLEAR,
         "\"Bonjour tout le monde\" #5d DISP "
         "\"Pixel row mode\" #125d DISP", ENTER)
        .noerror().wait(200).image("text-pixrow").test(ENTER);

    step("Displaying text, x-y coordinates");
    test(CLEAR, "\"Hello\" { 0 0 } DISP ", ENTER)
        .noerror().wait(200).image("text-xy").test(ENTER);

    step("Displaying text, x-y pixel coordinates");
    test(CLEAR, "\"Hello\" { #20d #20d } DISP ", ENTER)
        .noerror().wait(200).image("text-pixxy").test(ENTER);

    step("Displaying text, font ID");
    test(CLEAR, "\"Hello\" { 0 0 0 } DISP \"World\" { 0 1 2 } DISP ", ENTER)
        .noerror().wait(200).image("text-font").test(ENTER);

    step("Displaying text, erase and invert");
    test(CLEAR, "\"Inverted\" { 0 0 0 true true } DISP", ENTER)
        .noerror().wait(200).image("text-invert").test(ENTER);

    step("Displaying text, background and foreground");
    test(CLEAR,
         "0.25 Gray Foreground 0.75 Gray Background "
         "\"Grayed\" { 0 0 } Disp", ENTER)
        .noerror().wait(200).image("text-gray").test(ENTER);

    step("Displaying text, restore background and foreground");
    test(CLEAR,
         "0 Gray Foreground 1 Gray Background "
         "\"Grayed\" { 0 0 } Disp", ENTER)
        .noerror().wait(200).image("text-normal").test(ENTER);

    step("Displaying text, type check");
    test(CLEAR, "\"Bad\" \"Hello\" DISP", ENTER)
        .error("Bad argument type");

    step("Lines");
    test(CLEAR, "3 50 for i ⅈ i * exp i 2 + ⅈ * exp 5 * Line next", ENTER)
        .noerror().wait(200).image("lines").test(ENTER);

    step("Line width");
    test(CLEAR,
         "1 11 for i "
         "{ #000 } #0 i 20 * + + "
         "{ #400 } #0 i 20 * + + "
         "i LineWidth Line "
         "next "
         "1 LineWidth", ENTER)
        .noerror().wait(200).image("line-width").test(ENTER);

    step("Line width, grayed");
    test(CLEAR,
         "1 11 for i "
         "{ #000 } #0 i 20 * + + "
         "{ #400 } #0 i 20 * + + "
         "i 12 / gray foreground "
         "i LineWidth Line "
         "next "
         "1 LineWidth 0 Gray Foreground", ENTER)
        .noerror().wait(200).image("line-width-gray").test(ENTER);

    step("Circles");
    test(CLEAR,
         "1 11 for i "
         "{ 0 0 } i Circle "
         "{ 0 1 } i 0.25 * Circle "
         "next ", ENTER)
        .noerror().wait(200).image("circles").test(ENTER);

    step("Circles, complex coordinates");
    test(CLEAR,
         "2 150 for i "
         "ⅈ i 0.12 * * exp 0.75 0.05 i * + * 0.4 0.003 i * +  Circle "
         "next ", ENTER)
        .noerror().wait(200).image("circles-complex").test(ENTER);

    step("Circles, fill and patterns");
    test(CLEAR,
         "0 LineWidth "
         "2 150 for i "
         "i 0.0053 * gray Foreground "
         "ⅈ i 0.12 * * exp 0.75 0.05 i * + * 0.1 0.008 i * +  Circle "
         "next ", ENTER)
        .noerror().wait(200).image("circles-fill").test(ENTER);

    step("Ellipses");
    test(CLEAR,
         "0 gray foreground 1 LineWidth "
         "2 150 for i "
         "i 0.12 * ⅈ * exp 0.05 i * 0.75 + * "
         "i 0.17 * ⅈ * exp 0.05 i * 0.75 + * "
         " Ellipse "
         "next ", ENTER)
        .noerror().wait(200).image("ellipses").test(ENTER);

    step("Ellipses, fill and patterns");
    test(CLEAR,
         "0 LineWidth "
         "2 150 for i "
         "i 0.0047 * gray Foreground "
         "0.23 ⅈ * exp 5.75 0.01 i * - * "
         "1.27 ⅈ * exp 5.45 0.01 i * - * neg "
         " Ellipse "
         "next ", ENTER)
        .noerror().wait(200).image("ellipses-fill").test(ENTER);

    step("Rectangles");
    test(CLEAR,
         "0 gray foreground 1 LineWidth "
         "2 150 for i "
         "i 0.12 * ⅈ * exp 0.05 i * 0.75 + * "
         "i 0.17 * ⅈ * exp 0.05 i * 0.75 + * "
         " Rect "
         "next ", ENTER)
        .noerror().wait(200).image("rectangles").test(ENTER);

    step("Rectangles, fill and patterns");
    test(CLEAR,
         "0 LineWidth "
         "2 150 for i "
         "i 0.0047 * gray Foreground "
         "0.23 ⅈ * exp 5.75 0.01 i * - * "
         "1.27 ⅈ * exp 5.45 0.01 i * - * neg "
         " Rect "
         "next ", ENTER)
        .noerror().wait(200).image("rectangle-fill").test(ENTER);

    step("Rounded rectangles");
    test(CLEAR,
         "0 gray foreground 1 LineWidth "
         "2 150 for i "
         "i 0.12 * ⅈ * exp 0.05 i * 0.75 + * "
         "i 0.17 * ⅈ * exp 0.05 i * 0.75 + * "
         "0.8 RRect "
         "next ", ENTER)
        .noerror().wait(200).image("rounded-rectangle").test(ENTER);

    step("Rounded rectangles, fill and patterns");
    test(CLEAR,
         "0 LineWidth "
         "2 150 for i "
         "i 0.0047 * gray Foreground "
         "0.23 ⅈ * exp 5.75 0.01 i * - * "
         "1.27 ⅈ * exp 5.45 0.01 i * - * neg "
         "0.8 RRect "
         "next ", ENTER)
        .noerror().wait(200).image("rounded-rectangle-fill").test(ENTER);

    step("Clipping");
    test(CLEAR,
         "0 LineWidth CLLCD { 120 135 353 175 } Clip "
         "2 150 for i "
         "i 0.0053 * gray Foreground "
         "ⅈ i 0.12 * * exp 0.75 0.05 i * + * 0.1 0.008 i * +  Circle "
         "next "
         "{} Clip", ENTER)
        .wait(200).noerror().image("clip-circles").test(ENTER);

    step("Cleanup");
    test(CLEAR,
         "1 LineWidth 0 Gray Foreground 1 Gray Background "
         "{ -1 -1 } { 3 2 } rect",
         ENTER).noerror().wait(200).image("cleanup");
}



// ============================================================================
//
//   Sequencing tests
//
// ============================================================================

static void passfail(bool ok)
// ----------------------------------------------------------------------------
//   Print a pass/fail message
// ----------------------------------------------------------------------------
{
#define GREEN "\033[32m"
#define RED   "\033[41;97m"
#define RESET "\033[39;49;99;27m"
    fprintf(stderr, "%s\n", ok ? GREEN "[PASS]" RESET : RED "[FAIL]" RESET);
#undef GREEN
#undef RED
#undef RESET
}

tests &tests::begin(cstring name, bool disabled)
// ----------------------------------------------------------------------------
//   Beginning of a test
// ----------------------------------------------------------------------------
{
    if (sindex)
    {
        passfail(ok);
        if (!ok)
            show(failures.back());
    }

    tname = name;
    tindex++;
#define BLACK "\033[40;97m"
#define GREY  "\033[100;37m"
#define CLREOL "\033[K"
#define RESET "\033[39;49;27m"
    if (disabled)
        fprintf(stderr,  GREY "%3u: %-75s" CLREOL RESET "\n", tindex, tname);
    else
        fprintf(stderr, BLACK "%3u: %-75s" CLREOL RESET "\n", tindex, tname);
#undef BLACK
#undef CLREOL
#undef RESET
    sindex      = 0;
    ok          = true;
    explanation = "";

    // Start with a clean state
    clear();

    return *this;
}


tests &tests::istep(cstring name)
// ----------------------------------------------------------------------------
//  Beginning of a step
// ----------------------------------------------------------------------------
{
    record(tests, "Step %+s, catching up", name);
    sname = name;
    if (sindex++)
    {
        passfail(ok);
        if (!ok)
            show(failures.back());
    }
    cstring blk = "                                                            ";
    size_t  off = utf8_length(utf8(sname));
    cstring pad = blk + (off < 60 ? off : 60);
    fprintf(stderr, "%3u:  %03u: %s%s", tindex, sindex, sname, pad);
    cindex = 0;
    count++;
    ok          = true;
    explanation = "";

    return *this;
}


tests &tests::position(cstring sourceFile, uint sourceLine)
// ----------------------------------------------------------------------------
//  Record the position of the current test step
// ----------------------------------------------------------------------------
{
    file = sourceFile;
    line = sourceLine;
    return *this;
}


tests &tests::check(bool valid)
// ----------------------------------------------------------------------------
//   Record if a test fails
// ----------------------------------------------------------------------------
{
    cindex++;
    if (!valid)
        fail();
    return *this;
}


tests &tests::fail()
// ----------------------------------------------------------------------------
//   Report that a test failed
// ----------------------------------------------------------------------------
{
    failures.push_back(
        failure(file, line, tname, sname, explanation, tindex, sindex, cindex));
    ok = false;
    return *this;
}


tests &tests::summary()
// ----------------------------------------------------------------------------
//   Summarize the test results
// ----------------------------------------------------------------------------
{
    if (sindex)
        passfail(ok);

    if (failures.size())
    {
        fprintf(stderr, "Summary of %zu failures:\n", failures.size());
        cstring last = nullptr;
        uint    line = 0;
        for (auto f : failures)
            show(f, last, line);
    }
    fprintf(stderr, "Ran %u tests, %zu failures\n", count, failures.size());
    return *this;
}


tests &tests::show(tests::failure &f)
// ----------------------------------------------------------------------------
//   Show a single failure
// ----------------------------------------------------------------------------
{
    cstring last = nullptr;
    uint    line = 0;
    return show(f, last, line);
}


tests &tests::show(tests::failure &f, cstring &last, uint &line)
// ----------------------------------------------------------------------------
//   Show an individual failure
// ----------------------------------------------------------------------------
{
    if (f.test != last || f.line != line)
    {
        fprintf(stderr,
                "%s:%d:  Test #%u: %s\n",
                f.file,
                f.line,
                f.tindex,
                f.test);
        last = f.test;
    }
    fprintf(stderr,
            "%s:%d: %3u:%03u.%03u: %s\n",
            f.file,
            f.line,
            f.tindex,
            f.sindex,
            f.cindex,
            f.step);
    fprintf(stderr, "%s\n", f.explanation.c_str());
    return *this;
}


// ============================================================================
//
//   Utilities to build the tests
//
// ============================================================================

tests &tests::itest(tests::key k, bool release)
// ----------------------------------------------------------------------------
//   Type a given key directly
// ----------------------------------------------------------------------------
{
    extern int key_remaining();

    // Check for special key sequences
    switch (k)
    {
    case NOSHIFT:
    case LSHIFT:
    case RSHIFT:
    case ALPHA:
    case ALPHA_LS:
    case ALPHA_RS:
    case LOWERCASE:
    case LOWER_LS:
    case LOWER_RS:
        return shifts((k-NOSHIFT) & 1, (k-NOSHIFT) & 2,
                      (k-NOSHIFT) & 4, (k-NOSHIFT) & 8);

    case CLEAR:         return clear();
    case NOKEYS:        return nokeys();
    case REFRESH:       return refreshed();
    case LONGPRESS:     longpress = true; // Next key will be a long press
                        return *this;


    default: break;
    }


    // Wait for the RPL thread to process the keys (to be revisited on DM42)
    while (!key_empty())
        sys_delay(key_delay_time);

    uint lcd_updates = ui_refresh_count();
    record(tests,
           "Push key %d update %u->%u last %d",
           k, refresh_count, lcd_updates, last_key);
    refresh_count = lcd_updates;
    Stack.catch_up();
    last_key = k;

    key_push(k);
    if (longpress)
    {
        sys_delay(600);
        longpress = false;
        release   = false;
    }
    sys_delay(key_delay_time);

    if (release && k != RELEASE)
    {
        while (!key_remaining())
            sys_delay(key_delay_time);
        record(tests,
               "Release key %d update %u->%u last %d",
               k, refresh_count, lcd_updates, last_key);
        refresh_count = lcd_updates;
        Stack.catch_up();
        last_key = -k;
        key_push(RELEASE);

        // Wait for the RPL thread to process the keys
        keysync_sent++;
        record(tests, "Key sync sent %u done %u", keysync_sent, keysync_done);
        key_push(KEYSYNC);
        while (keysync_done != keysync_sent)
            sys_delay(key_delay_time);
        record(tests, "Key sync done %u sent %u", keysync_done, keysync_sent);
    }

    return *this;
}


tests &tests::itest(unsigned int value)
// ----------------------------------------------------------------------------
//    Test a numerical value
// ----------------------------------------------------------------------------
{
    char buffer[32];
    snprintf(buffer, sizeof(buffer), "%u", value);
    return itest(NOSHIFT, cstring(buffer));
}


tests &tests::itest(int value)
// ----------------------------------------------------------------------------
//   Test a signed numerical value
// ----------------------------------------------------------------------------
{
    if (value < 0)
        return itest(uint(-value), CHS);
    else
        return itest(uint(value));
}


tests &tests::itest(unsigned long value)
// ----------------------------------------------------------------------------
//    Test a numerical value
// ----------------------------------------------------------------------------
{
    char buffer[32];
    snprintf(buffer, sizeof(buffer), "%lu", value);
    return itest(NOSHIFT, cstring(buffer));
}


tests &tests::itest(long long value)
// ----------------------------------------------------------------------------
//   Test a signed numerical value
// ----------------------------------------------------------------------------
{
    if (value < 0)
        return itest((unsigned long long) -value, CHS);
    else
        return itest((unsigned long long) value);
}


tests &tests::itest(unsigned long long value)
// ----------------------------------------------------------------------------
//    Test a numerical value
// ----------------------------------------------------------------------------
{
    char buffer[32];
    snprintf(buffer, sizeof(buffer), "%llu", value);
    return itest(NOSHIFT, cstring(buffer));
}


tests &tests::itest(long value)
// ----------------------------------------------------------------------------
//   Test a signed numerical value
// ----------------------------------------------------------------------------
{
    if (value < 0)
        return itest(-value, CHS);
    else
        return itest(value);
}


tests &tests::itest(char c)
// ----------------------------------------------------------------------------
//   Type the character on the calculator's keyboard
// ----------------------------------------------------------------------------
{
    const char buf[] = { c, 0 };
    return itest(buf);
}


tests &tests::itest(cstring txt)
// ----------------------------------------------------------------------------
//   Type the string on the calculator's keyboard
// ----------------------------------------------------------------------------
{
    utf8 u = utf8(txt);

    while (*u)
    {
        unicode c = utf8_codepoint(u);
        u         = utf8_next(u);

        nokeys();

        bool alpha  = ui.alpha;
        bool shift  = false;
        bool xshift = false;
        bool lower  = ui.lowercase;
        key  k      = RELEASE;
        key  fn     = RELEASE;
        bool del    = false;
        bool bsp    = false;

        switch(c)
        {
        case 'A': k = A;            alpha = true; lower = false; break;
        case 'B': k = B;            alpha = true; lower = false; break;
        case 'C': k = C;            alpha = true; lower = false; break;
        case 'D': k = D;            alpha = true; lower = false; break;
        case 'E': k = E;            alpha = true; lower = false; break;
        case 'F': k = F;            alpha = true; lower = false; break;
        case 'G': k = G;            alpha = true; lower = false; break;
        case 'H': k = H;            alpha = true; lower = false; break;
        case 'I': k = I;            alpha = true; lower = false; break;
        case 'J': k = J;            alpha = true; lower = false; break;
        case 'K': k = K;            alpha = true; lower = false; break;
        case 'L': k = L;            alpha = true; lower = false; break;
        case 'M': k = M;            alpha = true; lower = false; break;
        case 'N': k = N;            alpha = true; lower = false; break;
        case 'O': k = O;            alpha = true; lower = false; break;
        case 'P': k = P;            alpha = true; lower = false; break;
        case 'Q': k = Q;            alpha = true; lower = false; break;
        case 'R': k = R;            alpha = true; lower = false; break;
        case 'S': k = S;            alpha = true; lower = false; break;
        case 'T': k = T;            alpha = true; lower = false; break;
        case 'U': k = U;            alpha = true; lower = false; break;
        case 'V': k = V;            alpha = true; lower = false; break;
        case 'W': k = W;            alpha = true; lower = false; break;
        case 'X': k = X;            alpha = true; lower = false; break;
        case 'Y': k = Y;            alpha = true; lower = false; break;
        case 'Z': k = Z;            alpha = true; lower = false; break;

        case 'a': k = A;            alpha = true; lower = true;  break;
        case 'b': k = B;            alpha = true; lower = true;  break;
        case 'c': k = C;            alpha = true; lower = true;  break;
        case 'd': k = D;            alpha = true; lower = true;  break;
        case 'e': k = E;            alpha = true; lower = true;  break;
        case 'f': k = F;            alpha = true; lower = true;  break;
        case 'g': k = G;            alpha = true; lower = true;  break;
        case 'h': k = H;            alpha = true; lower = true;  break;
        case 'i': k = I;            alpha = true; lower = true;  break;
        case 'j': k = J;            alpha = true; lower = true;  break;
        case 'k': k = K;            alpha = true; lower = true;  break;
        case 'l': k = L;            alpha = true; lower = true;  break;
        case 'm': k = M;            alpha = true; lower = true;  break;
        case 'n': k = N;            alpha = true; lower = true;  break;
        case 'o': k = O;            alpha = true; lower = true;  break;
        case 'p': k = P;            alpha = true; lower = true;  break;
        case 'q': k = Q;            alpha = true; lower = true;  break;
        case 'r': k = R;            alpha = true; lower = true;  break;
        case 's': k = S;            alpha = true; lower = true;  break;
        case 't': k = T;            alpha = true; lower = true;  break;
        case 'u': k = U;            alpha = true; lower = true;  break;
        case 'v': k = V;            alpha = true; lower = true;  break;
        case 'w': k = W;            alpha = true; lower = true;  break;
        case 'x': k = X;            alpha = true; lower = true;  break;
        case 'y': k = Y;            alpha = true; lower = true;  break;
        case 'z': k = Z;            alpha = true; lower = true;  break;

        case '0': k = KEY0;         shift = alpha; break;
        case '1': k = KEY1;         shift = alpha; break;
        case '2': k = KEY2;         shift = alpha; break;
        case '3': k = KEY3;         shift = alpha; break;
        case '4': k = KEY4;         shift = alpha; break;
        case '5': k = KEY5;         shift = alpha; break;
        case '6': k = KEY6;         shift = alpha; break;
        case '7': k = KEY7;         shift = alpha; break;
        case '8': k = KEY8;         shift = alpha; break;
        case '9': k = KEY9;         shift = alpha; break;
        case '+': k = ADD;          alpha = true;  shift = true; break;
        case '-': k = SUB;          alpha = true;  shift = true; break;
        case '*': k = MUL;          alpha = true; xshift = true; break;
        case '/': k = DIV;          alpha = true; xshift = true; break;
        case '.': k = DOT;          shift = alpha; break;
        case ',': k = DOT;          shift = !alpha; break;
        case ' ': k = RUNSTOP;      alpha = true;  break;
        case '?': k = KEY7;         alpha = true; xshift = true; break;
        case '!': k = ADD;          alpha = true; xshift = true; break;
        case '_': k = SUB;          alpha = true;  break;
        case '%': k = RCL;          alpha = true;  shift = true; break;
        case ':': k = KEY0;         alpha = true;  bsp   = true; break;
        case ';': k = KEY0;         alpha = true; xshift = true;  break;
        case '<': k = SIN;          alpha = true;  shift = true;  break;
        case '=': k = COS;          alpha = true;  shift = true;  break;
        case '>': k = TAN;          alpha = true;  shift = true;  break;
        case '^': k = INV;          alpha = true;  shift = true;  break;
        case '(': k = XEQ;          alpha = true;  shift = true;  del = true; break;
        case ')': k = XEQ;          alpha = true;  shift = true;  bsp = true; break;
        case '[': k = KEY9;         alpha = false; shift = true;  del = true; break;
        case ']': k = KEY9;         alpha = false; shift = true;  bsp = true; break;
        case '{': k = RUNSTOP;      alpha = true; xshift = true;  del = true; break;
        case '}': k = RUNSTOP;      alpha = true; xshift = true;  bsp = true; break;
        case '"': k = ENTER;        alpha = true; xshift = true;  bsp = true; break;
        case '\'': k = XEQ;         alpha = true; xshift = true;  bsp = true; break;
        case '&': k = KEY1;         alpha = true; xshift = true; break;
        case '@': k = KEY2;         alpha = true; xshift = true; break;
        case '$': k = KEY3;         alpha = true; xshift = true; break;
        case '#': k = KEY4;         alpha = true; xshift = true; break;
        case '\\': k = ADD;         alpha = true; xshift = true; break;
        case '\n': k = BSP;         alpha = true; xshift = true; break;
        case L'«': k = RUNSTOP;     alpha = false; shift = true; del = true; break;
        case L'»': k = RUNSTOP;     alpha = false; shift = true; bsp = true; break;
        case L'→': k = STO;         alpha = true; xshift = true; break;
        case L'·': k = MUL;         alpha = true;  shift = true; break;
        case L'×': k = MUL;         alpha = true;  shift = true; break;
        case L'÷': k = DIV;         alpha = true;  shift = true; break;
        case L'↑': k = C;           alpha = true; xshift = true; break;
        case L'ⅈ': k = G; fn = F1;  alpha = false; shift = true; break;
        case L'∡': k = G; fn = F2;  alpha = false; shift = true; break;
        case L'ρ': k = E;           alpha = true;  shift = true; break;
        case L'θ': k = E;           alpha = true; xshift = true; break;
        case L'π': k = I;           alpha = true;  shift = true; break;
        case L'Σ': k = A;           alpha = true;  shift = true; break;
        case L'∏': k = A;           alpha = true; xshift = true; break;
        case L'∆': k = B;           alpha = true; xshift = true; break;
        case L'≤': k = J;           alpha = true; xshift = true; break;
        case L'≠': k = K;           alpha = true; xshift = true; break;
        case L'≥': k = L;           alpha = true; xshift = true; break;
        case L'√': k = C;           alpha = true;  shift = true; break;
        case L'∫': k = KEY8;        alpha = true; xshift = true; break;
        }

        if (shift)
            xshift = false;
        else if (xshift)
            shift = false;

        if (k == RELEASE)
        {
            fprintf(stderr, "Cannot translate '%c' (%d)\n", c, c);
        }
        else
        {
            // Reach the required shift state
            shifts(shift, xshift, alpha, lower);

            // Send the key
            itest(k);

            // If we have a pair, like (), check if we need bsp or del
            if (bsp)
                itest(BSP, DOWN);
            else if (del)
                itest(SHIFT, BSP);

            // If we have a follow-up key, use that
            if (fn != RELEASE)
                itest(fn);
        }
    }
    return *this;
}


tests &tests::shifts(bool lshift, bool rshift, bool alpha, bool lowercase)
// ----------------------------------------------------------------------------
//   Reach the desired shift state from the current state
// ----------------------------------------------------------------------------
{
    // Must wait for the calculator to process our keys for valid state
    nokeys();

    // Check that we have no error here
    data_entry_noerror();

    // Check invalid input: can only have one shift
    if (lshift && rshift)
        lshift = false;

    // If not alpha, disable lowercase
    if (!alpha)
        lowercase = false;

    // First change lowercase state as necessary, since this messes up shift
    while (lowercase != ui.lowercase || alpha != ui.alpha)
    {
        data_entry_noerror();
        while (!ui.shift)
            itest(SHIFT, NOKEYS);
        itest(ENTER, NOKEYS);
    }

    while (rshift != ui.xshift)
        itest(SHIFT, NOKEYS);

    while (lshift != ui.shift)
        itest(SHIFT, NOKEYS);

    return *this;
}


tests &tests::itest(tests::WAIT delay)
// ----------------------------------------------------------------------------
//   Wait for a given delay
// ----------------------------------------------------------------------------
{
    sys_delay(delay.delay);
    return *this;
}


// ============================================================================
//
//    Test validation
//
// ============================================================================

tests &tests::clear(uint extrawait)
// ----------------------------------------------------------------------------
//   Make sure we are in a clean state
// ----------------------------------------------------------------------------
{
    nokeys(extrawait);
    key_push(CLEAR);
    nokeys(extrawait);
    noerror(extrawait);
    return *this;
}


tests &tests::ready(uint extrawait)
// ----------------------------------------------------------------------------
//   Check if the calculator is ready and we can look at it
// ----------------------------------------------------------------------------
{
    nokeys(extrawait);
    refreshed(extrawait);
    return *this;
}


tests &tests::nokeys(uint extrawait)
// ----------------------------------------------------------------------------
//   Check until the key buffer is empty, indicates that calculator is done
// ----------------------------------------------------------------------------
{
    uint start = sys_current_ms();
    uint wait_time = default_wait_time + extrawait;
    while (sys_current_ms() - start < wait_time && !key_empty())
        sys_delay(refresh_delay_time);
    if (!key_empty())
    {
        explain("Unable to get an empty keyboard buffer");
        fail();
        rt.clear_error();
    }
    return *this;
}


tests &tests::data_entry_noerror()
// ----------------------------------------------------------------------------
//  During data entry, check that no error message pops up
// ----------------------------------------------------------------------------
{
    // Check that we are not displaying an error message
    if (rt.error())
    {
        explain("Unexpected error message [", rt.error(), "] "
                "during data entry, cleared");
        fail();
        rt.clear_error();
    }
    return *this;
}


tests &tests::refreshed(uint extrawait)
// ----------------------------------------------------------------------------
//    Wait until the screen was updated by the calculator
// ----------------------------------------------------------------------------
{
    uint start     = sys_current_ms();
    uint wait_time = default_wait_time + extrawait;

    // Wait for a screen redraw
    record(tests, "Waiting for screen update");
    while (sys_current_ms() - start < wait_time &&
           ui_refresh_count() == refresh_count)
        sys_delay(refresh_delay_time);
    if (ui_refresh_count() == refresh_count)
    {
        explain("No screen refresh");
        fail();
        return *this;
    }

    // Wait for a stack update
    record(tests, "Waiting for key %d in stack at %u", last_key, start);
    start = sys_current_ms();
    bool found = false;
    bool updated = false;
    while (sys_current_ms() - start < wait_time)
    {
        if (!Stack.available())
        {
            sys_delay(refresh_delay_time);
        }
        else if (Stack.available() > 1)
        {
            record(tests, "Consume extra stack");
            Stack.consume();
            updated = true;
        }
        else if (Stack.key() == last_key)
        {
            found = true;
            record(tests, "Consume expected stack");
            break;
        }
        else
        {
            record(tests, "Wrong key %d", Stack.key());
            Stack.consume();
            updated = true;
        }
    }
    if (!found)
    {
        if (updated)
            explain("Stack was updated but for wrong key %d != %d",
                    Stack.key(), last_key);
        else
            explain("Stack was not updated in expected delay");
        fail();
    }

    record(tests,
           "Refreshed, key %d, needs=%u update=%u available=%u",
           Stack.key(),
           refresh_count,
           ui_refresh_count(),
           Stack.available());

    return *this;
}


tests &tests::wait(uint ms)
// ----------------------------------------------------------------------------
//   Force a delay after the calculator was ready
// ----------------------------------------------------------------------------
{
    record(tests, "Waiting %u ms", ms);
    sys_delay(ms);
    return *this;
}


tests &tests::want(cstring ref, uint extrawait)
// ----------------------------------------------------------------------------
//   We want something that looks like this (ignore spacing)
// ----------------------------------------------------------------------------
{
    record(tests, "Expect [%+s] ignoring spacing", ref);
    ready(extrawait);
    cindex++;

    uint start = sys_current_ms();
    uint wait_time = default_wait_time + extrawait;
    while (sys_current_ms() - start < wait_time)
    {
        if (rt.error())
        {
            explain("Expected output [", ref, "], "
                    "got error [", rt.error(), "] instead");
            return fail();
        }

        if (cstring out = cstring(Stack.recorded()))
        {
            record(tests, "Comparing [%s] to [%+s] ignoring spaces", out, ref);
            cstring iout = out;
            cstring iref = ref;
            while (true)
            {
                if (*out == 0 && *ref == 0)
                    return *this;   // Successful match

                if (isspace(*ref))
                {
                    while (*ref && isspace(*ref))
                        ref++;
                    if (!isspace(*out))
                        break;
                    while (*out && isspace(*out))
                        out++;
                }
                else
                {
                    if (*out != *ref)
                        break;
                    out++;
                    ref++;
                }
            }

            if (strcmp(ref, cstring(out)) == 0)
                return *this;
            explain("Expected output matching [", iref, "], "
                    "got [", iout, "] instead, "
                    "[", ref, "] differs from [", out, "]");
            return fail();
        }
        sys_delay(refresh_delay_time);
    }
    record(tests, "No output");
    explain("Expected output [", ref, "] but got no stack change");
    return fail();
}


tests &tests::expect(cstring output, uint extrawait)
// ----------------------------------------------------------------------------
//   Check that the output at first level of stack matches the string
// ----------------------------------------------------------------------------
{
    record(tests, "Expecting [%+s]", output);
    ready(extrawait);
    cindex++;
    uint start = sys_current_ms();
    uint wait_time = default_wait_time + extrawait;
    while (sys_current_ms() - start < wait_time)
    {
        if (rt.error())
        {
            explain("Expected output [", output, "], "
                    "got error [", rt.error(), "] instead");
            return fail();
        }
        if (utf8 out = Stack.recorded())
        {
            record(tests, "Comparing [%s] to [%+s] %+s", out, output,
                   strcmp(output, cstring(out)) == 0 ? "OK" : "FAIL");
            if (strcmp(output, cstring(out)) == 0)
                return *this;
            explain("Expected output [", output, "], "
                    "got [", cstring(out), "] instead");
            return fail();
        }
        sys_delay(refresh_delay_time);
    }
    record(tests, "No output");
    explain("Expected output [", output, "] but got no stack change");
    return fail();
}


tests &tests::expect(int output, uint extrawait)
// ----------------------------------------------------------------------------
//   Check that the output matches an integer value
// ----------------------------------------------------------------------------
{
    char num[32];
    snprintf(num, sizeof(num), "%d", output);
    return expect(num, extrawait);
}


tests &tests::expect(unsigned int output, uint extrawait)
// ----------------------------------------------------------------------------
//   Check that the output matches an integer value
// ----------------------------------------------------------------------------
{
    char num[32];
    snprintf(num, sizeof(num), "%u", output);
    return expect(num, extrawait);
}


tests &tests::expect(long output, uint extrawait)
// ----------------------------------------------------------------------------
//   Check that the output matches an integer value
// ----------------------------------------------------------------------------
{
    char num[32];
    snprintf(num, sizeof(num), "%ld", output);
    return expect(num, extrawait);
}


tests &tests::expect(unsigned long output, uint extrawait)
// ----------------------------------------------------------------------------
//   Check that the output matches an integer value
// ----------------------------------------------------------------------------
{
    char num[32];
    snprintf(num, sizeof(num), "%lu", output);
    return expect(num, extrawait);
}


tests &tests::expect(long long output, uint extrawait)
// ----------------------------------------------------------------------------
//   Check that the output matches an integer value
// ----------------------------------------------------------------------------
{
    char num[32];
    snprintf(num, sizeof(num), "%lld", output);
    return expect(num, extrawait);
}


tests &tests::expect(unsigned long long output, uint extrawait)
// ----------------------------------------------------------------------------
//   Check that the output matches an integer value
// ----------------------------------------------------------------------------
{
    char num[32];
    snprintf(num, sizeof(num), "%llu", output);
    return expect(num, extrawait);
}


tests &tests::match(cstring restr, uint extrawait)
// ----------------------------------------------------------------------------
//   Check that the output at first level of stack matches the string
// ----------------------------------------------------------------------------
{
    ready(extrawait);
    cindex++;

    uint start = sys_current_ms();
    uint wait_time = default_wait_time + extrawait;
    while (sys_current_ms() - start < wait_time)
    {
        if (rt.error())
        {
            explain("Expected output matching [", restr, "], "
                    "got error [", rt.error(), "] instead");
            return fail();
        }

        if (utf8 out = Stack.recorded())
        {
            regex_t    re;
            regmatch_t rm;

            regcomp(&re, restr, REG_EXTENDED | REG_ICASE);
            bool ok = regexec(&re, cstring(out), 1, &rm, 0) == 0 && rm.rm_so == 0 &&
                out[rm.rm_eo] == 0;
            regfree(&re);
            if (ok)
                return *this;
            explain("Expected output matching [", restr, "], "
                    "got [", out, "]");
            return fail();
        }
        sys_delay(refresh_delay_time);
    }
    explain("Expected output matching [", restr, "] but stack not updated");
    return fail();
}


tests &tests::image(cstring file, int x, int y, int w, int h, uint extrawait)
// ----------------------------------------------------------------------------
//   Check that the output in the screen matches what is in the file
// ----------------------------------------------------------------------------
{
    nokeys(extrawait);
    cindex++;

    // If it is not good, keep it on screen a bit longer
    uint start = sys_current_ms();
    uint wait_time = image_wait_time + extrawait;
    while (sys_current_ms() - start < wait_time)
    {
        if (image_match(file, x, y, w, h, false))
            return *this;
        sys_delay(refresh_delay_time);
    }

    explain("Expected screen to match [", file, "]");
    image_match(file, x, y, w, h, true);
    return fail();
}


tests &tests::image_noheader(cstring name, uint ignoremenus, uint extrawait)
// ----------------------------------------------------------------------------
//   Image, skipping the header area
// ----------------------------------------------------------------------------
{
    const int header_h = 20;
    const int menu_h = 22 * ignoremenus;
    return image(name, 0, header_h, LCD_W, LCD_H - header_h - menu_h, extrawait);
}


tests &tests::image_menus(cstring name, uint menus, uint extrawait)
// ----------------------------------------------------------------------------
//   Image, skipping the header area
// ----------------------------------------------------------------------------
{
    const int menu_h = 22 * menus;
    return image(name, 0, LCD_H-menu_h, LCD_W, menu_h, extrawait);
}


tests &tests::type(object::id ty, uint extrawait)
// ----------------------------------------------------------------------------
//   Check that the top of stack matches the type
// ----------------------------------------------------------------------------
{
    ready(extrawait);
    cindex++;

    uint start = sys_current_ms();
    uint wait_time = image_wait_time + extrawait;
    while (sys_current_ms() - start < wait_time)
    {
        if (rt.error())
        {
            explain("Expected type [", object::name(ty), "], "
                    "got error [", rt.error(), "] instead");
            return fail();
        }

        if (utf8 out = Stack.recorded())
        {
            object::id tty = Stack.type();
            if (tty == ty)
                return *this;
            explain("Expected type ", object::name(ty), " (", int(ty), ")"
                    " but got ", object::name(tty), " (", int(tty), ")");
            return fail();
        }
        sys_delay(refresh_delay_time);
    }
    explain("Expected type ", object::name(ty), " (", int(ty), ")"
            " but stack not updated");
    return fail();
}


tests &tests::shift(bool s, uint extrawait)
// ----------------------------------------------------------------------------
//   Check that the shift state matches expectations
// ----------------------------------------------------------------------------
{
    nokeys(extrawait);
    return check(ui.shift == s, "Expected shift ", s, ", got ", ui.shift);
}


tests &tests::xshift(bool x, uint extrawait)
// ----------------------------------------------------------------------------
//   Check that the right shift state matches expectations
// ----------------------------------------------------------------------------
{
    nokeys(extrawait);
    return check(ui.xshift == x, "Expected xshift ", x, " got ", ui.xshift);
}


tests &tests::alpha(bool a, uint extrawait)
// ----------------------------------------------------------------------------
//   Check that the alpha state matches expectations
// ----------------------------------------------------------------------------
{
    nokeys(extrawait);
    return check(ui.alpha == a, "Expected alpha ", a, " got ", ui.alpha);
}


tests &tests::lower(bool l, uint extrawait)
// ----------------------------------------------------------------------------
//   Check that the lowercase state matches expectations
// ----------------------------------------------------------------------------
{
    nokeys(extrawait);
    return check(ui.lowercase == l, "Expected alpha ", l, " got ", ui.alpha);
}


tests &tests::editing(uint extrawait)
// ----------------------------------------------------------------------------
//   Check that we are editing, without checking the length
// ----------------------------------------------------------------------------
{
    nokeys(extrawait);
    return check(rt.editing(),
                 "Expected to be editing, got length ",
                 rt.editing());
}


tests &tests::editing(size_t length, uint extrawait)
// ----------------------------------------------------------------------------
//   Check that the editor has exactly the expected length
// ----------------------------------------------------------------------------
{
    nokeys(extrawait);
    return check(rt.editing() == length,
                 "Expected editing length to be ",
                 length,
                 " got ",
                 rt.editing());
}


tests &tests::editor(cstring text, uint extrawait)
// ----------------------------------------------------------------------------
//   Check that the editor contents matches the text
// ----------------------------------------------------------------------------
{
    byte_p ed = nullptr;
    size_t sz = 0;

    nokeys(extrawait);

    uint start = sys_current_ms();
    uint wait_time = image_wait_time + extrawait;
    while (sys_current_ms() - start < wait_time)
    {
        if (rt.error())
        {
            explain("Expected editor [", text, "], "
                    "got error [", rt.error(), "] instead");
            return fail();
        }

        ed = rt.editor();
        sz = rt.editing();
        if (ed && sz == strlen(text) && memcmp(ed, text, sz) == 0)
            return *this;

        sys_delay(refresh_delay_time);
    }

    if (!ed)
        explain("Expected editor to contain [", text, "], "
                "but it's empty");
    if (sz != strlen(text))
        explain("Expected ", strlen(text), " characters in editor"
                " [", text, "], "
                "but got ", sz, " characters "
                " [", std::string(cstring(ed), sz), "]");
    if (memcmp(ed, text, sz))
        explain("Expected editor to contain [", text, "], "
                       "but it contains [", std::string(cstring(ed), sz), "]");

    fail();
    return *this;
}


tests &tests::cursor(size_t csr, uint extrawait)
// ----------------------------------------------------------------------------
//   Check that the cursor is at expected position
// ----------------------------------------------------------------------------
{
    nokeys(extrawait);
    return check(ui.cursor == csr,
                 "Expected cursor to be at position ", csr,
                 " but it's at position ", ui.cursor);
}


tests &tests::error(cstring msg, uint extrawait)
// ----------------------------------------------------------------------------
//   Check that the error message matches expectations
// ----------------------------------------------------------------------------
{
    utf8 err = nullptr;
    nokeys(extrawait);

    uint start = sys_current_ms();
    uint wait_time = image_wait_time + extrawait;
    while (sys_current_ms() - start < wait_time)
    {
        err = rt.error();
        if (!msg == !err && (!msg || strcmp(cstring(err), msg) == 0))
            return *this;
        sys_delay(refresh_delay_time);
    }
    if (!msg && err)
        explain("Expected no error, got [", err, "]").itest(CLEAR);
    if (msg && !err)
        explain("Expected error message [", msg, "], got none");
    if (msg && err && strcmp(cstring(err), msg) != 0)
        explain("Expected error message [", msg, "], "
                "got [", err, "]");
    fail();
    return *this;
}


tests &tests::command(cstring ref, uint extrawait)
// ----------------------------------------------------------------------------
//   Check that the command result matches expectations
// ----------------------------------------------------------------------------
{
    text_p cmdo = nullptr;
    size_t sz   = 0;
    utf8   cmd  = nullptr;

    nokeys(extrawait);

    uint start = sys_current_ms();
    uint wait_time = image_wait_time + extrawait;
    while (sys_current_ms() - start < wait_time)
    {
        cmdo = rt.command();
        cmd = cmdo->value(&sz);
        if (!ref == !cmd && (!ref || strcmp(ref, cstring(cmd)) == 0))
            return *this;

        sys_delay(refresh_delay_time);
    }


    if (!ref && cmd)
        explain("Expected no command, got [", cmd, "]");
    if (ref && !cmd)
        explain("Expected command [", ref, "], got none");
    if (ref && cmd && strcmp(ref, cstring(cmd)) != 0)
        explain("Expected command [", ref, "], got [", cmd, "]");

    fail();
    return *this;
}


tests &tests::source(cstring ref, uint extrawait)
// ----------------------------------------------------------------------------
//   Check that the source indicated in the editor matches expectations
// ----------------------------------------------------------------------------
{
    utf8 src = nullptr;
    nokeys();

    uint start = sys_current_ms();
    uint wait_time = image_wait_time + extrawait;
    while (sys_current_ms() - start < wait_time)
    {
        utf8 src = rt.source();
        if (!src == !ref && (!ref || strcmp(ref, cstring(src)) == 0))
            return *this;
        sys_delay(refresh_delay_time);
    }

    if (!ref && src)
        explain("Expected no source, got [", src, "]");
    if (ref && !src)
        explain("Expected source [", ref, "], got none");
    if (ref && src && strcmp(ref, cstring(src)) != 0)
        explain("Expected source [", ref, "], " "got [", src, "]");

    fail();
    return *this;
}
