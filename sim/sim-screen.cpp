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

// A copy of the LCD buffer
uint8_t lcd_copy[LCD_SCANLINE * LCD_H / 8];


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

    mainPixmap.fill(bgColor);
    mainScreen = screen.addPixmap(mainPixmap);
    mainScreen->setOffset(0.0, 0.0);

    setScene(&screen);
    setSceneRect(0, -5, screen_width, screen_height + 5);
    centerOn(qreal(screen_width) / 2, qreal(screen_height) / 2);
    scale = 1.0;
    setScale(4.0);

    for (size_t i = 0; i < sizeof(lcd_copy); i++)
        lcd_copy[i] = ~lcd_buffer[i];

    show();

    theScreen = this;
}


SimScreen::~SimScreen()
// ----------------------------------------------------------------------------
//   SimScreen destructor
// ----------------------------------------------------------------------------
{
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


void SimScreen::updatePixmap()
// ----------------------------------------------------------------------------
//   Recompute the pixmap
// ----------------------------------------------------------------------------
//   This should be done on the RPL thread to get a consistent picture
{
    // Monochrome screen
    QPainter pt(&mainPixmap);
    for (int y = 0; y < SIM_LCD_H; y++)
    {
        for (int xb = 0; xb < SIM_LCD_W/8; xb++)
        {
            unsigned byteoffs = y * (SIM_LCD_SCANLINE/8) + xb;
            if (uint8_t diffs = lcd_copy[byteoffs] ^ lcd_buffer[byteoffs])
            {
                for (int bit = 0; bit < 8; bit++)
                {
                    if ((diffs >> bit) & 1)
                    {
                        int on = (lcd_buffer[byteoffs] >> bit) & 1;
                        pt.setPen(on ? bgPen : fgPen);
                        pt.drawPoint(SIM_LCD_W - (8*xb + bit), y);
                    }
                }
                lcd_copy[byteoffs] = lcd_buffer[byteoffs];
            }
        }
    }
    pt.end();
}


void SimScreen::refreshScreen()
// ----------------------------------------------------------------------------
//   Transfer the pixmap to the screen
// ----------------------------------------------------------------------------
//   This must be done on the main screen
{
    mainScreen->setPixmap(mainPixmap);
    QGraphicsView::update();
    redraws++;
}
