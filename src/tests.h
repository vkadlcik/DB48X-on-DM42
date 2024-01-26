#ifndef TESTS_H
#define TESTS_H
// ****************************************************************************
//  tests.h                                                       DB48X project
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

#include "dmcp.h"
#include "object.h"
#include "runtime.h"
#include "target.h"

#include <vector>
#include <string>
#include <sstream>

struct tests
// ----------------------------------------------------------------------------
//   Run a series of tests
// ----------------------------------------------------------------------------
{
    tests()
        : tname(), sname(), tindex(), sindex(), cindex(), count(),
          ok(), longpress(), failures()
    { }

    // Run all tests
    void run(bool onlyCurrent);

    // Individual test categories
    void reset_settings();
    void shift_logic();
    void keyboard_entry();
    void data_types();
    void editor_operations();
    void stack_operations();
    void arithmetic();
    void global_variables();
    void local_variables();
    void for_loops();
    void conditionals();
    void logical_operations();
    void command_display_formats();
    void integer_display_formats();
    void fraction_display_formats();
    void decimal_display_formats();
    void integer_numerical_functions();
    void decimal_numerical_functions();
    void exact_trig_cases();
    void fraction_decimal_conversions();
    void complex_types();
    void complex_arithmetic();
    void complex_functions();
    void units_and_conversions();
    void list_functions();
    void sorting_functions();
    void vector_functions();
    void matrix_functions();
    void solver_testing();
    void numerical_integration_testing();
    void text_functions();
    void auto_simplification();
    void rewrite_engine();
    void expand_collect_simplify();
    void tagged_objects();
    void catalog_test();
    void cycle_test();
    void flags_by_name();
    void settings_by_name();
    void parsing_commands_by_name();
    void plotting();
    void plotting_all_functions();
    void graphic_commands();
    void hms_dms_operations();
    void online_help();
    void regression_checks();

    enum key
    {
        RELEASE    = 0,

        SIGMA      = KEY_SIGMA,
        INV        = KEY_INV,
        SQRT       = KEY_SQRT,
        LOG        = KEY_LOG,
        LN         = KEY_LN,
        XEQ        = KEY_XEQ,
        STO        = KEY_STO,
        RCL        = KEY_RCL,
        RDN        = KEY_RDN,
        SIN        = KEY_SIN,
        COS        = KEY_COS,
        TAN        = KEY_TAN,
        ENTER      = KEY_ENTER,
        SWAP       = KEY_SWAP,
        CHS        = KEY_CHS,
        EEX        = KEY_E,
        BSP        = KEY_BSP,
        UP         = KEY_UP,
        KEY7       = KEY_7,
        KEY8       = KEY_8,
        KEY9       = KEY_9,
        DIV        = KEY_DIV,
        DOWN       = KEY_DOWN,
        KEY4       = KEY_4,
        KEY5       = KEY_5,
        KEY6       = KEY_6,
        MUL        = KEY_MUL,
        SHIFT      = KEY_SHIFT,
        KEY1       = KEY_1,
        KEY2       = KEY_2,
        KEY3       = KEY_3,
        SUB        = KEY_SUB,
        EXIT       = KEY_EXIT,
        KEY0       = KEY_0,
        DOT        = KEY_DOT,
        RUNSTOP    = KEY_RUN,
        ADD        = KEY_ADD,
        F1         = KEY_F1,
        F2         = KEY_F2,
        F3         = KEY_F3,
        F4         = KEY_F4,
        F5         = KEY_F5,
        F6         = KEY_F6,
        SCREENSHOT = KEY_SCREENSHOT,
        SH_UP      = KEY_SH_UP,
        SH_DOWN    = KEY_SH_DOWN,

        A          = KEY_SIGMA,
        B          = KEY_INV,
        C          = KEY_SQRT,
        D          = KEY_LOG,
        E          = KEY_LN,
        F          = KEY_XEQ,
        G          = KEY_STO,
        H          = KEY_RCL,
        I          = KEY_RDN,
        J          = KEY_SIN,
        K          = KEY_COS,
        L          = KEY_TAN,
        M          = KEY_SWAP,
        N          = KEY_CHS,
        O          = KEY_E,
        P          = KEY_7,
        Q          = KEY_8,
        R          = KEY_9,
        S          = KEY_DIV,
        T          = KEY_4,
        U          = KEY_5,
        V          = KEY_6,
        W          = KEY_MUL,
        X          = KEY_1,
        Y          = KEY_2,
        Z          = KEY_3,
        UNDER      = KEY_SUB,
        COLON      = KEY_0,
        COMMA      = KEY_DOT,
        SPACE      = KEY_RUN,
        QUESTION   = KEY_ADD,

        // Special stuff
        CLEAR      = 100,       // Clear the calculator state
        NOKEYS     = 101,       // Wait until keys buffer is empty
        REFRESH    = 102,       // Wait until there is a screen refresh
        KEYSYNC    = 103,       // Wait for other side to process keys
        LONGPRESS  = 104,       // Force long press
        EXIT_PGM   = 105,       // Exiting program

        // Reaching a specific shift state
        NOSHIFT    = 110,       // Clear shifts
        LSHIFT     = 111,       // Left shift only
        RSHIFT     = 112,       // Right shift only
        RESERVED1  = 113,       // Left + Right shift
        ALPHA      = 114,       // Set alpha
        ALPHA_LS   = 115,       // Alpha + left shift
        ALPHA_RS   = 116,       // Alpha + right shit
        RESERVED2  = 117,       // Alpha + left shift + right shift
        LOWERCASE  = 122,       // Lowercase
        LOWER_LS   = 123,       // Lowercase with left shift
        LOWER_RS   = 124,       // Lowercase with right shift

    };

  protected:
    struct failure
    {
        failure(cstring     file,
                uint        line,
                cstring     test,
                cstring     step,
                std::string explanation,
                uint        ti,
                uint        si,
                int         ci)
            : file(file),
              line(line),
              test(test),
              step(step),
              explanation(explanation),
              tindex(ti),
              sindex(si),
              cindex(ci)
        {
        }
        cstring     file;
        uint        line;
        cstring     test;
        cstring     step;
        std::string explanation;
        uint        tindex;
        uint        sindex;
        uint        cindex;
    };

public:
    struct WAIT
    {
        WAIT(uint ms): delay(ms) {}
        uint delay;
    };

    // Naming / identifying tests
    tests &begin(cstring name);
    tests &istep(cstring name);
    tests &position(cstring file, uint line);
    tests &check(bool test);
    tests &fail();
    tests &summary();
    tests &show(failure &f, cstring &last, uint &line);
    tests &show(failure &f);

    // Used to build the tests
    tests &itest(key k, bool release = true);
    tests &itest(unsigned int value);
    tests &itest(int value);
    tests &itest(unsigned long value);
    tests &itest(long value);
    tests &itest(unsigned long long value);
    tests &itest(long long value);
    tests &itest(char c);
    tests &itest(cstring alpha);
    tests &itest(WAIT delay);

    template <typename First, typename... Args>
    tests &itest(First first, Args... args)
    {
        return itest(first).itest(args...);
    }

    tests &clear();
    tests &nokeys();
    tests &refreshed();
    tests &ready();
    tests &shifts(bool lshift, bool rshift, bool alpha, bool lowercase);
    tests &wait(uint ms);
    tests &want(cstring output);
    tests &expect(cstring output);
    tests &expect(int output);
    tests &expect(unsigned int output);
    tests &expect(long output);
    tests &expect(unsigned long output);
    tests &expect(long long output);
    tests &expect(unsigned long long output);
    tests &match(cstring regexp);
    tests &image(cstring name, int x=0, int y=0, int w=LCD_W, int h=LCD_H);
    tests &image_noheader(cstring name);
    tests &type(object::id ty);
    tests &shift(bool s);
    tests &xshift(bool x);
    tests &alpha(bool a);
    tests &lower(bool l);
    tests &editing();
    tests &editing(size_t length);
    tests &editor(cstring text);
    tests &cursor(size_t csr);
    tests &error(cstring msg);
    tests &noerr()      { return error(nullptr); }
    tests &data_entry_noerr();
    tests &command(cstring msg);
    tests &source(cstring msg);

    template<typename ...Args>
    tests &explain(Args... args)
    {
        if (explanation.length())
            explanation += "\n";
        explanation += file;
        explanation += ":";
        explanation += std::to_string(line);
        explanation += ":    ";
        return explain_more(args...);
    }

    template<typename T>
    tests &explain_more(T t)
    {
        std::ostringstream out;
        out << t;
        explanation += out.str();
        return *this;
    }

    template <typename T, typename ...Args>
    tests &explain_more(T t, Args... args)
    {
        explain_more(t);
        return explain_more(args...);
    }

    template<typename ...Args>
    tests &check(bool test, Args... args)
    {
        if (!test)
            explain(args...);
        return check(test);
    }

    bool image_match(cstring file, int x, int y, int w, int h, bool force);

protected:
    cstring              file;
    uint                 line;
    cstring              tname;
    cstring              sname;
    uint                 tindex;
    uint                 sindex;
    uint                 cindex;
    uint                 count;
    uint                 refresh;
    int                  lcd_update;
    int                  last_key;
    bool                 ok;
    bool                 longpress;
    std::vector<failure> failures;
    std::string          explanation;
};

#define step(...)       position(__FILE__, __LINE__).istep(__VA_ARGS__)
#define test(...)       position(__FILE__, __LINE__).itest(__VA_ARGS__)

// Synchronization between test thread and RPL thread
extern volatile uint keysync_sent;
extern volatile uint keysync_done;

#endif // TESTS_H
