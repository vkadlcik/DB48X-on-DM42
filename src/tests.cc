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
#include "stack.h"
#include "user_interface.h"

#include <regex.h>
#include <stdio.h>

extern bool run_tests;
extern volatile int lcd_needsupdate;
volatile uint keysync_sent = 0;
volatile uint keysync_done = 0;

RECORDER_DECLARE(errors);

uint wait_time  = 200;
uint delay_time = 2;
uint long_tests = 0;

#define TEST_CATEGORY(name, enabled, descr)                     \
    RECORDER_TWEAK_DEFINE(est_##name, enabled, "Test " descr);  \
    static inline bool check_##name(tests &t)                   \
    {                                                           \
        bool result = RECORDER_TWEAK(est_##name);               \
        if (!result)                                            \
            t.begin("Skipping " #name ": " descr);              \
        else                                                    \
            t.begin(#name ": " descr);                          \
        return result;                                          \
    }

#define TESTS(name, descr)      TEST_CATEGORY(name, true,  descr)
#define EXTRA(name, descr)      TEST_CATEGORY(name, false, descr)

#define BEGIN(name)     do { if (!check_##name(*this)) return; } while(0)

TESTS(current,          "Current test for latest commits");
TESTS(defaults,         "Reset settings to defaults");
TESTS(shifts,           "Shift logic");
TESTS(keyboard,         "Keyboard entry");
TESTS(types,            "Data types");
TESTS(stack,            "Stack operations");
TESTS(arithmetic,       "Arithmetic operations");
TESTS(globals,          "Global variables");
TESTS(locals,           "Local variables");
TESTS(for_loops,        "For loops");
TESTS(conditionals,     "Conditionals");
TESTS(logical,          "Logical operations");
TESTS(styles,           "Commands display formats");
TESTS(iformat,          "Integer display formats");
TESTS(dformat,          "Decimal display formats");
TESTS(ifunctions,       "Integer functions");
TESTS(dfunctions,       "Decimal functions");
TESTS(trigoptim,        "Special trigonometry optimzations");
TESTS(dfrac,            "Simple conversion to decimal and back");
TESTS(ctypes,           "Complex types");
TESTS(carith,           "Complex arithmetic");
TESTS(cfunctions,       "Complex functions");
TESTS(lists,            "List operations");
TESTS(text,             "Text operations");
TESTS(vectors,          "Vectors");
TESTS(matrices,         "Matrices");
TESTS(simplify,         "Auto-simplification of expressions");
TESTS(rewrites,         "Equation rewrite engine");
TESTS(expand,           "Expand");
TESTS(tagged,           "Tagged objects");
TESTS(regressions,      "Regression checks");
TESTS(plotting,         "Plotting, graphing and charting");
TESTS(graphics,         "Graphic commands");

EXTRA(plotfns,          "Plot all functions");
EXTRA(flags,            "Enable/disable every RPL flag");
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
        current();
    }
    else
    {
        shift_logic();
        keyboard_entry();
        data_types();
        arithmetic();
        global_variables();
        local_variables();
        for_loops();
        conditionals();
        command_display_formats();
        integer_display_formats();
        decimal_display_formats();
        integer_numerical_functions();
        decimal_numerical_functions();
        complex_types();
        complex_arithmetic();
        complex_functions();
        list_functions();
        text_functions();
        rewrite_engine();
        expand_collect_simplify();
        tagged_objects();
        plotting();
        plotting_all_functions();
        graphic_commands();
        flags_by_name();
        settings_by_name();
        parsing_commands_by_name();
        regression_checks();
    }
    summary();

    RECORDER_TRACE(errors) = tracing;

    if (run_tests)
        exit(failures.size() ? 1 : 0);
}


void tests::current()
// ----------------------------------------------------------------------------
//   Test the current thing (this is a temporary test)
// ----------------------------------------------------------------------------
{
    BEGIN(current);
    stack_operations();
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
        .test("ModesMenu", ENTER).noerr();
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
        .noerr()
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
    test(CLEAR, seps).editor(seps).wait(500);

    step("Separators with auto-spacing");
    cstring seps2     = "{}()[]";
    cstring seps2auto = "{ } ( ) []";
    test(CLEAR, seps2).editor(seps2auto).wait(500);

    step("Key repeat");
    test(CLEAR, LONGPRESS, SHIFT, LONGPRESS, A)
        .wait(1000)
        .test(RELEASE)
        .check(ui.cursor > 4);
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

    step("Symbols");
    cstring symbol = "ABC123Z";
    test(CLEAR, symbol, ENTER).type(object::ID_expression).expect("'ABC123Z'");

    step("Text");
    cstring string = "\"Hello World\"";
    test(CLEAR, string, ENTER).type(object::ID_text).expect(string);

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
         SHIFT, SHIFT, DOWN, SHIFT, F3, " 1 +", ENTER)
        .type(object::ID_expression).expect("'X⁻¹+(Y²+Z³)+1'");

    step("Fractions");
    test(CLEAR, "1/3", ENTER).type(object::ID_fraction).expect("¹/₃");
    test(CLEAR, "-80/60", ENTER).type(object::ID_neg_fraction).expect("-⁴/₃");
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
    test(CLEAR, "DetailedTypes", ENTER).noerr();
    test(CLEAR, "12 type", ENTER)
        .type(object::ID_neg_integer)
        .expect(~int(object::ID_integer));
    test(CLEAR, "'ABC*3' type", ENTER)
        .type(object::ID_neg_integer)
        .expect(~int(object::ID_expression));

    step("Type command (compatible mode)");
    test(CLEAR, "CompatibleTypes", ENTER).noerr();
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


void tests::stack_operations()
// ----------------------------------------------------------------------------
//   Test stack operations
// ----------------------------------------------------------------------------
{
    BEGIN(stack);

    step("Dup with ENTER")
        .test(CLEAR, "12", ENTER, ENTER, ADD).expect("24");
    step("Drop with Backspace")
        .test(CLEAR, "12 34", ENTER).noerr().expect("34")
        .test(BSP).noerr().expect("12")
        .test(BSP).noerr()
        .test(BSP).error("Too few arguments");

    step("Dup in program")
        .test(CLEAR, "13 Dup +", ENTER).expect("26");
    step("Dup2")
        .test(CLEAR, "13 25 Dup2 * + *", ENTER).expect("4 550");
    step("Over")
        .test(CLEAR, "13 25 Over / +", ENTER).expect("¹⁹⁴/₁₃");
    step("Rot")
        .test(CLEAR, "13 17 25 Rot / +", ENTER).expect("²⁴⁶/₁₃");
    step("Over in stack menu")
        .test(CLEAR, I, "13 25", F2, DIV, ADD).expect("¹⁹⁴/₁₃");
    step("Rot in stack menu")
        .test(CLEAR, "13 17 25", F1, DIV, ADD).expect("²⁴⁶/₁₃");
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
        .test(BSP).noerr()
        .test(BSP).error("Too few arguments");
    step("RollDn in stack menu")
        .test(CLEAR, "13 17 25 42 21 372 4", F6).expect("21")
        .test(BSP).expect("42")
        .test(BSP).expect("25")
        .test(BSP).expect("372")
        .test(BSP).expect("17")
        .test(BSP).expect("13")
        .test(BSP).noerr()
        .test(BSP).error("Too few arguments");
    step("DropN in stack menu")
        .test(CLEAR, "13 17 25 42 21 372 4", SHIFT, F6).expect("17")
        .test(BSP).expect("13")
        .test(BSP).noerr()
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
        .test(BSP).noerr()
        .test(BSP).error("Too few arguments");
    step("Drop2 in stack menu")
        .test(CLEAR, "13 17 25 42 21 372 4", SHIFT, F4).expect("21")
        .test(BSP).expect("42")
        .test(BSP).expect("25")
        .test(BSP).expect("17")
        .test(BSP).expect("13")
        .test(BSP).noerr()
        .test(BSP).error("Too few arguments");
    step("Dup2 in stack menu")
        .test(CLEAR, "13 17 25 42", SHIFT, F3).expect("42")
        .test(BSP).expect("25")
        .test(BSP).expect("42")
        .test(BSP).expect("25")
        .test(BSP).expect("17")
        .test(BSP).expect("13")
        .test(BSP).noerr()
        .test(BSP).error("Too few arguments");
    step("Simple stack commands from menu")
        .test(CLEAR, SHIFT, RUNSTOP,
              F1, F2, F3, F4, F5, F6,
              SHIFT, F1, SHIFT, F2, SHIFT, F3,
              SHIFT, F4, SHIFT, F5, SHIFT, F6,
              SHIFT, SHIFT, F1, SHIFT, SHIFT, F2, SHIFT, SHIFT, F3,
              SHIFT, SHIFT, F5, SHIFT, SHIFT, F6,
              ENTER)
        .expect("« Rot Over Depth Pick Roll RollDown "
                "Duplicate Drop Duplicate2 Drop2 DuplicateN DropN "
                "Swap LastArguments Clear LastX »").test(BSP).noerr();

    step("LastArg")
        .test(CLEAR, "1 2").shifts(false, false, false, false)
        .test(ADD).expect("3")
        .test(SHIFT, M).expect("2")
        .test(BSP).expect("1")
        .test(BSP).expect("3")
        .test(BSP).noerr()
        .test(BSP).error("Too few arguments");
    step("Undo")
        .test(CLEAR, "1 2").shifts(false, false, false, false)
        .test(ADD).expect("3")
        .test(SHIFT, SHIFT, M).expect("2")
        .test(BSP).expect("1")
        .test(BSP).noerr()
        .test(BSP).error("Too few arguments");
    step("LastX")
        .test(CLEAR, "1 2").shifts(false, false, false, false)
        .test(ADD).expect("3")
        .test(SHIFT, SHIFT, F5).expect("2")
        .test(BSP).expect("3")
        .test(BSP).noerr()
        .test(BSP).error("Too few arguments");
    step("ClearStk")
        .test(CLEAR, "1 2 3 4", ENTER)
        .test(SHIFT, SHIFT, F3).noerr()
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
    test(STO).noerr();
    step("Recall global variable");
    test(CLEAR, 1, ENTER, XEQ, "A", ENTER).expect("'A'");
    test("RCL", ENTER).noerr().expect("12 345");

    step("Store in long-name global variable");
    test(CLEAR, "\"Hello World\"", ENTER, XEQ, "SomeLongVariable", ENTER, STO)
        .noerr();
    step("Recall global variable");
    test(CLEAR, XEQ, "SomeLongVariable", ENTER, "recall", ENTER)
        .noerr()
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
    test(CLEAR, "'X+Y' 'eq' STO", ENTER).noerr();
    test(CLEAR, "'EQ' RCL", ENTER).expect("'X+Y'");
    test(CLEAR, "'equation' RCL", ENTER).expect("'X+Y'");
    test(CLEAR, "'Equation' PURGE", ENTER).noerr();

    step("Store and recall to StatsData");
    test(CLEAR, "[1 2 3] 'ΣData' STO", ENTER).noerr();
    test(CLEAR, "'ΣDat' RCL", ENTER).expect("[ 1 2 3 ]");
    test(CLEAR, "'StatsData' RCL", ENTER).expect("[ 1 2 3 ]");
    test(CLEAR, "'ΣData' PURGE", ENTER).noerr();

    step("Store and recall to StatsParameters");
    test(CLEAR, "{0} 'ΣParameters' STO", ENTER).noerr();
    test(CLEAR, "'ΣPar' RCL", ENTER).expect("{ 0 }");
    test(CLEAR, "'StatsParameters' RCL", ENTER).expect("{ 0 }");
    test(CLEAR, "'ΣPar' purge", ENTER).noerr();

    step("Store and recall to PlotParameters");
    test(CLEAR, "{1} 'PPAR' STO", ENTER).noerr();
    test(CLEAR, "'PlotParameters' RCL", ENTER).expect("{ 1 }");
    test(CLEAR, "'ppar' RCL", ENTER).expect("{ 1 }");
    test(CLEAR, "'PPAR' purge", ENTER).noerr();

    step("Numbered store and recall should fail by default");
    test(CLEAR, 5678, ENTER, 1234, ENTER, "STO", ENTER).error("Invalid name");
    test(CLEAR, 1234, ENTER, "RCL", ENTER).error("Invalid name");
    test(CLEAR, 1234, ENTER, "Purge", ENTER).error("Invalid name");

    step("Enable NumberedVariables");
    test(CLEAR, "NumberedVariables", ENTER).noerr();
    test(CLEAR, 5678, ENTER, 1234, ENTER, "STO", ENTER).noerr();
    test(CLEAR, 1234, ENTER, "RCL", ENTER).noerr().expect("5 678");
    test(CLEAR, 1234, ENTER, "Purge", ENTER).noerr();

    step("Disable NumberedVariables");
    test(CLEAR, "NoNumberedVariables", ENTER).noerr();
    test(CLEAR, 5678, ENTER, 1234, ENTER, "STO", ENTER).error("Invalid name");
    test(CLEAR, 1234, ENTER, "RCL", ENTER).error("Invalid name");
    test(CLEAR, 1234, ENTER, "Purge", ENTER).error("Invalid name");

    step("Store program in global variable");
    test(CLEAR, "« 1 + »", ENTER, XEQ, "INCR", ENTER, STO).noerr();
    step("Evaluate global variable");
    test(CLEAR, "A INCR", ENTER).expect("12 346");

    step("Purge global variable");
    test(CLEAR, XEQ, "A", ENTER, "PURGE", ENTER).noerr();
    test(CLEAR, XEQ, "INCR", ENTER, "PURGE", ENTER).noerr();
    test(CLEAR, XEQ, "SomeLongVariable", ENTER, "PURGE", ENTER).noerr();

    test(CLEAR, XEQ, "A", ENTER, "RCL", ENTER).error("Undefined name").clear();
    test(CLEAR, XEQ, "INCR", ENTER, "RCL", ENTER)
        .error("Undefined name")
        .clear();
    test(CLEAR, XEQ, "SomeLongVariable", ENTER, "RCL", ENTER)
        .error("Undefined name")
        .clear();

    step("Go to top-level");
    test(CLEAR, "Home", ENTER).noerr();
    step("Clear 'DirTest'");
    test(CLEAR, "'DirTest' pgdir", ENTER);
    step("Create directory");
    test(CLEAR, "'DirTest' crdir", ENTER).noerr();
    step("Enter directory");
    test(CLEAR, "DirTest", ENTER).noerr();
    step("Path function");
    test(CLEAR, "PATH", ENTER).expect("{ HomeDirectory DirTest }");
    step("Updir function");
    test(CLEAR, "UpDir path", ENTER).expect("{ HomeDirectory }");
    step("Enter directory again");
    test(CLEAR, "DirTest path", ENTER).expect("{ HomeDirectory DirTest }");
    step("Current directory content");
    test(CLEAR, "CurrentDirectory", ENTER).want("Directory { }");
    step("Store in subdirectory");
    test(CLEAR, "242 'Foo' STO", ENTER).noerr();
    step("Recall from subdirectory");
    test(CLEAR, "Foo", ENTER).expect("242");
    step("Recursive directory");
    test(CLEAR, "'DirTest2' crdir", ENTER).noerr();
    step("Entering sub-subdirectory");
    test(CLEAR, "DirTest2", ENTER).noerr();
    step("Path in sub-subdirectory");
    test(CLEAR, "path", ENTER).expect("{ HomeDirectory DirTest DirTest2 }");
    step("Find variable from level above");
    test(CLEAR, "Foo", ENTER).expect("242");
    step("Create local variable");
    test(CLEAR, "\"Hello\" 'Foo' sto", ENTER).noerr();
    step("Local variable hides variable above");
    test(CLEAR, "Foo", ENTER).expect("\"Hello\"");
    step("Updir shows shadowed variable again");
    test(CLEAR, "Updir Foo", ENTER).expect("242");
    step("Two independent variables with the same name");
    test(CLEAR, "DirTest2 Foo", ENTER).expect("\"Hello\"");
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
    test(XEQ, "LocTest", ENTER, STO).noerr();

    step("Calling a local block with numerical values");
    test(CLEAR, 1, ENTER, 2, ENTER, 3, ENTER, "LocTest", ENTER).expect("³/₅");

    step("Calling a local block with symbolic values");
    test(CLEAR,
         XEQ, "X", ENTER,
         XEQ, "Y", ENTER,
         XEQ, "Z", ENTER,
         "LocTest", ENTER)
        .expect("'(X+Y)×(X-Y)÷((Y+Z)×(Y-Z))'");

    step("Cleanup");
    test(CLEAR, XEQ, "LocTest", ENTER, "PurgeAll", ENTER).noerr();
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
    test(CLEAR, pgm, ENTER).noerr().type(object::ID_program).want(pgmo);
    test(RUNSTOP).noerr().type(object::ID_integer).expect(385);

    step("Algebraic 1..10");
    pgm  = "« 'X' 1 5 FOR i i SQ + NEXT »";
    pgmo = "« 'X' 1 5 for i i x² + next »";
    test(CLEAR, pgm, ENTER).noerr().type(object::ID_program).want(pgmo);
    test(RUNSTOP).noerr().type(object::ID_expression).expect("'X+1+4+9+16+25'");

    step("Stepping by 2");
    pgm  = "« 0 1 10 FOR i i SQ + 2 STEP »";
    pgmo = "« 0 1 10 for i i x² + 2 step »";
    test(CLEAR, pgm, ENTER).noerr().type(object::ID_program).want(pgmo);
    test(RUNSTOP).noerr().type(object::ID_integer).expect(165);

    step("Stepping by i");
    pgm  = "« 'X' 1 100 FOR i i SQ + i step »";
    pgmo = "« 'X' 1 100 for i i x² + i step »";
    test(CLEAR, pgm, ENTER).noerr().type(object::ID_program).want(pgmo);
    test(RUNSTOP)
        .noerr()
        .type(object::ID_expression)
        .expect("'X+1+4+16+64+256+1 024+4 096'");

    step("Negative stepping");
    pgm  = "« 0 10 1 FOR i i SQ + -1 STEP »";
    pgmo = "« 0 10 1 for i i x² + -1 step »";
    test(CLEAR, pgm, ENTER).noerr().type(object::ID_program).want(pgmo);
    test(RUNSTOP).noerr().type(object::ID_integer).expect(385);

    step("Negative stepping algebraic");
    pgm  = "« 'X' 10 1 FOR i i SQ + -1 step »";
    pgmo = "« 'X' 10 1 for i i x² + -1 step »";
    test(CLEAR, pgm, ENTER).noerr().type(object::ID_program).want(pgmo);
    test(RUNSTOP)
        .noerr()
        .type(object::ID_expression)
        .expect("'X+100+81+64+49+36+25+16+9+4+1'");

    step("Fractional");
    pgm  = "« 'X' 0.1 0.9 FOR i i SQ + 0.1 step »";
    pgmo = "« 'X' 0.1 0.9 for i i x² + 0.1 step »";
    test(CLEAR, pgm, ENTER).noerr().type(object::ID_program).want(pgmo);
    test(RUNSTOP)
        .noerr()
        .type(object::ID_expression)
        .expect("'X+0.01+0.04+0.09+0.16+0.25+0.36+0.49+0.64+0.81'");

    step("Fractional down");
    pgm  = "« 'X' 0.9 0.1 FOR i i SQ + -0.1 step »";
    pgmo = "« 'X' 0.9 0.1 for i i x² + -0.1 step »";
    test(CLEAR, pgm, ENTER).noerr().type(object::ID_program).want(pgmo);
    test(RUNSTOP)
        .noerr()
        .type(object::ID_expression)
        .expect("'X+0.81+0.64+0.49+0.36+0.25+0.16+0.09+0.04+0.01'");

    step("Execute at least once");
    pgm  = "« 'X' 10 1 FOR i i SQ + NEXT »";
    pgmo = "« 'X' 10 1 for i i x² + next »";
    test(CLEAR, pgm, ENTER).noerr().type(object::ID_program).want(pgmo);
    test(RUNSTOP).noerr().type(object::ID_expression).expect("'X+100'");
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
    test("1 base", ENTER).error("Invalid numeric base");
    test(CLEAR, "37 base", ENTER).error("Invalid numeric base");
    test(CLEAR, "0.3 base", ENTER).error("Bad argument type");
    test(CLEAR);

    step("Default word size");
    test("WordSize", ENTER).expect("64");
    step("Set word size to 16");
    test(CLEAR, "16 STWS", ENTER).noerr();

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
    test(CLEAR, "32 STWS", ENTER).noerr();
    test(CLEAR, "#12 not", ENTER).expect("#FFFF FFED₁₆");
    test("not", ENTER).expect("#12₁₆");

    step("Set word size to 30");
    test(CLEAR, "30 STWS", ENTER).noerr();
    test(CLEAR, "#142 not", ENTER).expect("#3FFF FEBD₁₆");
    test("not", ENTER).expect("#142₁₆");

    step("Set word size to 48");
    test(CLEAR, "48 STWS", ENTER).noerr();
    test(CLEAR, "#233 not", ENTER).expect("#FFFF FFFF FDCC₁₆");
    test("not", ENTER).expect("#233₁₆");

    step("Set word size to 64");
    test(CLEAR, "64 STWS", ENTER).noerr();
    test(CLEAR, "#64123 not", ENTER).expect("#FFFF FFFF FFF9 BEDC₁₆");
    test("not", ENTER).expect("#6 4123₁₆");

    step("Set word size to 128");
    test(CLEAR, "128 STWS", ENTER).noerr();
    test(CLEAR, "#12 not", ENTER).expect("#FFFF FFFF FFFF FFFF FFFF FFFF FFFF FFED₁₆");
    test("dup not", ENTER).expect("#12₁₆");
    test("xor not", ENTER).expect("#0₁₆");
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

    test(CLEAR, prgm, ENTER).noerr();
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
        .test("3 MantissaSpacing", ENTER)       .noerr()
        .test("5 FractionSpacing", ENTER)       .noerr()
        .test("4 BasedSpacing", ENTER)          .noerr()
        .test("NumberSpaces", ENTER)            .noerr()
        .test("BasedSpaces", ENTER)             .noerr();

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
    test(CLEAR, "FancyExponent", ENTER).noerr();

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
    test(ENTER).noerr();

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


void tests::decimal_display_formats()
// ----------------------------------------------------------------------------
//   Check the various display formats for decimal values
// ----------------------------------------------------------------------------
{
    BEGIN(dformat);

    step("Standard mode");
    test(CLEAR, "STD", ENTER).noerr();

    step("Small number");
    test(CLEAR, "1.03", ENTER)
        .type(object::ID_decimal)
        .expect("1.03");

    step("Zero");
    test(CLEAR, ".", ENTER).error("Syntax error");
    test(CLEAR, "0.", ENTER).type(object::ID_decimal).expect("0.");

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
    test(CLEAR, "4 FIX", ENTER).noerr();
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
    test(CLEAR, "24 FIX", ENTER).noerr();
    test(CLEAR, "1.01", ENTER).expect("1.01000 00000 00000 00000 0000");
    test(CLEAR, "1.0123 log", ENTER)
        .expect("0.01222 49696 22568 97092 2453");

    step("SCI 3 mode");
    test(CLEAR, "3 Sci", ENTER).noerr();
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
    test(CLEAR, "3 eng", ENTER).noerr();
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
    test(CLEAR, "3 sig", ENTER).noerr();
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
    test(CLEAR, "5 Sci", ENTER).noerr();
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
    test(CLEAR, "5 eng", ENTER).noerr();
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
    test(CLEAR, "5 sig", ENTER).noerr();
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
    test(CLEAR, "13 Sci", ENTER).noerr();
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
    test(CLEAR, "13 eng", ENTER).noerr();
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
    test(CLEAR, "13 sig", ENTER).noerr();
    test(CLEAR, "1.01", ENTER).expect("1.01")
        .test(CHS).expect("-1.01");
    test(CLEAR, "1.0123", ENTER).expect("1.0123");
    test(CLEAR, "10.12345", ENTER).expect("10.12345");
    test(CLEAR, "101.2543", ENTER).expect("101.2543");
    test(CLEAR, "1999.999", ENTER).expect("1 999.999");
    test(CLEAR, "19999999999999.", ENTER).expect("2.⁳¹³");
    test(CLEAR, "0.00000000001999999", ENTER).expect("1.99999 9⁳⁻¹¹")
        .test(CHS).expect("-1.99999 9⁳⁻¹¹");

    step("Reset defaults");
    test(CLEAR, "Std", ENTER).noerr();

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
    test(CLEAR, "34 PRECISION 64 SIG", ENTER).noerr();
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
        .expect("3.11526 47975 07788 16199 37694 70404 98442⁳⁻¹")
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
    test(CLEAR, "RAD", ENTER).noerr();

#define TFNA(name, arg, result)                                         \
    step(#name).test(CLEAR, #arg " " #name, ENTER).expect(result);
#define TFN(name, result)  TFNA(name, 0.321, result)

    TFN(sqrt, "5.66568 61896 86117 79925 47340 46967 69⁳⁻¹");
    TFN(sin, "3.15515 63859 27271 11306 59311 11434 63699⁳⁻¹");
    TFN(cos, "9.48920 37695 65830 17543 94513 28269 25533⁳⁻¹");
    TFN(tan, "3.32499 59243 64718 75108 70873 01027 37935⁳⁻¹");
    TFN(asin, "3.26785 17653 14954 63269 19976 45195 98267⁳⁻¹");
    TFN(acos, "1.24401 11502 63401 15596 21219 27120 15339");
    TFN(atan, "3.10609 79281 38899 17606 70005 14468 36027⁳⁻¹");
    TFN(sinh, "3.26541 16495 18063 57012 20656 38857 3434⁳⁻¹");
    TFN(cosh, "1.05196 44159 41947 53843 52241 43605 67798");
    TFN(tanh, "3.10410 84660 58860 21485 05020 93830 95885⁳⁻¹");
    TFN(asinh, "3.15728 26582 93796 17910 89454 71020 69687⁳⁻¹");
    TFNA(acosh, 1.321, "7.81230 20519 62526 14742 21716 16034 3493⁳⁻¹");
    TFN(atanh, "3.32761 58848 18145 95801 76417 05087 51085⁳⁻¹");
    TFN(log1p, "2.78389 02554 01882 66771 62834 21115 50952⁳⁻¹");
    TFN(lnp1, "2.78389 02554 01882 66771 62834 21115 50952⁳⁻¹");
    TFN(expm1, "3.78505 58089 37538 95447 43070 74914 12321⁳⁻¹");
    TFN(log, "-1.13631 41558 52121 18735 43303 10107 28989");
    TFN(log10, "-4.93494 96759 51279 21870 43085 72834 4906⁳⁻¹");
    TFN(exp, "1.37850 55808 93753 89544 74307 07491 41232");
    TFN(exp10, "2.09411 24558 50892 67051 98819 85846 25421");
    TFN(exp2, "1.24919 61256 53376 70052 14667 82085 80659");
    TFN(erf, "3.50144 22082 00238 23551 60324 50502 3913⁳⁻¹");
    TFN(erfc, "6.49855 77917 99761 76448 39675 49497 6087⁳⁻¹");
    TFN(tgamma, "2.78663 45408 45472 36795 07642 12781 773");
    TFN(lgamma, "1.02483 46099 57313 19869 10927 53834 887");
    TFN(gamma, "2.78663 45408 45472 36795 07642 12781 773");
    TFN(cbrt, "6.84702 12775 72241 61840 92773 26468 15⁳⁻¹");
    TFN(norm, "0.321");
#undef TFN

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
    test(CLEAR, "24 PRECISION 12 SIG", ENTER).noerr();
}


void tests::exact_trig_cases()
// ----------------------------------------------------------------------------
//   Special trig cases that are handled accurately for polar representation
// ----------------------------------------------------------------------------
{
    BEGIN(trigoptim);

    cstring unit_names[] = { "Grads", "Degrees", "PiRadians" };
    int circle[] = { 400, 360, 2 };

    for (uint unit = 0; unit < 3; unit++)
    {
        step(unit_names[unit]);
        test(CLEAR, unit_names[unit], ENTER).noerr();

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

    test(CLEAR, "DEG", ENTER).noerr();
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
            "1/3",          "3.33333 33333 33333 3333⁳⁻¹",
            "-1/7",         "-1.42857 14285 71428 5714⁳⁻¹",
            "22/7",         "3.14285 71428 57142 8571",
            "37/213",       "1.73708 92018 77934 2723⁳⁻¹"
        };

    BEGIN(dfrac);

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
    test(CLEAR, "355 113 / pi -", ENTER) .expect("'355/113-π'");
    test("→Num", ENTER).expect("2.66764 18906 24223 1237⁳⁻⁷");
}


void tests::complex_types()
// ----------------------------------------------------------------------------
//   Complex data types
// ----------------------------------------------------------------------------
{
    BEGIN(ctypes);

    step("Select degrees for the angle");
    test(CLEAR, "DEG", ENTER).noerr();

    step("Integer rectangular form");
    test(CLEAR, "0ⅈ0", ENTER)
        .type(object::ID_rectangular).expect("0+0ⅈ");
    test(CLEAR, "1ⅈ2", ENTER)
        .type(object::ID_rectangular).expect("1+2ⅈ");
    test(CLEAR, "3+ⅈ4", ENTER)
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
        .type(object::ID_polar).expect("0∡0°");
    test(CLEAR, "1∡90", ENTER)
        .type(object::ID_polar).expect("1∡90°");
    test(CLEAR, "1∡-90", ENTER)
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
    test("PiRadians", ENTER).expect("1∡¹/₂π");
    test("RAD", ENTER).expect("1∡1.57079 63267 9ℼ");
}


void tests::complex_arithmetic()
// ----------------------------------------------------------------------------
//   Complex arithmetic operations
// ----------------------------------------------------------------------------
{
    BEGIN(carith);

    step("Use degrees");
    test("DEG", ENTER).noerr();

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
        .type(object::ID_rectangular).expect("³⁰/₁₃+⁷/₁₃ⅈ");
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
        .expect("'a×c-b×d'+'a×d+b×c'ⅈ");
    step("Symbolic division");
    test(CLEAR, "a+bⅈ", ENTER, "c+dⅈ", DIV)
        .expect("'(a×c+b×d)÷(c²+d²)'+'(b×c-a×d)÷(c²+d²)'ⅈ");

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
        .expect("3.99208 29778+2.44168 91793 5⁳⁻¹ⅈ");
    step("Subtraction");
    test("1∡2", SUB)
        .expect("2.99269 21507 8+2.09269 42123 2⁳⁻¹ⅈ");
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
        .expect("'a×cos b+c×cos d'+'a×sin b+c×sin d'ⅈ");
    step("Symbolic substraction aligned");
    test(CLEAR, "a∡b", ENTER, "c∡b", ENTER, SUB)
        .expect("'a-c'∡b");
    step("Symbolic subtraction");
    test(CLEAR, "a∡b", ENTER, "c∡d", ENTER, SUB)
        .expect("'a×cos b-c×cos d'+'a×sin b-c×sin d'ⅈ");
    step("Symbolic multiplication");
    test(CLEAR, "a∡b", ENTER, "c∡d", ENTER, MUL)
        .expect("'a×c'∡'b+d'");
    step("Symbolic division");
    test(CLEAR, "a∡b", ENTER, "c∡d", ENTER, DIV)
        .expect("'a÷c'∡'b-d'");

    step("Precedence of complex numbers during rendering");
    test(CLEAR, "'2+3ⅈ' '3∡4' *", ENTER)
        .expect("'(2+3ⅈ)×(3∡4°)'");
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
    test(CLEAR, "34 PRECISION 20 SIG", ENTER).noerr();

    step("Using radians");
    test(CLEAR, "RAD", ENTER).noerr();

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
        .expect("'√((a⊿b+a)÷2)'+'sign (√((a⊿b-a)÷2))×√((a⊿b-a)÷2)'ⅈ");

    step("Symbolic sqrt in polar form");
    test(CLEAR, "a∡b", ENTER, SQRT)
        .expect("'√ a'∡'b÷2'");

    step("Cubed");
    test(CLEAR, "3+7ⅈ", ENTER, "cubed", ENTER)
        .expect("-414-154ⅈ");
    step("Cube root");
    test("cbrt", ENTER)
        .expect("7.61577 31058 63908 2857∡-9.28490 56188 33822 9639⁳⁻¹ℼ");

    step("Logarithm");
    test(CLEAR, "12+14ⅈ", ENTER, LN)
        .expect("2.91447 28088 05103 5368+8.62170 05466 72263 4884⁳⁻¹ⅈ");
    step("Exponential");
    test("exp", ENTER)
        .expect("18.43908 89145 85774 62∡8.62170 05466 72263 4884⁳⁻¹ℼ");

    step("Power");
    test(CLEAR, "3+7ⅈ", ENTER, "2-3ⅈ", ENTER, SHIFT, B)
        .expect("1 916.30979 15541 96293 8∡2.52432 98723 79583 8639ℼ");

    step("Sine");
    test(CLEAR, "4+2ⅈ", ENTER, SIN)
        .expect("-2.84723 90868 48827 8827-2.37067 41693 52001 6145ⅈ");

    step("Cosine");
    test(CLEAR, "3+11ⅈ", ENTER, COS)
        .expect("-29 637.47552 74860 62145-4 224.71967 95347 02126ⅈ");

    step("Tangent");
    test(CLEAR, "2+1ⅈ", ENTER, TAN)
        .expect("-2.43458 20118 57252 527⁳⁻¹+1.16673 62572 40919 8818ⅈ");

    step("Arc sine");
    test(CLEAR, "3+5ⅈ", ENTER, SHIFT, SIN)
        .expect("5.33999 06959 41686 1164⁳⁻¹+2.45983 15216 23434 5129ⅈ");

    step("Arc cosine");
    test(CLEAR, "7+11ⅈ", ENTER, SHIFT, COS)
        .expect("1.00539 67973 35154 2326-3.26167 13063 80062 6275ⅈ");

    step("Arc tangent");
    test(CLEAR, "9.+2ⅈ", ENTER, SHIFT, TAN)
        .expect("1.46524 96601 83523 3458+2.32726 05766 50298 8381⁳⁻²ⅈ");

    step("Hyperbolic sine");
    test(CLEAR, "4+2ⅈ", ENTER, "SINH", ENTER)
        .expect("-11.35661 27112 18172 906+24.83130 58489 46379 372ⅈ");

    step("Hyperbolic cosine");
    test(CLEAR, "3+11ⅈ", ENTER, "COSH", ENTER)
        .expect("4.43360 88910 78241 4161⁳⁻²-10.06756 33986 40475 46ⅈ");

    step("Hyperbolic tangent");
    test(CLEAR, "2+8ⅈ", ENTER, "TANH", ENTER)
        .expect("1.03564 79469 63237 6354-1.09258 84335 75253 1964⁳⁻²ⅈ");

    step("Hyperbolic arc sine");
    test(CLEAR, "3+5ⅈ", ENTER, SHIFT, "ASINH", ENTER)
        .expect("2.45291 37425 02811 7695+1.02382 17465 11782 9101ⅈ");

    step("Hyperbolic arc cosine");
    test(CLEAR, "7+11ⅈ", ENTER, SHIFT, "ACOSH", ENTER)
        .expect("3.26167 13063 80062 6275+1.00539 67973 35154 2326ⅈ");

    step("Hyperbolic arc tangent");
    test(CLEAR, "9.+2ⅈ", ENTER, SHIFT, "ATANH", ENTER)
        .expect("1.06220 79849 13164 9131⁳⁻¹+1.54700 47751 56404 9213ⅈ");

    step("Real to complex");
    test(CLEAR, "1 2 R→C", ENTER)
        .type(object::ID_rectangular).expect("1+2ⅈ");
    step("Symbolic real to complex");
    test(CLEAR, "a b R→C", ENTER)
        .type(object::ID_rectangular).expect("'a'+'b'ⅈ");

    step("Complex to real");
    test(CLEAR, "1+2ⅈ C→R", ENTER)
        .expect("2").test(BSP).expect("1");
    step("Symbolic complex to real");
    test(CLEAR, "a+bⅈ C→R", ENTER)
        .expect("b").test(BSP).expect("a");

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
    test(CLEAR, "1+1ⅈ arg", ENTER).expect("7.85398 16339 74483 0962⁳⁻¹");
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
    test(CLEAR, "24 PRECISION 12 SIG", ENTER).noerr();
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
    test(CLEAR, "{ A { D E { 1 2 \"Hello World\" } F } 2 3 }", ENTER,\
         "{ 2 3 3 5 } GET", ENTER)
        .expect("\"o\"");

    step("Array indexing");
    test(CLEAR, "[ A [ D E [ 1 2 \"Hello World\" ] F ] 2 3 ]", ENTER,\
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
        .type(object::ID_array).expect("[ \"ABC\" 'X' 3/2 ]");

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
        .expect("[ 'a×d' 'b×e' 'c×f' ]");

    step("Division (extension)");
    test(CLEAR, "[1 2  3 4 6][4 5 2 1 3] /", ENTER)
        .expect("[ 1/4 2/5 3/2 4 2 ]");
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
        .expect("[ 'a×x' 'b×x' 'c×x' ]");
    test(CLEAR, "x [a b c] *", ENTER)
        .expect("[ 'x×a' 'x×b' 'x×c' ]");

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
        .expect("[ 1 1/2 1/3 ]");

    step("Froebenius norm");
    test(CLEAR, "[1 2 3] ABS", ENTER)
        .expect("3.74165 73867 73941 3856");
    test(CLEAR, "[1 2 3] NORM", ENTER)
        .expect("3.74165 73867 73941 3856");

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
        .type(object::ID_array).expect("[ [ 1 2 3 ] [ 4 5 6 ] ]");

    step("Non-rectangular matrices");
    test(CLEAR, "[  [ 1.5  2.300 ] [ 3.02 ] ]", ENTER)
        .type(object::ID_array).expect("[ [ 1.5 2.3 ] [ 3.02 ] ]");

    step("Symbolic matrix");
    test(CLEAR, "[ [a b] [c d] ]", ENTER)
        .expect("[ [ a b ] [ c d ] ]");

    step("Non-homogneous data types");
    test(CLEAR, "[  [ \"ABC\"  'X' ] 3/2  [ 4 [5] [6 7] ] ]", ENTER)
        .type(object::ID_array)
        .expect("[ [ \"ABC\" 'X' ] 3/2 [ 4 [ 5 ] [ 6 7 ] ] ]");

    step("Addition");
    test(CLEAR, "[[1 2] [3 4]] [[5 6][7 8]] +", ENTER)
        .expect("[ [ 6 8 ] [ 10 12 ] ]");
    test(CLEAR, "[[a b][c d]] [[e f][g h]] +", ENTER)
        .expect("[ [ 'a+e' 'b+f' ] [ 'c+g' 'd+h' ] ]");

    step("Subtraction");
    test(CLEAR, "[[1 2] [3 4]] [[5 6][7 8]] -", ENTER)
        .expect("[ [ -4 -4 ] [ -4 -4 ] ]");
    test(CLEAR, "[[a b][c d]] [[e f][g h]] -", ENTER)
        .expect("[ [ 'a-e' 'b-f' ] [ 'c-g' 'd-h' ] ]");

    step("Multiplication (square)");
    test(CLEAR, "[[1 2] [3 4]] [[5 6][7 8]] *", ENTER)
        .expect("[ [ 19 22 ] [ 43 50 ] ]");
    test(CLEAR, "[[a b][c d]] [[e f][g h]] *", ENTER)
        .expect("[ [ 'a×e+b×g' 'a×f+b×h' ] [ 'c×e+d×g' 'c×f+d×h' ] ]");

    step("Multiplication (non-square)");
    test(CLEAR, "[[1 2 3] [4 5 6]] [[5 6][7 8][9 10]] *", ENTER)
        .expect("[ [ 46 52 ] [ 109 124 ] ]");
    test(CLEAR, "[[a b c d][e f g h]] [[x][y][z][t]] *", ENTER)
        .expect("[ [ 'a×x+b×y+c×z+d×t' ] [ 'e×x+f×y+g×z+h×t' ] ]");
    test(CLEAR, "[[a b c d][e f g h]] [x y z t] *", ENTER)
        .expect("[ 'a×x+b×y+c×z+d×t' 'e×x+f×y+g×z+h×t' ]");

    step("Division");
    test(CLEAR,
         "[[5 12 1968][17 2 1969][30 3 1993]] "
         "[[16 5 1995][21 5 1999][28 5 2009]] /", ENTER)
        .expect("[ [ 34/11 -52/11 -43/11 ] [ 3 357/10 -13 427/10 -16 433/10 ] [ -19/22 75/22 113/22 ] ]");
    test(CLEAR, "[[a b][c d]][[e f][g h]] /", ENTER)
        .expect("[ [ '(1÷e-f÷e×((e×0-g×1)÷(e×h-g×f)))×a+(0÷e-f÷e×((e×1-g×0)÷(e×h-g×f)))×c' '(1÷e-f÷e×((e×0-g×1)÷(e×h-g×f)))×b+(0÷e-f÷e×((e×1-g×0)÷(e×h-g×f)))×d' ] [ '(e×0-g×1)÷(e×h-g×f)×a+(e×1-g×0)÷(e×h-g×f)×c' '(e×0-g×1)÷(e×h-g×f)×b+(e×1-g×0)÷(e×h-g×f)×d' ] ]");

    step("Addition of constant (extension)");
    test(CLEAR, "[[1 2] [3 4]] 3 +", ENTER)
        .expect("[ [ 4 5 ] [ 6 7 ] ]");
    test(CLEAR, "[[a b] [c d]] x +", ENTER)
        .expect("[ [ 'a+x' 'b+x' ] [ 'c+x' 'd+x' ] ]");

    step("Subtraction of constant (extension)");
    test(CLEAR, "[[1 2] [3 4]] 3 -", ENTER)
        .expect("[ [ -2 -1 ] [ 0 1 ] ]");
    test(CLEAR, "[[a b] [c d]] x -", ENTER)
        .expect("[ [ 'a-x' 'b-x' ] [ 'c-x' 'd-x' ] ]");

    step("Multiplication by constant (extension)");
    test(CLEAR, "[[a b] [c d]] x *", ENTER)
        .expect("[ [ 'a×x' 'b×x' ] [ 'c×x' 'd×x' ] ]");
    test(CLEAR, "x [[a b] [c d]] *", ENTER)
        .expect("[ [ 'x×a' 'x×b' ] [ 'x×c' 'x×d' ] ]");

    step("Division by constant (extension)");
    test(CLEAR, "[[a b] [c d]] x /", ENTER)
        .expect("[ [ 'a÷x' 'b÷x' ] [ 'c÷x' 'd÷x' ] ]");
    test(CLEAR, "x [[a b] [c d]] /", ENTER)
        .expect("[ [ 'x÷a' 'x÷b' ] [ 'x÷c' 'x÷d' ] ]");

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
        .expect("[ [ -47/30 7/15 1/10 ] [ 17/15 1/15 -1/5 ] [ 1/10 -1/5 1/10 ] ]");
    test(CLEAR, "[[a b][c d]] INV", ENTER)
        .expect("[ [ '1÷a-b÷a×((a×0-c×1)÷(a×d-c×b))' '0÷a-b÷a×((a×1-c×0)÷(a×d-c×b))' ] [ '(a×0-c×1)÷(a×d-c×b)' '(a×1-c×0)÷(a×d-c×b)' ] ]");

    step("Invert with zero determinant");       // HP48 gets this one wrong
    test(CLEAR, "[[1 2 3][4 5 6][7 8 9]] INV", ENTER)
        .error("Divide by zero");

    step("Determinant");                        // HP48 gets this one wrong
    test(CLEAR, "[[1 2 3][4 5 6][7 8 9]] DET", ENTER)
        .expect("0");
    test(CLEAR, "[[1 2 3][4 5 6][7 8 19]] DET", ENTER)
        .expect("-30");

    step("Froebenius norm");
    test(CLEAR, "[[1 2] [3 4]] ABS", ENTER)
        .expect("5.47722 55750 51661 1346");
    test(CLEAR, "[[1 2] [3 4]] NORM", ENTER)
        .expect("5.47722 55750 51661 1346");

    step("Component-wise application of functions");
    test(CLEAR, "[[a b] [c d]] SIN", ENTER)
        .expect("[ [ 'sin a' 'sin b' ] [ 'sin c' 'sin d' ] ]");
}


void tests::auto_simplification()
// ----------------------------------------------------------------------------
//   Check auto-simplification rules for arithmetic
// ----------------------------------------------------------------------------
{
    BEGIN(simplify);

    step("Enable auto simplification");
    test(CLEAR, "AutoSimplify", ENTER).noerr();

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

    step("Applies when building a matrix");
    test(CLEAR, "[[3 0 2][2 0 -2][ 0 1 1 ]] [x y z] *", ENTER)
        .expect("[ '3×x+2×z' '2×x+-2×z' 'y+z' ]");

    step("Does not reduce matrices");
    test(CLEAR, "[a b c] 0 *", ENTER).expect("[ 0 0 0 ]");

    step("Does not apply to text");
    test(CLEAR, "\"Hello\" 0 +", ENTER)
        .expect("\"Hello0\"");

    step("Does not apply to lists");
    test(CLEAR, "{ 1 2 3 } 0 +", ENTER)
        .expect("{ 1 2 3 0 }");

    step("Disable auto simplification");
    test(CLEAR, "NoAutoSimplify", ENTER).noerr();

    step("When disabled, get the complicated expression");
    test(CLEAR, "[[3 0 2][2 0 -2][ 0 1 1 ]] [x y z] *", ENTER)
        .expect("[ '3×x+0×y+2×z' '2×x+0×y+-2×z' '0×x+1×y+1×z' ]");

    step("Re-enable auto simplification");
    test(CLEAR, "AutoSimplify", ENTER).noerr();
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
        .expect("'A×(C-sin B)'");

    step("Variable matching");
    test(CLEAR, "'A*(B+C)' 'X+X' 'X-sin X' rewrite", ENTER)
        .expect("'A×(B+C)'");
    test(CLEAR, "'A*(B+(B))' 'X+X' 'X-sin X' rewrite", ENTER)
        .expect("'A×(B-sin B)'");

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
        .expect("'(A+B)×(A+B)²'");

    step("Matching unique terms");
    test(CLEAR, "'(A+B+A)' 'X+U+X' '2*X+U' rewrite", ENTER)
        .expect("'2×A+B'");
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
        .expect("'A×C+B×C'");
    step("Single add, left");
    test(CLEAR, "'2*(A+B)' expand ", ENTER)
        .expect("'2×A+2×B'");

    step("Multiple adds");
    test(CLEAR, "'3*(A+B+C)' expand ", ENTER)
        .expect("'3×A+3×B+3×C'");

    step("Single sub, right");
    test(CLEAR, "'(A-B)*C' expand ", ENTER)
        .expect("'A×C-B×C'");
    step("Single sub, left");
    test(CLEAR, "'2*(A-B)' expand ", ENTER)
        .expect("'2×A-2×B'");

    step("Multiple subs");
    test(CLEAR, "'3*(A-B-C)' expand ", ENTER)
        .expect("'3×A-3×B-3×C'");

    step("Expand and collect a power");
    test(CLEAR, "'(A+B)^3' expand ", ENTER)
        .expect("'A×A×A+A×A×B+A×A×B+A×B×B+A×A×B+A×B×B+A×B×B+B×B×B'");
    test("collect ", ENTER)
        .expect("'2×(B↑2×A)+(A↑3+A↑2×(2×B)+B↑2×A+A↑2×B)+B↑3'");
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
}


void tests::flags_by_name()
// ----------------------------------------------------------------------------
//   Set and clear all flags by name
// ----------------------------------------------------------------------------
{
    BEGIN(flags);

#define ID(id)
#define FLAG(Enable, Disable)                           \
    step("Setting flag " #Enable)                       \
        .test(#Enable, ENTER)                           \
        .noerr();                                       \
    step("Clearing flag " #Disable " (default)")        \
        .test(#Disable, ENTER)                          \
        .noerr();
#define SETTING(Name, Low, High, Init)          \
    step("Setting " #Name " to default " #Init) \
        .noerr();
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
        .noerr();                                       \
    step("Setting " #Name " to its current value")      \
        .test("" #Name "", ENTER)                       \
        .noerr();
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
            if (SPECIAL(ty, inv,                name, "x⁻¹")    ||      \
                SPECIAL(ty, sq,                 name, "x²")     ||      \
                SPECIAL(ty, cubed,              name, "x³")     ||      \
                SPECIAL(ty, cbrt,               name, "∛")      ||      \
                SPECIAL(ty, hypot,              name, "⊿")      ||      \
                SPECIAL(ty, atan2,              name, "∠")      ||      \
                SPECIAL(ty, asin,               name, "sin⁻¹")  ||      \
                SPECIAL(ty, acos,               name, "cos⁻¹")  ||      \
                SPECIAL(ty, atan,               name, "tan⁻¹")  ||      \
                SPECIAL(ty, asinh,              name, "sinh⁻¹") ||      \
                SPECIAL(ty, acosh,              name, "cosh⁻¹") ||      \
                SPECIAL(ty, atanh,              name, "tanh⁻¹") ||      \
                SPECIAL(ty, RealToComplex,      name, "ℝ→ℂ")    ||      \
                SPECIAL(ty, ComplexToReal,      name, "ℂ→ℝ")    ||      \
                SPECIAL(ty, SumOfXSquares,      name, "ΣX²")    ||      \
                SPECIAL(ty, SumOfYSquares,      name, "ΣY²")    ||      \
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
    test(CLEAR, "pi", ENTER).expect("π");
    test(DOWN).editor("π");
    test(ENTER).expect("π");

    step("Bug 207: parsing of cos(X+pi)");
    test(CLEAR, "'COS(X+π)'", ENTER).expect("'cos(X+π)'");

    step("Bug 238: Parsing of power");
    test(CLEAR, "'X↑3'", ENTER).expect("'X↑3'");
    test(CLEAR, "'X×X↑(N-1)'", ENTER).expect("'X×X↑(N-1)'");

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
    test(CLEAR).shifts(false, false, false, false);
    test(SHIFT, RUNSTOP,
         SHIFT, ENTER, SHIFT, SHIFT, G,
         N,
         SHIFT, RUNSTOP,
         UP, BSP, DOWN, DOWN, UP,
         N,
         ENTER)
        .noerr().type(object::ID_program)
        .test(RUNSTOP)
        .noerr().type(object::ID_program).expect("« N »")
        .test(BSP)
        .noerr().type(object::ID_expression).expect("'→N'");
}


void tests::plotting()
// ----------------------------------------------------------------------------
//   Test the plotting functions
// ----------------------------------------------------------------------------
{
    BEGIN(plotting);

    step("Select radians");
    test(CLEAR, "RAD", ENTER).noerr();

    step("Function plot: Sine wave");
    test(CLEAR, "'sin(x)' FunctionPlot", ENTER).noerr()
        .wait(200).image("plot-sine");
    step("Function plot: Equation");
    test(CLEAR,
         SHIFT, ENTER, X, ENTER, ENTER, J, 3, MUL, M, 21, MUL, COS, 2, MUL, ADD,
         SHIFT, SHIFT, O, F1).noerr()
        .wait(200).image("plot-eq");
    step("Function plot: Program");
    test(CLEAR, SHIFT, RUNSTOP,
         I, SHIFT, F1, L, M, 41, MUL, J, MUL, ENTER, ENTER,
         SHIFT, SHIFT, O, F1).wait(200).image("plot-pgm").noerr();

    step("Polar plot: Program");
    test(CLEAR, SHIFT, RUNSTOP,
         61, MUL, L, SHIFT, C, 2, ADD, ENTER,
         SHIFT, SHIFT, O, F2).noerr().wait(200).image("polar-pgm");
    step("Polar plot: Equation");
    test(CLEAR, F, J, 611, MUL, SHIFT, ENTER, X, SHIFT, ENTER, SHIFT, ENTER,
         DOWN, MUL, K, 271, MUL, SHIFT, ENTER, X, LONGPRESS, SHIFT, DOWN,
         ADD, KEY2, DOT, KEY5, ENTER,
         SHIFT, SHIFT, O,
         ENTER, F2).noerr().wait(200).image("polar-eq");
    step("Polar plot: Zoom in X and Y");
    test(EXIT, "0.5 XSCALE 0.5 YSCALE", ENTER).noerr()
        .test(ENTER, F2).noerr().wait(200).image("polar-zoomxy");
    step("Polar plot: Zoom out Y");
    test(EXIT, "2 YSCALE", ENTER).noerr()
        .test(ENTER, F2).noerr().wait(200).image("polar-zoomy");
    step("Polar plot: Zoom out X");
    test(EXIT, "2 XSCALE", ENTER).noerr()
        .test(ENTER, F2).noerr().wait(200).image("polar-zoomx");

    step("Parametric plot: Program");
    test(CLEAR, SHIFT, RUNSTOP,
         "'9.5*sin(31.27*X)' eval '5.5*cos(42.42*X)' eval RealToComplex",
         ENTER, ENTER, F3)
        .noerr().wait(200).image("pplot-pgm");
    step("Parametric plot: Degrees");
    test("DEG 2 LINEWIDTH", ENTER, F3).noerr().wait(200).image("pplot-deg");
    step("Parametric plot: Equation");
    test(CLEAR,
         "3 LINEWIDTH 0.25 GRAY FOREGROUND "
         "'exp((0.17ⅈ5.27)*x+(1.5ⅈ8))' ParametricPlot", ENTER)
        .noerr().wait(200).image("pplot-eq");

    step("Bar plot");
    test(CLEAR,
         "[[ 1 -1 ][2 -2][3 -3][4 -4][5 -6][7 -8][9 -10]]", ENTER,
         33, MUL, K, 2, MUL,
         SHIFT, SHIFT, O, F5).noerr().wait(200).image("barplot");

    step("Scatter plot");
    test(CLEAR,
         "[[ -5 -5][ -3 0][ -5 5][ 0 3][ 5 5][ 3 0][ 5 -5][ 0 -3][-5 -5]]",
         ENTER,
         "4 LineWidth ScatterPlot", ENTER)
        .noerr().wait(200).image("scatterplot");

     step("Reset drawing parameters");
     test(CLEAR, "1 LineWidth 0 GRAY Foreground", ENTER).noerr();
}


void tests::plotting_all_functions()
// ----------------------------------------------------------------------------
//   Plot all real functions
// ----------------------------------------------------------------------------
{
    BEGIN(plotfns);

    step("Select radians");
    test(CLEAR, SHIFT, N, F2).noerr();

    step("Select 24-digit precision");
    test(CLEAR, SHIFT, O, 24, F6).noerr();

    step("Select plotting menu");
    test(CLEAR, SHIFT, SHIFT, O).noerr();

    uint dur = 300;

#define FUNCTION(name)                          \
    step("Plotting " #name);                    \
    test(CLEAR, "'" #name "(x)'", F1)           \
        .wait(dur).noerr()                      \
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
    test(CLEAR, SHIFT, N, F1).noerr();

    step("Reselect plotting menu");
    test(CLEAR, SHIFT, SHIFT, O).noerr();

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
        .noerr().wait(200).image("cllcd").test(ENTER);

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
        .wait(200).noerr().image("walkman").test(EXIT);

    step("Displaying text, compatibility mode");
    test(CLEAR,
         "\"Hello World\" 1 DISP "
         "\"Compatibility mode\" 2 DISP", ENTER)
        .noerr().wait(200).image("text-compat").test(ENTER);

    step("Displaying text, fractional row");
    test(CLEAR,
         "\"Gutentag\" 1.5 DrawText "
         "\"Fractional row\" 3.8 DrawText", ENTER)
        .noerr().wait(200).image("text-frac").test(ENTER);

    step("Displaying text, pixel row");
    test(CLEAR,
         "\"Bonjour tout le monde\" #5d DISP "
         "\"Pixel row mode\" #125d DISP", ENTER)
        .noerr().wait(200).image("text-pixrow").test(ENTER);

    step("Displaying text, x-y coordinates");
    test(CLEAR, "\"Hello\" { 0 0 } DISP ", ENTER)
        .noerr().wait(200).image("text-xy").test(ENTER);

    step("Displaying text, x-y pixel coordinates");
    test(CLEAR, "\"Hello\" { #20d #20d } DISP ", ENTER)
        .noerr().wait(200).image("text-pixxy").test(ENTER);

    step("Displaying text, font ID");
    test(CLEAR, "\"Hello\" { 0 0 0 } DISP \"World\" { 0 1 2 } DISP ", ENTER)
        .noerr().wait(200).image("text-font").test(ENTER);

    step("Displaying text, erase and invert");
    test(CLEAR, "\"Inverted\" { 0 0 0 true true } DISP", ENTER)
        .noerr().wait(200).image("text-invert").test(ENTER);

    step("Displaying text, background and foreground");
    test(CLEAR,
         "0.25 Gray Foreground 0.75 Gray Background "
         "\"Grayed\" { 0 0 } Disp", ENTER)
        .noerr().wait(200).image("text-gray").test(ENTER);

    step("Displaying text, restore background and foreground");
    test(CLEAR,
         "0 Gray Foreground 1 Gray Background "
         "\"Grayed\" { 0 0 } Disp", ENTER)
        .noerr().wait(200).image("text-normal").test(ENTER);

    step("Displaying text, type check");
    test(CLEAR, "\"Bad\" \"Hello\" DISP", ENTER)
        .error("Bad argument type");

    step("Lines");
    test(CLEAR, "3 50 for i ⅈ i * exp i 2 + ⅈ * exp 5 * Line next", ENTER)
        .noerr().wait(200).image("lines").test(ENTER);

    step("Line width");
    test(CLEAR,
         "1 11 for i "
         "{ #000 } #0 i 20 * + + "
         "{ #400 } #0 i 20 * + + "
         "i LineWidth Line "
         "next "
         "1 LineWidth", ENTER)
        .noerr().wait(200).image("line-width").test(ENTER);

    step("Line width, grayed");
    test(CLEAR,
         "1 11 for i "
         "{ #000 } #0 i 20 * + + "
         "{ #400 } #0 i 20 * + + "
         "i 12 / gray foreground "
         "i LineWidth Line "
         "next "
         "1 LineWidth 0 Gray Foreground", ENTER)
        .noerr().wait(200).image("line-width-gray").test(ENTER);

    step("Circles");
    test(CLEAR,
         "1 11 for i "
         "{ 0 0 } i Circle "
         "{ 0 1 } i 0.25 * Circle "
         "next ", ENTER)
        .noerr().wait(200).image("circles").test(ENTER);

    step("Circles, complex coordinates");
    test(CLEAR,
         "2 150 for i "
         "ⅈ i 0.12 * * exp 0.75 0.05 i * + * 0.4 0.003 i * +  Circle "
         "next ", ENTER)
        .noerr().wait(200).image("circles-complex").test(ENTER);

    step("Circles, fill and patterns");
    test(CLEAR,
         "0 LineWidth "
         "2 150 for i "
         "i 0.0053 * gray Foreground "
         "ⅈ i 0.12 * * exp 0.75 0.05 i * + * 0.1 0.008 i * +  Circle "
         "next ", ENTER)
        .noerr().wait(200).image("circles-fill").test(ENTER);

    step("Ellipses");
    test(CLEAR,
         "0 gray foreground 1 LineWidth "
         "2 150 for i "
         "i 0.12 * ⅈ * exp 0.05 i * 0.75 + * "
         "i 0.17 * ⅈ * exp 0.05 i * 0.75 + * "
         " Ellipse "
         "next ", ENTER)
        .noerr().wait(200).image("ellipses").test(ENTER);

    step("Ellipses, fill and patterns");
    test(CLEAR,
         "0 LineWidth "
         "2 150 for i "
         "i 0.0047 * gray Foreground "
         "0.23 ⅈ * exp 5.75 0.01 i * - * "
         "1.27 ⅈ * exp 5.45 0.01 i * - * neg "
         " Ellipse "
         "next ", ENTER)
        .noerr().wait(200).image("ellipses-fill").test(ENTER);

    step("Rectangles");
    test(CLEAR,
         "0 gray foreground 1 LineWidth "
         "2 150 for i "
         "i 0.12 * ⅈ * exp 0.05 i * 0.75 + * "
         "i 0.17 * ⅈ * exp 0.05 i * 0.75 + * "
         " Rect "
         "next ", ENTER)
        .noerr().wait(200).image("rectangles").test(ENTER);

    step("Rectangles, fill and patterns");
    test(CLEAR,
         "0 LineWidth "
         "2 150 for i "
         "i 0.0047 * gray Foreground "
         "0.23 ⅈ * exp 5.75 0.01 i * - * "
         "1.27 ⅈ * exp 5.45 0.01 i * - * neg "
         " Rect "
         "next ", ENTER)
        .noerr().wait(200).image("rectangle-fill").test(ENTER);

    step("Rounded rectangles");
    test(CLEAR,
         "0 gray foreground 1 LineWidth "
         "2 150 for i "
         "i 0.12 * ⅈ * exp 0.05 i * 0.75 + * "
         "i 0.17 * ⅈ * exp 0.05 i * 0.75 + * "
         "0.8 RRect "
         "next ", ENTER)
        .noerr().wait(200).image("rounded-rectangle").test(ENTER);

    step("Rounded rectangles, fill and patterns");
    test(CLEAR,
         "0 LineWidth "
         "2 150 for i "
         "i 0.0047 * gray Foreground "
         "0.23 ⅈ * exp 5.75 0.01 i * - * "
         "1.27 ⅈ * exp 5.45 0.01 i * - * neg "
         "0.8 RRect "
         "next ", ENTER)
        .noerr().wait(200).image("rounded-rectangle-fill").test(ENTER);

    step("Clipping");
    test(CLEAR,
         "0 LineWidth CLLCD { 120 135 353 175 } Clip "
         "2 150 for i "
         "i 0.0053 * gray Foreground "
         "ⅈ i 0.12 * * exp 0.75 0.05 i * + * 0.1 0.008 i * +  Circle "
         "next "
         "{} Clip", ENTER)
        .wait(200).noerr().image("clip-circles").test(ENTER);

    step("Cleanup");
    test(CLEAR,
         "1 LineWidth 0 Gray Foreground 1 Gray Background "
         "{ -1 -1 } { 3 2 } rect",
         ENTER).noerr().wait(200).image("cleanup");
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

tests &tests::begin(cstring name)
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
    fprintf(stderr, "%3u: %s\n", tindex, tname);
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
    fprintf(stderr, "%3u:  %03u: %-60s", tindex, sindex, sname);
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
    case ALPHA: return shifts(false, false, true, false);

    case LOWERCASE: return shifts(false, false, true, true);

    case LONGPRESS:
        longpress = true; // Next key will be a long press
        return *this;

    case CLEAR: return clear();

    case NOKEYS: return nokeys();

    case REFRESH: return refreshed();

    default: break;
    }


    // Wait for the RPL thread to process the keys (to be revisited on DM42)
    while (!key_empty())
        sys_delay(delay_time);

    record(tests,
           "Push key %d update %u->%u last %d",
           k, lcd_update, lcd_needsupdate, last_key);
    lcd_update = lcd_needsupdate;
    Stack.catch_up();
    last_key = k;

    key_push(k);
    if (longpress)
    {
        sys_delay(600);
        longpress = false;
        release   = false;
    }
    sys_delay(delay_time);

    if (release && k != RELEASE)
    {
        while (!key_remaining())
            sys_delay(delay_time);
        record(tests,
               "Release key %d update %u->%u last %d",
               k, lcd_update, lcd_needsupdate, last_key);
        lcd_update = lcd_needsupdate;
        Stack.catch_up();
        last_key = -k;
        key_push(RELEASE);

        // Wait for the RPL thread to process the keys
        keysync_sent++;
        record(tests, "Key sync sent %u done %u", keysync_sent, keysync_done);
        key_push(KEYSYNC);
        while (keysync_done != keysync_sent)
            sys_delay(delay_time);
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
    itest(cstring(buffer));
    return shifts(false, false, false, false);
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
    itest(cstring(buffer));
    return shifts(false, false, false, false);
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
    itest(cstring(buffer));
    return shifts(false, false, false, false);
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
        case ':': k = KEY0;         alpha = true;  del   = true; break;
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


tests &tests::shifts(bool shift, bool xshift, bool alpha, bool lowercase)
// ----------------------------------------------------------------------------
//   Reach the desired shift state from the current state
// ----------------------------------------------------------------------------
{
    // Must wait for the calculator to process our keys for valid state
    nokeys();

    // Check that we have no error here
    data_entry_noerr();

    // Check invalid input: can only have one shift
    if (shift && xshift)
        shift = false;

    // If not alpha, disable lowercase
    if (!alpha)
        lowercase = false;

    // First change lowercase state as necessary, since this messes up shift
    while (lowercase != ui.lowercase || alpha != ui.alpha)
    {
        data_entry_noerr();
        while (!ui.shift)
            itest(SHIFT, NOKEYS);
        itest(ENTER, NOKEYS);
    }

    while (xshift != ui.xshift)
        itest(SHIFT, NOKEYS);

    while (shift != ui.shift)
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

tests &tests::clear()
// ----------------------------------------------------------------------------
//   Make sure we are in a clean state
// ----------------------------------------------------------------------------
{
    nokeys();
    key_push(CLEAR);
    while (!key_empty())
        sys_delay(delay_time);
    return *this;
}


tests &tests::ready()
// ----------------------------------------------------------------------------
//   Check if the calculator is ready and we can look at it
// ----------------------------------------------------------------------------
{
    nokeys();
    refreshed();
    return *this;
}


tests &tests::nokeys()
// ----------------------------------------------------------------------------
//   Check until the key buffer is empty, indicates that calculator is done
// ----------------------------------------------------------------------------
{
    while (!key_empty())
        sys_delay(delay_time);
    return *this;
}


tests &tests::data_entry_noerr()
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


tests &tests::refreshed()
// ----------------------------------------------------------------------------
//    Wait until the screen was updated by the calculator
// ----------------------------------------------------------------------------
{
    // Wait for a screen redraw
    record(tests, "Waiting for screen update");
    while (lcd_needsupdate == lcd_update)
        sys_delay(delay_time);

    // Wait for a stack update
    uint32_t start = sys_current_ms();
    record(tests, "Waiting for key %d in stack at %u", last_key, start);
    while (sys_current_ms() - start < wait_time)
    {
        if (!Stack.available())
        {
            sys_delay(delay_time);
        }
        else if (Stack.available() > 1)
        {
            record(tests, "Consume extra stack");
            Stack.consume();
        }
        else if (Stack.key() == last_key)
        {
            record(tests, "Consume extra stack");
            break;
        }
        else
        {
            record(tests, "Wrong key %d", Stack.key());
            Stack.consume();
        }
    }

    record(tests,
           "Refreshed, key %d, needs=%u update=%u available=%u",
           Stack.key(),
           lcd_needsupdate,
           lcd_update,
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


tests &tests::want(cstring ref)
// ----------------------------------------------------------------------------
//   We want something that looks like this (ignore spacing)
// ----------------------------------------------------------------------------
{
    record(tests, "Expect [%+s] ignoring spacing", ref);
    ready();
    cindex++;
    if (rt.error())
    {
        explain("Expected output [",
                ref,
                "], got error [",
                rt.error(),
                "] instead");
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
    record(tests, "No output");
    explain("Expected output [", ref, "] but got no stack change");
    return fail();
}


tests &tests::expect(cstring output)
// ----------------------------------------------------------------------------
//   Check that the output at first level of stack matches the string
// ----------------------------------------------------------------------------
{
    record(tests, "Expecting [%+s]", output);
    ready();
    cindex++;
    if (rt.error())
    {
        explain("Expected output [",
                output,
                "], got error [",
                rt.error(),
                "] instead");
        return fail();
    }
    if (utf8 out = Stack.recorded())
    {
        record(tests,
               "Comparing [%s] to [%+s] %+s",
               out,
               output,
               strcmp(output, cstring(out)) == 0 ? "OK" : "FAIL");
        if (strcmp(output, cstring(out)) == 0)
            return *this;
        explain("Expected output [",
                output,
                "], "
                "got [",
                cstring(out),
                "] instead");
        return fail();
    }
    record(tests, "No output");
    explain("Expected output [", output, "] but got no stack change");
    return fail();
}


tests &tests::expect(int output)
// ----------------------------------------------------------------------------
//   Check that the output matches an integer value
// ----------------------------------------------------------------------------
{
    char num[32];
    snprintf(num, sizeof(num), "%d", output);
    return expect(num);
}


tests &tests::expect(unsigned int output)
// ----------------------------------------------------------------------------
//   Check that the output matches an integer value
// ----------------------------------------------------------------------------
{
    char num[32];
    snprintf(num, sizeof(num), "%u", output);
    return expect(num);
}


tests &tests::expect(long output)
// ----------------------------------------------------------------------------
//   Check that the output matches an integer value
// ----------------------------------------------------------------------------
{
    char num[32];
    snprintf(num, sizeof(num), "%ld", output);
    return expect(num);
}


tests &tests::expect(unsigned long output)
// ----------------------------------------------------------------------------
//   Check that the output matches an integer value
// ----------------------------------------------------------------------------
{
    char num[32];
    snprintf(num, sizeof(num), "%lu", output);
    return expect(num);
}


tests &tests::expect(long long output)
// ----------------------------------------------------------------------------
//   Check that the output matches an integer value
// ----------------------------------------------------------------------------
{
    char num[32];
    snprintf(num, sizeof(num), "%lld", output);
    return expect(num);
}


tests &tests::expect(unsigned long long output)
// ----------------------------------------------------------------------------
//   Check that the output matches an integer value
// ----------------------------------------------------------------------------
{
    char num[32];
    snprintf(num, sizeof(num), "%llu", output);
    return expect(num);
}


tests &tests::match(cstring restr)
// ----------------------------------------------------------------------------
//   Check that the output at first level of stack matches the string
// ----------------------------------------------------------------------------
{
    ready();
    cindex++;
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
        explain("Expected output matching [",
                restr,
                "], "
                "got [",
                out,
                "]");
        return fail();
    }
    explain("Expected output matching [", restr, "] but stack not updated");
    return fail();
}


tests &tests::image(cstring file)
// ----------------------------------------------------------------------------
//   Check that the output in the screen matches what is in the file
// ----------------------------------------------------------------------------
{
    ready();
    cindex++;
    if (!image_match(file))
    {
        explain("Expected screen to match [", file, "]");
        image_match(file, true);
        return fail();
    }
    return *this;
}


tests &tests::type(object::id ty)
// ----------------------------------------------------------------------------
//   Check that the top of stack matches the type
// ----------------------------------------------------------------------------
{
    ready();
    cindex++;
    if (utf8 out = Stack.recorded())
    {
        object::id tty = Stack.type();
        if (tty == ty)
            return *this;
        explain("Expected type ",
                object::name(ty),
                " (",
                int(ty),
                ")"
                " but got ",
                object::name(tty),
                " (",
                int(tty),
                ")");
        return fail();
    }
    explain("Expected type ",
            object::name(ty),
            " (",
            int(ty),
            ")"
            " but stack not updated");
    return fail();
}


tests &tests::shift(bool s)
// ----------------------------------------------------------------------------
//   Check that the shift state matches expectations
// ----------------------------------------------------------------------------
{
    nokeys();
    return check(ui.shift == s, "Expected shift ", s, ", got ", ui.shift);
}


tests &tests::xshift(bool x)
// ----------------------------------------------------------------------------
//   Check that the right shift state matches expectations
// ----------------------------------------------------------------------------
{
    nokeys();
    return check(ui.xshift == x, "Expected xshift ", x, " got ", ui.xshift);
}


tests &tests::alpha(bool a)
// ----------------------------------------------------------------------------
//   Check that the alpha state matches expectations
// ----------------------------------------------------------------------------
{
    nokeys();
    return check(ui.alpha == a, "Expected alpha ", a, " got ", ui.alpha);
}


tests &tests::lower(bool l)
// ----------------------------------------------------------------------------
//   Check that the lowercase state matches expectations
// ----------------------------------------------------------------------------
{
    nokeys();
    return check(ui.lowercase == l, "Expected alpha ", l, " got ", ui.alpha);
}


tests &tests::editing()
// ----------------------------------------------------------------------------
//   Check that we are editing, without checking the length
// ----------------------------------------------------------------------------
{
    ready();
    return check(rt.editing(),
                 "Expected to be editing, got length ",
                 rt.editing());
}


tests &tests::editing(size_t length)
// ----------------------------------------------------------------------------
//   Check that the editor has exactly the expected length
// ----------------------------------------------------------------------------
{
    ready();
    return check(rt.editing() == length,
                 "Expected editing length to be ",
                 length,
                 " got ",
                 rt.editing());
}


tests &tests::editor(cstring text)
// ----------------------------------------------------------------------------
//   Check that the editor contents matches the text
// ----------------------------------------------------------------------------
{
    ready();
    byte_p ed = rt.editor();
    size_t sz = rt.editing();

    if (!ed)
        return explain("Expected editor to contain [",
                       text,
                       "], "
                       "but it's empty")
            .fail();
    if (sz != strlen(text))
        return explain("Expected ",
                       strlen(text),
                       " characters in editor"
                       " [",
                       text,
                       "], "
                       "but got ",
                       sz,
                       " characters "
                       " [",
                       std::string(cstring(ed), sz),
                       "]")
            .fail();
    if (memcmp(ed, text, sz))
        return explain("Expected editor to contain [",
                       text,
                       "], "
                       "but it contains [",
                       std::string(cstring(ed), sz),
                       "]")
            .fail();

    return *this;
}


tests &tests::cursor(size_t csr)
// ----------------------------------------------------------------------------
//   Check that the cursor is at expected position
// ----------------------------------------------------------------------------
{
    ready();
    return check(ui.cursor == csr,
                 "Expected cursor to be at position ",
                 csr,
                 " but it's at position ",
                 ui.cursor);
}


tests &tests::error(cstring msg)
// ----------------------------------------------------------------------------
//   Check that the error message matches expectations
// ----------------------------------------------------------------------------
{
    ready();
    utf8 err = rt.error();

    if (!msg && err)
        return explain("Expected no error, got [", err, "]")
            .itest(CLEAR).fail();
    if (msg && !err)
        return explain("Expected error message [", msg, "], got none").fail();
    if (msg && err && strcmp(cstring(err), msg) != 0)
        return explain("Expected error message [", msg, "], "
                       "got [", err, "]")
            .fail();
    return *this;
}


tests &tests::command(cstring ref)
// ----------------------------------------------------------------------------
//   Check that the command result matches expectations
// ----------------------------------------------------------------------------
{
    ready();
    text_p cmdo = rt.command();
    size_t sz = 0;
    utf8 cmd = cmdo->value(&sz);

    if (!ref && cmd)
        return explain("Expected no command, got [", cmd, "]").fail();
    if (ref && !cmd)
        return explain("Expected command [", ref, "], got none").fail();
    if (ref && cmd && strcmp(ref, cstring(cmd)) != 0)
        return explain("Expected command [", ref, "], got [", cmd, "]").fail();

    return *this;
}


tests &tests::source(cstring ref)
// ----------------------------------------------------------------------------
//   Check that the source indicated in the editor matches expectations
// ----------------------------------------------------------------------------
{
    ready();
    utf8 src = rt.source();

    if (!ref && src)
        return explain("Expected no source, got [", src, "]").fail();
    if (ref && !src)
        return explain("Expected source [", ref, "], got none").fail();
    if (ref && src && strcmp(ref, cstring(src)) != 0)
        return explain("Expected source [", ref, "], " "got [", src, "]")
            .fail();

    return *this;
}
