# Introduction to RPL

The original RPL (*Reverse Polish Lisp*) programming language was designed and
implemented by Hewlett Packard for their calculators from the mid-1980s until
2015 (the year the HP50g was discontinued). It is based on older calculators
that used RPN (*Reverse Polish Notation*). Whereas RPN had a limited stack size of
4, RPL has a stack size only limited by memory and also incorporates
programmatic concepts from the Lisp programming language.

The first implementation of RPL accessible by the user was on the HP28C, circa
1987, which had an HP Saturn processor. More recent implementations (e.g., HP49,
HP50g) run through a Saturn emulation layer on an ARM based processor. These
ARM-based HP calculators would be good targets for a long-term port of DB48X.

DB48X is a fresh implementation of RPL on ARM, initially targetting the
SwissMicros DM42 calculator. This has [implications on the design](#design-overview)
of this particular implementation of RPL.

## The RPL stack

The RPL stack can grow arbitrarily in size.

By convention, and following RPN usage, this document gives the names `X`, `Y`,
`Z` and `T` to the first four levels of the stack. This is used to describe the
operations on the stack with synthetic stack diagrams showing the state of the
stack before and after the operation.

For example, the addition of two objects in levels 1 and 2 with the result
deposited in stack level 1 can be described in synthetic form using the
following stack diagram:

`Y` `X` ▶ `Y+X`

The duplication

## Algebraic mode

Unlike earlier RPN calculators, RPL includes complete support for algebraic
objects written using the standard precedence rules in mathematics. In RPL,
algebraic expressions are placed between ticks. For example, `'2+3×5'` will
evaluate as `17`: the multiplication `3×5`, giving `15`, is performed before the
addition `2+15`, which gives `17`.

Algebraic expressions are not evaluated automatically. The _R/S_ key (bound to
the [Evaluate](#evaluate) function) will compute their value.


## Integers

The DB48X version of RPL distinguishes between integer values, like `123`, and
[decimal values](#decimal-numbers), like `123.` Integer values are represented
internally in a compact and efficient format, saving memory and making
computations faster. All values between -127 and 127 can be stored in two bytes.
All values between -16383 and 16383 in three bytes.

Integers can be [as large as memory permits](#big-integers).


## Big integers

The DB48X version of RPL can perform computations on arbitrarily large integers,
limited only by available memory, enabling for example the exact computation of
`100!` and making it possible to address problems that require exact integer
computations, like exploring the Syracuse conjecture.


## Decimal numbers

Decimal numbers are used to represent values with a fractional part.
DB48X supports three decimal numbers, using the 32-bit, 64-bit and 128-bit
[binary decimal representation](#intel-decimal-floating-point-math).
In memory, all decimal numbers use one additional byte: a 32-bit decimal number
uses 5 bytes, a 128-bit binary decimal number uses 17 bytes.

The 32-bit format offers a 7 digits mantissa and has a maximum exponent
of 96. The 64-bit format offers a 16 digits mantissa and has a maximum
exponent of 384. The 128-bit format offers a 34 digits mantissa and a maximum
exponent of 6144.

The [Precision](#precision) command selects the default precision.

Note that a future implementation of DB48X is expected to feature
variable-precision decimal numbers similar to [newRPL](#newRPL-project).


## Based numbers

Based numbers are used to perform computations in any base. The most common
bases used in computer science, 2, 8, 10 and 16, have special shortcuts.
The [Bases Menu](#bases-menu) list operations on based numbers.

Like integers, based numbers can be [arbitrary large](#big-integers).
However, operations on based numbers can be truncated to a specific number of
bits using the [STWS](#stws) command. This makes it possible to perform
computations simulating a 16-bit or 256-bit processor.


## Complex numbers

Complex numbers can be represented in rectangular form or polar form.
The rectangular form will show as something like `2+3ⅈ` on the display, where
`2` is the real part and `3` is the imaginary part. The polar form will show as
something like `1∡90°` on the display, where `1` is the modulus and `90°` is the
argument. The two forms can be mixed and matched in operations. The calculator
typically selects the most efficient form for a given operation.

Available operations on complex numbers include basic arithmetic, trigonometric,
logarithms, exponential and hyperbolic functions, as well as a few specific
functions such as [conj](#conj) or [arg](#arg). These functions are available in
the [Complex Menu](#complex-menu).


## Equations

Algebraic expressions and equations are represented between quotes, for example
`X+1` or `A+B=C`. Many functions such as circular functions, exponential, logs
or hyperbolic functions can apply to algebraic expressions.


## Lists

Lists are sequence of items between curly braces, such as `{ 1 'A' "Hello" }`.
They can contain an arbitrary number of elements, and can be nested.

Operations such as `sin` apply to all elements on a list.


## Vectors and matrices

Vector and matrices represent tables of numbers, and are represented between
square brackets, for example `[1 2 3]` for a vector and `[[1 2] [3 4]` for a 2x2
matrix.

Vector and matrices follow their own arithmetic rules. Vectors are
one-dimensional, matrices are two-dimensional. DB48X also supports tables with a
higher number of dimensions, but only offers limited operations on them.

DB48X implements vector addition, subtraction, multipplication and division,
which apply component-wise. Multiplication and division are an extension
compared to the HP48.

DB48X also implements matrix addition, subtraction, multiplication and
division. Like on the HP48, the division of matrix `A` by matrix `B` is
interpreted as left-multiplying `A` by the inverse of `B`.

As another extension, algebraic functions such as `sin` apply to all elements in
a vector or matrix in turn.
