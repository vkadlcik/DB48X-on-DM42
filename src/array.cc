// ****************************************************************************
//  array.cc                                                      DB48X project
// ****************************************************************************
//
//   File Description:
//
//     Implementation of arrays (vectors, matrices and maybe tensors)
//
//
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

#include "array.h"
#include "arithmetic.h"



// ============================================================================
//
//    Array
//
// ============================================================================

PARSE_BODY(array)
// ----------------------------------------------------------------------------
//    Try to parse this as a program
// ----------------------------------------------------------------------------
{
    return list::list_parse(ID_array, p, '[', ']');
}


RENDER_BODY(array)
// ----------------------------------------------------------------------------
//   Render the program into the given program buffer
// ----------------------------------------------------------------------------
{
    return o->list_render(r, '[', ']');
}


bool array::is_vector(size_t *size) const
// ----------------------------------------------------------------------------
//   Check if this is a vector, and if so, push all elements on stack
// ----------------------------------------------------------------------------
{
    bool result = type() == ID_array;
    if (result)
    {
        size_t count = 0;
        for (object_p obj : *this)
        {
            id oty = obj->type();
            if (oty == ID_array || oty == ID_list)
                result = false;
            else if (!rt.push(obj))
                result = false;
            if (!result)
                break;
            count++;
        }
        if (!result)
            rt.drop(count);
        else
            *size = count;
    }
    return result;
}


bool array::is_matrix(size_t *rows, size_t *cols) const
// ----------------------------------------------------------------------------
//   Check if this is a vector, and if so, push all elements on stack
// ----------------------------------------------------------------------------
{
    bool result = type() == ID_array;
    if (result)
    {
        size_t depth = rt.depth();
        size_t r = 0;
        size_t c = 0;
        bool first = true;

        for (object_p robj : *this)
        {
            id oty = robj->type();
            result = oty == ID_array;
            if (result)
            {
                size_t rcol = 0;
                result = array_p(robj)->is_vector(&rcol);
                if (result && first)
                    c = rcol;
                else if (rcol != c)
                    result = false;
                first = false;
            }
            if (!result)
                break;
            r++;
        }
        if (!result)
        {
            rt.drop(rt.depth() - depth);
        }
        else
        {
            *rows = r;
            *cols = c;
        }
    }
    return result;
}


array_g operator-(array_r x)
// ----------------------------------------------------------------------------
//   Negate all elements in an array
// ----------------------------------------------------------------------------
{
    return array::map(neg::evaluate, x);
}

array_g operator+(array_r x, array_r y)
// ----------------------------------------------------------------------------
//   Add two arrays
// ----------------------------------------------------------------------------
{
    size_t rx = 0, cx = 0, ry = 0, cy = 0;
    size_t depth = rt.depth();
    object::id ty = x->type();
    if (x->is_vector(&cx))
    {
        if (!y->is_vector(&cy))
        {
            rt.type_error();
            goto err;
        }
        if (cx != cy)
        {
            rt.dimension_error();
            goto err;
        }

        scribble scr;
        for (size_t c = 0; c < cx; c++)
        {
            algebraic_g xi = rt.stack(cx + cy + ~c)->as_algebraic();
            algebraic_g yi = rt.stack(cy + ~c)->as_algebraic();
            if (!xi || !yi)
            {
                rt.type_error();
                goto err;
            }
            xi = xi + yi;
            if (!rt.append(xi->size(), byte_p(xi.Safe())))
                goto err;
        }

        rt.drop(rt.depth() - depth);
        return array_p(list::make(ty, scr.scratch(), scr.growth()));
    }

    if (x->is_matrix(&rx, &cx))
    {
        if (!y->is_matrix(&ry, &cy))
        {
            rt.type_error();
            goto err;
        }
        if (cx != cy || rx != ry)
        {
            rt.dimension_error();
            goto err;
        }

        scribble scr;
        size_t py = cx*rx;
        size_t px = py + cy*ry;
        for (size_t r = 0; r < rx; r++)
        {
            array_g row = nullptr;
            {
                scribble sr;
                for (size_t c = 0; c < cx; c++)
                {
                    size_t i = r * cx + c;
                    algebraic_g xi = rt.stack(px + ~i)->as_algebraic();
                    algebraic_g yi = rt.stack(py + ~i)->as_algebraic();
                    if (!xi || !yi)
                    {
                        rt.type_error();
                        goto err;
                    }
                    xi = xi + yi;
                    if (!rt.append(xi->size(), byte_p(xi.Safe())))
                        goto err;
                }
                row = array_p(list::make(ty, sr.scratch(), sr.growth()));
            }
            if (!row)
                goto err;
            if (!rt.append(row->object::size(), byte_p(row.Safe())))
                goto err;
        }

        rt.drop(rt.depth() - depth);
        return array_p(list::make(ty, scr.scratch(), scr.growth()));
    }

err:
    rt.drop(rt.depth() - depth);
    return nullptr;
}


array_g operator-(array_r x, array_r y);
array_g operator*(array_r x, array_r y);
array_g operator/(array_r x, array_r y);
