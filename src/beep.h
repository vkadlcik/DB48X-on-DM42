#ifndef BEEP_H
#define BEEP_H
// ****************************************************************************
//  beep.h                                                        DB48X project
// ****************************************************************************
//
//   File Description:
//
//     Emit a beep
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

#include <rpl.h>

inline void beep(int freq, int dur)
// ----------------------------------------------------------------------------
//
// ----------------------------------------------------------------------------
{
    start_buzzer_freq(freq * 1000);
    sys_delay(dur);
    stop_buzzer();
}



#endif // BEEP_H
