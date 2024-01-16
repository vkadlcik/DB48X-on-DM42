# Performance measurements

This sections tracks some performance measurements across releases.

## NQueens (DM42)

Performance recording for various releases on DM42 with `small` option (which is
the only one that fits all releases). This is for the same `NQueens` benchmark,
all times in milliseconds, best of 5 runs, on USB power, with presumably no GC.


| Version | Time    | PGM Size  | QSPI Size | Note                    |
|---------|---------|-----------|-----------|-------------------------|
| 0.6.0   | 1183    | 409252    |  187516   | New table-free decimal  |
| 0.5.2   | 1310    | 711228    | 1548076   |                         |
| 0.5.1   |         |           |           |                         |
| 0.4.10+ | 1205    | 651108    |           | RPL stack runloop       |
| 0.4.10  | 1070    | 650116    |           | Focused optimizations   |
| 0.4.9+  | 1175    |           |           | Range-based type checks |
| 0.4.9+  | 1215    |           |           | Remove busy animation   |
| 0.4.9   | 1447    | 646028    | 1531868   | No LastArgs in progs    |
| 0.4.8   | 1401    | 633932    | 1531868   |                         |
| 0.4.7   | 1397    | 628188    | 1531868   |                         |
| 0.4.6   | 1380    | 629564    | 1531868   |                         |
| 0.4.5   | 1383    | 624572    | 1531868   |                         |
| 0.4.4   | 1377    | 624656    | 1531868   | Implements Undo/LastArg |
| 0.4.3S  | 1278    | 617300    | 1523164   | 0.4.3 build "small"     |
| 0.4.3   | 1049    | 717964    | 1524812   | Switch to -Os           |
| 0.4.2   | 1022    | 708756    | 1524284   |                         |
| 0.4.1   | 1024    | 687444    | 1522788   |                         |
| 0.4     |  998    | 656516    | 1521748   | Feature tests 7541edf   |
| 0.3.1   |  746    | 618884    | 1517620   | Faster busy 3f3ab4b     |
| 0.3     |  640    | 610820    | 1516900   | Busy anim 4ab3c97       |
| 0.2.4   |  522    | 597372    | 1514292   |                         |
| 0.2.3   |  526    | 594724    | 1514276   | Switching to -O2        |
| 0.2.2   |  723    | 540292    | 1512980   |                         |


## NQueens (DM32)

Performance recording for various releases on DM32 with `fast` build option.
This is for the same `NQueens` benchmark, all times in milliseconds,
best of 5 runs. There is no GC column, because it's harder to trigger given how
much more memory the calculator has. Also, experimentally, the numbers for the
USB and battery measurements are almost identical at the moment. As I understand
it, there are plans for a USB overclock like on the DM42, but at the moment it
is not there.


| Version | Time    | PGM Size  | QSPI Size | Note                    |
|---------|---------|-----------|-----------|-------------------------|
| 0.6.0   | 1751    | 467260    |  187948   | New table-free decimal  |
| 0.5.2   | 1752    | 856228    | 1550436   |                         |
| 0.5.1   | 1746    |           |           |                         |
| 0.5.0   | 1723    |           |           |                         |
| 0.4.10+ | 1804    | 761252    |           | RPL stack runloop       |
| 0.4.10  | 1803    | 731052    |           | Focused optimizations   |
| 0.4.9   | 2156    | 772732    | 1534316   | No LastArg in progs     |
| 0.4.8   | 2201    | 749892    | 1534316   |                         |
| 0.4.7   | 2209    | 742868    | 1534316   |                         |
| 0.4.6   | 2204    | 743492    | 1534316   |                         |
| 0.4.5   | 2171    | 730092    | 1534316   |                         |
| 0.4.4   | 2170    | 730076    | 1534316   | Implements Undo/LastArg |
| 0.4.3   | 2081    | 718020    | 1527092   |                         |
| 0.4.2   | 2242    | 708756    | 1524284   |                         |
| 0.4.1   | 2152    | 687500    | 1522788   |                         |
| 0.4     |         |           |           | Feature tests 7541edf   |
| 0.3.1   |         |           |           |                         |
| 0.3     |         |           |           |                         |
| 0.2.4   |         |           |           |                         |
| 0.2.3   |         |           |           |                         |


## Collatz conjecture check

This test checks the tail recursion optimization in the RPL interpreter.
The code can be found in the `CBench` program in the `Demo.48S` state.
The HP48 cannot run the benchmark because it does not have integer arithmetic.

Timing on 0.4.10 are:

* HP50G: 397.438s
* DM32: 28.507s (14x faster)
* DM42: 15.769s (25x faster)

| Version | DM32 ms | DM42 ms |
|---------|---------|---------|
| 0.6.0   | 26256   |  15355  |
| 0.5.2   | 26733   |  15695  |
| 0.4.10  | 28507   |  15769  |



## SumTest (decimal performance)

VP = Variable Precision
ID = Intel Decimal Library

For 100000 loops

| Version      | DM32 ms | DM42 ms |
|--------------|---------|---------|
| 0.6.0 (VP24) | 2377390 | 1768510 |
| 0.5.2 (ID)   |  215421 |  143412 |


For 1000 loops

| Version      | DM32 ms | DM42 ms |
|--------------|---------|---------|
| 0.6.2 (VP36) |   62012 |   42269 |
| 0.6.2 (VP24) |   34898 |   23714 |
| 0.6.2 (VP12) |   16017 |   10782 |
| 0.6.2 (VP6)  |    7436 |    5842 |
| 0.6.0 (VP24) |   23773 |   17685 |
| 0.5.2 (ID)   |    2154 |    1434 |

## Drawing `sin X` with `FunctionPlot`

| Configuration   | DM32 ms    | DM42 ms    |
|-----------------|------------|------------|
| ID              | 2332-5140  |            |
| VP24            | 3683-6005  |            |
| VP36            | 6567-10186 |            |
| VP48            | 8377-10259 |            |

Crash at precision 3
