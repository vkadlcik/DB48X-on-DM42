#ifndef SIM_WINDOW_H
#define SIM_WINDOW_H
// ****************************************************************************
//  sim-window.h                                                  DB48X project
// ****************************************************************************
//
//   File Description:
//
//     Main window of the simulator
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

#include <QMainWindow>
#include <QFile>
#include "sim-rpl.h"
#include "ui_sim-window.h"
#include "tests.h"


class TestsThread : public QThread
// ----------------------------------------------------------------------------
//   A thread to run the automated tests
// ----------------------------------------------------------------------------
{
public:
    TestsThread(QObject *parent): QThread(parent) {}
    ~TestsThread()     { while (!isFinished()) terminate(); }
    void run()
    {
        tests TestSuite;
        TestSuite.run();
    }
};


class Highlight : public QWidget
{
    Q_OBJECT;
public:
    Highlight(QWidget *parent): QWidget(parent) {}
    void paintEvent(QPaintEvent *);
};

class MainWindow : public QMainWindow
{
    Q_OBJECT;

    Ui::MainWindow ui;
    RPLThread      rpl;
    TestsThread    tests;

public:
    explicit MainWindow(QWidget *parent = 0);
    ~MainWindow();

    void pushKey(int key);
    static MainWindow * theMainWindow() { return mainWindow; }

protected:
    virtual void keyPressEvent(QKeyEvent *ev);
    virtual void keyReleaseEvent(QKeyEvent *ev);
    bool         eventFilter(QObject *obj, QEvent *ev);
    void         resizeEvent(QResizeEvent *event);

protected:
    static MainWindow *mainWindow;
    Highlight *highlight;
};

#endif // SIM_WINDOW_H
