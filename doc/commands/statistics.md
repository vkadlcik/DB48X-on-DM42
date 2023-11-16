# Statistics

## RDZ
Initialize random number generator with a seed


## RAND
Generate a random real number

## ΣData (ΣDAT)

The `ΣData` variable contains the statistics data, in the form of a matrix.

*Note*: The `ΣData` name is considered a command internally, and as such,
is subject to `CommandDisplayMode` and not `NamesDisplayMode`.

## ΣParameters (ΣPAR)

The `ΣParameters` variable contains the statistics parameters, as a list with
five elements:

`{ xcol ycol intercept slope fit }`

The `xcol` value is an integer starting at 1, indicating the independent column.
The `ycol` value similarly indicates the dependent column.

The `intercept` and `slope` are the parameters for the linear regression.
The `fit` value is the type of fit being used:
(`LinFit`, `ExpFit`, `PwrFit`, `LogFit`);

*Note*: The `ΣParameters` name is considered a command internally, and as such,
is subject to `CommandDisplayMode` and not `NamesDisplayMode`.

## Σ+

Add data to the statistics data array `ΣData`.

* If data is a real or complex number, statistics data is single-column

* If data is a vector, statistics data has the same number of columns as the
  size of the vector.

## Σ-

Remove the last data entered in the statistics array, and pushes it on the stack.

## RecallΣ (RCLΣ)

Recall statistics data and puts it on the stack

## StoreΣ (STOΣ)

Stores an array from the stack as statistics data in the `ΣData` variable.

## ClearΣ (CLΣ)

Clear statistics data.

## Average (MEAN, AVG)

Compute the average (mean) of the values in the statistics data.
If there is a single column of data, the result is a real number.
Otherwise, it is a vector for each column of data.

## Median

Compute the median of the values in the statistics data array `ΣData`.

## MinΣ

Compute the smallest of the values in the statistics data array `ΣData`.

## MaxΣ

Compute the largest of the values in the statistics data array `ΣData`.

## ΣSize (NΣ)

Return the number of data rows in the statistics data array `ΣData`.

## ΣX

Return the sum of values in the `XCol` column of the statistics data array `ΣData`.

## ΣY

Return the sum of values in the `YCol` column of the statistics data array `ΣData`.

## ΣXY

Return the sum of the product of values in the `XCol` and `YCol` columns of the
statistics data array `ΣData`.

## ΣX²

Return the sum of the squares of the values in the `XCol` column of the
statistics data array `ΣData`.

## ΣY²

Return the sum of the squares of the values in the `YCol` column of the
statistics data array `ΣData`.

## Total (TOT)

Returns the sum of all columns in the statistics data array `ΣData`.

## Variance (VAR)

Calculates the sample variance of the coordinate values in each of the columns
in the current statistics matrix (`ΣData`).

## Correlation (CORR)

Returns the correlation coefficient of the independent and dependent data
columns in the current statistics matrix (reserved variable `ΣData`).

The columns are specified by the first two elements in the reserved variable
`ΣParameters`, set by `XCol` and `YCol`, respectively. If `ΣParameters` does not
exist, `Correlation` creates it and sets the elements to their default values
(1 and 2).

## Covariance (COV)

Returns the sample covariance of the independent and dependent data columns in
the current statistics matrix (reserved variable `ΣData`).

The columns are specified by the first two elements in the reserved variable
`ΣParameters`, set by `XCol` and `YCol`, respectively. If `ΣParameters` does not
exist, `Correlation` creates it and sets the elements to their default values
(1 and 2).

## StandardDeviation (SDEV)

Calculates the sample standard deviation of each of the columns of coordinate values in the current statistics matrix (reserved variable `ΣData`).

`StandardDeviation`  returns a vector of numbers, or a single number there is only one column of data.

The standard deviation is the square root of the `Variance`.

CMD(StandardDeviation)                  ALIAS(StandardDeviation,        "SDev")
CMD(Bins)
CMD(PopulationVariance)                 ALIAS(PopulationVariance,       "PVar")
CMD(PopulationStandardDeviation)        ALIAS(PopulationStandardDeviation, "PSDev")
CMD(PopulationCovariance)               ALIAS(PopulationCovariance,     "PCov")
CMD(IndependentColumn)                  ALIAS(IndependentColumn,        "XCol")
CMD(DependentColumn)                    ALIAS(DependentColumn,          "YCol")
NAMED(DataColumns,      "ColΣ")
CMD(Intercept)
CMD(Slope)
CMD(LinearRegression)                   ALIAS(LinearRegression,         "LR")
CMD(BestFit)
CMD(LinearFit)                          ALIAS(LinearFit,                "LinFit")
CMD(ExponentialFit)                     ALIAS(ExponentialFit,           "ExpFit")
CMD(PowerFit)                           ALIAS(PowerFit,                 "PwrFit")
CMD(LogarithmicFit)                     ALIAS(LogarithmicFit,           "LogFit")
