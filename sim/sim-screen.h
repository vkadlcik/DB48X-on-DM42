#ifndef SIM_SCREEN_H
#define SIM_SCREEN_H
// ****************************************************************************
//  screen.h                                                      DB48X project
// ****************************************************************************
//
//   File Description:
//
//     Simulate the screen of the DM42
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

#include <QGraphicsView>
#include <QGraphicsPixmapItem>
#include <QTimer>


extern int lcd_needsupdate;
extern uint8_t lcd_buffer[];

class Screen : public QGraphicsView
// ----------------------------------------------------------------------------
//    Screen emulation
// ----------------------------------------------------------------------------
{
    Q_OBJECT;

public:
    int                  screen_width;
    int                  screen_height;
    qreal                scale;

    QColor               bgColor;
    QColor               fgColor;
    QPen                 bgPen;
    QPen                 fgPen;

    QTimer               screenTimer;
    QGraphicsScene       screen;
    QGraphicsPixmapItem *mainScreen;
    QPixmap              mainPixmap;

  public:
    explicit Screen(QWidget *parent = 0);
    ~Screen();

public:
    void                 setPixel(int x, int y, int on);
    void                 setScale(qreal _scale);

public slots:
    void update();
};

#endif // SIMSCREEN_H
