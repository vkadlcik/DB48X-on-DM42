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

#include <stdio.h>


extern volatile int lcd_needsupdate;

void tests::run()
// ----------------------------------------------------------------------------
//   Run all test categories
// ----------------------------------------------------------------------------
{
    tindex = sindex = cindex = count = 0;
    failures.clear();

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

    step("Integers");
    test(CLEAR, "1", ENTER)
        .type(object::ID_integer).expect("1");
    test(CLEAR, "-1", ENTER)
        .type(object::ID_neg_integer).expect("-1");
}



// ============================================================================
//
//   Sequencing tests
//
// ============================================================================

tests &tests::begin(cstring name)
// ----------------------------------------------------------------------------
//   Beginning of a test
// ----------------------------------------------------------------------------
{
    if (sindex)
        fprintf(stderr, "[%s]\n", ok ? "PASS" : "FAIL");

    tname = name;
    tindex++;
    fprintf(stderr, "%3u: %s\n", tindex, tname);
    sindex = 0;
    ok = true;

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
        fprintf(stderr, "[%s]\n", ok ? "PASS" : "FAIL");
    fprintf(stderr, "%3u:  %03u: %-64s", tindex, sindex, sname);
    cindex = 0;
    count++;
    ok = true;

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
    failures.push_back(failure(tname, sname, tindex, sindex, cindex));
    ok = false;
    return *this;
}


tests &tests::summary()
// ----------------------------------------------------------------------------
//   Summarize the test results
// ----------------------------------------------------------------------------
{
    if (sindex)
        fprintf(stderr, "[%s]\n", ok ? "PASS" : "FAIL");

    if (failures.size())
    {
        fprintf(stderr, "Summary of %zu failures:\n", failures.size());\
        cstring last = nullptr;
        for (auto s : failures)
        {
            if (s.test != last)
            {
                fprintf(stderr, "%3u: %s\n", s.tindex, s.test);
                last = s.test;
            }
            fprintf(stderr, "%3u:%03u.%03u: %s\n",
                    s.tindex, s.sindex, s.cindex, s.step);
        }
    }
    fprintf(stderr, "Ran %u tests, %zu failures\n",
            count, failures.size());
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
    return test(cstring(buffer));
}


tests &tests::test(int value)
// ----------------------------------------------------------------------------
//   Test a signed numerical value
// ----------------------------------------------------------------------------
{
    char buffer[32];
    snprintf(buffer, sizeof(buffer), "%d", value);
    return test(cstring(buffer));
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
    case '+': k = ADD;          shift = alpha; break;
    case '-': k = SUB;          shift = alpha; break;
    case '*': k = MUL;          alpha = true; xshift = true; break;
    case '/': k = DIV;          alpha = true; xshift = true; break;
    case '.': k = DOT;          shift = alpha; break;
    case ',': k = DOT;          shift = !alpha; break;
    case ' ': k = RUNSTOP;      alpha = true;  break;
    case '?': k = RUNSTOP;      alpha = true;  xshift = true; break;
    case '!': k = RUNSTOP;      alpha = true;  shift  = true; break;
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
    case '#': k = KEY3;         alpha = true; xshift = true; break;
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


tests &tests::test(struct wait delay)
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
    lcd_update = lcd_needsupdate;
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
    if (object *top = rt.top())
    {
        char buffer[256];
        top->render(buffer, sizeof(buffer), rt);
        if (strcmp(output, buffer) == 0)
            return *this;
    }
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


tests &tests::type(object::id ty)
// ----------------------------------------------------------------------------
//   Check that the top of stack matches the type
// ----------------------------------------------------------------------------
{
    ready();
    cindex++;
    runtime &rt = runtime::RT;
    if (object *top = rt.top())
        if (top->type() == ty)
            return *this;
    return fail();
}


tests &tests::shift(bool s)
// ----------------------------------------------------------------------------
//   Check that the shift state matches expectations
// ----------------------------------------------------------------------------
{
    nokeys();
    return check(Input.shift == s);
}


tests &tests::xshift(bool x)
// ----------------------------------------------------------------------------
//   Check that the right shift state matches expectations
// ----------------------------------------------------------------------------
{
    nokeys();
    return check(Input.xshift == x);
}


tests &tests::alpha(bool a)
// ----------------------------------------------------------------------------
//   Check that the alpha state matches expectations
// ----------------------------------------------------------------------------
{
    nokeys();
    return check(Input.alpha == a);
}


tests &tests::lower(bool l)
// ----------------------------------------------------------------------------
//   Check that the lowercase state matches expectations
// ----------------------------------------------------------------------------
{
    nokeys();
    return check(Input.lowercase == l);
}


tests &tests::editing()
// ----------------------------------------------------------------------------
//   Check that we are editing, without checking the length
// ----------------------------------------------------------------------------
{
    ready();
    return check(runtime::RT.editing());
}


tests &tests::editing(size_t length)
// ----------------------------------------------------------------------------
//   Check that the editor has exactly the expected length
// ----------------------------------------------------------------------------
{
    ready();
    return check(runtime::RT.editing() == length);
}


tests &tests::editor(cstring text)
// ----------------------------------------------------------------------------
//   Check that the editor contents matches the text
// ----------------------------------------------------------------------------
{
    ready();
    runtime    &rt = runtime::RT;
    const char *ed = rt.editor();
    size_t      sz = rt.editing();
    return check(ed && sz == strlen(text) && memcmp(ed, text, sz) == 0);
}


tests &tests::cursor(size_t csr)
// ----------------------------------------------------------------------------
//   Check that the cursor is at expected position
// ----------------------------------------------------------------------------
{
    ready();
    return check(Input.cursor == csr);
}


tests &tests::error(cstring msg)
// ----------------------------------------------------------------------------
//   Check that the error message matches expectations
// ----------------------------------------------------------------------------
{
    ready();
    runtime    &rt = runtime::RT;
    cstring    err = rt.error();
    return check(msg ? (err && strcmp(err, msg) == 0) : err == nullptr);
}


tests &tests::command(cstring ref)
// ----------------------------------------------------------------------------
//   Check that the command result matches expectations
// ----------------------------------------------------------------------------
{
    ready();
    runtime    &rt = runtime::RT;
    cstring    cmd = rt.command();
    return check(ref ? (cmd && strcmp(cmd, ref) == 0) : cmd == nullptr);
}


tests &tests::source(cstring ref)
// ----------------------------------------------------------------------------
//   Check that the source indicated in the editor matches expectations
// ----------------------------------------------------------------------------
{
    ready();
    runtime &rt  = runtime::RT;
    cstring  src = rt.source();
    return check(ref ? (src && strcmp(src, ref) == 0) : src == nullptr);
}
