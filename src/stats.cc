// ****************************************************************************
//  stats.cc                                                      DB48X project
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
#include "integer.h"
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


symbol_p StatsParameters::Access::name()
// ----------------------------------------------------------------------------
//   Return the name for the variable
// ----------------------------------------------------------------------------
{
    object_p cmd = command::static_object(ID_StatsParameters);
    return cmd->as_symbol(false);
}


bool StatsParameters::Access::parse(list_g parms)
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
        bool valid = false;
        switch(index)
        {
        case 0:
            if (object_p xc = obj->algebraic_child(0))
                xcol = xc->as_uint32(1, true);
            break;
        case 1:
            if (object_p yc = obj->algebraic_child(1))
                ycol = yc->as_uint32(2, true);
            break;
        case 2:
            intercept = obj->algebraic_child(2);
            break;
        case 3:
            slope = obj->algebraic_child(3);
            break;
        case 4:
            type = obj->type();
            if (type >= object::ID_LinearFit &&
                type <= object::ID_LogarithmicFit)
                model = type;
            break;
        default:
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


bool StatsParameters::Access::parse(symbol_p name)
// ----------------------------------------------------------------------------
//   Parse stats parameters from a variable name
// ----------------------------------------------------------------------------
{
    if (object_p obj = directory::recall_all(name))
        if (list_p parms = obj->as<list>())
            return parse(parms);
    return false;
}


bool StatsParameters::Access::write(symbol_p name) const
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
    : data(), columns(), rows()
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


symbol_p StatsData::Access::name()
// ----------------------------------------------------------------------------
//   Return the name for the variable
// ----------------------------------------------------------------------------
{
    object_p cmd = command::static_object(ID_StatsData);
    return cmd->as_symbol(false);
}


bool StatsData::Access::parse(array_g values)
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


bool StatsData::Access::parse(symbol_p name)
// ----------------------------------------------------------------------------
//   Parse stats data from a variable name
// ----------------------------------------------------------------------------
{
    if (object_p obj = directory::recall_all(name))
        if (array_p values = obj->as<array>())
            return parse(values);
    return false;
}


bool StatsData::Access::write(symbol_p name) const
// ----------------------------------------------------------------------------
//   Write statistical data to variable or disk
// ----------------------------------------------------------------------------
{
    if (directory *dir = rt.variables(0))
        if (data.Safe())
            return dir->store(name, data.Safe());
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
    StatsData::Access stats;
    if (rt.push(stats.data.Safe()))
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
            if (array_p values = obj->as<array>())
            {
                StatsData::Access stats;
                if (stats.parse(values))
                    return OK;
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

COMMAND_BODY(DataSize)
// ----------------------------------------------------------------------------
//   Return the number of entries in statistics data
// ----------------------------------------------------------------------------
{
    StatsData::Access stats;
    integer_p count = integer::make(stats.rows);
    if (count && rt.push(count))
        return OK;
    return ERROR;
}


COMMAND_BODY(Mean)
// ----------------------------------------------------------------------------
//   Compute the mean of all input data
// ----------------------------------------------------------------------------
{

    return OK;
}



COMMAND_BODY(Median)
// ----------------------------------------------------------------------------
//
// ----------------------------------------------------------------------------
{
    return OK;
}



COMMAND_BODY(MinData)
// ----------------------------------------------------------------------------
//
// ----------------------------------------------------------------------------
{
    return OK;
}



COMMAND_BODY(MaxData)
// ----------------------------------------------------------------------------
//
// ----------------------------------------------------------------------------
{
    return OK;
}


COMMAND_BODY(SumOfX)
// ----------------------------------------------------------------------------
//
// ----------------------------------------------------------------------------
{
    return OK;
}



COMMAND_BODY(SumOfY)
// ----------------------------------------------------------------------------
//
// ----------------------------------------------------------------------------
{
    return OK;
}



COMMAND_BODY(SumOfXY)
// ----------------------------------------------------------------------------
//
// ----------------------------------------------------------------------------
{
    return OK;
}



COMMAND_BODY(SumOfXSquares)
// ----------------------------------------------------------------------------
//
// ----------------------------------------------------------------------------
{
    return OK;
}



COMMAND_BODY(SumOfYSquares)
// ----------------------------------------------------------------------------
//
// ----------------------------------------------------------------------------
{
    return OK;
}





COMMAND_BODY(Variance)
// ----------------------------------------------------------------------------
//
// ----------------------------------------------------------------------------
{
    return OK;
}



COMMAND_BODY(Correlation)
// ----------------------------------------------------------------------------
//
// ----------------------------------------------------------------------------
{
    return OK;
}



COMMAND_BODY(Covariance)
// ----------------------------------------------------------------------------
//
// ----------------------------------------------------------------------------
{
    return OK;
}



COMMAND_BODY(StandardDeviation)
// ----------------------------------------------------------------------------
//
// ----------------------------------------------------------------------------
{
    return OK;
}



COMMAND_BODY(PopulationVariance)
// ----------------------------------------------------------------------------
//
// ----------------------------------------------------------------------------
{
    return OK;
}



COMMAND_BODY(PopulationStandardDeviation)
// ----------------------------------------------------------------------------
//
// ----------------------------------------------------------------------------
{
    return OK;
}



COMMAND_BODY(PopulationCovariance)
// ----------------------------------------------------------------------------
//
// ----------------------------------------------------------------------------
{
    return OK;
}



COMMAND_BODY(Bins)
// ----------------------------------------------------------------------------
//
// ----------------------------------------------------------------------------
{
    return OK;
}


COMMAND_BODY(Total)
// ----------------------------------------------------------------------------
//
// ----------------------------------------------------------------------------
{
    return OK;
}



COMMAND_BODY(IndependentColumn)
// ----------------------------------------------------------------------------
//
// ----------------------------------------------------------------------------
{
    return OK;
}



COMMAND_BODY(DependentColumn)
// ----------------------------------------------------------------------------
//
// ----------------------------------------------------------------------------
{
    return OK;
}



COMMAND_BODY(DataColumns)
// ----------------------------------------------------------------------------
//
// ----------------------------------------------------------------------------
{
    return OK;
}



COMMAND_BODY(Intercept)
// ----------------------------------------------------------------------------
//
// ----------------------------------------------------------------------------
{
    return OK;
}



COMMAND_BODY(Slope)
// ----------------------------------------------------------------------------
//
// ----------------------------------------------------------------------------
{
    return OK;
}



COMMAND_BODY(LinearRegression)
// ----------------------------------------------------------------------------
//
// ----------------------------------------------------------------------------
{
    return OK;
}


COMMAND_BODY(BestFit)
// ----------------------------------------------------------------------------
//
// ----------------------------------------------------------------------------
{
    return OK;
}


COMMAND_BODY(LinearFit)
// ----------------------------------------------------------------------------
//
// ----------------------------------------------------------------------------
{
    return OK;
}


COMMAND_BODY(ExponentialFit)
// ----------------------------------------------------------------------------
//
// ----------------------------------------------------------------------------
{
    return OK;
}


COMMAND_BODY(PowerFit)
// ----------------------------------------------------------------------------
//
// ----------------------------------------------------------------------------
{
    return OK;
}


COMMAND_BODY(LogarithmicFit)
// ----------------------------------------------------------------------------
//
// ----------------------------------------------------------------------------
{
    return OK;
}
