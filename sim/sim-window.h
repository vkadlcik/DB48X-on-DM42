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



class MainWindow : public QMainWindow
{
    Q_OBJECT;

    Ui::MainWindow ui;
    RPLThread      rpl;

  public:
    explicit MainWindow(QWidget *parent = 0);
    ~MainWindow();

  protected:
    virtual void keyPressEvent(QKeyEvent *ev);
    virtual void keyReleaseEvent(QKeyEvent *ev);
    bool         eventFilter(QObject *obj, QEvent *ev);
    void         resizeEvent(QResizeEvent *event);
};

#endif // SIM_WINDOW_H
