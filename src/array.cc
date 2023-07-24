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

RECORDER(matrix, 16, "Determinant computation");
RECORDER(matrix_error, 16, "Errors in matrix computations");



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


static bool add_sub_dimension(size_t rx, size_t cx,
                              size_t ry, size_t cy,
                              size_t *rr, size_t *cr)
// ----------------------------------------------------------------------------
//   For addition and subtraction, we need identical dimensions
// ----------------------------------------------------------------------------
{
    *rr = cx;
    *cr = rx;
    return cx == cy && rx == ry;
}


static algebraic_g matrix_op(object::id op,
                             size_t r, size_t c,
                             size_t rx, size_t cx,
                             size_t ry, size_t cy)
// ----------------------------------------------------------------------------
//   Perform a matrix component-wise operation
// ----------------------------------------------------------------------------
{
    size_t py = cx*rx;
    size_t px = py + cy*ry;
    size_t i = r * cx + c;
    object_p x = rt.stack(px + ~i);
    object_p y = rt.stack(py + ~i);
    if (!x || !y)
        return nullptr;
    algebraic_g xa = x->as_algebraic();
    algebraic_g ya = y->as_algebraic();
    if (!xa || !ya)
    {
        rt.type_error();
        return nullptr;
    }
    switch(op)
    {
    case object::ID_add: return xa + ya;
    case object::ID_sub: return xa - ya;
    case object::ID_mul: return xa * ya;
    case object::ID_div: return xa / ya;
    default:             rt.type_error(); return nullptr;
    }
}


static algebraic_g vector_op(object::id op, size_t c, size_t cx, size_t cy)
// ----------------------------------------------------------------------------
//   Add two elements in a vector
// ----------------------------------------------------------------------------
{
    return matrix_op(op, 0, c, 1, cx, 1, cy);
}


static algebraic_g vector_add(size_t c, size_t cx, size_t cy)
// ----------------------------------------------------------------------------
//   Addition of vector elements
// ----------------------------------------------------------------------------
{
    return vector_op(object::ID_add, c, cx, cy);
}


static algebraic_g matrix_add(size_t r, size_t c,
                              size_t rx, size_t cx,
                              size_t ry, size_t cy)
// ----------------------------------------------------------------------------
//   Addition of matrix elements
// ----------------------------------------------------------------------------
{
    return matrix_op(object::ID_add, r, c, rx, cx, ry, cy);
}


static algebraic_g vector_sub(size_t c, size_t cx, size_t cy)
// ----------------------------------------------------------------------------
//   Subtraction of vector elements
// ----------------------------------------------------------------------------
{
    return vector_op(object::ID_sub, c, cx, cy);
}


static algebraic_g matrix_sub(size_t r, size_t c,
                              size_t rx, size_t cx,
                              size_t ry, size_t cy)
// ----------------------------------------------------------------------------
//   Subtraction of matrix elements
// ----------------------------------------------------------------------------
{
    return matrix_op(object::ID_sub, r, c, rx, cx, ry, cy);
}


static bool mul_dimension(size_t rx, size_t cx,
                          size_t ry, size_t cy,
                          size_t *rr, size_t *cr)
// ----------------------------------------------------------------------------
//   For multiplication, need matching rows and columns
// ----------------------------------------------------------------------------
{
    *rr = rx;
    *cr = cy;
    return cx == ry;
}


static algebraic_g vector_mul(size_t c, size_t cx, size_t cy)
// ----------------------------------------------------------------------------
//   Multiplication of vector elements
// ----------------------------------------------------------------------------
{
    return vector_op(object::ID_mul, c, cx, cy);
}


static algebraic_g matrix_mul(size_t r, size_t c,
                              size_t rx, size_t cx,
                              size_t ry, size_t cy)
// ----------------------------------------------------------------------------
//   Compute one element in matrix multiplication
// ----------------------------------------------------------------------------
{
    size_t py = cy*ry;
    size_t px = py + cx*rx;

    algebraic_g e;
    if (ry != cx)
        record(matrix_error,
               "Inconsistent matrix size rx=%u cx=%u ry=%u cy%=u",
               rx, cx, ry, cy);

    for (size_t i = 0; i < cx; i++)
    {
        size_t ix = r * cx + i;
        size_t iy = cy * i + c;
        object_p x = rt.stack(px + ~ix);
        object_p y = rt.stack(py + ~iy);
        if (!x || !y)
            return nullptr;
        algebraic_g xa = x->as_algebraic();
        algebraic_g ya = y->as_algebraic();
        if (!xa || !ya)
        {
            rt.type_error();
            return nullptr;
        }
        e = e + xa * ya;
        if (!e)
            return nullptr;
    }
    return e;
}


static bool div_dimension(size_t rx, size_t cx,
                          size_t ry, size_t cy,
                          size_t *rr, size_t *cr)
// ----------------------------------------------------------------------------
//   For division, not yet implemented
// ----------------------------------------------------------------------------
{
    return false; // Not yet
}


static algebraic_g vector_div(size_t c, size_t cx, size_t cy)
// ----------------------------------------------------------------------------
//   Division of vector elements
// ----------------------------------------------------------------------------
{
    return vector_op(object::ID_div, c, cx, cy);
}


static algebraic_g matrix_div(size_t r, size_t c,
                              size_t rx, size_t cx,
                              size_t ry, size_t cy)
// ----------------------------------------------------------------------------
//   Compute one element in matrix division
// ----------------------------------------------------------------------------
{
    return matrix_op(object::ID_div, r, c, rx, cx, ry, cy);
}


array_g array::do_matrix(array_r x, array_r y,
                         dimension_fn dim, vector_fn vec, matrix_fn mat)
// ----------------------------------------------------------------------------
//   Perform a matrix or vector operation
// ----------------------------------------------------------------------------
{
    size_t rx = 0, cx = 0, ry = 0, cy = 0, rr = 0, cr = 0;
    size_t depth = rt.depth();
    object::id ty = x->type();
    if (x->is_vector(&cx))
    {
        if (!y->is_vector(&cy))
        {
            rt.type_error();
            goto err;
        }
        if (!dim(1, cx, 1, cy, &rr, &cr))
        {
            rt.dimension_error();
            goto err;
        }

        scribble scr;
        for (size_t c = 0; c < cx; c++)
        {
            algebraic_g e = vec(c, cx, cy);
            if (!e || !rt.append(e->size(), byte_p(e.Safe())))
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
        if (!dim(rx, cx, ry, cy, &rr, &cr))
        {
            rt.dimension_error();
            goto err;
        }

        scribble scr;
        for (size_t r = 0; r < rr; r++)
        {
            array_g row = nullptr;
            {
                scribble sr;
                for (size_t c = 0; c < cr; c++)
                {
                    algebraic_g e = mat(r, c, rx, cx, ry, cy);
                    if (!e || !rt.append(e->size(), byte_p(e.Safe())))
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


array_g operator+(array_r x, array_r y)
// ----------------------------------------------------------------------------
//   Add two arrays
// ----------------------------------------------------------------------------
{
    return array::do_matrix(x, y, add_sub_dimension, vector_add, matrix_add);
}


array_g operator-(array_r x, array_r y)
// ----------------------------------------------------------------------------
//   Subtract two arrays
// ----------------------------------------------------------------------------
{
    return array::do_matrix(x, y, add_sub_dimension, vector_sub, matrix_sub);
}


array_g operator*(array_r x, array_r y)
// ----------------------------------------------------------------------------
//   Multiply two arrays
// ----------------------------------------------------------------------------
{
    return array::do_matrix(x, y, mul_dimension, vector_mul, matrix_mul);
}


array_g operator/(array_r x, array_r y)
// ----------------------------------------------------------------------------
//   Divide two arrays
// ----------------------------------------------------------------------------
{
    return array::do_matrix(x, y, div_dimension, vector_div, matrix_div);
}
