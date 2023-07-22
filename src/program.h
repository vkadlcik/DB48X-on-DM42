#ifndef PROGRAM_H
#define PROGRAM_H
// ****************************************************************************
//  program.h                                                     DB48X project
// ****************************************************************************
//
//   File Description:
//
//     Implementation of RPL programs and blocks
//
//     Programs are lists with a special way to execute
//
//
//
//
//
//
// ****************************************************************************
//   (C) 2023 Christophe de Dinechin <christophe@dinechin.org>
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

#include "list.h"

GCP(program);
GCP(block);

struct program : list
// ----------------------------------------------------------------------------
//   A program is a list with « and » as delimiters
// ----------------------------------------------------------------------------
{
    program(gcbytes bytes, size_t len, id type = ID_program)
        : list(bytes, len, type) {}

    static bool      interrupted(); // Program interrupted e.g. by EXIT key
    static program_p parse(utf8 source, size_t size);

public:
    OBJECT_DECL(program);
    PARSE_DECL(program);
    RENDER_DECL(program);
    EVAL_DECL(program);
    EXEC_DECL(program);
};
typedef const program *program_p;


struct block : program
// ----------------------------------------------------------------------------
//   A block inside a program, e.g. in loops
// ----------------------------------------------------------------------------
{
    block(gcbytes bytes, size_t len, id type = ID_block)
        : program(bytes, len, type) {}

public:
    OBJECT_DECL(block);
    PARSE_DECL(block);
    RENDER_DECL(block);
    EVAL_DECL(block);
};

#endif // PROGRAM_H
