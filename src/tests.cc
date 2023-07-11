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


extern volatile int lcd_needsupdate;

RECORDER_DECLARE(errors);

uint wait_time  = 10;
uint delay_time = 2;

void tests::run(bool onlyCurrent)
// ----------------------------------------------------------------------------
//   Run all test categories
// ----------------------------------------------------------------------------
{
    tindex = sindex = cindex = count = 0;
    failures.clear();

    auto tracing           = RECORDER_TRACE(errors);
    RECORDER_TRACE(errors) = false;

    Settings               = settings(); // Reset to default settings

    current();
    if (!onlyCurrent)
    {
        reset_settings();
        shift_logic();
        keyboard_entry();
        data_types();
        arithmetic();
        global_variables();
        local_variables();
        for_loops();
    }
    summary();

    RECORDER_TRACE(errors) = tracing;
}


void tests::current()
// ----------------------------------------------------------------------------
//   Test the current thing (this is a temporary test)
// ----------------------------------------------------------------------------
{
    step("Current test");
    logical_operations();
}


void tests::reset_settings()
// ----------------------------------------------------------------------------
//   Use settings that make the results predictable on screen
// ----------------------------------------------------------------------------
{
    begin("Reset settings");
    step("Numerical settings").test("StandardDisplay", ENTER).noerr();
    step("Switching to degrees").test("Degrees", ENTER).noerr();
    step("Using long form for commands").test("LongForm", ENTER).noerr();
    step("Using dot as fractional mark").test("DecimalDot", ENTER).noerr();
    step("Setting trailing decimal").test("TrailingDecimal", ENTER).noerr();
    step("Using default 34-digit precision")
        .test("34 Precision", ENTER)
        .noerr();
    step("Using 1E10, not fancy unicode exponent")
        .test("ClassicExponent", ENTER)
        .noerr();
    step("Using 64-bit word size").test("64 StoreWordSize", ENTER).noerr();
    step("Disable spacing")
        .test("0 NumberSpacing", ENTER)
        .noerr()
        .test("0 MantissaSpacing", ENTER)
        .noerr()
        .test("0 FractionSpacing", ENTER)
        .noerr()
        .test("0 BasedSpacing", ENTER)
        .noerr();
}


void tests::shift_logic()
// ----------------------------------------------------------------------------
//   Test all keys and check we have the correct output
// ----------------------------------------------------------------------------
{
    begin("Shift logic");
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
    begin("Keyboard logic");

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
    begin("Data types");

    step("Positive integer");
    test(CLEAR, "1", ENTER).type(object::ID_integer).expect("1");
    step("Negative integer");
    test(CLEAR, "1", CHS, ENTER).type(object::ID_neg_integer).expect("-1");

    step("Binary based integer");
    test(CLEAR, "#10010101b", ENTER)
        .type(object::ID_bin_integer)
        .expect("#10010101b");
    test(CLEAR, "#101B", ENTER).type(object::ID_bin_integer).expect("#101b");

    step("Decimal based integer");
    test(CLEAR, "#12345d", ENTER)
        .type(object::ID_dec_integer)
        .expect("#12345d");
    test(CLEAR, "#123D", ENTER).type(object::ID_dec_integer).expect("#123d");

    step("Octal based integer");
    test(CLEAR, "#12345o", ENTER)
        .type(object::ID_oct_integer)
        .expect("#12345o");
    test(CLEAR, "#123O", ENTER).type(object::ID_oct_integer).expect("#123o");

    step("Hexadecimal based integer");
    test(CLEAR, "#1234ABCDH", ENTER)
        .type(object::ID_hex_integer)
        .expect("#1234ABCDh");
    test(CLEAR, "#DEADBEEFH", ENTER)
        .type(object::ID_hex_integer)
        .expect("#DEADBEEFh");

    step("Arbitrary base input");
    test(CLEAR, "8#777", ENTER).type(object::ID_based_integer).expect("#1FF");
    test(CLEAR, "2#10000#ABCDE", ENTER)
        .type(object::ID_based_integer)
        .expect("#ABCDE");

    step("Symbols");
    cstring symbol = "ABC123Z";
    test(CLEAR, symbol, ENTER).type(object::ID_equation).expect("'ABC123Z'");

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
    test(CLEAR, XEQ, X, ENTER, KEY1, ADD).type(object::ID_equation).expect(eqn);
    cstring eqn2 = "'sin(X+1)'";
    test(SIN).type(object::ID_equation).expect(eqn2);
    test(DOWN, ENTER).type(object::ID_equation).expect(eqn2);
    step("Equation parsing and simplification");
    test(CLEAR, "'(((A))+(B))-(C+D)'", ENTER)
        .type(object::ID_equation)
        .expect("'A+B-(C+D)'");
    step("equation fancy rendering");
    test(CLEAR,
         XEQ,
         X,
         ENTER,
         INV,
         XEQ,
         Y,
         ENTER,
         SHIFT,
         SQRT,
         XEQ,
         Z,
         ENTER,
         "CUBED",
         ENTER,
         ADD,
         ADD)
        .type(object::ID_equation)
        .expect("'X⁻¹+(Y²+Z³)'");
    step("Equation fancy parsing from editor");
    test(DOWN, "   ", ENTER).type(object::ID_equation).expect("'X⁻¹+(Y²+Z³)'");

    step("Fractions");
    test(CLEAR, "1/3", ENTER).type(object::ID_fraction).expect("1/3");
    test(CLEAR, "20/60", ENTER).type(object::ID_fraction).expect("1/3");
    test(CLEAR, "-80/60", ENTER).type(object::ID_neg_fraction).expect("-4/3");

    step("Large integers");
    cstring b = "123456789012345678901234567890123456789012345678901234567890";
    cstring mb =
        "-123456789012345678901234567890123456789012345678901234567890";
    test(CLEAR, b, ENTER).type(object::ID_bignum).expect(b);
    test(DOWN, ENTER).type(object::ID_bignum).expect(b);
    test(CHS).type(object::ID_neg_bignum).expect(mb);
    test(DOWN, ENTER).type(object::ID_neg_bignum).expect(mb);

    step("Large fractions");
    cstring bf =
        "123456789012345678901234567890123456789012345678901234567890/"
        "123456789012345678901234567890123456789012345678901234567891";
    cstring mbf =
        "-123456789012345678901234567890123456789012345678901234567890/"
        "123456789012345678901234567890123456789012345678901234567891";
    test(CLEAR, bf, ENTER).type(object::ID_big_fraction).expect(bf);
    test(DOWN, ENTER).type(object::ID_big_fraction).expect(bf);
    test(CHS).type(object::ID_neg_big_fraction).expect(mbf);
    test(DOWN, ENTER).type(object::ID_neg_big_fraction).expect(mbf);

    clear();
}


void tests::arithmetic()
// ----------------------------------------------------------------------------
//   Tests for basic arithmetic operations
// ----------------------------------------------------------------------------
{
    begin("Arithmetic");

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
        .expect("9223372036854775807");
    test(CLEAR, (1ULL << 63) - 3ULL, CHS, ENTER, -2, ADD)
        .type(object::ID_neg_integer)
        .expect("-9223372036854775807");

    test(CLEAR, ~0ULL, ENTER, 1, ADD)
        .type(object::ID_bignum)
        .expect("18446744073709551616");
    test(CLEAR, ~0ULL, CHS, ENTER, -2, ADD)
        .type(object::ID_neg_bignum)
        .expect("-18446744073709551617");

    step("Adding ten small integers at random");
    srand48(sys_current_ms());
    for (int i = 0; i < 10; i++)
    {
        large x = (lrand48() & 0xFFFFFF) - 0x800000;
        large y = (lrand48() & 0xFFFFFF) - 0x800000;
        test(CLEAR, x, ENTER, y, ADD)
            .explain("Computing ", x, " + ", y, ", ")
            .expect(x + y);
    }

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
        .expect("-18446744073709551616");
    test(CLEAR, -3, ENTER, 0xFFFFFFFFFFFFFFFFull, SUB)
        .type(object::ID_neg_bignum)
        .expect("-18446744073709551618");

    step("Subtracting ten small integers at random");
    for (int i = 0; i < 10; i++)
    {
        large x = (lrand48() & 0xFFFFFF) - 0x800000;
        large y = (lrand48() & 0xFFFFFF) - 0x800000;
        test(CLEAR, x, ENTER, y, SUB)
            .explain("Computing ", x, " - ", y, ", ")
            .expect(x - y);
    }

    step("Integer multiplication");
    test(CLEAR, 1, ENTER, 1, MUL).type(object::ID_integer).expect("1");
    test(3, MUL).type(object::ID_integer).expect("3");
    test(-3, MUL).type(object::ID_neg_integer).expect("-9");
    test(2, MUL).type(object::ID_neg_integer).expect("-18");
    test(-7, MUL).type(object::ID_integer).expect("126");

    step("Multiplying ten small integers at random");
    for (int i = 0; i < 10; i++)
    {
        large x = (lrand48() & 0xFFFFFF) - 0x800000;
        large y = (lrand48() & 0xFFFFFF) - 0x800000;
        test(CLEAR, x, ENTER, y, MUL)
            .explain("Computing ", x, " * ", y, ", ")
            .expect(x * y);
    }

    step("Integer division");
    test(CLEAR, 210, ENTER, 2, DIV).type(object::ID_integer).expect("105");
    test(5, DIV).type(object::ID_integer).expect("21");
    test(-3, DIV).type(object::ID_neg_integer).expect("-7");
    test(-7, DIV).type(object::ID_integer).expect("1");

    step("Dividing ten small integers at random");
    for (int i = 0; i < 10; i++)
    {
        large x = (lrand48() & 0x3FFF) - 0x4000;
        large y = (lrand48() & 0x3FFF) - 0x4000;
        test(CLEAR, x * y, ENTER, y, DIV)
            .explain("Computing ", x * y, " / ", y, ", ")
            .expect(x);
    }

    step("Division with fractional output");
    test(CLEAR, 1, ENTER, 3, DIV).expect("1/3");
    test(CLEAR, 2, ENTER, 5, DIV).expect("2/5");

    step("Manual computation of 100!");
    test(CLEAR, 1, ENTER);
    for (uint i = 1; i <= 100; i++)
        test(i, MUL);
    wait(30); // Takes its sweet time to display (GC?)
    expect(
        "9332621544394415268169923885626670049071596826438162146859296389521"
        "7599993229915608941463976156518286253697920827223758251185210916864"
        "000000000000000000000000");
    step("Manual division by all factors of 100!");
    for (uint i = 1; i <= 100; i++)
        test(i * 997 % 101, DIV);
    expect(1);

    step("Manual computation of 997/100!");
    test(CLEAR, 997, ENTER);
    for (uint i = 1; i <= 100; i++)
        test(i * 997 % 101, DIV);
    expect(
        "997/"
        "9332621544394415268169923885626670049071596826438162146859296389521"
        "7599993229915608941463976156518286253697920827223758251185210916864"
        "000000000000000000000000");
}


void tests::global_variables()
// ----------------------------------------------------------------------------
//   Tests for access to global variables
// ----------------------------------------------------------------------------
{
    begin("Global variables");

    step("Store in global variable");
    test(CLEAR, 12345, ENTER).expect(12345);
    test(XEQ, "A", ENTER).expect("'A'");
    test(STO).noerr();
    step("Recall global variable");
    test(CLEAR, XEQ, "A", ENTER).expect("'A'");
    test("RCL", ENTER).noerr().expect(12345);

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
    step("Recall invalid variable object");
    test(1234, ENTER, "RCL", ENTER).error("Invalid name").clear();

    step("Store program in global variable");
    test(CLEAR, "« 1 + »", ENTER, XEQ, "INCR", ENTER, STO).noerr();
    step("Evaluate global variable");
    test(CLEAR, "A INCR", ENTER).expect(12346);

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
}


void tests::local_variables()
// ----------------------------------------------------------------------------
//   Tests for access to local variables
// ----------------------------------------------------------------------------
{
    begin("Local variables");

    step("Creating a local block");
    cstring source = "« → A B C « A B + A B - × B C + B C - × ÷ » »";
    test(CLEAR, source, ENTER).type(object::ID_program).expect(source);
    test(XEQ, "LocTest", ENTER, STO).noerr();

    step("Calling a local block with numerical values");
    test(CLEAR, 1, ENTER, 2, ENTER, 3, ENTER, "LocTest", ENTER).expect("3/5");

    step("Calling a local block with symbolic values");
    test(CLEAR,
         XEQ,
         "X",
         ENTER,
         XEQ,
         "Y",
         ENTER,
         XEQ,
         "Z",
         ENTER,
         "LocTest",
         ENTER)
        .expect("'(X+Y)×(X-Y)÷((Y+Z)×(Y-Z))'");

    step("Cleanup");
    test(CLEAR, XEQ, "LocTest", ENTER, "PurgeAll", ENTER).noerr();
}


void tests::for_loops()
// ----------------------------------------------------------------------------
//   Test simple for loops
// ----------------------------------------------------------------------------
{
    begin("For loops");

    step("Simple 1..10");
    cstring pgm  = "« 0 1 10 FOR i i SQ + NEXT »";
    cstring pgmo = "« 0 1 10 for i i x² + next »";
    test(CLEAR, pgm, ENTER).noerr().type(object::ID_program).expect(pgmo);
    test(RUNSTOP).noerr().type(object::ID_integer).expect(385);

    step("Algebraic 1..10");
    pgm  = "« 'X' 1 5 FOR i i SQ + NEXT »";
    pgmo = "« 'X' 1 5 for i i x² + next »";
    test(CLEAR, pgm, ENTER).noerr().type(object::ID_program).expect(pgmo);
    test(RUNSTOP).noerr().type(object::ID_equation).expect("'X+1+4+9+16+25'");

    step("Stepping by 2");
    pgm  = "« 0 1 10 FOR i i SQ + 2 STEP »";
    pgmo = "« 0 1 10 for i i x² + 2 step »";
    test(CLEAR, pgm, ENTER).noerr().type(object::ID_program).expect(pgmo);
    test(RUNSTOP).noerr().type(object::ID_integer).expect(165);

    step("Stepping by i");
    pgm  = "« 'X' 1 100 FOR i i SQ + i step »";
    pgmo = "« 'X' 1 100 for i i x² + i step »";
    test(CLEAR, pgm, ENTER).noerr().type(object::ID_program).expect(pgmo);
    test(RUNSTOP)
        .noerr()
        .type(object::ID_equation)
        .expect("'X+1+4+16+64+256+1024+4096'");

    step("Negative stepping");
    pgm  = "« 0 10 1 FOR i i SQ + -1 STEP »";
    pgmo = "« 0 10 1 for i i x² + -1 step »";
    test(CLEAR, pgm, ENTER).noerr().type(object::ID_program).expect(pgmo);
    test(RUNSTOP).noerr().type(object::ID_integer).expect(385);

    step("Negative stepping algebraic");
    pgm  = "« 'X' 10 1 FOR i i SQ + -1 step »";
    pgmo = "« 'X' 10 1 for i i x² + -1 step »";
    test(CLEAR, pgm, ENTER).noerr().type(object::ID_program).expect(pgmo);
    test(RUNSTOP)
        .noerr()
        .type(object::ID_equation)
        .expect("'X+100+81+64+49+36+25+16+9+4+1'");

    step("Fractional");
    pgm  = "« 'X' 0.1 0.9 FOR i i SQ + 0.1 step »";
    pgmo = "« 'X' 0.1 0.9 for i i x² + 0.1 step »";
    test(CLEAR, pgm, ENTER).noerr().type(object::ID_program).expect(pgmo);
    test(RUNSTOP)
        .noerr()
        .type(object::ID_equation)
        .expect("'X+0.01+0.04+0.09+0.16+0.25+0.36+0.49+0.64+0.81'");

    step("Fractional down");
    pgm  = "« 'X' 0.9 0.1 FOR i i SQ + -0.1 step »";
    pgmo = "« 'X' 0.9 0.1 for i i x² + -0.1 step »";
    test(CLEAR, pgm, ENTER).noerr().type(object::ID_program).expect(pgmo);
    test(RUNSTOP)
        .noerr()
        .type(object::ID_equation)
        .expect("'X+0.81+0.64+0.49+0.36+0.25+0.16+0.09+0.04+0.01'");

    step("Execute at least once");
    pgm  = "« 'X' 10 1 FOR i i SQ + NEXT »";
    pgmo = "« 'X' 10 1 for i i x² + next »";
    test(CLEAR, pgm, ENTER).noerr().type(object::ID_program).expect(pgmo);
    test(RUNSTOP).noerr().type(object::ID_equation).expect("'X+100'");
}


void tests::logical_operations()
// ----------------------------------------------------------------------------
//   Perform logical operations on small and big integers
// ----------------------------------------------------------------------------
{
    begin("Logical operations");

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
    lcd_update = lcd_needsupdate;
    record(tests, "Step %+s, catching up", name);
    Stack.catch_up();
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

    // Catch up with stack output
    Stack.catch_up();
    lcd_update = lcd_needsupdate;

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
    while (!key_remaining())
        sys_delay(delay_time);

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
        key_push(RELEASE);
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
        bool del    = false;
        bool bsp    = false;

        switch (c)
        {
        case 'A':
            k     = A;
            alpha = true;
            lower = false;
            break;
        case 'B':
            k     = B;
            alpha = true;
            lower = false;
            break;
        case 'C':
            k     = C;
            alpha = true;
            lower = false;
            break;
        case 'D':
            k     = D;
            alpha = true;
            lower = false;
            break;
        case 'E':
            k     = E;
            alpha = true;
            lower = false;
            break;
        case 'F':
            k     = F;
            alpha = true;
            lower = false;
            break;
        case 'G':
            k     = G;
            alpha = true;
            lower = false;
            break;
        case 'H':
            k     = H;
            alpha = true;
            lower = false;
            break;
        case 'I':
            k     = I;
            alpha = true;
            lower = false;
            break;
        case 'J':
            k     = J;
            alpha = true;
            lower = false;
            break;
        case 'K':
            k     = K;
            alpha = true;
            lower = false;
            break;
        case 'L':
            k     = L;
            alpha = true;
            lower = false;
            break;
        case 'M':
            k     = M;
            alpha = true;
            lower = false;
            break;
        case 'N':
            k     = N;
            alpha = true;
            lower = false;
            break;
        case 'O':
            k     = O;
            alpha = true;
            lower = false;
            break;
        case 'P':
            k     = P;
            alpha = true;
            lower = false;
            break;
        case 'Q':
            k     = Q;
            alpha = true;
            lower = false;
            break;
        case 'R':
            k     = R;
            alpha = true;
            lower = false;
            break;
        case 'S':
            k     = S;
            alpha = true;
            lower = false;
            break;
        case 'T':
            k     = T;
            alpha = true;
            lower = false;
            break;
        case 'U':
            k     = U;
            alpha = true;
            lower = false;
            break;
        case 'V':
            k     = V;
            alpha = true;
            lower = false;
            break;
        case 'W':
            k     = W;
            alpha = true;
            lower = false;
            break;
        case 'X':
            k     = X;
            alpha = true;
            lower = false;
            break;
        case 'Y':
            k     = Y;
            alpha = true;
            lower = false;
            break;
        case 'Z':
            k     = Z;
            alpha = true;
            lower = false;
            break;

        case 'a':
            k     = A;
            alpha = true;
            lower = true;
            break;
        case 'b':
            k     = B;
            alpha = true;
            lower = true;
            break;
        case 'c':
            k     = C;
            alpha = true;
            lower = true;
            break;
        case 'd':
            k     = D;
            alpha = true;
            lower = true;
            break;
        case 'e':
            k     = E;
            alpha = true;
            lower = true;
            break;
        case 'f':
            k     = F;
            alpha = true;
            lower = true;
            break;
        case 'g':
            k     = G;
            alpha = true;
            lower = true;
            break;
        case 'h':
            k     = H;
            alpha = true;
            lower = true;
            break;
        case 'i':
            k     = I;
            alpha = true;
            lower = true;
            break;
        case 'j':
            k     = J;
            alpha = true;
            lower = true;
            break;
        case 'k':
            k     = K;
            alpha = true;
            lower = true;
            break;
        case 'l':
            k     = L;
            alpha = true;
            lower = true;
            break;
        case 'm':
            k     = M;
            alpha = true;
            lower = true;
            break;
        case 'n':
            k     = N;
            alpha = true;
            lower = true;
            break;
        case 'o':
            k     = O;
            alpha = true;
            lower = true;
            break;
        case 'p':
            k     = P;
            alpha = true;
            lower = true;
            break;
        case 'q':
            k     = Q;
            alpha = true;
            lower = true;
            break;
        case 'r':
            k     = R;
            alpha = true;
            lower = true;
            break;
        case 's':
            k     = S;
            alpha = true;
            lower = true;
            break;
        case 't':
            k     = T;
            alpha = true;
            lower = true;
            break;
        case 'u':
            k     = U;
            alpha = true;
            lower = true;
            break;
        case 'v':
            k     = V;
            alpha = true;
            lower = true;
            break;
        case 'w':
            k     = W;
            alpha = true;
            lower = true;
            break;
        case 'x':
            k     = X;
            alpha = true;
            lower = true;
            break;
        case 'y':
            k     = Y;
            alpha = true;
            lower = true;
            break;
        case 'z':
            k     = Z;
            alpha = true;
            lower = true;
            break;

        case '0':
            k     = KEY0;
            shift = alpha;
            break;
        case '1':
            k     = KEY1;
            shift = alpha;
            break;
        case '2':
            k     = KEY2;
            shift = alpha;
            break;
        case '3':
            k     = KEY3;
            shift = alpha;
            break;
        case '4':
            k     = KEY4;
            shift = alpha;
            break;
        case '5':
            k     = KEY5;
            shift = alpha;
            break;
        case '6':
            k     = KEY6;
            shift = alpha;
            break;
        case '7':
            k     = KEY7;
            shift = alpha;
            break;
        case '8':
            k     = KEY8;
            shift = alpha;
            break;
        case '9':
            k     = KEY9;
            shift = alpha;
            break;
        case '+':
            k     = ADD;
            alpha = true;
            shift = true;
            break;
        case '-':
            k     = SUB;
            alpha = true;
            shift = true;
            break;
        case '*':
            k      = MUL;
            alpha  = true;
            xshift = true;
            break;
        case '/':
            k      = DIV;
            alpha  = true;
            xshift = true;
            break;
        case '.':
            k     = DOT;
            shift = alpha;
            break;
        case ',':
            k     = DOT;
            shift = !alpha;
            break;
        case ' ':
            k     = RUNSTOP;
            alpha = true;
            break;
        case '?':
            k      = KEY7;
            alpha  = true;
            xshift = true;
            break;
        case '!':
            k      = ADD;
            alpha  = true;
            xshift = true;
            break;
        case '_':
            k     = SUB;
            alpha = true;
            break;
        case '%':
            k     = RCL;
            alpha = true;
            shift = true;
            break;
        case ':':
            k     = KEY0;
            alpha = true;
            del   = true;
            break;
        case ';':
            k      = KEY0;
            alpha  = true;
            xshift = true;
            break;
        case '<':
            k     = SIN;
            alpha = true;
            shift = true;
            break;
        case '=':
            k     = COS;
            alpha = true;
            shift = true;
            break;
        case '>':
            k     = TAN;
            alpha = true;
            shift = true;
            break;
        case '^':
            k     = INV;
            alpha = true;
            shift = true;
            break;
        case '(':
            k     = XEQ;
            alpha = true;
            shift = true;
            del   = true;
            break;
        case ')':
            k     = XEQ;
            alpha = true;
            shift = true;
            bsp   = true;
            break;
        case '[':
            k      = KEY9;
            alpha  = true;
            xshift = true;
            del    = true;
            break;
        case ']':
            k      = KEY9;
            alpha  = true;
            xshift = true;
            bsp    = true;
            break;
        case '{':
            k      = RUNSTOP;
            alpha  = true;
            xshift = true;
            del    = true;
            break;
        case '}':
            k      = RUNSTOP;
            alpha  = true;
            xshift = true;
            bsp    = true;
            break;
        case '"':
            k      = ENTER;
            alpha  = true;
            xshift = true;
            bsp    = true;
            break;
        case '\'':
            k      = XEQ;
            alpha  = true;
            xshift = true;
            bsp    = true;
            break;
        case '&':
            k      = KEY1;
            alpha  = true;
            xshift = true;
            break;
        case '@':
            k      = KEY2;
            alpha  = true;
            xshift = true;
            break;
        case '$':
            k      = KEY3;
            alpha  = true;
            xshift = true;
            break;
        case '#':
            k      = KEY4;
            alpha  = true;
            xshift = true;
            break;
        case '\\':
            k      = ADD;
            alpha  = true;
            xshift = true;
            break;
        case '\n':
            k      = BSP;
            alpha  = true;
            xshift = true;
            break;
        case L'«':
            k     = RUNSTOP;
            alpha = false;
            shift = true;
            del   = true;
            break;
        case L'»':
            k     = RUNSTOP;
            alpha = false;
            shift = true;
            bsp   = true;
            break;
        case L'→':
            k      = STO;
            alpha  = true;
            xshift = true;
            break;
        case L'×':
            k     = MUL;
            alpha = true;
            shift = true;
            break;
        case L'÷':
            k     = DIV;
            alpha = true;
            shift = true;
            break;
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

    // Check that we are not displaying an error message
    if (rt.error())
    {
        explain("Unexpected error message [",
                rt.error(),
                "] "
                "during data entry, cleared");
        fail();
        rt.clear_error();
    }

    // Check invalid input: can only have one shift
    if (shift && xshift)
        shift = false;

    // First change lowercase state as necessary, since this messes up shift
    while (lowercase != ui.lowercase)
    {
        while (!ui.shift)
            itest(SHIFT, NOKEYS);
        itest(ENTER, NOKEYS);
    }

    // Enter alpha mode using Shift-Enter so that we can shift afterwards
    if (alpha != ui.alpha)
    {
        if (shift || xshift)
        {
            if (!alpha)
            {
                while (ui.alpha)
                    itest(LONGPRESS, SHIFT, NOKEYS);
            }
            else
            {
                while (!ui.shift)
                    itest(SHIFT, NOKEYS);
                itest(ENTER, NOKEYS);
            }
        }
        else
        {
            // Keep pressing shift until we get alpha
            while (ui.alpha != alpha)
                itest(LONGPRESS, SHIFT, NOKEYS);
        }
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
    {
        lcd_update = lcd_needsupdate;
        Stack.catch_up();
        sys_delay(delay_time);
    }
    return *this;
}


tests &tests::refreshed()
// ----------------------------------------------------------------------------
//    Wait until the screen was updated by the calculator
// ----------------------------------------------------------------------------
{
    record(tests, "Waiting for refresh");

    // Wait for a screen redraw
    while (lcd_needsupdate == lcd_update)
        sys_delay(delay_time);

    // Wait for a stack update
    uint32_t start = sys_current_ms();
    while (!Stack.available() && sys_current_ms() - start < wait_time)
        sys_delay(delay_time);

    // Check that we have latest stack update
    while (Stack.available() > 1)
        Stack.consume();

    record(tests,
           "Refreshed, needs=%u update=%u available=%u",
           lcd_needsupdate,
           lcd_update,
           Stack.available());
    lcd_update = lcd_needsupdate;

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


tests &tests::expect(cstring output)
// ----------------------------------------------------------------------------
//   Check that the output at first level of stack matches the string
// ----------------------------------------------------------------------------
{
    record(tests, "Expecting [%+s]", output);
    ready();
    cindex++;
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
        return explain("Expected no error, got [", err, "]").fail();
    if (msg && !err)
        return explain("Expected error message [", msg, "], got none").fail();
    if (msg && err && strcmp(cstring(err), msg) != 0)
        return explain("Expected error message [",
                       msg,
                       "], "
                       "got [",
                       err,
                       "]")
            .fail();
    return *this;
}


tests &tests::command(cstring ref)
// ----------------------------------------------------------------------------
//   Check that the command result matches expectations
// ----------------------------------------------------------------------------
{
    ready();
    utf8 cmd = rt.command();

    if (!ref && cmd)
        return explain("Expected no command, got [", cmd, "]").fail();
    if (ref && !cmd)
        return explain("Expected command [", ref, "], got none").fail();
    if (ref && cmd && strcmp(ref, cstring(cmd)) != 0)
        return explain("Expected command [",
                       ref,
                       "], "
                       "got [",
                       cmd,
                       "]")
            .fail();

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
        return explain("Expected source [",
                       ref,
                       "], "
                       "got [",
                       src,
                       "]")
            .fail();

    return *this;
}
