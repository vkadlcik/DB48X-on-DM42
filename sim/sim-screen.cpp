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

#include "dmcp.h"
#include "sim-dmcp.h"

#include <QBitmap>
#include <QGraphicsPixmapItem>
#include <QTimer>
#include <target.h>


SimScreen *SimScreen::theScreen = nullptr;


SimScreen::SimScreen(QWidget *parent)
// ----------------------------------------------------------------------------
//   Initialize the screen
// ----------------------------------------------------------------------------
    : QGraphicsView(parent),
      screen_width(SIM_LCD_W),
      screen_height(SIM_LCD_H),
      scale(1),
      bgColor(230, 230, 230),
      fgColor(0, 0, 0),
      bgPen(bgColor),
      fgPen(fgColor),
      mainPixmap(SIM_LCD_W, SIM_LCD_H),
      redraws(0)
{
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

    theScreen = this;
}


SimScreen::~SimScreen()
// ----------------------------------------------------------------------------
//   SimScreen destructor
// ----------------------------------------------------------------------------
{
}


void SimScreen::setPixel(int x, int y, int on)
// ----------------------------------------------------------------------------
//    Set the given screen
// ----------------------------------------------------------------------------
{
    // Pixels[offset]->setBrush(GrayBrush[color & 15]);
    QPainter pt(&mainPixmap);
    pt.setPen(on ? fgPen : bgPen);
    pt.drawPoint(SIM_LCD_W - x, y);
    pt.end();
}


void SimScreen::setScale(qreal sf)
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


void SimScreen::update()
// ----------------------------------------------------------------------------
//   Refresh the screen
// ----------------------------------------------------------------------------
{
    // Monochrome screen
    screen.setBackgroundBrush(QBrush(fgColor));
    QPainter      pt(&mainPixmap);

    for (int y = 0; y < SIM_LCD_H; y++)
    {
        for (int x = 0; x < SIM_LCD_W; x++)
        {
            unsigned bo = y * SIM_LCD_SCANLINE + x;
            int on = (lcd_buffer[bo/8] >> (bo % 8)) & 1;
            pt.setPen(on ? bgPen : fgPen);
            pt.drawPoint(SIM_LCD_W - x, y);
        }
    }
    pt.end();

    mainScreen->setPixmap(mainPixmap);
    QGraphicsView::update();
    redraws++;
}
