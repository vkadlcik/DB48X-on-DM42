#ifndef TYPES_H
#define TYPES_H
// ****************************************************************************
//  types.h                                                       DB48X project
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

#include <cstdint>
#include <cstddef>



// ============================================================================
//
//    Basic data types
//
// ============================================================================

typedef unsigned           uint;
typedef uint8_t            byte;
typedef unsigned long long ularge;
typedef long long          ilarge;
typedef const char        *cstring;
typedef const char        *utf8;
typedef unsigned           utf8code;

// Indicate that an argument may be unused
#define UNUSED          __attribute__((unused))

#endif // TYPES_H
