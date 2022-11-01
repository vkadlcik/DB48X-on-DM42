// ****************************************************************************
//  algebraic.cc                                                DB48X project
// ****************************************************************************
//
//   File Description:
//
//     Shared code for all algebraic commands
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

#include "algebraic.h"

#include "arithmetic.h"
#include "integer.h"
#include "parser.h"
#include "renderer.h"
#include "runtime.h"
#include "settings.h"
#include "stack-cmds.h"

#include <ctype.h>
#include <stdio.h>


RECORDER(algebraic,       16, "RPL Algebraics");
RECORDER(algebraic_error, 16, "Errors processing a algebraic");


OBJECT_HANDLER_BODY(algebraic)
// ----------------------------------------------------------------------------
//    RPL handler for algebraics
// ----------------------------------------------------------------------------
{
    record(algebraic, "Algebraic %+s on %p", object::name(op), obj);
    switch(op)
    {
    case EXEC:
    case EVAL:
        record(algebraic_error, "Invoked default algebraic handler");
        rt.error("Algebraic is not implemented");
        return ERROR;

    default:
        // Check if anyone else knows how to deal with it
        return DELEGATE(command);
    }
}


bool algebraic::real_promotion(gcobj &x, object::id type)
// ----------------------------------------------------------------------------
//   Promote the value x to the given type
// ----------------------------------------------------------------------------
{
    object::id xt = x->type();
    if (xt == type)
        return true;

    record(algebraic, "Real promotion of %p from %+s to %+s",
           (object_p) x, object::name(xt), object::name(type));
    runtime &rt = runtime::RT;
    switch(xt)
    {
    case ID_integer:
    {
        integer_p i    = x->as<integer>();
        ularge    ival = i->value<ularge>();
        switch (type)
        {
        case ID_decimal32:
            x = rt.make<decimal32>(ID_decimal32, ival);
            return true;
        case ID_decimal64:
            x = rt.make<decimal64>(ID_decimal64, ival);
            return true;
        case ID_decimal128:
            x = rt.make<decimal128>(ID_decimal128, ival);
            return true;
        default:
            break;
        }
        record(algebraic_error,
               "Cannot promote integer %p (%llu) from %+s to %+s",
               i, ival, object::name(xt), object::name(type));
    }
    case ID_neg_integer:
    {
        integer_p i    = x->as<neg_integer>();
        large     ival = -i->value<large>();
        switch (type)
        {
        case ID_decimal32:
            x = rt.make<decimal32>(ID_decimal32, ival);
            return true;
        case ID_decimal64:
            x = rt.make<decimal64>(ID_decimal64, ival);
            return true;
        case ID_decimal128:
            x = rt.make<decimal128>(ID_decimal128, ival);
            return true;
        default:
            break;
        }
        record(algebraic_error,
               "Cannot promote neg_integer %p (%lld) from %+s to %+s",
               i, ival, object::name(xt), object::name(type));
    }

    case ID_decimal32:
    {
        decimal32_p d = x->as<decimal32>();
        bid32       dval = d->value();
        switch (type)
        {
        case ID_decimal64:
            x = rt.make<decimal64>(ID_decimal64, dval);
            return true;
        case ID_decimal128:
            x = rt.make<decimal128>(ID_decimal128, dval);
            return true;
        default:
            break;
        }
        record(algebraic_error,
               "Cannot promote decimal32 %p from %+s to %+s",
               d, object::name(xt), object::name(type));
    }

    case ID_decimal64:
    {
        decimal64_p d = x->as<decimal64>();
        bid64       dval = d->value();
        switch (type)
        {
        case ID_decimal64:
            x = rt.make<decimal64>(ID_decimal64, dval);
            return true;
        case ID_decimal128:
            x = rt.make<decimal128>(ID_decimal128, dval);
            return true;
        default:
            break;
        }
        record(algebraic_error,
               "Cannot promote decimal64 %p from %+s to %+s",
               d, object::name(xt), object::name(type));
    }
    default:
        break;
    }

    return false;
}


object::id algebraic::real_promotion(gcobj &x)
// ----------------------------------------------------------------------------
//   Promote the value x to a type selected based on preferences
// ----------------------------------------------------------------------------
{
    // Auto-selection of type
    uint16_t prec = Settings.precision;
    id       type = prec > BID64_MAXDIGITS ? ID_decimal128
                  : prec > BID32_MAXDIGITS ? ID_decimal64
                                           : ID_decimal32;
    return real_promotion(x, type) ? type : ID_object;
}



// ============================================================================
//
//   Simple operators
//
// ============================================================================

ALGEBRAIC_BODY(inv)
// ----------------------------------------------------------------------------
//   Invert is implemented as 1/x
// ----------------------------------------------------------------------------
{
    // Apparently there is a div function getting in the way, see man div(3)
    using div = struct div;
    RT.push(RT.make<integer>(ID_integer, 1));
    run<Swap>();
    run<div>();
    return OK;
}


ALGEBRAIC_BODY(neg)
// ----------------------------------------------------------------------------
//   Negate is implemented as 0-x
// ----------------------------------------------------------------------------
{
    RT.push(RT.make<integer>(ID_integer, 0));
    run<Swap>();
    run<sub>();
    return OK;
}


ALGEBRAIC_BODY(sq)
// ----------------------------------------------------------------------------
//   Square is implemented as "dup mul"
// ----------------------------------------------------------------------------
{
    run<Dup>();
    run<mul>();
    return OK;
}


ALGEBRAIC_BODY(cubed)
// ----------------------------------------------------------------------------
//   Cubed is implemented as "dup dup mul mul"
// ----------------------------------------------------------------------------
{
    run<Dup>();
    run<Dup>();
    run<mul>();
    run<mul>();
    return OK;
}
