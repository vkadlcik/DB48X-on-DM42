#ifndef SIM_RPL_H
#define SIM_RPL_H
// ****************************************************************************
//  sim-rpl.h                                                     DB48X project
// ****************************************************************************
//
//   File Description:
//
//     The thread running the RPL program
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

#include <QThread>
#include <QTimer>

class RPLThread:public QThread
// ----------------------------------------------------------------------------
//    Thread running the RPL program
// ----------------------------------------------------------------------------
{
public:
    RPLThread(QObject * parent);
    ~RPLThread();

    void run();
};

#endif // RPLTHREAD_H
