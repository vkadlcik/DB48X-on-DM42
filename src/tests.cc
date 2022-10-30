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
#include "input.h"
#include "settings.h"

#include <stdio.h>
#include <regex.h>


extern volatile int lcd_needsupdate;

void tests::run()
// ----------------------------------------------------------------------------
//   Run all test categories
// ----------------------------------------------------------------------------
{
    tindex = sindex = cindex = count = 0;
    failures.clear();

    bool verbose = false;
    if (verbose)
        fprintf(stderr,
                "Initial settings:\n"
                "  Precision:       %u\n"
                "  Displayed:       %u\n"
                "  Display mode:    %u\n"
                "  Decimal dot:     %c\n"
                "  Exponent:        %c\n"
                "  Angle mode:      %u\n"
                "  Base:            %u\n"
                "  Command format:  %u\n",
                Settings.precision,
                Settings.displayed,
                Settings.display_mode,
                Settings.decimalDot,
                Settings.exponentChar,
                Settings.angle_mode,
                Settings.base,
                Settings.command_fmt);

    Settings = settings();       // Reset to default settings
    Settings.exponentChar = 'E'; // Standard E is easier for us to match

    if (verbose)
        fprintf(stderr,
                "Updated settings:\n"
                "  Precision:       %u\n"
                "  Displayed:       %u\n"
                "  Display mode:    %u\n"
                "  Decimal dot:     %c\n"
                "  Exponent:        %c\n"
                "  Angle mode:      %u\n"
                "  Base:            %u\n"
                "  Command format:  %u\n",
                Settings.precision,
                Settings.displayed,
                Settings.display_mode,
                Settings.decimalDot,
                Settings.exponentChar,
                Settings.angle_mode,
                Settings.base,
                Settings.command_fmt);

    arithmetic();
    shift_logic();
    keyboard_entry();
    data_types();

    summary();
}


void tests::shift_logic()
// ----------------------------------------------------------------------------
//   Test all keys and check we have the correct output
// ----------------------------------------------------------------------------
{
    begin("Shift logic");
    step("Shift state must be cleared at start")
        .shift(false).xshift(false).alpha(false).lower(false);
    step("Shift works")
        .test(SHIFT)
        .shift(true).xshift(false).alpha(false).lower(false);
    step("Shift-Shift is Alpha")
        .test(SHIFT)
        .shift(false).xshift(false).alpha(true).lower(false);
    step("Third shift clears all shifts")
        .test(SHIFT)
        .shift(false).xshift(false).alpha(false).lower(false);

    step("Shift pass two")
        .test(SHIFT)
        .shift(true).xshift(false).alpha(false).lower(false);
    step("Shift pass two: Shift-Shift is Alpha")
        .test(SHIFT)
        .shift(false).xshift(false).alpha(true).lower(false);
    step("Shift pass two: Third shift clears all shifts")
        .test(SHIFT)
        .shift(false).xshift(false).alpha(false).lower(false);

    step("Long-press shift is right shift")
        .test(SHIFT, false).wait(600).test(RELEASE)
        .shift(false).xshift(true);
    step("Clearing right shift")
        .test(SHIFT)
        .shift(false).xshift(false);

    step("Typing alpha")
        .test(SHIFT, SHIFT, A)
        .shift(false).alpha(true).lower(false)
        .editor("A");
    step("Selecting lowercase with Shift-ENTER")
        .test(SHIFT, ENTER)
        .alpha(true).lower(true);
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
    cstring seps = "[](){}\"Hello\"'Test'";
    test(CLEAR, seps).editor(seps).wait(500);

    step("Key repeat");
    test(CLEAR, SHIFT, SHIFT, LONGPRESS, A).wait(1000).test(RELEASE)
        .check(Input.cursor > 4);
}


void tests::data_types()
// ----------------------------------------------------------------------------
//   Check the basic data types
// ----------------------------------------------------------------------------
{
    begin("Data types");

    step("Positive integer");
    test(CLEAR, "1", ENTER)
        .type(object::ID_integer).expect("1");
    step("Negative integer");
    test(CLEAR, "1", CHS, ENTER)
        .type(object::ID_neg_integer).expect("-1");

    step("Binary based integer");
    test(CLEAR, "#10010101b", ENTER)
        .type(object::ID_bin_integer).expect("#10010101b");
    test(CLEAR, "#101B", ENTER)
        .type(object::ID_bin_integer).expect("#101b");

    step("Decimal based integer");
    test(CLEAR, "#12345d", ENTER)
        .type(object::ID_dec_integer).expect("#12345d");
    test(CLEAR, "#123D", ENTER)
        .type(object::ID_dec_integer).expect("#123d");

    step("Octal based integer");
    test(CLEAR, "#12345o", ENTER)
        .type(object::ID_oct_integer).expect("#12345o");
    test(CLEAR, "#123O", ENTER)
        .type(object::ID_oct_integer).expect("#123o");

    step("Hexadecimal based integer");
    test(CLEAR, "#1234ABCDH", ENTER)
        .type(object::ID_hex_integer).expect("#1234ABCDh");
    test(CLEAR, "#DEADBEEFH", ENTER)
        .type(object::ID_hex_integer).expect("#DEADBEEFh");

    step("Symbols");
    cstring symbol = "ABC123Z";
    test(CLEAR, symbol, ENTER)
        .type(object::ID_symbol).expect(symbol);

    step("Text");
    cstring string = "\"Hello World\"";
    test(CLEAR, string, ENTER)
        .type(object::ID_text).expect(string);

    clear();
}


void tests::arithmetic()
// ----------------------------------------------------------------------------
//   Tests for basic arithmetic operations
// ----------------------------------------------------------------------------
{
    begin("Arithmetic");

    step("Integer addition");
    test(CLEAR, 1, ENTER, 1, ADD)
        .type(object::ID_integer).expect("2");
    test(1, ADD)
        .type(object::ID_integer).expect("3");
    test(-1, ADD)
        .type(object::ID_integer).expect("2");
    test(-1, ADD)
        .type(object::ID_integer).expect("1");
    test(-1, ADD)
        .type(object::ID_integer).expect("0");
    test(-1, ADD)
        .type(object::ID_neg_integer).expect("-1");
    test(-1, ADD)
        .type(object::ID_neg_integer).expect("-2");
    test(-1, ADD)
        .type(object::ID_neg_integer).expect("-3");
    test(1, ADD)
        .type(object::ID_neg_integer).expect("-2");
    test(1, ADD)
        .type(object::ID_neg_integer).expect("-1");
    test(1, ADD)
        .type(object::ID_integer).expect("0");

    step("Integer addition overflow");
    test(CLEAR, ~0ULL-1, ENTER, 1, ADD)
        .type(object::ID_integer).expect("18446744073709551615");
    test(CLEAR, ~0ULL-3, CHS, ENTER, -2, ADD)
        .type(object::ID_neg_integer).expect("-18446744073709551614");

    test(CLEAR, ~0ULL, ENTER, 1, ADD)
        .type(object::ID_decimal128).expect("1.8446744073709551616E19");
    test(CLEAR, ~0ULL, CHS, ENTER, -2, ADD)
        .type(object::ID_decimal128).expect("-1.8446744073709551617E19");

    step("Adding ten small integers at random");
    srand48(sys_current_ms());
    for (int i = 0; i < 10; i++)
    {
        int x = (lrand48() & 0xFFFF) - 0x8000;
        int y = (lrand48() & 0xFFFF) - 0x8000;
        test(CLEAR, x, ENTER, y, ADD)
            .explain("Computing ", x, " + ", y, ", ")
            .expect(x + y);
    }

    step("Integer subtraction");
    test(CLEAR, 1, ENTER, 1, SUB)
        .type(object::ID_integer).expect("0");
    test(1, SUB)
        .type(object::ID_neg_integer).expect("-1");
    test(-1, SUB)
        .type(object::ID_integer).expect("0");
    test(-1, SUB)
        .type(object::ID_integer).expect("1");
    test(-1, SUB)
        .type(object::ID_integer).expect("2");
    test(1, SUB)
        .type(object::ID_integer).expect("1");
    test(1, SUB)
        .type(object::ID_integer).expect("0");
    test(3, SUB)
        .type(object::ID_neg_integer).expect("-3");
    test(-1, SUB)
        .type(object::ID_neg_integer).expect("-2");
    test(1, SUB)
        .type(object::ID_neg_integer).expect("-3");
    test(-3, SUB)
        .type(object::ID_integer).expect("0");

    step("Integer subtraction overflow");
    test(CLEAR, 0xFFFFFFFFu, CHS, ENTER, 1, SUB)
        .type(object::ID_decimal128).expect("-4294967296");
    test(CLEAR, -3, ENTER, 0xFFFFFFFFu, SUB)
        .type(object::ID_decimal128).expect("-4294967298");

    step("Subtracting ten small integers at random");
    for (int i = 0; i < 10; i++)
    {
        int x = (lrand48() & 0xFFFF) - 0x8000;
        int y = (lrand48() & 0xFFFF) - 0x8000;
        test(CLEAR, x, ENTER, y, SUB)
            .explain("Computing ", x, " - ", y, ", ")
            .expect(x - y);
    }

    step("Integer multiplication");
    test(CLEAR, 1, ENTER, 1, MUL)
        .type(object::ID_integer).expect("1");
    test(3, MUL)
        .type(object::ID_integer).expect("3");
    test(-3, MUL)
        .type(object::ID_neg_integer).expect("-9");
    test(2, MUL)
        .type(object::ID_neg_integer).expect("-18");
    test(-7, MUL)
        .type(object::ID_integer).expect("126");

    step("Multiplying ten small integers at random");
    for (int i = 0; i < 10; i++)
    {
        int x = (lrand48() & 0xFFFF) - 0x8000;
        int y = (lrand48() & 0xFFFF) - 0x8000;
        test(CLEAR, x, ENTER, y, MUL)
            .explain("Computing ", x, " * ", y, ", ")
            .expect(x * y);
    }

    step("Integer division");
    test(CLEAR, 210, ENTER, 2, DIV)
        .type(object::ID_integer).expect("105");
    test(5, DIV)
        .type(object::ID_integer).expect("21");
    test(-3, DIV)
        .type(object::ID_neg_integer).expect("-7");
    test(-7, DIV)
        .type(object::ID_integer).expect("1");

    step("Dividing ten small integers at random");
    for (int i = 0; i < 10; i++)
    {
        int x = (lrand48() & 0x3FFF) - 0x4000;
        int y = (lrand48() & 0x3FFF) - 0x4000;
        test(CLEAR, x * y, ENTER, y, DIV)
            .explain("Computing ", x * y, " / ", y, ", ")
            .expect(x);
    }

    step("Division with real promotion");
    test(CLEAR, 1, ENTER, 3, DIV)
        .match("0.333*");
    test(CLEAR, 2, ENTER, 3, DIV)
        .match("0.66*7");
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
    sindex = 0;
    ok = true;
    explanation = "";

    // Start with a clean state
    clear();

    return *this;
}


tests &tests::step(cstring name)
// ----------------------------------------------------------------------------
//  Beginning of a step
// ----------------------------------------------------------------------------
{
    lcd_update = lcd_needsupdate;
    sname = name;
    if (sindex++)
    {
        passfail(ok);
        if (!ok)
            show(failures.back());
    }
    fprintf(stderr, "%3u:  %03u: %-64s", tindex, sindex, sname);
    cindex = 0;
    count++;
    ok = true;
    explanation = "";

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
    failures.push_back(failure(tname, sname, explanation,
                               tindex, sindex, cindex));
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
        fprintf(stderr, "Summary of %zu failures:\n", failures.size());\
        cstring last = nullptr;
        for (auto f : failures)
            show(f, last);
    }
    fprintf(stderr, "Ran %u tests, %zu failures\n",
            count, failures.size());
    return *this;
}


tests &tests::show(tests::failure &f)
// ----------------------------------------------------------------------------
//   Show a single failure
// ----------------------------------------------------------------------------
{
    cstring last = nullptr;
    return show(f, last);
}


tests &tests::show(tests::failure &f, cstring &last)
// ----------------------------------------------------------------------------
//   Show an individual failure
// ----------------------------------------------------------------------------
{
    if (f.test != last)
    {
        fprintf(stderr, "%3u: %s\n", f.tindex, f.test);
        last = f.test;
    }
    fprintf(stderr, "%3u:%03u.%03u: %s\n",
            f.tindex, f.sindex, f.cindex, f.step);
    fprintf(stderr, "==== %s\n", f.explanation.c_str());
    return *this;
}




// ============================================================================
//
//   Utilities to build the tests
//
// ============================================================================

tests &tests::test(tests::key k, bool release)
// ----------------------------------------------------------------------------
//   Type a given key directly
// ----------------------------------------------------------------------------
{
    extern int key_remaining();

    // Check for special key sequences
    switch(k)
    {
    case ALPHA:
        return shifts(false, false, true, false);

    case LOWERCASE:
        return shifts(false, false, true, true);

    case LONGPRESS:
        longpress = true;       // Next key will be a long press
        return *this;

    case CLEAR:
        return clear();

    case NOKEYS:
        return nokeys();

    case REFRESH:
        return refreshed();

    default:
        break;
    }


    // Wait for the RPL thread to process the keys (to be revisited on DM42)
    while (!key_remaining())
        sys_delay(20);

    key_push(k);
    if (longpress)
    {
        sys_delay(600);
        longpress = false;
        release = false;
    }
    sys_delay(20);

    if (release && k != RELEASE)
    {
        while (!key_remaining())
            sys_delay(20);
        key_push(RELEASE);
    }

    return *this;
}


tests &tests::test(uint value)
// ----------------------------------------------------------------------------
//    Test a numerical value
// ----------------------------------------------------------------------------
{
    char buffer[32];
    snprintf(buffer, sizeof(buffer), "%u", value);
    test(cstring(buffer));
    return shifts(false, false, false, false);
}


tests &tests::test(int value)
// ----------------------------------------------------------------------------
//   Test a signed numerical value
// ----------------------------------------------------------------------------
{
    if (value < 0)
        return test(uint(-value), CHS);
    else
        return test(uint(value));
}


tests &tests::test(ularge value)
// ----------------------------------------------------------------------------
//    Test a numerical value
// ----------------------------------------------------------------------------
{
    char buffer[32];
    snprintf(buffer, sizeof(buffer), "%llu", (unsigned long long) value);
    test(cstring(buffer));
    return shifts(false, false, false, false);
}


tests &tests::test(large value)
// ----------------------------------------------------------------------------
//   Test a signed numerical value
// ----------------------------------------------------------------------------
{
    if (value < 0)
        return test(ularge(-value), CHS);
    else
        return test(ularge(value));
}


tests &tests::test(char c)
// ----------------------------------------------------------------------------
//   Type the character on the calculator's keyboard
// ----------------------------------------------------------------------------
{
    nokeys();

    bool alpha  = Input.alpha;
    bool shift  = false;
    bool xshift = false;
    bool lower  = Input.lowercase;
    key  k      = RELEASE;
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
    case '?': k = RUNSTOP;      alpha = true;  xshift = true; break;
    case '!': k = ADD;          alpha = true;  xshift  = true; break;
    case '_': k = SUB;          alpha = true;  break;
    case '%': k = RCL;          alpha = true;  break;
    case ':': k = KEY0;         alpha = true;  del = true; break;
    case ';': k = KEY0;         alpha = true; xshift = true;  break;
    case '<': k = SIN;          alpha = true;  shift = true;  break;
    case '=': k = COS;          alpha = true;  shift = true;  break;
    case '>': k = TAN;          alpha = true;  shift = true;  break;
    case '^': k = INV;          alpha = true;  shift = true;  break;
    case '(': k = LOG;          alpha = true;  shift = true;  del = true; break;
    case ')': k = LOG;          alpha = true;  shift = true;  bsp = true; break;
    case '[': k = LN;           alpha = true;  shift = true;  del = true; break;
    case ']': k = LN;           alpha = true;  shift = true;  bsp = true; break;
    case '{': k = XEQ;          alpha = true;  shift = true;  del = true; break;
    case '}': k = XEQ;          alpha = true;  shift = true;  bsp = true; break;
    case '"': k = SWAP;         alpha = true;  shift = true;  bsp = true; break;
    case '\'': k = CHS;         alpha = true;  shift = true;  bsp = true; break;
    case '&': k = KEY1;         alpha = true; xshift = true; break;
    case '@': k = KEY2;         alpha = true; xshift = true; break;
#if 0
    case '#': k = KEY3;         alpha = true; xshift = true; break;
#else
    case '#': k = KEY4;         alpha = false; shift = true; break;
#endif
    case '$': k = KEY4;         alpha = true; xshift = true; break;
    case '\\': k = ADD;         alpha = true; xshift = true; break;
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
        test(k);

        // If we have a pair, like (), check if we need bsp or del
        if (bsp)
            test(BSP, DOWN);
        else if (del)
            test(SHIFT, BSP);
    }

    return *this;
}


tests &tests::test(cstring alpha)
// ----------------------------------------------------------------------------
//   Type the string on the calculator's keyboard
// ----------------------------------------------------------------------------
{
    while (*alpha)
        test(*alpha++);
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
    runtime &rt = runtime::RT;
    if (rt.error())
    {
        explain("Unexpected error message [", rt.error(), "] "
                "during data entry, cleared");
        rt.error(nullptr);
        ok = false;
    }

    // Check invalid input: can only have one shift
    if (shift && xshift)
        shift = false;

    // First change lowercase state as necessary, since this messes up shift
    while (lowercase != Input.lowercase)
    {
        while (!Input.shift)
            test(SHIFT, NOKEYS);
        test(ENTER, NOKEYS);
    }

    // Enter alpha mode using Shift-Enter so that we can shift afterwards
    if (alpha != Input.alpha)
    {
        if (shift || xshift)
        {
            if (!alpha)
            {
                while (Input.alpha)
                    test(SHIFT, NOKEYS);
            }
            else
            {
                while (!Input.shift)
                    test(SHIFT, NOKEYS);
                test(ENTER, NOKEYS);
            }
        }
        else
        {
            // Keep pressing shift until we get alpha
            while (Input.alpha != alpha)
                test(SHIFT, NOKEYS);
        }
    }

    while (xshift != Input.xshift)
    {
        if (xshift)
            test(LONGPRESS, SHIFT, NOKEYS, RELEASE, NOKEYS);
        else
            test(SHIFT, NOKEYS);
    }

    while (shift != Input.shift)
    {
        test(SHIFT, NOKEYS);
    }

    return *this;
}


tests &tests::test(tests::WAIT delay)
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
    Input.clear_editor();
    runtime &rt = runtime::RT;
    while (rt.depth())
        rt.pop();
    rt.error(nullptr);
    rt.command(utf8());
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
        sys_delay(20);
    return *this;
}


tests &tests::refreshed()
// ----------------------------------------------------------------------------
//    Wait until the screen was updated by the calculator
// ----------------------------------------------------------------------------
{
    while (lcd_needsupdate == lcd_update)
        sys_delay(20);
    return *this;
}


tests &tests::wait(uint ms)
// ----------------------------------------------------------------------------
//   Force a delay after the calculator was ready
// ----------------------------------------------------------------------------
{
    sys_delay(ms);
    return *this;
}


tests &tests::expect(cstring output)
// ----------------------------------------------------------------------------
//   Check that the output at first level of stack matches the string
// ----------------------------------------------------------------------------
{
    ready();
    cindex++;
    runtime &rt = runtime::RT;
    if (object_p top = rt.top())
    {
        char buffer[256];
        top->render(buffer, sizeof(buffer), rt);
        if (strcmp(output, buffer) == 0)
            return *this;
        explain("Expected output [", output, "], got [", buffer, "] instead");
        return fail();
    }
    explain("Expected output [", output, "] but got no object");
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


tests &tests::match(cstring restr)
// ----------------------------------------------------------------------------
//   Check that the output at first level of stack matches the string
// ----------------------------------------------------------------------------
{
    ready();
    cindex++;
    runtime &rt = runtime::RT;
    if (object_p top = rt.top())
    {
        regex_t    re;
        regmatch_t rm;
        char       buffer[256];

        regcomp(&re, restr, REG_EXTENDED | REG_ICASE);
        top->render(buffer, sizeof(buffer), rt);
        bool ok =
            regexec(&re, buffer, 1, &rm, 0) == 0 &&
            rm.rm_so == 0 &&
            buffer[rm.rm_eo] == 0;
        regfree(&re);
        if (ok)
            return *this;
        explain("Expected output matching [", restr, "], "
                "got [", buffer, "]");
        return fail();
    }
    explain("Expected output matching [", restr, "] but got no object");
    return fail();
}


tests &tests::type(object::id ty)
// ----------------------------------------------------------------------------
//   Check that the top of stack matches the type
// ----------------------------------------------------------------------------
{
    ready();
    cindex++;
    runtime &rt = runtime::RT;
    if (object_p top = rt.top())
    {
        object::id tty = top->type();
        if (tty == ty)
            return *this;
        explain("Expected type ", object::name(ty), " (", int(ty), ")"
                " but got ", object::name(tty), " (", int(tty), ")");
        return fail();
    }
    explain("Expected type ", object::name(ty), " (", int(ty), ")"
            " but got no object");
    return fail();
}


tests &tests::shift(bool s)
// ----------------------------------------------------------------------------
//   Check that the shift state matches expectations
// ----------------------------------------------------------------------------
{
    nokeys();
    return check(Input.shift == s,
                 "Expected shift ", s, ", got ", Input.shift);
}


tests &tests::xshift(bool x)
// ----------------------------------------------------------------------------
//   Check that the right shift state matches expectations
// ----------------------------------------------------------------------------
{
    nokeys();
    return check(Input.xshift == x,
                 "Expected xshift ", x, " got ", Input.xshift);
}


tests &tests::alpha(bool a)
// ----------------------------------------------------------------------------
//   Check that the alpha state matches expectations
// ----------------------------------------------------------------------------
{
    nokeys();
    return check(Input.alpha == a,
                 "Expected alpha ", a, " got ", Input.alpha);
}


tests &tests::lower(bool l)
// ----------------------------------------------------------------------------
//   Check that the lowercase state matches expectations
// ----------------------------------------------------------------------------
{
    nokeys();
    return check(Input.lowercase == l,
                 "Expected alpha ", l, " got ", Input.alpha);
}


tests &tests::editing()
// ----------------------------------------------------------------------------
//   Check that we are editing, without checking the length
// ----------------------------------------------------------------------------
{
    ready();
    return check(runtime::RT.editing(),
                 "Expected to be editing, got length ", runtime::RT.editing());
}


tests &tests::editing(size_t length)
// ----------------------------------------------------------------------------
//   Check that the editor has exactly the expected length
// ----------------------------------------------------------------------------
{
    ready();
    return check(runtime::RT.editing() == length,
                 "Expected editing length to be ", length,
                 " got ", runtime::RT.editing());
}


tests &tests::editor(cstring text)
// ----------------------------------------------------------------------------
//   Check that the editor contents matches the text
// ----------------------------------------------------------------------------
{
    ready();
    runtime &rt = runtime::RT;
    byte_p   ed = rt.editor();
    size_t   sz = rt.editing();

    if (!ed)
        return explain("Expected editor to contain [", text, "], "
                       "but it's empty")
            .fail();
    if (sz != strlen(text))
        return explain("Expected ", strlen(text), " characters in editor"
                       " [", text, "], "
                       "but got ", sz, " characters "
                       " [", std::string(cstring(ed), sz), "]")
            .fail();
    if (memcmp(ed, text, sz))
        return explain("Expected editor to contain [", text, "], "
                       "but it contains [", std::string(cstring(ed), sz), "]")
            .fail();

    return *this;
}


tests &tests::cursor(size_t csr)
// ----------------------------------------------------------------------------
//   Check that the cursor is at expected position
// ----------------------------------------------------------------------------
{
    ready();
    return check(Input.cursor == csr,
                 "Expected cursor to be at position ", csr,
                 " but it's at position ", Input.cursor);
}


tests &tests::error(cstring msg)
// ----------------------------------------------------------------------------
//   Check that the error message matches expectations
// ----------------------------------------------------------------------------
{
    ready();
    runtime &rt  = runtime::RT;
    utf8     err = rt.error();

    if (!msg && err)
        return explain("Expected no error, got [", err, "]").fail();
    if (msg && !err)
        return explain("Expected error message [", msg, "], got none").fail();
    if (msg && err && strcmp(cstring(err), msg) != 0)
        return explain("Expected error message [", msg, "], "
                       "got [", err, "]").fail();
    return *this;
}


tests &tests::command(cstring ref)
// ----------------------------------------------------------------------------
//   Check that the command result matches expectations
// ----------------------------------------------------------------------------
{
    ready();
    runtime &rt  = runtime::RT;
    utf8     cmd = rt.command();

    if (!ref && cmd)
        return explain("Expected no command, got [", cmd, "]").fail();
    if (ref && !cmd)
        return explain("Expected command [", ref, "], got none").fail();
    if (ref && cmd && strcmp(ref, cstring(cmd)) != 0)
        return explain("Expected command [", ref, "], "
                       "got [", cmd, "]").fail();

    return *this;
}


tests &tests::source(cstring ref)
// ----------------------------------------------------------------------------
//   Check that the source indicated in the editor matches expectations
// ----------------------------------------------------------------------------
{
    ready();
    runtime &rt  = runtime::RT;
    utf8     src = rt.source();

    if (!ref && src)
        return explain("Expected no source, got [", src, "]").fail();
    if (ref && !src)
        return explain("Expected source [", ref, "], got none").fail();
    if (ref && src && strcmp(ref, cstring(src)) != 0)
        return explain("Expected source [", ref, "], "
                       "got [", src, "]").fail();

    return *this;
}
