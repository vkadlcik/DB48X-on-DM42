// ****************************************************************************
//  stats.cc                                                      DB48X project
// ****************************************************************************
//
//   File Description:
//
//     Implementation of statistics functions
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

#include "stats.h"

#include "arithmetic.h"
#include "compare.h"
#include "integer.h"
#include "tag.h"
#include "variables.h"


// ============================================================================
//
//   Stats parameters access
//
// ============================================================================

StatsParameters::Access::Access()
// ----------------------------------------------------------------------------
//   Default values
// ----------------------------------------------------------------------------
    : model(command::ID_LinearFit),
      xcol(1),
      ycol(2),
      intercept(integer::make(0)),
      slope(integer::make(0))
{
    parse(name());
}


StatsParameters::Access::~Access()
// ----------------------------------------------------------------------------
//   Save values on exit
// ----------------------------------------------------------------------------
{
    write();
}


object_p StatsParameters::Access::name()
// ----------------------------------------------------------------------------
//   Return the name for the variable
// ----------------------------------------------------------------------------
{
    return command::static_object(ID_StatsParameters);

}


bool StatsParameters::Access::parse(list_p parms)
// ----------------------------------------------------------------------------
//   Parse a stats parameters list
// ----------------------------------------------------------------------------
{
    if (!parms)
        return false;

    uint index = 0;
    object::id type;
    for (object_p obj: *parms)
    {
        bool valid = true;
        switch(index)
        {
        case 0:
        case 1:
            (index ? ycol : xcol) = obj->as_uint32(1, true);
            valid = !rt.error();
            break;
        case 2:
        case 3:
            valid = obj->is_real() || obj->is_complex();
            if (valid)
                (index == 2 ? intercept : slope) = algebraic_p(obj);
            break;
        case 4:
            type = obj->type();
            valid = (type >= object::ID_LinearFit &&
                     type <= object::ID_LogarithmicFit);
            if (valid)
                model = type;
            break;
        default:
            valid = false;
            break;
        }
        if (!valid)
        {
            rt.invalid_stats_parameters_error();
            return false;
        }
        index++;
    }
    return true;
}


bool StatsParameters::Access::parse(object_p name)
// ----------------------------------------------------------------------------
//   Parse stats parameters from a variable name
// ----------------------------------------------------------------------------
{
    if (object_p obj = directory::recall_all(name, false))
        if (list_p parms = obj->as<list>())
            return parse(parms);
    return false;
}


bool StatsParameters::Access::write(object_p name) const
// ----------------------------------------------------------------------------
//   Write stats parameters back to variable
// ----------------------------------------------------------------------------
{
    if (directory *dir = rt.variables(0))
    {
        integer_g xc = integer::make(xcol);
        integer_g yc = integer::make(ycol);
        object_g  m  = command::static_object(model);
        object_g par = list::make(xc, yc, slope, intercept, m);
        if (par)
            return dir->store(name, par);
    }
    return false;
}



// ============================================================================
//
//   Stats data access
//
// ============================================================================

StatsData::Access::Access()
// ----------------------------------------------------------------------------
//   Default values, load variable if it exists
// ----------------------------------------------------------------------------
    : data(), original_data(), columns(), rows()
{
    parse(name());
}


StatsData::Access::~Access()
// ----------------------------------------------------------------------------
//   Save values on exit
// ----------------------------------------------------------------------------
{
    write();
}


object_p StatsData::Access::name()
// ----------------------------------------------------------------------------
//   Return the name for the variable
// ----------------------------------------------------------------------------
{
    return command::static_object(ID_StatsData);
}


bool StatsData::Access::parse(array_p values)
// ----------------------------------------------------------------------------
//   Parse a stats data array
// ----------------------------------------------------------------------------
//   We want a rectangular data array with only numerical values
{
    if (!values)
        return false;

    columns = 0;
    rows    = 0;

    for (object_p row : *values)
    {
        if (array_p ra = row->as<array>())
        {
            size_t ccount = 0;
            for (object_p column : *ra)
            {
                ccount++;
                if (!column->is_real() && !column->is_complex())
                    goto err;
            }
            if (rows > 0 && columns != ccount)
                goto err;
            columns = ccount;
        }
        else
        {
            if (rows > 0 && columns != 1)
                goto err;
            if (!row->is_real() && !row->is_complex())
                goto err;
            columns = 1;
        }
        rows++;
    }

    data = values;
    return true;

err:
    rt.invalid_stats_data_error();
    return false;
}


bool StatsData::Access::parse(object_p name)
// ----------------------------------------------------------------------------
//   Parse stats data from a variable name
// ----------------------------------------------------------------------------
{
    if (object_p obj = directory::recall_all(name, false))
    {
        object::id oty = obj->type();
        if (oty == object::ID_text || oty == object::ID_symbol)
        {
            obj = directory::recall_all(obj, true);
            if (!obj)
                return false;
        }

        if (array_p values = obj->as<array>())
        {
            if (parse(values))
            {
                original_data = data;
                return true;
            }
        }
    }

    return false;
}


bool StatsData::Access::write(object_p name) const
// ----------------------------------------------------------------------------
//   Write statistical data to variable or disk
// ----------------------------------------------------------------------------
{
    if (data && +data != +original_data)
    {
        if (directory *dir = rt.variables(0))
        {
            if (object_p existing = dir->recall_all(name, false))
            {
                object::id nty = existing->type();
                if (nty == object::ID_text || nty == object::ID_symbol)
                    name = existing;
            }
            return dir->store(name, +data);
        }
    }
    return false;
}




// ============================================================================
//
//   Statistics data entry
//
// ============================================================================

COMMAND_BODY(AddData)
// ----------------------------------------------------------------------------
//   Add data to the stats data
// ----------------------------------------------------------------------------
{
    if (rt.args(1))
    {
        if (object_p value = rt.top())
        {
            size_t columns = 1;
            if (array_p row = value->as<array>())
            {
                columns = 0;
                for (object_p item : *row)
                {
                    columns++;
                    if (!item->is_real() && !item->is_complex())
                    {
                        rt.invalid_stats_data_error();
                        return ERROR;
                    }
                }
            }
            else if (value->is_real() || value->is_complex())
            {
                value = array::wrap(value);
            }
            else
            {
                rt.type_error();
                return ERROR;
            }

            StatsData::Access stats;
            if (stats.rows && columns != stats.columns)
            {
                rt.invalid_stats_data_error();
                return ERROR;
            }

            if (!stats.data)
                stats.data = array_p(array::make(ID_array, nullptr, 0));
            stats.data = stats.data->append(value);
            rt.drop();
            return OK;
        }
    }

    return ERROR;
}


COMMAND_BODY(RemoveData)
// ----------------------------------------------------------------------------
//   Remove data from the statistics data
// ----------------------------------------------------------------------------
{
    StatsData::Access stats;
    if (stats.rows >= 1)
    {
        size_t   size   = 0;
        object_p first  = stats.data->objects(&size);
        size_t   offset = 0;
        object_p last   = first;
        object_p obj    = first;
        while (offset < size)
        {
            size_t osize = obj->size();
            last = obj;
            obj += osize;
            offset += osize;
        }

        object_g removed = rt.clone(last);
        if (!rt.push(removed))
            return ERROR;

        size = last - first;
        stats.data = array_p(array::make(ID_array, byte_p(first), size));
        return OK;
    }
    rt.invalid_stats_data_error();
    return ERROR;
}



COMMAND_BODY(RecallData)
// ----------------------------------------------------------------------------
//  Recall stats data
// ----------------------------------------------------------------------------
{
    if (directory *dir = rt.variables(0))
        if (object_p value = dir->recall(command::static_object(ID_StatsData)))
            if (rt.push(value))
                return OK;
    return ERROR;
}



COMMAND_BODY(StoreData)
// ----------------------------------------------------------------------------
//   Store stats data
// ----------------------------------------------------------------------------
{
    if (rt.args(1))
    {
        if (object_p obj = rt.top())
        {
            id ty = obj->type();
            if (ty == ID_array)
            {
                StatsData::Access stats;
                if (stats.parse(array_p(obj)))
                {
                    rt.clear_error();
                    rt.drop();
                    return OK;
                }
            }
            else if (ty == ID_text || ty == ID_symbol)
            {
                if (directory *dir = rt.variables(0))
                {
                    if (dir->store(command::static_object(ID_StatsData), obj))
                    {
                        rt.drop();
                        return OK;
                    }
                }
            }
            else
            {
                rt.type_error();
            }
        }
    }
    return ERROR;
}


COMMAND_BODY(ClearData)
// ----------------------------------------------------------------------------
//  Clear statistics data
// ----------------------------------------------------------------------------
{
    StatsData::Access stats;
    stats.data = array_p(array::make(ID_array, nullptr, 0));
    return OK;
}



// ============================================================================
//
//    Basic analysis of the data
//
// ============================================================================

algebraic_p StatsAccess::fit_transform(algebraic_r x, uint col) const
// ----------------------------------------------------------------------------
//   Adjust data to be able to perform standard linear interpolation
// ----------------------------------------------------------------------------
//   There are four curve fitting models:
//   1. Linear fit:     y = a*x + b
//   2. Exp fit:        y = b * exp(a*x)
//   3. Log fit:        y = a * ln(x) + b
//   4. Power fit:      y = x ^ a * b
//
//   In order to find the best fit, data is adjusted during processing:
//   1. Linear fit:     no change
//   2. Exp fit:        ln(y) = a*x + ln(b)
//   3. Log fit:        y = a*ln(x) + b
//   4. Power fit:      ln(y) = a*ln(x) + ln(b)
{
    bool dolog = false;
    switch (model)
    {
    default:
    case object::ID_LinearFit:                                          break;
    case object::ID_ExponentialFit: dolog = col == ycol;                break;
    case object::ID_LogarithmicFit: dolog = col == xcol;                break;
    case object::ID_PowerFit:       dolog = col == xcol || col == ycol; break;
    }
    if (dolog)
        return log::evaluate(x);
    return x;
}


algebraic_p StatsAccess::num_rows() const
// ----------------------------------------------------------------------------
//   Return number of rows
// ----------------------------------------------------------------------------
{
    return integer::make(rows);
}


algebraic_p StatsAccess::sum(sum_fn op, uint scol) const
// ----------------------------------------------------------------------------
//   Run a sum on a single column
// ----------------------------------------------------------------------------
{
    algebraic_g s = integer::make(0);
    algebraic_g x;
    for (object_p row : *data)
    {
        if (array_p a = row->as<array>())
        {
            uint col = 1;
            for (object_p item : *a)
            {
                if (!item->is_real() && !item->is_complex())
                {
                    rt.invalid_stats_data_error();
                    return nullptr;
                }
                if (col == scol)
                {
                    x = algebraic_p(item);
                    x = fit_transform(x, scol);
                    s = op(s, x);
                    break;
                }
                col++;
            }
        }
        else if (scol == 1)
        {
            if (!row->is_real() && !row->is_complex())
            {
                rt.invalid_stats_data_error();
                return nullptr;
            }
            x = algebraic_p(row);
            x = fit_transform(x, scol);
            s = op(s, x);
        }
        else
        {
            break;
        }
    }
    return s;
}


algebraic_p StatsAccess::sum(sxy_fn op, uint xcol, uint ycol) const
// ----------------------------------------------------------------------------
//   Run a sum on a single column
// ----------------------------------------------------------------------------
{
    algebraic_g s = integer::make(0);
    algebraic_g x, y;
    for (object_p row : *data)
    {
        if (array_p a = row->as<array>())
        {
            size_t col = 1;
            x = nullptr;
            y = nullptr;
            for (object_p item : *a)
            {
                if (!item->is_real() && !item->is_complex())
                {
                    rt.invalid_stats_data_error();
                    return nullptr;
                }
                if (col == xcol)
                {
                    x = algebraic_p(item);
                    x = fit_transform(x, col);
                }
                if (col == ycol)
                {
                    y = algebraic_p(item);
                    y = fit_transform(y, col);
                }
                if (x && y)
                {
                    s = op(s, x, y);
                    break;
                }
                col++;
            }
        }
        else if (xcol == 1 && ycol == 1)
        {
            if (!row->is_real() && !row->is_complex())
            {
                rt.invalid_stats_data_error();
                return nullptr;
            }
            x = algebraic_p(row);
            y = x;
            x = fit_transform(x, 1);
            y = fit_transform(y, 1);
            s = op(s, x, y);
        }
        else
        {
            break;
        }
    }
    return s;
}


static algebraic_p sum1(algebraic_r s, algebraic_r x)
// ----------------------------------------------------------------------------
//   Simply add values
// ----------------------------------------------------------------------------
{
    return s + x;
}


static algebraic_p smallest(algebraic_r s, algebraic_r x)
// ----------------------------------------------------------------------------
//   Simply add values
// ----------------------------------------------------------------------------
{
    int test = 0;
    comparison::compare(&test, s, x);
    return test < 0 ? s : x;
}


static algebraic_p largest(algebraic_r s, algebraic_r x)
// ----------------------------------------------------------------------------
//   Simply add values
// ----------------------------------------------------------------------------
{
    int test = 0;
    comparison::compare(&test, s, x);
    return test > 0 ? s : x;
}


static algebraic_p sum2(algebraic_r s, algebraic_r x)
// ----------------------------------------------------------------------------
//   Add squares
// ----------------------------------------------------------------------------
{
    return s + x * x;
}


static algebraic_p sumxy(algebraic_r s, algebraic_r x, algebraic_r y)
// ----------------------------------------------------------------------------
//   Add squares
// ----------------------------------------------------------------------------
{
    return s + x * y;
}


algebraic_p StatsAccess::sum_x() const
// ----------------------------------------------------------------------------
//   Return the sum of values in the X column
// ----------------------------------------------------------------------------
{
    return sum(sum1, xcol);
}


algebraic_p StatsAccess::sum_y() const
// ----------------------------------------------------------------------------
//   Return the sum of values in the Y column
// ----------------------------------------------------------------------------
{
    return sum(sum1, ycol);
}


algebraic_p StatsAccess::sum_xy() const
// ----------------------------------------------------------------------------
//   Return the sum of product of values in X and Y column
// ----------------------------------------------------------------------------
{
    return sum(sumxy, xcol, ycol);
}


algebraic_p StatsAccess::sum_x2() const
// ----------------------------------------------------------------------------
//   Return the sum of squares of values in the X column
// ----------------------------------------------------------------------------
{
    return sum(sum2, xcol);
}


algebraic_p StatsAccess::sum_y2() const
// ----------------------------------------------------------------------------
//   Return the sum of squares of values in the Y column
// ----------------------------------------------------------------------------
{
    return sum(sum2, ycol);
}


algebraic_p StatsAccess::total(sum_fn op) const
// ----------------------------------------------------------------------------
//    Perform an iterative operation on all items
// ----------------------------------------------------------------------------
{
    algebraic_g result;
    algebraic_g row, x, y;
    array_g arow;
    for (object_p robj : *data)
    {
        object::id rty = robj->type();
        bool is_array = rty == object::ID_array;
        bool is_value = object::is_real(rty) || object::is_complex(rty);
        if (!is_value && !is_array)
        {
            rt.type_error();
            return nullptr;
        }

        if (is_array && columns == 1)
        {
            robj = array_p(robj)->objects();
            if (!robj)
                return nullptr;
            is_array = false;
        }
        row = algebraic_p(robj);
        if (result)
        {
            if (is_array)
            {
                array_g ra = array_p(robj);
                arow = array_p(array::make(object::ID_array, nullptr, 0));
                if (!arow)
                    return nullptr;
                if (array_p ares = result->as<array>())
                {
                    array::iterator ai = ares->begin();
                    for (object_p cobj : *ra)
                    {
                        object_p aobj = *ai++;
                        if (!aobj)
                            return nullptr;
                        x = aobj->as_algebraic();
                        y = cobj->as_algebraic();
                        if (!x || !y)
                            return nullptr;
                        x = op(x, y);
                        arow = arow->append(x);
                    }
                    row = +arow;
                }
                else
                {
                    rt.invalid_stats_data_error();
                    return nullptr;
                }
            }
            else
            {
                row = op(result, row);
            }
        }
        result = row;
    }
    return result;
}


algebraic_p StatsAccess::total(sxy_fn op, algebraic_r arg) const
// ----------------------------------------------------------------------------
//    Perform an iterative operation on all items
// ----------------------------------------------------------------------------
{
    algebraic_g result;
    algebraic_g row, x, y, a;
    array_g     arow;
    bool        arg_is_array = arg->type() == object::ID_array;
    for (object_p robj : *data)
    {
        object::id rty = robj->type();
        bool is_array = rty == object::ID_array;
        bool is_value = object::is_real(rty) || object::is_complex(rty);
        if (!is_value && !is_array)
        {
            rt.type_error();
            return nullptr;
        }

        if (is_array && columns == 1)
        {
            robj = array_p(robj)->objects();
            if (!robj)
                return nullptr;
            is_array = false;
        }
        row = algebraic_p(robj);
        if (is_array)
        {
            array_g ra = array_p(robj);
            arow = array_p(array::make(object::ID_array, nullptr, 0));
            if (!arow)
                return nullptr;
            array::iterator argi =
                arg_is_array ? array_p(+arg)->begin() : ra->begin();
            array_p ares = result ? result->as<array>() : nullptr;
            array::iterator ai = ares ? ares->begin() : ra->begin();
            for (object_p cobj : *ra)
            {
                object_p aobj = ares ? *ai++ : integer::make(0);
                if (!aobj)
                    return nullptr;
                x = aobj->as_algebraic();
                y = cobj->as_algebraic();
                if (!x || !y)
                    return nullptr;
                a = arg_is_array ? algebraic_p(*argi++) : +arg;
                x = op(x, y, a);
                if (!x)
                    return nullptr;
                arow = arow->append(x);
                if (!arow)
                    return nullptr;
            }
            row = +arow;
        }
        else if (result)
        {
            row = op(result, row, arg);
        }
        result = row;
    }
    return result;
}


algebraic_p StatsAccess::total() const
// ----------------------------------------------------------------------------
//  Perform a sum of the columns
// ----------------------------------------------------------------------------
{
    return total(sum1);
}


algebraic_p StatsAccess::min() const
// ----------------------------------------------------------------------------
//  Find the minimum of all columns
// ----------------------------------------------------------------------------
{
    return total(smallest);
}


algebraic_p StatsAccess::max() const
// ----------------------------------------------------------------------------
//  Find the maximum of all columns
// ----------------------------------------------------------------------------
{
    return total(largest);
}


algebraic_p StatsAccess::average() const
// ----------------------------------------------------------------------------
//   Compute the average value
// ----------------------------------------------------------------------------
{
    if (rows <= 0)
    {
        rt.insufficient_stats_data_error();
        return nullptr;
    }
    if (algebraic_g sum = total())
    {
        algebraic_g count = integer::make(rows);
        sum = sum / count;
        return sum;
    }
    return nullptr;
}


static algebraic_p do_variance(algebraic_r s, algebraic_r x, algebraic_r mean)
// ----------------------------------------------------------------------------
//   Compute the terms of the variance
// ----------------------------------------------------------------------------
{
    algebraic_g xdev = (x - mean);
    return s + xdev * xdev;
}


algebraic_p StatsAccess::variance() const
// ----------------------------------------------------------------------------
//   Compute the variance (used for `Variance` and `StandardDeviation`)
// ----------------------------------------------------------------------------
{
    if (rows <= 1)
    {
        rt.insufficient_stats_data_error();
        return nullptr;
    }
    if (algebraic_g mean = average())
    {
        algebraic_g sum = total(do_variance, mean);
        algebraic_g num = integer::make(rows - 1);
        sum = sum / num;
        return sum;
    }
    return nullptr;
}


algebraic_p StatsAccess::standard_deviation() const
// ----------------------------------------------------------------------------
//   Compute the standard deviation
// ----------------------------------------------------------------------------
{
    algebraic_g var = variance();
    if (array_p vara = var->as<array>())
        return vara->map(sqrt::evaluate);
    return sqrt::evaluate(var);
}


algebraic_p StatsAccess::correlation() const
// ----------------------------------------------------------------------------
//   Compute the correlation
// ----------------------------------------------------------------------------
{
    if (rows <= 0)
    {
        rt.insufficient_stats_data_error();
        return nullptr;
    }

    algebraic_g n     = integer::make(rows);
    algebraic_g avg_x = sum_x() / n;
    algebraic_g avg_y = sum_y() / n;
    algebraic_g num   = integer::make(0);
    algebraic_g den_x = num;
    algebraic_g den_y = num;
    algebraic_g x, y, sq;

    for (object_g row : *data)
    {
        array_g ra = row->as<array>();
        if (!ra)
        {
            rt.insufficient_stats_data_error();
            return nullptr;
        }
        size_t col = 1;
        x = nullptr;
        y = nullptr;
        for (object_g cobj : *ra)
        {
            if (col == xcol)
            {
                x = cobj->as_algebraic();
                x = fit_transform(x, col);
            }
            if (col == ycol)
            {
                y = cobj->as_algebraic();
                y = fit_transform(y, col);
            }
            if (x && y)
            {
                num = num + (x - avg_x) * (y - avg_y);
                sq = x - avg_x;
                den_x = den_x + sq * sq;
                sq = y - avg_y;
                den_y = den_y + sq * sq;
                break;
            }
            col++;
        }
    }

    return num / sqrt::evaluate(den_x * den_y);
}


algebraic_p StatsAccess::covariance(bool population) const
// ----------------------------------------------------------------------------
//   Compute the covariance
// ----------------------------------------------------------------------------
{
    if (rows <= 1)
    {
        rt.insufficient_stats_data_error();
        return nullptr;
    }
    algebraic_g n     = integer::make(rows);
    algebraic_g avg_x = sum_x() / n;
    algebraic_g avg_y = sum_y() / n;
    algebraic_g num   = integer::make(0);
    algebraic_g x, y;

    for (object_g row : *data)
    {
        array_g ra = row->as<array>();
        if (!ra)
        {
            rt.insufficient_stats_data_error();
            return nullptr;
        }
        size_t col = 1;
        x = nullptr;
        y = nullptr;
        for (object_g cobj : *ra)
        {
            if (col == xcol)
            {
                x = cobj->as_algebraic();
                x = fit_transform(x, col);
            }
            if (col == ycol)
            {
                y = cobj->as_algebraic();
                y = fit_transform(y, col);
            }
            if (x && y)
            {
                num = num + (x - avg_x) * (y - avg_y);
                break;
            }
            col++;
        }
    }

    n = integer::make(rows - !population);
    return num / n;
}


algebraic_p StatsAccess::covariance() const
// ----------------------------------------------------------------------------
//   Covariance
// ----------------------------------------------------------------------------
{
    return covariance(false);
}


algebraic_p StatsAccess::population_covariance() const
// ----------------------------------------------------------------------------
//   Compute the population covariance
// ----------------------------------------------------------------------------
{
    return covariance(true);
}


static algebraic_p do_popvar(algebraic_r s, algebraic_r x, algebraic_r mean)
// ----------------------------------------------------------------------------
//   Compute the terms of the population variance
// ----------------------------------------------------------------------------
{
    algebraic_g xdev = (x - mean);
    return s + xdev * xdev;
}


algebraic_p StatsAccess::population_variance() const
// ----------------------------------------------------------------------------
//   Compute the population variance
// ----------------------------------------------------------------------------
{
    if (rows <= 0)
    {
        rt.insufficient_stats_data_error();
        return nullptr;
    }
    if (algebraic_g mean = average())
    {
        algebraic_g sum = total(do_popvar, mean);
        algebraic_g num = integer::make(rows);
        sum = sum / num;
        return sum;
    }
    return nullptr;
}


algebraic_p StatsAccess::population_standard_deviation() const
// ----------------------------------------------------------------------------
//   Compute the population variance
// ----------------------------------------------------------------------------
{
    algebraic_g pvar = population_variance();
    if (array_p pvara = pvar->as<array>())
        return pvara->map(sqrt::evaluate);
    return sqrt::evaluate(pvar);
}


object::result StatsAccess::evaluate(eval_fn op, bool two_columns)
// ----------------------------------------------------------------------------
//   Evaluate a a given statistical function for RPL
// ----------------------------------------------------------------------------
{
    StatsAccess stats;
    if (!stats || (two_columns && !stats.two_columns()))
        return object::ERROR;

    algebraic_g value;
    object::id fit = stats.model;
    if (fit != object::ID_LinearFit && Settings.LinearFitSums())
        stats.model = object::ID_LinearFit;
    value = (stats.*op)();
    stats.model = fit;
    return value && rt.push(+value) ? object::OK : object::ERROR;
}



// ============================================================================
//
//   User-level data analysis commands
//
// ============================================================================

COMMAND_BODY(DataSize)
// ----------------------------------------------------------------------------
//   Return the number of entries in statistics data
// ----------------------------------------------------------------------------
{
    return StatsAccess::evaluate(&StatsAccess::num_rows, false);
}


COMMAND_BODY(Total)
// ----------------------------------------------------------------------------
//   Compute the sum of items
// ----------------------------------------------------------------------------
{
    return StatsAccess::evaluate(&StatsAccess::total, false);
}


COMMAND_BODY(Average)
// ----------------------------------------------------------------------------
//   Compute the mean of all input data
// ----------------------------------------------------------------------------
{
    return StatsAccess::evaluate(&StatsAccess::average, false);
}


COMMAND_BODY(Median)
// ----------------------------------------------------------------------------
//  Find the median of the input data
// ----------------------------------------------------------------------------
{
    rt.unimplemented_error();
    return ERROR;
}



COMMAND_BODY(MinData)
// ----------------------------------------------------------------------------
//  Find the minimum of all data
// ----------------------------------------------------------------------------
{
    return StatsAccess::evaluate(&StatsAccess::min, false);
}



COMMAND_BODY(MaxData)
// ----------------------------------------------------------------------------
//  Find the maximum of all data
// ----------------------------------------------------------------------------
{
    return StatsAccess::evaluate(&StatsAccess::max, false);
}


COMMAND_BODY(SumOfX)
// ----------------------------------------------------------------------------
//  Compute the sum of X values
// ----------------------------------------------------------------------------
{
    return StatsAccess::evaluate(&StatsAccess::sum_x, true);
}



COMMAND_BODY(SumOfY)
// ----------------------------------------------------------------------------
//  Compute the sum of Y values
// ----------------------------------------------------------------------------
{
    return StatsAccess::evaluate(&StatsAccess::sum_y, true);
}



COMMAND_BODY(SumOfXY)
// ----------------------------------------------------------------------------
//  Compute the sum of X*Y products
// ----------------------------------------------------------------------------
{
    return StatsAccess::evaluate(&StatsAccess::sum_xy, true);
}



COMMAND_BODY(SumOfXSquares)
// ----------------------------------------------------------------------------
//  Compute the sum of X squared
// ----------------------------------------------------------------------------
{
    return StatsAccess::evaluate(&StatsAccess::sum_x2, true);
}



COMMAND_BODY(SumOfYSquares)
// ----------------------------------------------------------------------------
//  Compute the sum of Y squared
// ----------------------------------------------------------------------------
{
    return StatsAccess::evaluate(&StatsAccess::sum_y2, true);
}


COMMAND_BODY(Variance)
// ----------------------------------------------------------------------------
//   Compute the variance
// ----------------------------------------------------------------------------
{
    return StatsAccess::evaluate(&StatsAccess::variance, false);
}


COMMAND_BODY(StandardDeviation)
// ----------------------------------------------------------------------------
//   Compute the standard deviation (square root of variance)
// ----------------------------------------------------------------------------
{
    return StatsAccess::evaluate(&StatsAccess::standard_deviation, false);
}


COMMAND_BODY(Correlation)
// ----------------------------------------------------------------------------
//  Compute the correlation
// ----------------------------------------------------------------------------
{
    return StatsAccess::evaluate(&StatsAccess::correlation, true);
}



COMMAND_BODY(Covariance)
// ----------------------------------------------------------------------------
//   Compute the covariance
// ----------------------------------------------------------------------------
{
    return StatsAccess::evaluate(&StatsAccess::covariance, true);
}


COMMAND_BODY(PopulationVariance)
// ----------------------------------------------------------------------------
//  Compute the population variance
// ----------------------------------------------------------------------------
{
    return StatsAccess::evaluate(&StatsAccess::population_variance, true);
}



COMMAND_BODY(PopulationStandardDeviation)
// ----------------------------------------------------------------------------
//  Compute population standard deviation
// ----------------------------------------------------------------------------
{
    return StatsAccess::evaluate(&StatsAccess::population_standard_deviation, false);
}



COMMAND_BODY(PopulationCovariance)
// ----------------------------------------------------------------------------
//  Compute population covariance
// ----------------------------------------------------------------------------
{
    return StatsAccess::evaluate(&StatsAccess::population_covariance, true);
}



COMMAND_BODY(FrequencyBins)
// ----------------------------------------------------------------------------
//  Compute frequency bits for the independent column in the data
// ----------------------------------------------------------------------------
{
    rt.unimplemented_error();
    return ERROR;
}


static object::result set_columns(bool setx, bool sety)
// ----------------------------------------------------------------------------
//   Set a default column from the stack
// ----------------------------------------------------------------------------
{
    if (rt.args(setx + sety))
    {
        StatsParameters::Access stats;
        if (!stats)
            return object::ERROR;
        if (setx)
        {
            if (object_p arg = rt.stack(sety))
            {
                stats.xcol = arg->as_uint32(1, true);
                if (rt.error())
                    return object::ERROR;
            }
        }
        if (sety)
        {
            if (object_p arg = rt.stack(0))
            {
                stats.ycol = arg->as_uint32(2, true);
                if (rt.error())
                    return object::ERROR;
            }
        }
        return object::OK;
    }
    return object::ERROR;
}


COMMAND_BODY(IndependentColumn)
// ----------------------------------------------------------------------------
//   Set the independent (X) column
// ----------------------------------------------------------------------------
{
    return set_columns(true, false);
}



COMMAND_BODY(DependentColumn)
// ----------------------------------------------------------------------------
//  Set the depeendent (Y) column
// ----------------------------------------------------------------------------
{
    return set_columns(false, true);
}



COMMAND_BODY(DataColumns)
// ----------------------------------------------------------------------------
//  Set both X and Y
// ----------------------------------------------------------------------------
{
    return set_columns(true, true);
}



COMMAND_BODY(Intercept)
// ----------------------------------------------------------------------------
//   Return current intercept value
// ----------------------------------------------------------------------------
{
    return StatsAccess::evaluate(&StatsAccess::intercept_value, true);
}



COMMAND_BODY(Slope)
// ----------------------------------------------------------------------------
//   Return the current slope
// ----------------------------------------------------------------------------
{
    return StatsAccess::evaluate(&StatsAccess::slope_value, true);
}



COMMAND_BODY(LinearRegression)
// ----------------------------------------------------------------------------
//   Compute linear regression
// ----------------------------------------------------------------------------
{
    StatsAccess stats;
    if (!stats)
        return ERROR;
    algebraic_g n = stats.num_rows();
    algebraic_g sx2 = stats.sum_x2();
    algebraic_g sx = stats.sum_x();
    algebraic_g sy = stats.sum_y();
    algebraic_g sxy = stats.sum_xy();
    algebraic_g ssxx = sx2 - sx * sx / n;
    algebraic_g ssxy = sxy - sx * sy / n;
    algebraic_g slope = ssxy / ssxx;
    algebraic_g intercept = (sy - slope * sx) / n;
    if (stats.model == ID_ExponentialFit || stats.model == ID_PowerFit)
        intercept = exp::evaluate(intercept);
    if (!intercept || !slope)
        return ERROR;
    stats.intercept = intercept;
    stats.slope = slope;
    tag_g itag = tag::make("Intercept", +intercept);
    tag_g stag = tag::make("Slope", +slope);
    if (!itag || !stag)
        return ERROR;
    if (!rt.push(+itag) || !rt.push(+stag))
        return ERROR;
    return OK;
}


COMMAND_BODY(BestFit)
// ----------------------------------------------------------------------------
//  Try the four fit modes, and select the one with the highest correlation
// ----------------------------------------------------------------------------
{
    StatsAccess stats;
    if (!stats)
        return ERROR;

    algebraic_g best_correlation, correlation, test;
    object::id  best_model = ID_LinearFit;
    for (uint type = ID_LinearFit; type <= ID_LogarithmicFit; type++)
    {
        stats.model = object::id(type);
        correlation = stats.correlation();
        bool is_best = !best_correlation;
        if (!is_best)
        {
            test = correlation > best_correlation;
            if (!test)
                return ERROR;
            is_best = test->as_truth(false);
        }
        if (is_best)
        {
            best_correlation = correlation;
            best_model = stats.model;
        }
    }
    stats.model = best_model;
    return OK;
}


static object::result set_fit(object::id type)
// ----------------------------------------------------------------------------
//   Set the fit model in statistics parameters
// ----------------------------------------------------------------------------
{
    StatsParameters::Access stats;
    stats.model = type;
    return object::OK;
}


COMMAND_BODY(LinearFit)
// ----------------------------------------------------------------------------
//   Select linear fit
// ----------------------------------------------------------------------------
{
    return set_fit(ID_LinearFit);
}


COMMAND_BODY(ExponentialFit)
// ----------------------------------------------------------------------------
//   Select exponential fit
// ----------------------------------------------------------------------------
{
    return set_fit(ID_ExponentialFit);
}


COMMAND_BODY(PowerFit)
// ----------------------------------------------------------------------------
//   Select power fit
// ----------------------------------------------------------------------------
{
    return set_fit(ID_PowerFit);
}


COMMAND_BODY(LogarithmicFit)
// ----------------------------------------------------------------------------
//   Select logarithmic fit
// ----------------------------------------------------------------------------
{
    return set_fit(ID_LogarithmicFit);
}
