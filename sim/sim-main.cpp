// ****************************************************************************
//  main.cpp                                                      DB48X project
// ****************************************************************************
//
//   File Description:
//
//     DM42 simulator for the DB48 project
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

#include "recorder.h"
#include "sim-rpl.h"
#include "sim-window.h"

#include <QApplication>
#include <QWindow>

RECORDER(options, 32, "Information about command line options");

bool run_tests = false;

int main(int argc, char *argv[])
// ----------------------------------------------------------------------------
//   Main entry point for the simulator
// ----------------------------------------------------------------------------
{
    const char *traces = getenv("DB48X_TRACES");
    recorder_trace_set(".*(error|warning)s?");
    if (traces)
        recorder_trace_set(traces);
    recorder_dump_on_common_signals(0, 0);
    record(options,
           "Simulator invoked as %+s with %d arguments", argv[0], argc-1);
    for (int a = 1; a < argc; a++)
    {
        record(options, "  %u: %+s", a, argv[a]);
        if (argv[a][0] == '-' && argv[a][1] == 't')
            recorder_trace_set(argv[a]+2);
        if (argv[a][0] == '-' && argv[a][1] == 'T')
            run_tests = true;
    }

#if QT_VERSION < 0x060000
    QCoreApplication::setAttribute(Qt::AA_EnableHighDpiScaling);
#endif // QT version 6

    QApplication a(argc, argv);
    MainWindow w;
    w.show();
    return a.exec();
}
