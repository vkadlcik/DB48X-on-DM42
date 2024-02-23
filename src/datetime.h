#ifndef DATETIME_H
#define DATETIME_H
// ****************************************************************************
//  datetime.h                                                    DB48X project
// ****************************************************************************
//
//   File Description:
//
//     Date and time related functions
//
//
//
//
//
//
//
//
// ****************************************************************************
//   (C) 2024 Christophe de Dinechin <christophe@dinechin.org>
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

#include "algebraic.h"
#include "command.h"
#include "types.h"

#include <dmcp.h>


// Convert time and date to their components
bool to_time(object_p time, tm_t &tm);
uint to_date(object_p date, dt_t &dt, tm_t &tm);

// Convert date value to Julian day number, or 0 if fails
ularge julian_day_number(algebraic_p date);
ularge julian_day_number(int d, int m, int y);

// Convert algebraic to HMS or DMS
algebraic_p to_hms_dms(algebraic_r x);

// Convert the top of stack ot HMS or DMS unit
object::result to_hms_dms(cstring name);

// Convert a value from HMS or DMS input
algebraic_p from_hms_dms(algebraic_g x, cstring name);

// Convert the top of stack from HMS or DMS unit
object::result from_hms_dms(cstring name);

// Rendering
void render_time(renderer &r, algebraic_g &value,
                 cstring hrs, cstring min, cstring sec,
                 uint base, bool ampm);
size_t render_dms(renderer &r, algebraic_g value,
                  cstring deg, cstring min, cstring sec);
size_t render_date(renderer &r, algebraic_g date);



// System date and time
COMMAND_DECLARE(Date);          // Return current date
COMMAND_DECLARE(SetDate);       // Set current date
COMMAND_DECLARE(Time);          // Return current time
COMMAND_DECLARE(SetTime);       // Set current time
COMMAND_DECLARE(DateTime);      // Return current date and time

// HMS and DMS operations
COMMAND_DECLARE(ToHMS);         // Convert from decimal to H:MM:SS format
COMMAND_DECLARE(FromHMS);       // Convert from H:MM:SS format to decimal
COMMAND_DECLARE(ToDMS);         // Convert from decimal to H:MM:SS format
COMMAND_DECLARE(FromDMS);       // Convert from H:MM:SS format to decimal
COMMAND_DECLARE(HMSAdd);        // Add numbers in HMS format
COMMAND_DECLARE(HMSSub);        // Subtract numbers in HMS format
COMMAND_DECLARE(DMSAdd);        // Add numbers in HMS format
COMMAND_DECLARE(DMSSub);        // Subtract numbers in HMS format

#endif // DATETIME_H
