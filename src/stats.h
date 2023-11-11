#ifndef STATS_H
#define STATS_H
// ****************************************************************************
//  stats.h                                                       DB48X project
// ****************************************************************************
//
//   File Description:
//
//     Statistics
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

#include "algebraic.h"
#include "command.h"
#include "list.h"
#include "symbol.h"
#include "target.h"


struct StatsParameters
// ----------------------------------------------------------------------------
//   A replication of the StatsParameters / ΣPAR vaariable
// ----------------------------------------------------------------------------
{
    StatsParameters();

    object::id  model;
    algebraic_g xcol;
    algebraic_g ycol;
    algebraic_g intercept;
    algebraic_g slope;

    bool parse(list_g list);
    bool parse(symbol_g name);
    bool parse(cstring name);
    bool parse();
};


COMMAND_DECLARE(AddData);
COMMAND_DECLARE(RemoveData);
COMMAND_DECLARE(RecallData);
COMMAND_DECLARE(StoreData);
COMMAND_DECLARE(ClearData);
COMMAND_DECLARE(DataSize);
COMMAND_DECLARE(Mean);
COMMAND_DECLARE(Median);
COMMAND_DECLARE(MinData);
COMMAND_DECLARE(MaxData);
COMMAND_DECLARE(SumOfX);
COMMAND_DECLARE(SumOfY);
COMMAND_DECLARE(SumOfXY);
COMMAND_DECLARE(SumOfXSquares);
COMMAND_DECLARE(SumOfYSquares);
COMMAND_DECLARE(Variance);
COMMAND_DECLARE(Correlation);
COMMAND_DECLARE(Covariance);
COMMAND_DECLARE(StandardDeviation);
COMMAND_DECLARE(PopulationVariance);
COMMAND_DECLARE(PopulationStandardDeviation);
COMMAND_DECLARE(PopulationCovariance);
COMMAND_DECLARE(Bins);
COMMAND_DECLARE(Total);
COMMAND_DECLARE(IndependentColumn);
COMMAND_DECLARE(DependentColumn);
COMMAND_DECLARE(DataColumns);
COMMAND_DECLARE(Intercept);
COMMAND_DECLARE(Slope);
COMMAND_DECLARE(LinearRegression);
COMMAND_DECLARE(BestFit);
COMMAND_DECLARE(LinearFit);
COMMAND_DECLARE(ExponentialFit);
COMMAND_DECLARE(PowerFit);
COMMAND_DECLARE(LogarithmicFit);

#endif // STATS_H
