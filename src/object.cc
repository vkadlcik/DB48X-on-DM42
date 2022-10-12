// ****************************************************************************
//  object.cc                                                     DB48X project
// ****************************************************************************
//
//   File Description:
//
//     Runtime support for objects
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

#include <object.h>
#include <runtime.h>
#include <stdio.h>


runtime &object::RT = runtime::RT;


const object::handler_fn object::handler[] =
// ----------------------------------------------------------------------------
//   The list of all possible handler
// ----------------------------------------------------------------------------
{
    object::handle
};


const size_t object::handlers =
// ----------------------------------------------------------------------------
//   Size of the handler table
// ----------------------------------------------------------------------------
    sizeof(object::handler) / sizeof(object::handler[0]);


int object::handle(runtime &rt,
                   command  cmd,
                   void    *arg,
                   object  *obj,
                   object  *payload)
// ----------------------------------------------------------------------------
//   Default handler for object
// ----------------------------------------------------------------------------
{
    switch(cmd)
    {
    case EVAL:
        rt.error("Invalid object");
        return -1;
    case SIZE:
        return payload - obj;
    case PARSE:
        return parser::SKIP;
    case RENDER:
    {
        renderer *out = (renderer *) arg;
        return snprintf(out->begin, out->end - out->begin, "<Unknown object %p>", obj);
    }
    default:
        rt.error("Invalid command for default object");
        return -1;
    }
}
