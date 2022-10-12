// ****************************************************************************
//  screen.cpp                                                    DB48X project
// ****************************************************************************
//
//   File Description:
//
//
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

#include "sim-screen.h"

#include <QBitmap>
#include <QGraphicsPixmapItem>
#include <QTimer>
#include <dmcp.h>
#include <target.h>


Screen::Screen(QWidget *parent)
// ----------------------------------------------------------------------------
//   Initialize the screen
// ----------------------------------------------------------------------------
    : QGraphicsView(parent),
      screen_width(LCD_W),
      screen_height(LCD_H),
      scale(1),
      bgColor(230, 230, 230),
      fgColor(0, 0, 0),
      screenTimer(new QTimer(this)),
      mainPixmap(LCD_W, LCD_H)
{
    connect(&screenTimer, SIGNAL(timeout()), this, SLOT(update()));

    bgPen.setColor(bgColor);
    bgPen.setStyle(Qt::NoPen);
    bgPen.setWidthF(0.05);

    screen.clear();
    screen.setBackgroundBrush(QBrush(Qt::black));

    mainScreen = screen.addPixmap(mainPixmap);
    mainScreen->setOffset(0.0, 0.0);

    setScene(&screen);
    setSceneRect(0, -5, screen_width, screen_height + 5);
    centerOn(qreal(screen_width) / 2, qreal(screen_height) / 2);
    scale = 1.0;
    setScale(4.0);

    show();
}


Screen::~Screen()
// ----------------------------------------------------------------------------
//   Screen destructor
// ----------------------------------------------------------------------------
{
}


void Screen::setPixel(int x, int y, int on)
// ----------------------------------------------------------------------------
//    Set the given screen
// ----------------------------------------------------------------------------
{
    // Pixels[offset]->setBrush(GrayBrush[color & 15]);

    QPainter pt(&mainPixmap);
    pt.setPen(on ? fgPen : bgPen);
    pt.drawPoint(LCD_W - x, y);
}


void Screen::setScale(qreal sf)
// ----------------------------------------------------------------------------
//   Adjust the scaling factor
// ----------------------------------------------------------------------------
{
    QGraphicsView::scale(sf / scale, sf / scale);
    this->scale = sf;

    QSize s;
    s.setWidth(0);
    s.setHeight((screen_height + 5) * scale);
    setMinimumSize(s);
}


void Screen::update()
// ----------------------------------------------------------------------------
//   Refresh the screen
// ----------------------------------------------------------------------------
{
    if (lcd_needsupdate)
    {
        lcd_needsupdate = 0;
    }
    else
    {
        screenTimer.setSingleShot(true);
        screenTimer.start(20);
        return;
    }

    // Monochrome screen
    screen.setBackgroundBrush(QBrush(bgColor));
    QPainter      pt(&mainPixmap);

    for (int y = 0; y < LCD_H; y++)
    {
        for (int x = 0; x < LCD_W; x++)
        {
            unsigned bo = y * LCD_SCANLINE + x;
            int on = (lcd_buffer[bo/8] >> (bo % 8)) & 1;
            setPixel(x, y, on);
        }
    }
    pt.end();

    mainScreen->setPixmap(mainPixmap);
    QGraphicsView::update();
    screenTimer.setSingleShot(true);
    screenTimer.start(20);
}
