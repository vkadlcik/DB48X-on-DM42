// ****************************************************************************
//  command.h                                                          DB48X project
// ****************************************************************************
//
//   File Description:
//
//     List of all commands for RPL opcodes
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

COMMAND(EVAL)                   // Evaluate the object (e.g. push on stack)
COMMAND(SIZE)                   // Compute the size of the object
COMMAND(PARSE)                  // Parse the object
COMMAND(RENDER)                 // Render the object

#undef COMMAND
