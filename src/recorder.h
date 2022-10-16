#ifndef RECORDER_DB48X_H
#define RECORDER_DB48X_H
// ****************************************************************************
//  recorder.h                                                    DB48X project
// ****************************************************************************
//
//   File Description:
//
//     Include the flight recorder when building on simulator
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

#ifdef SIMULATOR
#include "../recorder/recorder.h"
#else
// The DM42 has so little memory (70K) that we can't use it for recorders
#define RECORDER(Name, Size, Info)
#define RECORDER_TRACE(Name)    0
#define RECORDER_ENABLED(Name)  0
#define RECORDER_DEFINE(Name, Size, Info)
#define RECORDER_DECLARE(Name)
#define RECORDER_TWEAK_DECLARE(Name)
#define RECORDER_TWEAK_DEFINE(Name, Value, Info)
#define RECORDER_TWEAK(Name)    0
#define RECORD(...)     do { } while(0)
#define record(...)     do { } while(0)
#endif

#endif // RECORDER_DB48X_H
