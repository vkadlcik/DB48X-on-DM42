// ****************************************************************************
//  compare.cc                                                    DB48X project
// ****************************************************************************
//
//   File Description:
//
//     Implementation of comparisons
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

#include "compare.h"

#include "decimal-32.h"
#include "decimal-64.h"
#include "decimal128.h"
#include "integer.h"

object::result comparison::condition(bool &value, object_p cond)
// ----------------------------------------------------------------------------
//   Check if an object is true or false
// ----------------------------------------------------------------------------
{
    if (cond)
    {
        switch(cond->type())
        {
        case ID_hex_integer:
        case ID_oct_integer:
        case ID_bin_integer:
        case ID_dec_integer:
        case ID_integer:
        case ID_neg_integer:
            value = integer_p(cond)->value<ularge>() != 0;
            return OK;
        case ID_decimal128:
            value = !decimal128_p(cond)->is_zero();
            return OK;
        case ID_decimal64:
            value = !decimal64_p(cond)->is_zero();
            return OK;
        case ID_decimal32:
            value = !decimal32_p(cond)->is_zero();
            return OK;
        case ID_True:
            value = true;
            return OK;
        case ID_False:
            value = false;
            return OK;
        default:
            RT.type_error();
        }
    }
    return ERROR;
}


template <typename Cmp>
object::result comparison::evaluate()
// ----------------------------------------------------------------------------
//   The actual evaluation for all binary operators
// ----------------------------------------------------------------------------
{
        return compare(Cmp::make_result);
}


object::result comparison::compare(int *cmp, object_p left, object_p right)
// ----------------------------------------------------------------------------
//   Compare objects left and right, return -1, 0 or +1
// ----------------------------------------------------------------------------
{
    gcobj y = left;
    gcobj x = right;
    if (!x || !y)
        return ERROR;

    id xt = x->type();
    id yt = y->type();

    /* Integer types */
    bool ok = false;

    if (!ok && is_integer(xt) && is_integer(yt))
    {
        // Check if we have a neg_integer vs another integer type
        if ((xt == ID_neg_integer) != (yt == ID_neg_integer))
        {
            *cmp = xt == ID_neg_integer ? 1 : -1;
            return OK;
        }

        integer_p xi     = integer_p(object_p(x));
        integer_p yi     = integer_p(object_p(y));
        ularge    xv     = xi->value<ularge>();
        ularge    yv     = yi->value<ularge>();
        int       cmpval = yv < xv ? -1 : yv > xv ? 1 : 0;
        if (xt == ID_neg_integer)
            cmpval = -cmpval;
        *cmp = cmpval;
        return OK;
    }

    /* Real data types */
    if (!ok && real_promotion(x, y))
    {
        /* Here, x and y have the same type, a decimal type */
        int rlt = 0;
        int rgt = 0;
        xt = x->type();
        switch(xt)
        {
        case ID_decimal32:
        {
            bid32 xv = decimal32_p(object_p(x))->value();
            bid32 yv = decimal32_p(object_p(y))->value();
            bid32_quiet_unordered(&rlt, &yv.value, &xv.value);
            if (rlt)
                return ERROR;
            bid32_quiet_less(&rlt, &yv.value, &xv.value);
            bid32_quiet_greater(&rgt, &yv.value, &xv.value);
            break;
        }
        case ID_decimal64:
        {
            bid64 xv = decimal64_p(object_p(x))->value();
            bid64 yv = decimal64_p(object_p(y))->value();
            bid64_quiet_unordered(&rlt, &yv.value, &xv.value);
            if (rlt)
                return ERROR;
            bid64_quiet_less(&rlt, &yv.value, &xv.value);
            bid64_quiet_greater(&rgt, &yv.value, &xv.value);
            break;
        }
        case ID_decimal128:
        {
            bid128 xv = decimal128_p(object_p(x))->value();
            bid128 yv = decimal128_p(object_p(y))->value();
            bid128_quiet_unordered(&rlt, &yv.value, &xv.value);
            if (rlt)
                return ERROR;
            bid128_quiet_less(&rlt, &yv.value, &xv.value);
            bid128_quiet_greater(&rgt, &yv.value, &xv.value);
            break;
        }
        default:
            return ERROR;
        }
        *cmp = rgt - rlt;
        return OK;
    }

    if (!ok && xt == ID_text && yt == ID_text)
    {
        // Lexical comparison
        size_t xl = 0;
        size_t yl = 0;
        utf8 xs = text_p(object_p(x))->value(&xl);
        utf8 ys = text_p(object_p(y))->value(&yl);
        size_t l = xl < yl ? xl : yl;

        // REVISIT: Unicode sorting?
        for (uint k = 0; k < l; k++)
        {
            if (int d = ys[k] - xs[k])
            {
                *cmp = (d > 0) - (d < 0);
                return OK;
            }
        }

        *cmp = (yl > xl) - (yl < xl);
        return OK;
    }

    // All other cases are errors
   return ERROR;
}


object::result comparison::compare(comparison_fn comparator)
// ----------------------------------------------------------------------------
//   Compare items from the stack
// ----------------------------------------------------------------------------
{
    object_p y = RT.stack(1);
    object_p x = RT.stack(0);
    if (!x || !y)
        return ERROR;

    int cmp = 0;
    result r = compare(&cmp, y, x);
    if (r != OK)
    {
        RT.type_error();
        return r;
    }

    RT.pop();
    RT.pop();
    id type = comparator(cmp) ? ID_True : ID_False;
    if (RT.push(command::static_object(type)))
        return OK;
    return ERROR;
}


template<>
object::result comparison::evaluate<TestSame>()
// ----------------------------------------------------------------------------
//   For "same", we want the same type, no promotion
// ----------------------------------------------------------------------------
{
    object_p y = RT.stack(1);
    object_p x = RT.stack(0);
    if (!x || !y)
        return ERROR;

    // Check that the objects are strictly identical
    bool same = false;
    id xt = x->type();
    id yt = y->type();
    if (xt == yt)
    {
        size_t xs = x->size();
        size_t ys = y->size();
        if (xs == ys)
            same = memcmp(x, y, xs) == 0;
    }
    RT.pop();
    RT.pop();
    id type = same ? ID_True : ID_False;
    if (RT.push(command::static_object(type)))
        return OK;
    return ERROR;
}




// ============================================================================
//
//   Instatiations
//
// ============================================================================

template object::result comparison::evaluate<TestLT>();
template object::result comparison::evaluate<TestLE>();
template object::result comparison::evaluate<TestEQ>();
template object::result comparison::evaluate<TestGT>();
template object::result comparison::evaluate<TestGE>();
template object::result comparison::evaluate<TestNE>();
