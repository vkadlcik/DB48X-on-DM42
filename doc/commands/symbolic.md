# Operations with Symbolic Expressions

## Rewrite

Applies an arbitrary transformation on equations. The first argument is the
equation to transform. The second argument is the pattern to match. The third
argument is the replacement pattern. Patterns can contain variable names, which
are substituted with the corresponding sub-expression.

`Eq` `From` `To` ▶ `Eq`

Examples:
* `'A+B+0' 'X+0' 'X' rewrite` returns `'A+B'`
* `'A+B+C' 'X+Y' 'Y-X' rewrite` returns `'C-(B-A)`


## AUTOSIMPLIFY
Reduce numeric subexpressions


## RULEMATCH
Find if an expression matches a rule pattern


## RULEAPPLY
Match and apply a rule to an expression repeatedly


## TOFRACTION
Convert number to fraction


## RULEAPPLY1
Match and apply a rule to an expression only once


## TRIGSIN
Simplify replacing cos(x)^2+sin(x)^2=1


## ALLROOTS
Expand powers with rational exponents to consider all roots


## CLISTCLOSEBRACKET


## RANGE
Create a case-list of integers in the given range.


## ASSUME
Apply certain assumptions about a variable to an expression.
