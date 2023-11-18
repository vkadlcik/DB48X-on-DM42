# Operations with Lists

## TOLIST
Assemble a list from its elements


## INNERCOMP
Split a list into its elements


## CMDDOLIST
Do a procedure with elements of lists


## DOSUBS
Do a procedure on a subset of a list


## MAP
Do a procedure on each element of a list, recursively


## MAPINNERCOMP
Do a procedure on each element recursively, return individual elements


## STREAM
Do a procedure on consecutive elements of a list


## DELTALIST
First differences on the elements of a list


## SUMLIST
Sum of all elements in a list


## PRODLIST
Product of all elements in a list


## ADD
Concatenate lists and/or elements


## Sort

Sort elements in a list or array, sorting by increasing values when comparing
numers, text or symbols.

This may be a little slower than `QuickSort`, but is useful to sort
lists or arrays of numerical values or text values.

## QuickSort

Sort elements in a list or array using the memory representation of objects.
This guarantees a consistent sorting order, but one that does not necessarily
preserve numerical or textual properties, unlike `Sort`. Comparisons are,
however, significantly faster than `Sort`.

## ReverseSort

Sort a list or array by value, in reverse order compared to `Sort`.

## ReverseQuickSort

Sort a list or array using the memory representation of objects, in reverse
order compared to `QuickSort`.

## ReverseList (REVLIST)

Reverse the order of elements in a list


## ADDROT
Add elements to a list, keep only the last N elements


## SEQ
Assemble a list from results of sequential procedure
