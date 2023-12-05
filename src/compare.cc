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
#include "expression.h"
#include "integer.h"
#include "locals.h"


template <typename Cmp>
object::result comparison::evaluate()
// ----------------------------------------------------------------------------
//   The actual evaluation for all binary operators
// ----------------------------------------------------------------------------
{
    return compare(Cmp::make_result, Cmp::static_id);
}


template <typename Cmp>
algebraic_g comparison::evaluate(algebraic_r x, algebraic_r y)
// ----------------------------------------------------------------------------
//   The actual evaluation for all binary operators
// ----------------------------------------------------------------------------
{
    return compare(Cmp::make_result, Cmp::static_id, x, y);
}


bool comparison::compare(int *cmp, algebraic_r x, algebraic_r y)
// ----------------------------------------------------------------------------
//   Compare objects left and right, return -1, 0 or +1
// ----------------------------------------------------------------------------
{
    // Check if we had some error earlier, if so propagate
    if (!x.Safe() || !y.Safe())
        return false;
    id xt = x->type();
    id yt = y->type();

    /* Integer types */
    bool ok = false;

    if (is_integer(xt) && is_integer(yt))
    {
        // Check if this is a bignum comparison
        if (is_bignum(xt) || is_bignum(yt))
        {
            algebraic_g xa = algebraic_p(x.Safe());
            algebraic_g ya = algebraic_p(y.Safe());
            if (!is_bignum(xt))
                xt = bignum_promotion(xa);
            if (!is_bignum(yt))
                yt = bignum_promotion(ya);
            bignum_g xb = bignum_p(xa.Safe());
            bignum_g yb = bignum_p(ya.Safe());
            int cmpval = bignum::compare(xb, yb);
            *cmp = cmpval < 0 ? -1 : cmpval > 0 ? 1 : 0;
            return true;
        }

        // Check if we have a neg_integer vs another integer type
        if ((xt == ID_neg_integer) != (yt == ID_neg_integer))
        {
            *cmp = xt == ID_neg_integer ? -1 : 1;
            return true;
        }

        integer_p xi     = integer_p(object_p(x));
        integer_p yi     = integer_p(object_p(y));
        ularge    xv     = xi->value<ularge>();
        ularge    yv     = yi->value<ularge>();
        int       cmpval = xv < yv ? -1 : xv > yv ? 1 : 0;
        if (xt == ID_neg_integer)
            cmpval = -cmpval;
        *cmp = cmpval;
        return true;
    }

    /* Real data types */
    algebraic_g xa = algebraic_p(x.Safe());
    algebraic_g ya = algebraic_p(y.Safe());
    if (!ok && real_promotion(xa, ya))
    {
        /* Here, x and y have the same type, a decimal type */
        int rlt = 0;
        int rgt = 0;
        xt = xa->type();
        switch(xt)
        {
#ifndef CONFIG_NO_DECIMAL32
        case ID_decimal32:
        {
            bid32 xv = decimal32_p(object_p(xa))->value();
            bid32 yv = decimal32_p(object_p(ya))->value();
            bid32_quiet_unordered(&rlt, &xv.value, &yv.value);
            if (rlt)
                return false;
            bid32_quiet_less(&rlt, &xv.value, &yv.value);
            bid32_quiet_greater(&rgt, &xv.value, &yv.value);
            break;
        }
#endif // CONFIG_NO_DECIMAL32
#ifndef CONFIG_NO_DECIMAL64
        case ID_decimal64:
        {
            bid64 xv = decimal64_p(object_p(xa))->value();
            bid64 yv = decimal64_p(object_p(ya))->value();
            bid64_quiet_unordered(&rlt, &xv.value, &yv.value);
            if (rlt)
                return false;
            bid64_quiet_less(&rlt, &xv.value, &yv.value);
            bid64_quiet_greater(&rgt, &xv.value, &yv.value);
            break;
        }
#endif // CONFIG_NO_DECIMAL64
#ifndef CONFIG_NO_DECIMAL128
        case ID_decimal128:
        {
            bid128 xv = decimal128_p(object_p(xa))->value();
            bid128 yv = decimal128_p(object_p(ya))->value();
            bid128_quiet_unordered(&rlt, &xv.value, &yv.value);
            if (rlt)
                return false;
            bid128_quiet_less(&rlt, &xv.value, &yv.value);
            bid128_quiet_greater(&rgt, &xv.value, &yv.value);
            break;
        }
#endif // CONFIG_NO_DECIMAL128
        default:
            return false;
        }
        *cmp = rgt - rlt;
        return true;
    }

    if (!ok && ((xt == ID_text && yt == ID_text) ||
                (xt == ID_symbol && yt == ID_symbol)))
    {
        // Lexical comparison
        size_t xl = 0;
        size_t yl = 0;
        utf8 xs = text_p(object_p(x.Safe()))->value(&xl);
        utf8 ys = text_p(object_p(y.Safe()))->value(&yl);
        size_t l = xl < yl ? xl : yl;

        // REVISIT: Unicode sorting?
        for (uint k = 0; k < l; k++)
        {
            if (int d = xs[k] - ys[k])
            {
                *cmp = (d > 0) - (d < 0);
                return true;
            }
        }

        *cmp = (xl > yl) - (xl < yl);
        return true;
    }

    if (!ok && ((xt == ID_list && yt == ID_list) ||
                (xt == ID_array && yt == ID_array)))
    {
        list_p xl = list_p(x.Safe());
        list_p yl = list_p(y.Safe());
        list::iterator xi = xl->begin();
        list::iterator xe = xl->end();
        list::iterator yi = yl->begin();
        list::iterator ye = yl->end();

        // Lexicographic comparison of arrays and lists
        while(xi != xe && yi != ye)
        {
            object_p xo = *xi++;
            object_p yo = *yi++;
            if (xo->is_algebraic() && yo->is_algebraic())
            {
                algebraic_g xa = algebraic_p(xo);
                algebraic_g ya = algebraic_p(yo);
                if (compare(cmp, xa, ya))
                    if (*cmp)
                        return true;
            }
            else if (int d = xo->compare_to(yo))
            {
                *cmp = d;
                return true;
            }
        }
        *cmp = (xi != xe) - (yi != ye);
        return true;
    }

    // All other cases are errors
    return false;
}


object::result comparison::compare(comparison_fn comparator, id op)
// ----------------------------------------------------------------------------
//   Compare items from the stack
// ----------------------------------------------------------------------------
{
    if (!rt.args(2))
        return  ERROR;

    object_p x = rt.stack(1);
    object_p y = rt.stack(0);
    if (!x || !y)
        return ERROR;
    if (!x->is_algebraic() || !y->is_algebraic())
    {
        rt.type_error();
        return ERROR;
    }

    algebraic_g xa = algebraic_p(x);
    algebraic_g ya = algebraic_p(y);
    algebraic_g ra = compare(comparator, op, xa, ya);

    if (ra)
        if (rt.drop(2))
            if (rt.push(ra.Safe()))
                return OK;

    return ERROR;
}


algebraic_g comparison::compare(comparison_fn comparator,
                                id            op,
                                algebraic_r   x,
                                algebraic_r   y)
// ----------------------------------------------------------------------------
//   Compare two algebraic values without using the stack
// ----------------------------------------------------------------------------
{
    int cmp = 0;
    if (compare(&cmp, x, y))
    {
        // Could evaluate the result, return True or False
        id type = comparator(cmp) ? ID_True : ID_False;
        return algebraic_p(command::static_object(type));
    }

    // Otherwise, need to build an equation with the comparison
    expression_p eq = expression::make(op, x, y);
    return eq;
}


object::result comparison::is_same(bool names)
// ----------------------------------------------------------------------------
//   Check if two objects are strictly identical
// ----------------------------------------------------------------------------
//   If 'names' is true, evaluate names (behavior of '==' aka TestSame)
//   If 'names' is false, do not evaluate names (behavior of 'same')
{
    object_p y = rt.stack(1);
    object_p x = rt.stack(0);
    if (!x || !y)
        return ERROR;

    // Check that the objects are strictly identical
    bool same = false;
    id xt = x->type();
    id yt = y->type();

    if (names && xt != yt)
    {
        if (xt == ID_symbol)
        {
            x = ((symbol_p) x)->recall();
            xt = x->type();
        }
        else if (xt == ID_local)
        {
            x = ((local_p) x)->recall();
            xt = x->type();
        }

        if (yt == ID_symbol)
        {
            y = ((symbol_p) y)->recall();
            yt = y->type();
        }
        else if (yt == ID_local)
        {
            y = ((local_p) y)->recall();
            yt = y->type();
        }

    }

    if (xt == yt)
    {
        size_t xs = x->size();
        size_t ys = y->size();
        if (xs == ys)
            same = memcmp(x, y, xs) == 0;
    }
    rt.pop();
    rt.pop();
    id type = same ? ID_True : ID_False;
    if (rt.push(command::static_object(type)))
        return OK;
    return ERROR;
}


template<>
object::result comparison::evaluate<TestSame>()
// ----------------------------------------------------------------------------
//   For "==", we want the same type, no promotion, but evaluate names
// ----------------------------------------------------------------------------
{
    return is_same(true);
}


template<>
object::result comparison::evaluate<same>()
// ----------------------------------------------------------------------------
//   For "same", we want the same type, no promotion
// ----------------------------------------------------------------------------
{
    return is_same(false);
}


bool smaller_magnitude(algebraic_r x, algebraic_r y)
// ----------------------------------------------------------------------------
//   Compare magnitude
// ----------------------------------------------------------------------------
{
    algebraic_p cmp = abs::run(x) < abs::run(y);
    return cmp && cmp->as_truth(false);
}



// ============================================================================
//
//   Instantiations
//
// ============================================================================

template object::result comparison::evaluate<TestLT>();
template object::result comparison::evaluate<TestLE>();
template object::result comparison::evaluate<TestEQ>();
template object::result comparison::evaluate<TestGT>();
template object::result comparison::evaluate<TestGE>();
template object::result comparison::evaluate<TestNE>();



// ============================================================================
//
//    Commands for True and False
//
// ============================================================================

COMMAND_BODY(True)
// ----------------------------------------------------------------------------
//   Evaluate as self
// ----------------------------------------------------------------------------
{
    if (rt.args(0))
        if (rt.push(command::static_object(ID_True)))
            return OK;
    return ERROR;
}

COMMAND_BODY(False)
// ----------------------------------------------------------------------------
//   Evaluate as self
// ----------------------------------------------------------------------------
{
    if (rt.args(0))
        if (rt.push(command::static_object(ID_False)))
            return OK;
    return ERROR;
}



// ============================================================================
//
//   C++ interface
//
// ============================================================================

algebraic_g operator==(algebraic_r x, algebraic_r y)
// ----------------------------------------------------------------------------
//   Equality operation on algebraic objects
// ----------------------------------------------------------------------------
{
    return TestEQ::evaluate(x, y);
}


algebraic_g operator<=(algebraic_r x, algebraic_r y)
// ----------------------------------------------------------------------------
//   Less or equal operation on algebraic objects
// ----------------------------------------------------------------------------
{
    return TestLE::evaluate(x, y);
}


algebraic_g operator>=(algebraic_r x, algebraic_r y)
// ----------------------------------------------------------------------------
//   Gretter or equal operation on algebraic objects
// ----------------------------------------------------------------------------
{
    return TestGE::evaluate(x, y);
}


algebraic_g operator!=(algebraic_r x, algebraic_r y)
// ----------------------------------------------------------------------------
//   Inequality operation on algebraic objects
// ----------------------------------------------------------------------------
{
    return TestNE::evaluate(x, y);
}


algebraic_g operator<(algebraic_r x, algebraic_r y)
// ----------------------------------------------------------------------------
//   Less  operation on algebraic objects
// ----------------------------------------------------------------------------
{
    return TestLT::evaluate(x, y);
}


algebraic_g operator>(algebraic_r x, algebraic_r y)
// ----------------------------------------------------------------------------
//   Gretter operation on algebraic objects
// ----------------------------------------------------------------------------
{
    return TestGT::evaluate(x, y);
}
