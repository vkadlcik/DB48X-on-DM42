# Overview

## db50x on DM42

The db50x project intends to rebuild and improve the user experience of the
legendary HP48 family of calculators, notably their *"Reverse Polish Lisp"*
 [(RPL)](#Introduction-to-RPL)
language with its rich set of data types and built-in functions.

This project is presently targeting the **SwissMicro DM42 calculator**
and leveraging its built-in software platform, known as **DMCP**. This is
presumably the calculator you are currently running this software on.

## Table of contents

* [Using the on-line help](#help)
* [State of the project](#state-of-the-project)
* [Design overview](#design-overview)
* [Keyboard interaction](#keyboard-interaction)
* [Soft menus](#soft-menus)
* [Differences with other RPLs](#differences-with-other-RPLs)
* [Built-in help](#help)
* [Acknowledgements and credits](#acknowledgements-and-credits)


## State of the project

This is currently **UNSTABLE** software. Please only consider installing this if
you are a developer and interested in contributing. Please refer to the web site
of the project on GitHub for details and updates.

## Design overview

The objective is to re-create an RPL-like experience, but to optimize it for the
existing DM42 physical hardware. Ideally, db48x should be fully usable without a
keyboard overlay. though one is [being worked on](../Keyboard-Layout.png).

Compared to the original HP48, the DM42 has a much larger screen, but no
annunciators (it is a fully bitmap screen). It has a keyboard with dedicated
soft-menu (function) keys, but only one shift key (whereas the HP48 has two),
lacks a dedicated alpha key, does not provides left or right arrow keys (only up
and down), and has no space key (_SPC_ on the HP48).


## Keyboard interaction

The keyboard differences force us to revisit the user interaction with the
calculator compared to the HP48:

* The single yellow 🟨 key cycles between three states, *Shift*, *Right
  Shift* and no shift.  This double-shift shortcut appears necessary because
  RPL calculators like the HP48 have a rather full keyboard even with two shift
  keys.

* The less-frequently used functions can be accessed after a
  double-press on 🟨, which in the rest of this documentation will be shown
  as 🟦, and will correspond to blue functions on the keyboard overlay.

* Since RPL uses alphabetic entry (also called *Alpha* mode) a lot more
  frequently than on the HP42, making it quickly accessible seems important, so
  there are [three distinct ways to activate it](#alpha-mode).

* The _▲_ and _▼_ keys move the cursor *left* and *right* while editing
  instead of *up* and *down*. These cursor movements are much more useful for a
  text-based program editing as found in RPL.

* Using 🟨 _▲_ and 🟨 _▼_ moves the cursor up and down.  When not editing, _▲_
  and _▼_ behave like on the HP48, i.e. _▲_ enters the *interactive stack* (not
  yet implemented) and _▼_ edits the object on the first level of the stack.

* Long-pressing arrow keys, the _←_ (also known as *Backspace*) or text entry
  keys in Alpha mode activates auto-repeat.

* Long-pressing keys that would directly trigger a function (e.g. _SIN_),
  including function keys associated with a soft-menu, will show up the
  [built-in help](#help) for the corresponding function.


### Alpha mode

Entering alphabetic characters is done using *Alpha* mode. These alphabetic
characters are labeled on the right of each key on the DM42's keyboard.

When *Alpha* mode is active, an _ABC_ indicator shows up in the annunciator area
at the top of the screen. For lowercase entry, the indicator changes to _abc_.

There are three ways to enter *Alpha* mode:

* The first method is to use 🟨 _ENTER_ as indicated by the _ALPHA_ yellow label
  on the DM42 ENTER key. This cycles between *Alpha* _ABC_, *Lowercase* _abc_
  and *Normal* entry modes.

* The second method is to hold 🟨 for more than half a second. This cycles
  between *Alpha* _ABC_ and *Normal* entry modes, and cannot be used to type
  lowercase characters.

* The third method is to hold one of the arrow keys _▲_ or _▼_ *while* typing on
  the keyboard. This is called *transient alpha mode* because *Alpha* mode ends
  as soon as the arrow key is released. Using _▲_ enters uppercase characters,
  while _▼_ uses lowercase characters.

There is no equivalent of the HP48's "single-Alpha" mode. Alpha mode is either
_transient_ (when you hold one of the arrow keys) or _sticky_ (with 🟨 _ENTER_
or by holding 🟨).

Alpha mode is cancelled when pressing_ENTER_ or _EXIT_.

Since the DM42's alphabetic keys overlap with the numeric keys (unlike the
HP48), as well as with operations such as _×_ and _÷_, using 🟨 in Alpha mode
brings back numbers. This means 🟨 cannot be used for lowercase, but as
indicated above, there are two other methods to enter lowercase
characters.

Using 🟨 or 🟦 in combination with keys other than the numeric keypad
gives a variety of special characters.


### Key mapping

Some keys that have little use or no direct equivalent for RPL are remapped
as follows:

* _Σ+_ is used to call [ToolsMenu](#ToolsMenu), which select a menu based on
  context, notably the content of the stack.

* _Σ-_(i.e.🟨 _Σ+_) will select [LastMenu](#LastMenu), i.e. return to the
  previous menu.

* 🟦 _Σ+_ selects [MainMenu](#MainMenu), the top-level menu giving access
  to all other menus and features in db50x (see also the [Catalog](#catalog)
  feature).

* _XEQ_ opens an algebraic expression, i.e. it shows `''` on the command-line
  and switches to equation entry. It can be remembered as *Execute Equation* and
  can be used to evaluate expressions in [algebraic mode](#algebraic-mode)
  instead of RPN. While inside an equation, _XEQ_ enters parentheses.

* _GTO_ opens the [BranchesMenu](#BranchesMenu), with RPL branches and loops,
  e.g. `IF` `THEN` or `DO` `WHILE`, as well as conditional tests.

* _COMPLEX_ opens the [ComplexMenu](#ComplexMenu), not just build a complex
  like on the DM42. The [ComplexMenu](#ComplexMenu) includes features to enter
  complex numbers in rectangular or polar form, as well as complex-specific
  functions like [Conjugate](#Conjugate).

* _RCL_ opens the [VariablesMenu](#VariablesMenu) menu listing user variables.
  This plays the role of _VARS_ on the HP48.

* _%_ (🟨 _RCL_) opens the [FractionsMenu](#FractionsMenu), to access operations
  on fractions.

* _R↓_ will open the [StackMenu](#StackMenu), containing operations on the
  stack.

* _π_ (🟨 _R↓_) will open the [ConstantsMenu](#ConstantsMenu) (π being one of
  them), with the option to get the symbolic or numerical value. The values of
  constants come from a file named `CONSTANTS.CSV` on disk.

* _X⇆Y_ executes the matching [Swap](#swap) function

* _LAST x_ (🟨 _X⇆Y_) executes [LastArg](#LastArguments) command, recalling the
  arguments of the last command. There is also a [LastX](#LastX) command for
  compatibility with RPN, which is available from the [StackMenu](#StackMenu).

* _Undo_(🟦 _X⇆Y_) restores the previous state of the stack. This is like
  `Last Stack` on the HP48, but it is a real command that can be used in
  programs.

* _+/-_ executes the equivalent RPL `Negate` function

* _Modes_ (🟨 _+/-_ ) calls the [ModesMenu](#ModesMenu), with submenus for
  various settings, including computation precision, display modes, etc.

* _Obj_ (🟦 _+/-_ ) calls the [ObjectMenu](#ObjectMenu), with various
  object-related operations.

* _Disp_ (🟨 _EEX_ ) calls the [DisplayModesMenu](#DisplayModesMenu), which
  controls settings related to the display, such as number of digits shown or
  separators.

* _Clear_ calls a [ClearThingsMenu](#ClearThingsMenu) with options to clear
  various items, including [ClearStack](#ClearStack) and
  [ClearMenu](#ClearMenu).

* _SST_ and _BST_ (🟨 _▲_ and _▼_) move the cursor *up* and *down* in the text
 editor. In direct mode, _BST_ selects the *Best* editor for the object, and
  *Sst* selects single-step evaluation.

* _SOLVER_ (🟨 _7_) shows the [SolverMenu](#SolverMenu), with submenus for
  numerical and symbolic solvers.

* _∫f(x)_ (🟨 _8_) shows the [SymbolicMenu](#SymbolicMenu), with symbolic and
  numerical integration and derivation features.

* _MATRIX_ (🟨 _9_) enters the `[` and `]` characters, which are vector and
  matrix delimiters in RPL.  🟦 _9_ shows the [MatrixMenu](#MatrixMenu) with
  operations on vectors, matrices and tensors.

* _STAT_ (🟨 _÷_) shows the [StatisticsMenu](#StatisticsMenu)

* _BASE_ (🟨 _4_) shows the [BasesMenu](#BasesMenu), with operations on
  based numbers and facilities for entering hexadecimal numbers.

* _CONVERT_ (🟨 _5_) shows a [UnitsMenu](#UnitsMenu) with units and and
  conversion functions.

* _FLAGS_ (🟨 _6_) shows the [FlagsMenu](#FlagsMenu) with operations on user and
  system flags.

* _PROB_ (🟨 _×_) shows the [ProbabilitiesMenu](#ProbabilitiesMenu), with
  functions such as [Factorial](#factorial), [Combinations](#combinations) or
  [Random](#random).

* _ASSIGN_ (🟨 _1_) makes it possible to assign any function to any key. These
  special functions are then selected by using _Custom_ (🟨 _2_), which
  corresponds roughly to _USR_ on the HP48.

* _PGM.FCN_ (🟨 _1_) shows the [ProgramMenu](#ProgramMenu), with all
  general-purpose programming operations, categorized as sub-menus.

* _PRINT_ (🟨 _-_) shows the [IOMenu](#IOMenu).

* _EXIT_ corresponds to what the HP48 manual calls _Attn_, and typically
  cancels the current activity. It can also be used to interrupt a running
  program.

* _OFF_ (🟨 _EXIT_) shuts down the calculator. The state of the calculator is
  preserved.

* _SAVE_ (🟦 _EXIT_) saves the current state of the calculator to disk. This
  state can be transferred to another machine, and survives system reset or
  firmware upgrades.

* _SETUP_ (🟨 _0_) shows the DM42's built-in [SystemMenu](#SystemMenu), for
  example to load the original DM42 program, activate USB disk, and to access
  some calculator preferences.

* _Show_ selects the [ShowMenu](#ShowMenu), with various ways to display objects
  on the stack, such as [ShowBest](#ShowBest), [ShowSymbolic](#ShowSymbolic),
  [ShowGraphical](#ShowGraphical), [ShowCompact](#ShowCompact).

* The _R/S_ keys inserts a space in the editor, and maps to
 [Evaluate](#evaluate) otherwise.

* _PRGM_ (🟨 _R/S_) inserts the delimiters for an RPL program, `«` and `»`,
  while 🟦 _R/S_ inserts the list delimiters, `{` and `}`.

* _CATALOG_ (🟨 _+_) shows a complete context-sensitive catalog of all
  available functions, and enables auto-completion using the soft-menu
  keys. Note that the `+` key activates the catalog while in *Alpha* mode.

* _HELP_ (🟦 _+_) activates the context-sensitive help system.


## Soft menus

The DM42 has 6 dedicated soft-menu keys at the top of the keyboard. Most of the
advanced features of db50x can be accessed through these soft menus.

Menus are organized internally as a hierarchy, where menus can refer to other
menus. A special menu, [MainMenu](#MainMenu), accessible via the 🟦 _Σ+_,
contains all other menus.

Menus can contain up to 18 entries at once, 6 being directly accessible, 6
more being shown when using the 🟨 key, and 6 more with 🟦. Three rows of
functions are shown on screen, with the active row highlighted.

A long press on a function key invokes the on-line help for the associated
function.

When a menu contains more than 18 entries, then the _F6_ function key turns into
a `▶︎`, and 🟨 _F6_ turns into `◀`︎. These keys can be used to
navigate across the available menu entries. This replaces the _NXT_ and _PREV_
keys on HP calculators.

The `Variables` menu (_RCL_ key) is special in the sense that:

* Selecting an entry *evaluates* that menu entry, for example to run a program

* The 🟨 function *recalls* its name without evaluating it.

* The 🟦 function *stores* into the variable.


## Differences with other RPLs

Multiple implementations of RPL exist, most of them from Hewlett-Packard.
A good reference to understand the differences between the various existing
implementations from HP is the
[HP50G Advanced User's Reference Manual](https://www.hpcalc.org/details/7141).

There are a number of intentional differences in design between db50x and the
HP48, HP49 or HP50G's implementations of RPL. There are also a number of
unintentional differences, since the implementation is completely new.

#### User interface

* db50x features an extensive built-in help system, which you are presently
  using. Information for that help system is stored using a regular *markdown*
  file named `/HELP/db50x.md`, stored in the calculator's flash storage.

* db50x features auto-completion for commands while typing, through
  the  _Catalog_ key ([CatalogMenu](#CatalogMenu)).

* Many RPL words exist in short and long form, and a user preference selects how
  a program shows. For example, the [Negate](#negate) command, which the HP48
  calls `NEG`, can display, based on user preferences, as `NEG`, `neg`, `Neg` or
  `Negate`. In the help, it will be shown as **Negate (NEG)**.

* The db50x dialect of RPL is not case sensitive, but it is case-respecting.
  For example, if your preference is to display built-in functions in long form,
  typing `inv` or `INV` will show up as `Invert` in the resulting program.
  This means that the space of "reserved words" is larger in db50x than in other
  RPL implementations. Notably, on HP's implementations, `DUP` is a keyword but
  you can use `DuP` as a valid variable name. This is not possible in db50x.


#### Representation of objects

* Internally, the calculator deals with various representations for
  numbers. Notably, it keeps integer values and fractions in exact form for
  as long as possible to optimize both performance and memory usage.
  This is somewhat similar to what the HP49 and HP50 implemented, where there is
  a difference between `2` (where `TYPE` returns 28) and `2.` (where `TYPE`
  return 0).

* The calculator features at least 3 floating-point precisions using 32-bit,
  64-bit and 128-bit respectively, provided by the DMCP's existing Intel Binary
  Decimal Floating-Point library. The 128-bit format gives the calculator 34
  significant digits of precision, like the DM42. db50x may support other
  formats in the future, like the arbitrary-precision floating-point found in
  newRPL.

* Based numbers with an explicit base, like `#123h` keep their base, which makes
  it possible to show on stack binary and decimal numbers side by side. Mixed
  operations convert to the base in stack level X, so that `#10d #A0h +`
  evaluates as `#AAh`. Based numbers without an explicit base change base
  depending on the `Base` setting, much like based numbers on the HP48.

* The storage of data in memory uses a denser format than on the HP48.
  Therefore, objects will almost always use less space on db50x. Notably, the
  most frequently used functions and data types consume only one byte on db50x,
  as opposed to 5 nibbles (2.5 bytes) on the HP48.

* Numerical equality can be tested with `=`,  whereas object equality is tested
  using `==`. For example, `0=0.0` is true, but `0==0.0` is false, because `0`
  is an integer whereas `0.0` is a floating-point.


#### Alignment with the DM42

* db50x borrows to the DM42 the idea of _special variables_, which are variables
  with a special meaning. For example, the `Precision` special variable is the
  current operating precision for floating point, in number of digits. While
  there is a `SetPrecision` command, it is also possible to use `'Precision'
  STO`. This does not imply that there is an internal `Precision` variable
  somewhere. Special variables are available for most settings.

* All built-in soft-key menus are named, with names ending in [Menu](#Menu). For
  example, the [VariablesMenu](#VariablesMenu) is the menu listing global
  variables in the current directory. There is no menu number, but the
  [Menu](#Menu) special variable holds the name of the current menu, and
  [LastMenu](#LastMenu) the name of the previous one.

* The db50x also provides full-screen setup menus, taking advantage of the DM42
  existing system menus. It is likely that the same menu objects used for
  softkey menus will be able to control system menus, with a different function
  to start the interaction.

* The whole banking and flash access storage mechanism of the HP48 will be
  replaced with a system that works well with FAT USB storage. It should be
  possible to directly use a part of the flash storage to store RPL programs,
  either in source or compiled form.


### List operation differences

The application of a same operation on arrays or matrices has never been very
consistent nor logical across RPL models from HP.

* On HP48 and HP50, `{ 1 2 3 } 4 +` gives `{1 2 3 4}`. However, `{ 1 2 3} 4 *`
  gives a type error on the HP48 but applies the operation to list elements on
  the HP50, yielding `{ 4 8 12}`.

* For arrays, `[ 1 2 3 ] 4 +` fails on both the HP48 and HP50, but
  `[ 1 2 3 ] 4 *` works.

* The HP50 has a `MAP` function, which works both for list and matrices.
  `[ 1 2 3 ] « 3 + »` will return `[ 4 5 6 ]`, and `{ 1 2 3 } « 3 * »` will
  return `{ 3 6 9 }`. That function has no direct equivalent on the HP48.

db50x considers lists as bags of items and treat them as a whole when it makes
sense, whereas arrays are focusing more on the values they contain, and will
operate on these items when it makes sense. Therefore:

* `{ 1 2 3 } 4 +` gives `{ 1 2 3 4 }`, `{ 1 2 3 } 2 -` gives `{ 1 3 }`, and
  `{ 1 2 3 } 3 ×` gives `{ 1 2 3 1 2 3 1 2 3 }`. The `÷` operator does not work
  on lists.

* `[ 1 2 3 ] 4 +` gives `[ 5 6 7 ]`, `[ 1 2 3 ] 2 -` gives `[ -1 0 1 ]`,
  `[ 1 2 3 ] 3 ×` gives `[ 3 6 9 ]` and `[ 1 2 3 ] 5 ÷` gives
  `[ 1/5 2/5 3/5 ]`.


### Vectors and matrices differences

* On db50x, vectors like `[ 1 2 3 ]` are very similar to lists. The primary
  difference is the behavior in the presence of arithmetic operators.
  On lists, addition is concatenation, e.g. `{ 1 2 3} { 4 5 6} +` is
  `{ 1 2 3 4 5 6 }`, whereas on vectors represents vector addition, e.g.
  `[1 2 3] [4 5 6] +` is `[5 7 9]`. However, unlike on the HP original
  implementation, a vector can contain any type of object, so that you can
  do `[ "ABC" "DEF" ] [ "GHI" "JKL" ] +` and obtain `[ "ABCGHI" "DEFJKL" ]`.

* Size enforcement on vectors only happens _during these operations_, not while
  you enter vectors from the command line. It is legal in db50x to have a
  non-rectangular array like `[[1 2 3] [4 5]]`, or even an array with mixed
  objects like `[ "ABC" 3 ]`. Size or type errors on such objects may occur
  if/when arithmetic operations are performed.

* In particular, a matrix is nothing but a vector of vectors. db50x also
  supports arrays with dimensions higher than 2, like `[[[1 2 3]]]`.

* As a consequence, The `GET` and `GETI` functions work differently on
  matrices. Consider a matrix like `[[ 7 8 9 ][ 4 5 6 ][ 1 2 3 ]]`. On the HP48,
  running `1 GET` on this object gives `7`, and the valid range of index values
  is 1 through 9. On db50x, that object is considered as an array of vectors, so
  `1 GET` returns `[7 8 9]`.  This is intentional. The behavior of `{ 1 1 } GET`
  is identical on both platforms, and is extended to multi-dimensional arrays,
  so that `[[[4 5 6]]] { 1 1 2 } GET` returns `5`.

* Matrices and vectors can contain integer values or fractions. This is closer
  to the HP50G implementation than the HP48's. In some cases, this leads to
  different results between the implementations. If you compute the inverse of
  `[[1 2 3][4 5 6][7 8 9]` on the HP48, you get a matrix with large values, and
  the HP48 finds a small, but non-zero determinant for that matrix. The HP50G
  produces a matrix with infinities. db50x by default produces a `Divide by
  zero` error.

* db50x accept matrices and vectors as input to algebraic functions, and returns
  a matrix or vector with the function applied to all elements. For example,
  `[a b c] sin ` returns `[ 'sin a' 'sin b' 'sin c' ]`.

* Similarly, db50x accept operations between a constant and a vector or matrix.
  This applies the same binary operation to all components of the vector or
  matrix. `[ a b c ] x +` returns `[ 'a+x' 'b+x' 'c+x' ]`. Consistent with that
  logic, `inv` works on vectors, and inverts each component, so that
  `[1 2 3] inv` gives `[1/1 1/2 1/3]`.


### Equations handling differences

* The db50x dialect of RPL accepts equations with "empty slots". During equation
  evaluation, the value of these empty slots will be taken from the stack. In
  the equation, a slot is represented as `()`.

* For example, the equation `()+sin(cos())` will read two values from the
  stack. If evaluated in a stack that contains `A` and `B`, it will evaluate a
  `A+sin(cos(B))`.

* This feature is an accident of implementation. It is recommended to use local
  variables to more precisely control where stack input is used in the
  equation. Ideally, you should write the above equation as
  `→ a b 'a+sin(cos(b))'` if you want better compatibility with other RPL
  implementations.


### Unicode support

db50x has almost complete support for Unicode, and stores text internally using
the UTF-8 encoding. The built-in font has minor deviations in appearance for a
few RPL-specific glyphs.

Overall, a text file produced by db50x should appear reliably in your
favorite text editor, which should normally be GNU Emacs. This is notably the
case for state files with extension `.48S` which you can find in the `STATE`
directory on the calculator.


## Help

The db50x project includes an extensive built-in help, which you are presently
reading. This help is stored as a `HELP/db50x.md` file on the calculator. You
can also read it from a web browser directly on the GitHub page of the project.

The db50x help viewer works roughly simiilarly to the DM42's, but with history
tracking and the ability to directly access help about a given function by
holding a key for more than half a second.

To navigate the help on the calculator, use the following keys:

* The soft menu keys at the top of the keyboard, references as _F1_ through
  _F6_, correspond to the functions shown in the six labels at the bottom of the
  screen.

* While the help is shown, the keys _▼_ and _▲_ on the keyboard scroll
  through the text.

* The _F1_ key returns to the [Home](#overview) (overview).

* The _F2_ and _F3_ keys (labels `Page▲` and `Page▼`) scroll the text one full
  page at a time.

* The _F4_ and _F5_ keys (labels `Link▲` and `Link▼`) select the previous and
  next link respectively. The keys _÷_ and _9_ also select the previous
  link, while the keys _×_ and _3_ can also be used to select the next link.

* The _F6_ key correspond to the `←Menu` label, and returns one step back in
  the help history. The _←_ key achieves the same effect.

* To follow a highlighted link, click on the _ENTER_ key.


## Acknowledgements and credits

db50x is Free Software, see the LICENSE file for details.
You can obtain the source code for this software at the following URL:
https://github.com/c3d/db50x-on-DM42.

### Authors

This software is (C) 2022-2023 Christophe de Dinechin and the db50x team.

Additional contributors to the project include:

* Camille Wormser
* Jeff, aka spiff72

The authors would like to acknowledge

* [Hewlett and Packard](#hewlett-and-packard)
* [The Maubert Team](#the-maubert-team)
* [Museum of HP calculators](#hp-museum)
* [The newRPL project](#newrpl-project)
* [The WP43 and C47 projects](#wp43-and-c47-projects)
* [SwissMicro's DMCP](#swissmicros-dmcp)
* [Intel Decimal Floating-Point Math Library v2.2](#intel-decimal-floating-point-math)


### Hewlett and Packard

Hand-held scientific calculators changed forever when Hewlett and Packard asked
their engineers to design and produce the HP35, then again when their company
introduced the first programmable hand-held calculator with the HP65, and
finally when they introduced the RPL programming language with the HP28.

Christophe de Dinechin, the primary author of db50x, was lucky enough to meet
both Hewlett and Packard in person, and this was a truly inspiring experience.
Launching the Silicon Valley is certainly no small achievement, but this pales
in comparison to bringing RPN and RPL to the world.


### The Maubert Team

Back in the late 1980s and early 1990s, a team of young students with a passion
for HP calculators began meeting on a regular basis at or around a particular
electronics shop in Paris called "Maubert Electronique", exchanging
tips about how to program the HP48 in assembly language or where to get precious
technical documentation.

A lot of their productions, notably the HP48 Metakernel, can still be found
on [hpcalc.org](https://www.hpcalc.org/hp48/apps/mk/) to this day. A few of
these early heroes would go on to change the
[history of Hewlett-Packard calculators](https://www.hpcalc.org/goodbyeaco.php),
including Cyrille de Brébisson, Jean-Yves Avenard and Gerald Squelart.

Another key contributor of that era is Paul Courbis, who had
carefully reverse-engineered and documented
[the internals of RPL calculators](https://literature.hpcalc.org/items/1584),
allowing his readers to waste countless hours debugging PacMan and Lemmings
clones for these wonderful little machines.


### HP Museum

The [HP Museum](https://www.hpmuseum.org) not only extensively documents the
history of RPN and RPL calcuators, it also provides a
[very active forum](https://www.hpmuseum.org/forum/) for calculator enthusiasts
all over the world.


### newRPL project

[newRPL](https://newrpl.wiki.hpgcc3.org/doku.php) is a project initiated by
Claudio Lapilli to implement a native version of RPL, initially targeting
ARM-based HP calculators such as the HP50G.

db50x inherits many ideas from newRPL, including, but not limited to:

* Implementing RPL natively for ARM CPUs
* Adding indicators in the cursor to indicate current status
* Integrating a catalog of functions to the command line

A first iteration of db50x started as a
[branch of newRPL](https://github.com/c3d/db48x/), although the
current implementation had to restart from scratch due to heavy space
constraints on the DM42.


### WP43 and C47 projects

The db50x took several ideas and some inspiration from the
[WP43](https://gitlab.com/rpncalculators/wp43) and
[C47](https://47calc.com) projects.

Walter Bonin initiated the WP43 firwmare for the DM42 as a "superset of the
legendary HP42S RPN Scientific".

C47 (initially called C43) is a variant of that firmware initiated by Jaco
Mostert, which focuses on compatibility with the existing DM42, notably with
respect to keyboard layout.

db50x borrowed at least the following from these projects:

* The very idea of writing a new firmware for the DM42
* The idea of converting standard Unicode TrueType fonts into bitmaps
  (with some additional contributions from newRPL)
* How to recompute the CRC for QSPI images so that the DM42 loads them,
  thanks to Ben Titmus
* At least some aspects of the double-shift logic and three-level menus
* The original keyboard layout template and styling, with special thanks
  to DA MacDonald.


### SwissMicros DMCP

[SwissMicros](https://www.swissmicros.com/products) offers a range of
RPN calculators that emulate well-known models from Hewlett-Packard.
This includes the [DM42](https://www.swissmicros.com/product/dm42),
which is currently the primary target for the db50x firmware.

Special thanks and kudos to Michael Steinmann and his team for keeping
the shining spirit of HP RPN calculators alive.

The DM42 version of the db50x software relies on
[SwissMicro's DMCP SDK](https://github.com/swissmicros/SDKdemo), which
is released under the following BSD 3-Clause License:

Copyright (c) 2015-2022, SwissMicros
All rights reserved.

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions are met:

* Redistributions of source code must retain the above copyright notice, this
  list of conditions and the following disclaimer.

* Redistributions in binary form must reproduce the above copyright notice,
  this list of conditions and the following disclaimer in the documentation
  and/or other materials provided with the distribution.

* Neither the name of the copyright holder nor the names of its
  contributors may be used to endorse or promote products derived from
  this software without specific prior written permission.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE
FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.


### Intel Decimal Floating-Point Math

Floating-point computations in db50x take advantage of
[Intel's decimal floating-point math library](https://www.intel.com/content/www/us/en/developer/articles/tool/intel-decimal-floating-point-math-library.html),
 which is released with the following end-user license agreement:

Copyright (c) 2018, Intel Corp.

All rights reserved.

Redistribution and use in source and binary forms, with or without modification,
are permitted provided that the following conditions are met:

* Redistributions of source code must retain the above copyright notice, this
  list of conditions and the following disclaimer.

* Redistributions in binary form must reproduce the above copyright notice,
  his list of conditions and the following disclaimer in the documentation
  and/or other materials provided with the distribution.

* Neither the name of Intel Corporation nor the names of its contributors
  may be used to endorse or promote products derived from this software
  without specific prior written permission.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
DISCLAIMED.

IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT,
INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING,
BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE
OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF
ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
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
ARM-based HP calculators would be good targets for a long-term port of db50x.

db50x is a fresh implementation of RPL on ARM, initially targetting the
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

The db50x version of RPL distinguishes between integer values, like `123`, and
[decimal values](#decimal-numbers), like `123.` Integer values are represented
internally in a compact and efficient format, saving memory and making
computations faster. All values between -127 and 127 can be stored in two bytes.
All values between -16383 and 16383 in three bytes.

Integers can be [as large as memory permits](#big-integers).


## Big integers

The db50x version of RPL can perform computations on arbitrarily large integers,
limited only by available memory, enabling for example the exact computation of
`100!` and making it possible to address problems that require exact integer
computations, like exploring the Syracuse conjecture.


## Decimal numbers

Decimal numbers are used to represent values with a fractional part.
db50x supports three decimal numbers, using the 32-bit, 64-bit and 128-bit
[binary decimal representation](#intel-decimal-floating-point-math).
In memory, all decimal numbers use one additional byte: a 32-bit decimal number
uses 5 bytes, a 128-bit binary decimal number uses 17 bytes.

The 32-bit format offers a 7 digits mantissa and has a maximum exponent
of 96. The 64-bit format offers a 16 digits mantissa and has a maximum
exponent of 384. The 128-bit format offers a 34 digits mantissa and a maximum
exponent of 6144.

The [Precision](#precision) command selects the default precision.

Note that a future implementation of db50x is expected to feature
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
one-dimensional, matrices are two-dimensional. db50x also supports tables with a
higher number of dimensions, but only offers limited operations on them.

db50x implements vector addition, subtraction, multipplication and division,
which apply component-wise. Multiplication and division are an extension
compared to the HP48.

db50x also implements matrix addition, subtraction, multiplication and
division. Like on the HP48, the division of matrix `A` by matrix `B` is
interpreted as left-multiplying `A` by the inverse of `B`.

As another extension, algebraic functions such as `sin` apply to all elements in
a vector or matrix in turn.
# Menus

Menus display at the bottom of the screen, and can be activated using the keys
on the top row of the calculator. Menus can refer to other menus. The calculator
keeps a history of the menus you visited previously, and you can return to an
earlier menu with the `BackMenu` function.


Here are the main menus in db50x, in alphabetical order.

## MainMenu

The *Main menu* gives access to _all_ the functions in your calculator, sorted
by cathegory. It includes the following submenus:

* [Math](#MathMenu): Mathematical operations
* [Symb](#SymbolicMenu): Symbolic operations
* [Units](#UnitsMenu): Unit conversions
* [System](#SystemMenu): System configuration
* [Prog](#ProggramMenu): Programming
* [Vars](#VariablesMenu): User variables


## MathMenu

The *Math menu* gives access to mathematical functions like [SIN](#sin) in your
calculator. It includes the following submenus:

* [Arith](#ArithmeticMenu): Arithmetic functions
* [Base](#BaseMenu): Based numbers
* [Trans](#TranscendentalMenu): Transcendental functions
* [Stats](#StatisticsMenu): Statistics
* [Lists](#ListsMenu): List operations
* [Matrix](#MatrixMenu): Matrices and vectors
* [Solve](#SolverMenu): Numerical solver


## VariablesMenu (VARS)

The variables menu displays the variables in the current directory.
It is a three row menu, where for each variable:

* The primary function [evaluates the variable](#VariablesMenuExecute)
* The first shifted function [recalls the variable](#VariablesMenuRecall)
* The second shifted function [stores in the variable](#VariablesMenuStore)

## VariablesMenuExecute

Hitting the primary function in the [Vars menu](#VariablesMenu) evaluates the
corresponding variable.

## VariablesMenuRecall

Hitting the first shifted function in the [Vars menu](#VariablesMenu) will
[recall](#Recall) the corresponding variable on the stack.

## VariablesMenuStore

Hitting the second shifted function in the [Vars menu](#VariablesMenu) will
[store](#Store) the top of stack in the corresponding variable.


## ToolsMenu

The `ToolsMenu` maps to the _A_ key (_Σ+_ on the original DM42 keyboard).
It invokes a context-dependent menu adapted to the top level of the stack.


## LastMenu

The `LastMenu` function, which is the shifted function for the _ A _ key,
returns back in the history of past visited menus.
# Operations with Angles

## TAGDEG
Mark a number as an angle in degrees


## TAGRAD
Mark a number as an angle in radians


## TAGGRAD
Mark a number as an angle in grads (gons)


## TAGDMS
Mark a number as an angle in DMS (DD.MMSS)


## ANGTODEG
Convert an angle to degrees


## ANGTORAD
Convert an angle to radians


## ANGTOGRAD
Convert an angle to grads (gons)


## ANGTODMS
Convert an angle to DMS (DD.MMSS)


## TORECT
Convert vector or complex to cartesian coordinates


## TOPOLAR
Convert vector or complex to polar coordinates


## TOSPHER
Convert vector or complex to spherical coordinates

# Arithmetic

## + (add)

Add two values.

* For integer, fractional, decimal or complex numbers, this performs the
  expected numerical addition. For example, `1 2 +` is `3`.
* For equations and symbols, build a sum, eliminating zero additions if
  [autosimplify](#autosimplify) is active.
* For lists, concatenate lists, or add objets to a list. For example, `{ A } { B
  } +` is `{ A B }`, and `{ A B C } "D" +` is `{ A B C "D" }`.
* For text, concatenate text, or concatenate the text representation of an
  object to an existing text. For example `"X" "Y" + ` gives `"XY"`, and
  `"X=" 1 +` gives `"X=1"`.

## - (sub)

Subtract two values

* For integer, fractional, decimal or complex numbers, this performs the
  expected numerical subtraction. For example, `1 2 -` is `-1`.
* For equations and symbols, build a difference, eliminating subtraction of 0 if
  [autosimplify](#autosimplify) is active.

## + (add)

Add two values.

* For integer, fractional, decimal or complex numbers, this performs the
  expected numerical addition. For example, `1 2 +` is `3`.
* For vectors and matrices, add individual elements. For example,
  `[ 1 2 3 ] [ 4 5 6 ] +` is `[ 5 7 9 ]`.
* For equations and symbols, build a sum, eliminating zero additions
  when [autosimplify](#autosimplify) is active.
* For lists, concatenate lists, or add objets to a list. For example, `{ A } { B
  } +` is `{ A B }`, and `{ A B C } "D" +` is `{ A B C "D" }`.
* For text, concatenate text, or concatenate the text representation of an
  object to an existing text. For example `"X" "Y" + ` gives `"XY"`, and
  `"X=" 1 +` gives `"X=1"`.

## - (sub)

Subtract two values

* For integer, fractional, decimal or complex numbers, this performs the
  expected numerical subtraction. For example, `1 2 -` is `-1`.
* For vectors and matrices, subtract individual elements. For example,
  `[ 1 2 3 ] [ 1 3 0 ] -` is `[ 0 -1 3 ]`.
* For equations and symbols, build a difference, eliminating subtraction of 0
  when [autosimplify](#autosimplify) is active.


## × (*, mul)

Multiply two values.

* For integer, fractional, decimal or complex numbers, this performs the
  expected numerical multiplication. For example, `3 2 *` is `6`.
* For vectors, multiply individual elements (this is a deviation from HP48).
  For example, `[ 1 2 3 ] [ 4 5 6 ] +` is `[ 4 10 18 ]`.
* For matrices, perform a matrix multiplication.
* For a matrix and a vector, apply the matrix to the vector.
* For equations and symbols, build a product, eliminating mulitplication by 1
  or 0 when [autosimplify](#autosimplify) is active.
* For a list and a positive integer, repeat the list For example, `{ A } 3 *`
  is `{ A A A }`.
* For a text and a positive integer, repeat the text. For example `"X" 3 * `
  gives `"XXX"`.


## ÷ (/, div)

Divide two values two values

* For integer, build a fraction. For example `1 7 /` gives `1/7`.
* For fractional, decimal or complex numbers, this performs the
  expected numerical division. For example, `1. 2. /` is `0.5`.
* For vectors, divide individual elements. For example,
  `[ 1 2 3 ] [ 3 2 1 ] /` is `[ 1/3 1 3 ]`.
* For equations and symbols, build a ratio, eliminating division by one
  and division of 0 when [autosimplify](#autosimplify) is active.


## ↑ (^, pow)

Raise to the power

* For integer, fractional, decimal or complex numbers, this raises the
  value in level 2 to the value in level 1. For example, `2 3 ↑` is `8`.
* For vectors, raise individual elements in the first vector to the power of the
  corresponding element in the second vector.
* For equations and synbols, build an expression, eliminating special cases
  when [autosimplify](#autosimplify) is active.


## xroot

Raise to the inverse power. `X Y xroot` is equivalent to `X Y inv pow`.


# Integer arithmetic and polynomials

## SETPREC
Set the current system precision


## GETPREC
Get the current system precision


## FLOOR
Largest integer less than the input


## CEIL
Smallest integer larger than the input


## IP
Integer part of a number


## FP
Fractional part of a number


## MODSTO
Set the current system modulo for all MOD operations


## MODRCL
Get the current system modulo


## POWMOD
Power operator MOD the current system modulo


## MOD
Remainder of the integer division


## SQ
Square of the input


## NEXTPRIME
Smallest prime number larger than the input


## FACTORIAL
Factorial of a number


## ISPRIME
Return true/false (1/0) if a number is prime or not


## MANT
Mantissa of a real number (M*10<sup>exp</sup>)


## XPON
Exponent of a number represented as (M*10<sup>exp</sup>)


## SIGN
Sign of a number, -1, 0 or 1.

For complex numbers, returns a unit number on the unit circle with the same
argument as the original number.


## PERCENT
Percentage of a number


## PERCENTCH
Percentage of change on a number


## PERCENTTOT
Get percentage of a total


## GCD
Greatest common divisor


## LCM
Least common multiple


## IDIV2
Integer division, get quoteiant and remainder


## IQUOT
Quotient of the integer division


## ADDTMOD
Addition operator MOD the current system modulo


## SUBTMOD
Subtraction operator MOD the current system modulo


## MULTMOD
Multiplication operator MOD the current system modulo


## PEVAL
Evaluation of polynomial given as vector of coefficients


## PCOEF
Coefficients of monic polynomial with the given roots


## IEGCD
Extended euclidean algorithm


## IABCUV
Find integers u,v to solve a*u+b*v=c


## PTCHEBYCHEFF
Nth Tchebycheff polynomial


## PLEGENDRE
Nth Legendre polynomial


## PHERMITE
Nth Hermite polynomial as used by physics


## PTCHEBYCHEFF2
Nth Tchebycheff polynomial of the second kind


## PHERMITE2
Nth Hermite polynomial as used in probabilities


## DIV2
Polynomial euclidean division as symbolic


## PDIV2
Polynomial euclidean division as coefficient vector


## PDER
Derivative of polynomial as coefficient vector


## PINT
Integration of polynomials as coefficient vector


## PMUL
Multiplication of polynomials as coefficient vectors


## PADD
Addition of polynomials as coefficient vector


## PSUB
Subtraction of polynomials as coefficient vector


## MIN
Smallest of 2 objects


## MAX
Largest of 2 objects


## RND
Round a number to the given number of figures


## TRNC
Truncate a number to the given number of figures


## DIGITS
Extract digits from a real number


## PROOT
All roots of a polynomial


## PREVPRIME
Largest prime smaller than the input


## FACTORS
Factorize a polynomial or number
# Base functions

## Evaluate (EVAL)

Evaluate the object at stack level 1.

Mapped to the _ R/S _ key

`X` ▶ Result of `X` evaluation

## Drop

## Negate (NEG)

Negate the value in level 1.

Mapped to the _ +/- _ key

`X` ▶ `0-X`

## Invert (INV)

Invert the value in level 1

Mapped to the _ 1/X _ key

`X` ▶ `1/X`
# Bitwise operations

## STWS
Store current word size in bits (0-63)


## RCWS
Recall the currnent word size in bits


## BOR
Bitwise OR operation


## BAND
Bitwise AND operator


## BXOR
Bitwise XOR operation


## BLSL
Bitwise logical shift left


## BLSR
Bitwise logical shift right


## BASR
Bitwise arithmetic shift right


## BRL
Bitwise rotate left


## BRR
Bitwise rotate right


## BNOT
Bitwise inversion of bits


## BADD
Bitwise addition with overflow


## BSUB
Bitwise subtraction with overflow


## BMUL
Bitwise multiplication


## BDIV
Bitwise integer division


## BNEG
Bitwise negation

# Bitmaps

## TOSYSBITMAP

# Comments

## STRIPCOMMENTS
Remove all comments from a compiled program

# Operations with Complex Numbers

## RE
Real part of a complex number


## IM
Imaginary part of a complex number


## ARG
Argument of a complex number


## CONJ
Conjugate of a complex number


## CPLX2REAL
Split Complex into two Reals


## REAL2CPLX
Make Complex from real and imaginary parts

# Lists, Matrix and String commands

## PUT
Replace an item in a composite


## PUTI
Replace an item and increase index


## GET
Extract an item from a composite


## GETI
Extract an item and increase index


## HEAD
Extract the first item in a composite


## TAIL
Removes the first item in a composite


## OBJDECOMP
Explode an object into its components


## REPL
Replace elements in a composite


## POS
Find the position of an element in a composite


## NPOS
Find object in a composite, starting from index N


## POSREV
Find the position of an element, starting from the end


## NPOSREV
Find the position from the end, starting at index N


## SUB
Extract a group of elements from a composite


## SIZE
Number of elements in a composite


## RHEAD
Returns the last element from the composite


## RTAIL
Removes the last element from the composite

# Constants

## PICONST


## ICONST


## ECONST


## JCONST

# Variables

Variables are named storage for RPL values.

## Store (STO)
Store an object into a global variable

## Recall (RCL)
Recall the contents of a variable


## StoreAdd (STO+)
Add to the content of a variable


## StoreSubtract (STO-)
Subtract from the contents of a variable


## StoreMultiply (STO×)
Multiply contents of a variable


## StoreDivide (STO÷)
Divide the content of a variable


## Increment (INCR)
Add one to the content of a variable


## Decrement (DECR)
Subtract one from content of a variable


## Purge

Delete a global variable from the current directory

*Remark*: `Purge` only removes a variable from the current directory, not the
enclosing directories. Since [Recall](#Recall) will fetch variable values from
enclosing directories, it is possible that `'X' Purge 'X' Recall` will fetch a
value for `X` from an enclosing directory. Use [PurgeAll](#PurgeAll) if you want
to purge a variable including in enclosing directories.

## PurgeAll

Delete a global variable from the current directory and enclosing directories.

*Remark*: If a variable with the same name exists in multiple enclosing
directories, `PurgeAll` may purge multiple variables. Use [Purge](#Purge) if you
want to only purge a variable in the current directory.


## CreateDirectory (CRDIR)
Create new directory


## PurgeDirectory (PGDIR)
Purge entire directory tree


## UpDirectory (UPDIR)
Change current directory to its parent


## HomeDirectory (HOME)
Change current directory to HOME


## DirectoryPath (PATH)
Get a path to the current directory


## Variables (VARS)
List all visible variables in a directory


## ALLVARS
List all variables in a directory


## ORDER
Sort variables in a directory


## QUOTEID
Add single quotes to a variable name


## UNQUOTEID
Remove single quotes from a variable name


## HIDEVAR
Hide a variable (make invisible)


## UNHIDEVAR
Make a hidden variable visible


## CLVAR
Purge all variables and empty subdirectories in current directory


## LOCKVAR
Make variable read-only


## UNLOCKVAR
Make variable read/write


## RENAME
Change the name of a variable


## TVARS
List variables of a specific type


## TVARSE
List all variables with extended type information


## SADD
Apply command ADD to the stored contents of the variable


## SPROP
Store a property to a variable


## RPROP
Recall a property of a variable


## PACKDIR
Pack a directory in an editable object
# Errors and error handlers

## EXITRPL
Panic exit - abort the RPL engine.


## EVAL1NEXT
Perform EVAL1 on the next object in a secondary and skips it


## RESUME
End error handler and resume execution of main program


## DOERR
Issue an error condition


## ERRN
Recall the previous error code


## ERRM
Recall the previous error message


## ERR0
Clear previous error code


## HALT
Halt the execution of RPL code


## CONT
Continue execution of a halted program


## SST
Single-step through a halted program, skip over subroutines


## SSTIN
Single-step through a halted program, goes into subroutines


## KILL
Terminate a halted program


## SETBKPOINT
Set a breakpoint on a halted program


## CLRBKPOINT
Remove a breakpoint


## DBUG
Halt the given program at the first instruction for debugging


## BLAMEERR
Issue an error condition, blame other program for it


## EXIT
Early exit from the current program or loop

# Flow control

## If

The `if` statement provides conditional structurs that let a program make decisions. It comes in two forms:

* `if` *condition* `then` *true-clause* `end`: This evaluates *condition* and, if true, evaluates *true-clause*.

* `if` *condition* `then` *true-clause* `else` *false-clause* `end`: This evaluates *condition* and, if true, evaluates *true-clause*, otherwise evaluates *false-clause*.

A condition is true if:
* It is a number with a non-zero value
* It is the word `True`

A condition is false if:
* It is a number with a zero value
* It is the word `False`


## CASE
Conditional CASE ... THEN ... END THEN ... END END statement


## THENCASE
Conditional CASE ... THEN ... END THEN ... END END statement


## ENDTHEN
Conditional CASE ... THEN ... END THEN ... END END statement


## ENDCASE
Conditional CASE ... THEN ... END THEN ... END END statement


## FOR
Loop FOR ... NEXT/STEP statement


## START
Loop START ... NEXT/STEP statement


## NEXT
Loop FOR/START ... NEXT statement


## STEP
Loop FOR/START ... STEP statement


## DO
Loop DO ... UNTIL ... END statement


## UNTIL
Loop DO ... UNTIL ... END statement


## ENDDO
Loop DO ... UNTIL ... END statement


## WHILE
Loop WHILE ... REPEAT ... END statement


## REPEAT
Loop WHILE ... REPEAT ... END statement


## ENDWHILE
Loop WHILE ... REPEAT ... END statement


## IFERR
Conditional IFERR ... THEN ... ELSE ... END statement


## THENERR
Conditional IFERR ... THEN ... ELSE ... END statement


## ELSEERR
Conditional IFERR ... THEN ... ELSE ... END statement


## ENDERR
Conditional IFERR ... THEN ... ELSE ... END statement


## FORUP
Loop FORUP ... NEXT/STEP statement


## FORDN
Loop FORUP ... NEXT/STEP statement
# Menus, Flags and System Settings

## SETLOCALE
Change the separator symbols


## SETNFMT
Change the display format for numbers


## SF
Set a flag


## CF
Clear a flag


## FCTEST
Test if a flag is clear


## FSTEST
Test if a flag is set


## FCTESTCLEAR
Test if a flag is clear, then clear it


## FSTESTCLEAR
Test if a flag is set, then clear it


## TMENU
Display the given menu on the active menu area


## TMENULST
Display the given menu on the menu area the user used last


## TMENUOTHR
Display the given menu on the menu are the user did not use last


## MENUSWAP
Swap the contents of menu areas 1 and 2


## MENUBK
Display the previous menu on the active menu area


## MENUBKLST
Display the previous menu on the area the user used last


## MENUBKOTHR
Display the previous menu on the area the user did not use last


## RCLMENU
Recall the active menu


## RCLMENULST
Recall the menu the user used last


## RCLMENUOTHR
Recall the menu the user did not use last


## DEG
Set the angle mode flags to degrees


## GRAD
Set the angle mode flags to grads (gons)


## RAD
Set the angle mode flags to radians


## DMS
Set the angle mode to DMS (as DD.MMSS)


## ASNKEY
Assign a custom definition to a key


## DELKEY
Remove a custom key definition


## STOKEYS
Store and replace all custom key definitions


## RCLKEYS
Recall the list of all custom key definitions



## TYPEE
Get extended type information from an object


## GETLOCALE
Get the current separator symbols


## GETNFMT
Recall the current display format for numbers


## RCLF
Recall all system flags


## STOF
Store and replace all system flags


## VTYPE
Get type information on the contents of a variable


## VTYPEE
Get extended type information on the contents of a variable


## FMTSTR
Do →STR using a specific numeric format
# Fonts

## FNTSTO
Install a user font for system use


## FNTRCL
Recall a system font


## FNTPG
Purge a user-installed system font


## FNTSTK
Recall name of current font for stack area


## FNT1STK
Recall name of current font for stack level 1


## FNTMENU
Recall name of current font for menu area


## FNTCMDL
Recall name of current font for command line area


## FNTSTAT
Recall name of current font for status area


## FNTPLOT
Recall name of current font for plot objects


## FNTFORM
Recall name of current font for forms


## STOFNTSTK
Change current font for stack area


## STOFNT1STK
Change current font for stack level 1


## STOFNTMENU
Change current font for menu area


## STOFNTCMDL
Change current font for command line area


## STOFNTSTAT
Change current font for status area


## STOFNTPLOT
Change current font for plot objects


## STOFNTFORM
Change current font for forms


## FNTHELP
Recall name of current font for help


## FNTHLPT
Recall name of current font for help title


## STOFNTHELP
Change current font for help text


## STOFNTHLPT
Change current font for help title

# Graphic commands

db50x features a number of graphic commands. While displaying graphics, the
stack and headers will no longer be updated.

## Coordinates

db50x recognizes the following types of coordinates

* *Pixel coordinates* are specified using based numbers such as `#0`, and
  correspond to exact pixels on the screen, and . Pixels are counted starting
  from the top-left corner of the screen, with the horizontal coordinate going
  from `10#0` to `10#399`, and the vertical coordinate going from `10#0` to
  `10#239`.

* *User unit coordinates* are scaled according to the content of the `PPAR` or
  `PlotParameters` reserved variables.

* *Text coordinates* are given on a square grid with a size corresponding to the
  height of a text line in the selected font. They can be fractional.

Coordinates can be given using one the following object types:

* A complex number, where the real part represents the horizontal coordinate and
  the imaginary part represents the vertical coordinate.

* A 2-element list or vector containing the horizontal and vertical coordinates.

* A 1-element list of vector containing one of the above.

For some operations, the list or vector can contain additional parameters beyond
the coordinates. The selection of unit or pixel coordinates is done on a per
coordinate basis. For exmaple, `{ 0 0 }` will be the origin in user coordinates,
in the center of the screen if no `PPAR` or `PlotParameters` variable is
present.

Note that unlike on the HP48, a complex value in db50x can
contain a based number.


## ClearLCD (cllcd)

Clear the LCD display, and block updates of the header or menu areas.


## DrawText (disp)

Draw the text or object in level 2 at the position indicated by level 1. A text
is drawn without the surrounding quotation marks.

If the position in level 1 is an integer, fraction or real number, it is
interpreted as a line number starting at 1 for the top of the screen. For
example, `"Hello" 1 disp` will draw `Hello` at the top of the screen.

If the position in level 1 is a complex number or a list, it is interpreted as
specifying both the horizontal or vertical coordinates, in either pixel or unit
coordinates. For example `"Hello" { 0 0 } disp` will draw `Hello` starting in
the center of the screen.

Text is drawn using the stack font by default, using the
[foreground](#foreground) and [background](#background) patterns.

If level 1 contains a list with more than 2 elements, additional elements
provide:

* A *font number* for the text

* An *erase* flag (default true) which indicates whether the background for the
  text should be drawn or not.

* An *invert* flag (default false) which, if set, will swap the foreground and
  background patterns.

For example, `"Hello" { #0 #0 0 true true } DrawText` will draw `Hello` in the
top-left corner (`#0 #0`) with the largest (editor) font (font identifier `0`),
erasing the background (the first `true`), in reverse colors (the second
`true`).

## DrawLine (line)

Draw a line between two points specified by level 1 and level 2 of the stack.

The width of the line is specified by [LineWidth](#linewidth). The line is drawn
using the [foreground](#foreground) pattern.


## PlotParameters (PPAR)

The `PlotParameters` reserved variable defines the plot parameters, as a list,
with the following elements:

* *Lower Left* coordinates as a complex (default `-10-6i`)

* *Upper Right* coordinates as a complex (default `10+6i`)

* *Independent variable* name (default `x`)

* *Resolution* specifying the interval between values of the independent
  variable (default `0`). A binary numnber specifies a resolution in pixels.

* *Axes* which can be a complex giving the origin of the axes (default `0+0i`),
  or a list containing the origin, the tick mark specification, and the names of
  the axes.

* *Type* of plot (default `function`)

* *Dependent variable* name (default `y`)
# Local Variables

## LSTO
Store to a new local variable


## LRCL
Recall content of local variable


## HIDELOCALS
Hide local variables from subroutines


## UNHIDELOCALS
Unhide local variables from subroutines


## INTERNAL_NEWNLOCALS

# Arbitrary data containers

## MKBINDATA
Create binary data container object


## BINPUTB
Store bytes into binary data object


## BINGETB
Extract binary data as list of bytes


## BINPUTW
Store 32-bit words into binary data object


## BINGETW
Extract data from a binary data object as a list of 32-bit words


## BINPUTOBJ
Store an entire object into a binary data container


## BINGETOBJ
Extract an entire object from a binary data container


## BINMOVB
Copy binary data block into a binary data object


## BINMOVW
Copy 32-bit words between binary data objects

# User Libraries

## CRLIB
Create a library from current directory


## ATTACH
Install a library


## DETACH
Uninstall a library


## LIBMENU
Show a menu within a library


## LIBMENUOTHR
Show library menu in the other menu


## LIBMENULST
Show library menu in the last used menu


## LIBSTO
Store private library data


## LIBRCL
Recall private library data


## LIBDEFRCL
Recall private data with default value


## LIBCLEAR
Purge all private data for a specific library

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


## SORT
Sort elements in a list


## REVLIST
Reverse the order of elements in a list


## ADDROT
Add elements to a list, keep only the last N elements


## SEQ
Assemble a list from results of sequential procedure

# Operations with Matrices and vectors

## TOARRAY
Assemble an array from its elements


## ARRAYDECOMP
Split an array into its elements


## TOCOL
Split an array into column vectors


## ADDCOL
Instert a column into an array


## REMCOL
Remove a column from an array


## FROMCOL
Assemble a matrix from its columns


## TODIAG
Extract diagonal elements from a matrix


## FROMDIAG
Create a matrix with the given diagonal elements


## TOROW
Split an array into its row vectors


## ADDROW
Insert a row into an array


## REMROW
Remove a row from an array


## FROMROW
Assemble an array from its rows


## TOV2
Assemble a vector from two values


## TOV3
Assemble a vector from three values


## FROMV
Split a vector into its elements


## AXL
Convert a matrix to list and vice versa


## BASIS
Find vectors forming a basis of the subspace represented by the matrix


## CHOLESKY
Perform Cholesky decomposition on a matrix


## CNRM
Column norm (one norm) of a matrix


## CON
Assemble an array with given constant value


## COND
Column norm condition number of a matrix


## CROSS
Cross produce of vectors


## CSWP
Swap two columns in a matrix


## DET
Determinant of a matrix


## DIAGMAP


## DOT
Internal product (dot product) of vectors


## EGV


## EGVL
Compute the eigenvalues of a matrix


## GRAMSCHMIDT


## HADAMARD
Multiply corresponding elements in a matrix


## HILBERT
Assemble a Hilbert symbolic array


## IBASIS
Find a basis of the intersection of two vector spaces


## IDN
Assemble an identity matrix


## IMAGE
Find a basis of the image of a linear application


## ISOM


## JORDAN


## KER
Find a basis for the kernel of a linear application


## LQ


## LSQ


## LU
LU factorization of a matrix


## MAD


## MKISOM


## PMINI
Minimal polynomial of a matrix


## QR
QR Decomposition of a matrix


## RANK
Rank of a matrix


## RANM
Assemble a matrix with random numbers


## RCI
Multiply a row by a constant


## RCIJ
Multiply a row by a constant and add to other row


## RDM
Change dimensions of an array


## REF
Reduce matrix to echelon form (upper triangular form)


## RNRM
Row norm (infinity norm) of a matrix


## RREF
Fully reduce to row-reduced echelon form


## RREFMOD


## RSD
Residual R=B-A*X' on a system A*X=B


## RSWP
Swap two rows in a matrix


## SCHUR


## SNRM


## SRAD


## SVD


## SVL


## SYLVESTER


## TRACE
Sum of the items in the diagonal of a matrix


## TRAN
Transpose a matrix


## TRN
Complex conjugate transpose of a matrix


## VANDERMONDE


## LDUP
Decompose A into LDUP such that P*A=L*D<sup>-1</sup>*U


## MMAP
Apply expression or program to the elements of a matrix

# Numerical functions

## ∫ (Integrate)

Perform a numerical integration of a function for a specified variable on a
numerical interval. For example `2 3 'X*(X-3)' 'X' Integrate` returns `-7/6`.

The function takes four arguments:

* The lower bound of the integration range
* The higher bound of the integration range
* The program or expression to evaluate
* The integration variable


## Root

Root-finder command. Returns a real number that is a value of the specified
variable for which the specified program or algebraic object most nearly
evaluates to zero or a local extremum. For example, `'X^2=3' 'X' 0` returns
`X:1.732050807568877293527446341953458`.

The function takes three arguments:

* The program or expression to evaluate
* The variable to solve for
* An initial guess, or a list containing an upper and lower guess.
# Cycle

Cycle through various representations of the object on the first level of the stack.

* Polar <-> Rectangular for complex numbers
* Decimal <-> Fraction
* Integer <-> Based (cycles through the 2, 8, 10 and 16 base)
* Array <-> List <-> Program
* Text <-> Symbol
# Scalable plots and graphics

## BEGINPLOT
Initialize a new current plot object


## EDITPLOT
Set the current plot object to the given graphic


## ENDPLOT
Finish current plot object and leave it on the stack


## STROKECOL
Change the current stroke color


## STROKETYPE
Change current stroke type


## FILLCOL
Change the current fill color


## FILLTYPE
Change the current fill type


## FILL
Fill the last polygon


## STROKE
Draw the outline of the last polygon


## FILLSTROKE
Draw the outline and fill the last polygon


## MOVETO
Move current coordinates


## LINETO
Draw a line


## CIRCLE
Draw a circle


## RECTANG
Draw a rectangle


## CTLNODE
Add a control node to the current polygon


## CURVE
Draw a curve using all previous control points


## BGROUP


## EGROUP


## DOGROUP


## BASEPT


## TRANSLATE


## ROTATE


## SCALE


## CLEARTRANSF


## SETFONT


## TEXTHEIGHT


## TEXTOUT


## INITRENDER
Set which library will be used as default renderer


## DORENDER
Render a graphics object using the current renderer


## PANVIEW
Shift the center of viewport to render graphics


## ROTVIEW


## SCLVIEW
Set scale to render graphics


## VIEWPORT


## VIEWALL

# SD Card

## SDRESET
Reset the file system module


## SDSETPART
Set active partition


## SDSTO
Store a an object into a file


## SDRCL
Recall an object from a file


## SDCHDIR
Change current directory


## SDUPDIR
Change to parent directory


## SDCRDIR
Create a new directory


## SDPGDIR
Delete an entire directory


## SDPURGE
Delete a file


## SDOPENRD
Open a file for read-only operation


## SDOPENWR
Open a file for writing


## SDOPENAPP
Open a file in append mode


## SDOPENMOD
Open a file in modify mode


## SDCLOSE
Close an open file


## SDREADTEXT
Read text from an open file (UTF-8 encoding)


## SDWRITETEXT
Write text to a file (UTF-8 encoding)


## SDREADLINE
Read one line of text from a file


## SDSEEKSTA
Move position to given offset from start of file


## SDSEEKEND
Move position to given offset from end of file


## SDSEEKCUR
Move position to given offset from the current point.


## SDTELL
Get the current position


## SDFILESIZE
Get the file size in bytes


## SDEOF
Return true if last operation reached end of file


## SDOPENDIR
Open a directory to scan entries


## SDNEXTFILE
Get the next entry in a directory that is a file


## SDNEXTDIR
Get the next entry in a directory that is a subdirectory


## SDNEXTENTRY
Get the next entry in a directory


## SDMOVE
Move or rename a file


## SDCOPY
Copy a file


## SDPATH
Get the path to current directory


## SDFREE
Get the free space in the current volume


## SDARCHIVE
Create a full calculator backup on a file


## SDRESTORE
Restore from a backup stored in a file


## SDGETPART
Get the current partition number

# Settings

The calculator has a number of user-configurable settings:

* [Display](#display-settings)
* [Angles](#angle-settings)
* [Command display](#command-display)
* [Decimal separator](#decimal-separator-settings)
* [Precision](#precision-settings)
* [Base](#base-settings)
* [User interface](#user-interface)

The current preferences can be retrieved and saved using the `Modes` command.

## Modes

Returns a program that will restore the current settings. This program can be saved into a variable to quickly restore a carefully crafted set of preferences. Note that the calculator automatically restores the mode when it [loads a state](#States).

# Display settings

The display mode controls how db50x displays numbers. Regardless of the display
mode, numbers are always stored with full precision.

db50x has five display mode (one more than the HP48)s:

* [Standard mode](#StandardDisplay)
* [Fixed mode](#FixedDisplay)
* [Scientific mode](#ScientificDisplay)
* [Engineering mode](#EngineeringDisplay))
* [Significant digits mode](#SignificantDisplay))

## StandardDisplay (STD)

Display numbers using full precision. All significant digts to the right of the
decimal separator are shown, up to 34 digits.

## FixedDisplay (FIX)

Display numbers rounded to a specific number of decimal places.

## ScientificDisplay (SCI)

Display numbers in scientific notation, i.e. with a mantissa and an
exponent. The mantissa has one digit to the left of the decimal separator and
shows the specified number of decimal places.

## EngineeringDisplay (SCI)

Display nunmbers as a mantissa with a sepcified number of digits, followed by an
exponent that is a multiple of 3.

## SignificantDisplay (SIG)

Display up to the given number of digits without trailing zero. This mode is
useful because db50x can compute with large precision, and it may be useful to
not see all digits. `StndardDisplay` is equivalent to `34 SignificantDisplay`,
while `12 SignificantDisplay` should approximate the HP48 standard mode using
12 significant digits.

## StandardExponent

Select the maximum exponent before switching to scientific notation. The default value is 9, meaning that display uses scientific notation for exponents outside of -9..9.

## MinimumSignificantDigits

Select the minimum number of significant digits before switching to scientific notation in `FIX` mode.

The default value is 0, which is similar to how HP calculators perform. For example, with `2 FIX`, the value `0.055` will display as `0.06`, and `0.0055` will display as `0.01`.

A higher value will switch to scienfic mode to show at least the given number of digits. For instance, with `2 FIX`, if the value is `1`, then `0.055` will still display as `0.06` but `0.0055` will display as `5.50E-3`. If the value is `2`, then `0.055` will display as `5.5E-2`.


## TrailingDecimal

Display a trailing decimal separator to distinguish decimal from integer types. With this setting, `1.0` will display as `1.`. This can be disabled with [NoTrailingDecimal](#NoTrailingDecimal).


## NoTrailingDecimal

Hide the trailing decimal separator for decimal values with no fractional part. In that mode, `1.0` and `1` will both display identically, although the internal representation is different, the former being a floating-point value while the latter is an integer value.

## FancyExponent

Display the exponent in scientific mode using a fancy rendering that is visually similar to the normal mathematical notation.

## ClassicExponent

Display the exponent in scientific mode in a way reminiscent of classical HP48 calculators, for example `1.23E-4`.

# Angle settings

The angle mode determines how the calculator interprets angle arguments and how
it returns angle results.

db50x has four angle modes:

* [Degrees](#Degrees): A full circle is 360 degress
* [Radians](#Radians): A full circle is 2π radians
* [Grads](#Grads): A full circle is 400 radians
* [PiRadians](#PiRadians): Radians shown as multiple of π

## Degrees (DEG)

Select degrees as the angular unit. A full circle is 360 degrees.

## Radians (RAD)

Select radians as the angular unit. A full circle is 2π radians,
and the angle is shown as a numerical value.

## Grads (GRAD)

Select grads as the angular unit. A full circle is 400 grads.

## PiRadians (PIRAD)

Select multiples of π as the angular unit. A full circle is 2π radians,
shown as a multiple of π.


# Command display

db50x can display commands either using a short legacy spelling, usually
identical to what is used on the HP-48 series of calculators, or use an
alternative longer spelling. For example, the command to store a value in a
variable is called `STO` in the HP-48, and can also be spelled `Store` in db50x.

Commands are case insensitive, and all spellings are accepted as input
irrespective of the display mode.

db50x has four command spelling modes:

* [Lowercase](#LowerCase): Display `sto`
* [Uppercase](#UpperCase): Display `STO`
* [Capitalized](#Capitalized): Display `Sto`
* [Long form](#LongForm): Display `Store`

## LowerCase

Display comands using the short form in lower case, for example `sto`.

## UpperCase

Display comands using the short form in upper case, for example `STO`.

## Capitalized

Display comands using the short form capitalized, for example `Sto`.

## LongForm

Display comands using the long form, for example `Store`.

# Decimal separator settings

The decimal separator can be either a dot (`1.23`) or a comma (`1,23`).

## DecimalDot

Select the dot as a decimal separator, e.g.  `1.23`

## DecimalComma

Select the comma as a decimal separator, e.g.  `1,23`

# Precision settings

## Precision

Set the default computation precision, given as a number of decimal digits. For example, `7 Precision` will ensure at least 7 decimal digits for compuation, and `1.0 3 /` will compute `0.3333333` in that case.

In the current implementation, this selects one of three decimal formats:

* The `decimal32` for up to 7 digits mantissa and an exponents up to 96
* The `decimal64` for up to 16 digits mantissa and an exponents up to 384
* The `decimal128` for up to 34 digits mantissa and an exponents up to 6144

The intent in the long run is to allow arbitrary precision like in newRPL.


# Base settings

Integer values can be reprecended in a number of different bases:

* [Binary](#Binary) is base 2
* [Ocgtal](#Octal) is base 8
* [Decimal](#Decimal) is base 10
* [Hexadecimal](#Hexadecimal) is base 16

## Binary (BIN)

Selects base 2

## Octal (OCT)

Selects base 8

## Decimal (DEC)

Selects base 10

## Hexadecimal (HEX)

Selects base 16

## Base

Select an arbitrary base for computations

## StoreWordSize (STWS)

Store the word size for binary computations

## WordSize (RCWS)

Recall the word size for binary computations

## MaxRewrites

Defines the maximum number of rewrites in an equation.

[Equations rewrites](#rewrite) can go into infinite loops, e.g. `'X+Y' 'A+B'
'B+A' rewrite` can never end, since it keeps rewriting terms. This setting
indicates how many attempts at rewriting will be done before erroring out.

## MaxBigNumBits

Define the maxmimum number of bits for a large integer.

Large integer operations can take a very long time, notably when displaying them
on the stack. With the default value of 1024 bits, you can compute `100!` but
computing `200!` will result in an error, `Number is too big`. You can however
compute it seting a higher value for `MaxBigNumBits`, for example
`2048 MaxBigNumBits`.

## ToFractionIterations (→QIterations, →FracIterations)

Define the maximum number of iterations converting a decimal value to a
fraction. For example, `1 →FracIterations 3.1415926 →Frac` will give `22/7`,
whereas `3 →FracIterations 3.1415926 →Frac` will give `355/113`.

## ToFractionDigits (→QDigits, →FracDigits)

Define the maximum number of digits of precision converting a decimal value to a
fraction. For example, `2 →FracDigits 3.1415926 →Frac` will give `355/113`.


# User interface

Various user-interface aspects can be customized, including the appearance of
Soft-key menus. Menus can show on one or three rows, with 18 (shifted) or 6
(flat) functions per page, and there are two possible visual themes for the
labels, rounded or square.

## ThreeRowsMenus

Display menus on up to three rows, with shift and double-shift functions showns
above the primary menu function.

## SingleRowMenus

Display menus on a single row, with labels changing using shift.

## FlatMenus

Display menus on a single row, flattened across multiple pages.

## RoundedMenu

Display menus using rounded black or white tabs.

## SquareMenus

Display menus using square white tabs.

## CursorBlinkRate

Set the cursor blink rate in millisecond, between 50ms (20 blinks per second)
and 5000ms (blinking every 5 seconds).

# States

The calculator can save and restore state in files with extension `.48S`.
This feature is available through the `Setup` menu (Shift-`0`).

The following information is stored in state files:

* Global variables
* Stack contents
* Settings
# Numeric solvers

## NUMINT
Numerical integration (adaptive Simpson)


## ROOT
Root seeking within an interval


## MSOLVE
Multiple non-linear equation solver/optimization search


## BISECT
Root seeking (bisection method)

# Stack manipulation

## Clear
Remove all objects from the stack


## Depth
Get the current stack depth


## Drop
Remove an object from the stack


## Drop2
Remove two objects form the stack


## DropN
Remove N objects from the stack, N being given in level 1.


## Duplicate (DUP)
Duplicate an object on the stack


## Duplicate2 (DUP2)
Duplicate two objects on the stack


## DuplicateTwice (DUPDUP)
Duplicate the same object twice on the stack

## DuplicateN (DUPN)
Duplicate a group of N objects, N being given in stack level 1

## LastArguments (LASTARG)
Put the last arguments back on the stack

## LastX
Put the last first argument on the stack.

This command does not exist on HP RPL calculators, and is here to make it easier
to adapt RPN programs that use LastX a bit more often.

## Undo
Restore the stack to its last state before executing an interactive command.
Note that this command can be used from a program, but it will restore the state
prior to program execution.

## NDUPN
Replicate one object N times and return N


## NIP
Remove object at level 2 on the stack


## Over
Duplicate object at level 2 on the stack


## PICK
Duplicate object at position N on the stack


## PICK3
Duplicate object at level 3 on the stack


## ROLL
Move object at level N to level 1


## ROLLD
Move object from level 1 to level N


## ROT
Move object from level 3 to level 1


## SWAP
Exchange objects in levels 1 and 2

Mapped to _X⇆Y_ key

`Y` `X` ▶ `X` `Y`


## UNPICK
Move object from level 1 to level N.


## UNROT
Move object from level 1 to level 3


## IFT
Evaluate objects on the stack conditionally


## IFTE
Evaluate objects on the stack conditionally


## STKPUSH
Push a snapshot of the current stack on the undo stack


## STKPOP
Pop a stack snapshot from the undo stack


## STKDROP
Drop a snapshot from the undo stack


## STKPICK
Copy snapshot in level N to the current stack


## STKDEPTH
Get the depth of the undo stack


## STKNEW
Push a snapshot of the current stack on the undo stack and clears the current stack
# Statistics

## RDZ
Initialize random number generator with a seed


## RAND
Generate a random real number

# Operations with Strings

## TOUTF
Create a Utf8 string from a list of code points


## FROMUTF
List all code points in a Utf8 string


## TOSTR
Decompile any object (convert to string)


## FROMSTR
Compile a string into RPL objects


## SREV
Reverse the characters on a string


## NTOKENS
Number of tokens in a string


## NTHTOKEN
Token at position N in a string


## NTHTOKENPOS
Position of token N in a string


## TRIM
Remove characters at end of string


## RTRIM
Remove characters at start of string


## SSTRLEN
Length of string in characters


## STRLENCP
Length of string in Unicode code points


## TONFC
Normalize a string to Unicode NFC


## SREPL
Find and replace text in a string


## TODISPSTR
Decompile formatted for display


## TOEDITSTR
Decompile formatted for edit

# Operations with Symbolic Expressions

## Rewrite

Applies an arbitrary transformation on equations. The first argument is the
equation to transform. The second argument is the pattern to match. The third
argument is the replacement pattern. Patterns can contain variable names, which
are substituted with the corresponding sub-expression.

In the matching pattern, variables with a name that begins with `i`, `j`, `k`,
`l`, `m`, `n`, `p` or `q` must match a non-zero positive integer. When such a
match happens, the expression is evaluated after rewrite in order to compute
values such as `3-1`.

Additionally, variables with a name that begins with `u`, `v` or `w` must
be _unique_ within the pattern. This is useful for term-reordering rules,
such as `'x*u*x' 'x*x*u'`, which should not match `a*a*a` where it is a no-op.

`Eq` `From` `To` ▶ `Eq`

Examples:
* `'A+B+0' 'X+0' 'X' rewrite` returns `'A+B'`
* `'A+B+C' 'X+Y' 'Y-X' rewrite` returns `'C-(B-A)`
* `'(A+B)^3' 'X^N' 'X*X^(N-1)' rewrite` returns `(A+B)*(A+B)^2`.


## AutoSimplify

Enable automatic reduction of numeric subexpressions according to usual
arithmetic rules. After evaluating `AutoSimplify` `'X+0`' will evaluate as `'X'`
and '`X*1-B*0'` witll evaluate as `'X'`.

The opposite setting is [NoAutoSimplify](#noautosimplify)

## NoAutoSimplify

Disable automatic reduction of numeric subexpressions according to usual
arithmetic rules. After evaluating `NoAutoSimplify`, equations such as`'X+0`'
or `X*1-B*0` will no longer be simplified during evaluation.

The opposite setting is [AutoSimplify](#autosimplify)


## RULEMATCH
Find if an expression matches a rule pattern


## RULEAPPLY
Match and apply a rule to an expression repeatedly


## →Num (→Decimal, ToDecimal)

Convert fractions and symbolic constants to decimal form.
For example, `1/4 →Num` results in `0.25`.

## →Frac (→Q, ToFraction)

Convert decimal values to fractions. For example `1.25 →Frac` gives `5/4`.
The precision of the conversion in digits is defined by
[→FracDigits](#ToFractionDigits), and the maximum number of iterations for the
conversion is defined by [→FracDigits](#ToFractionIterations)

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
# Time, Alarms and System Commands

## SETDATE
Set current system date in MM.DDYYYY


## DATEADD
Add days to a date in MM.DDYYYY


## SETTIME
Set current time as HH.MMSS


## TOHMS
Convert decimal time to HH.MMSS


## FROMHMS
Convert time in HH.MMSS to decimal


## HMSADD
Add time in HH.MMSS format


## HMSSUB
Subtract time in HH.MMSS format


## TICKS
Return system clock in microseconds


## TEVAL
Perform EVAL and measure elapsed time


## DATE
Current system date as MM.DDYYYY


## DDAYS
Number of days between dates in MM.DDYYYY


## TIME
Current time in HH.MMSS


## TSTR


## ACK
Acknowledge oldest alarm (dismiss)


## ACKALL
Acknowledge (dismiss) all alarms


## RCLALARM
Recall specified alarm


## STOALARM
Create a new alarm


## DELALARM
Delete an existing alarm


## FINDALARM
Get first alarm due after the given time


## Version

Return db50x version information as text.

 ▶ `"Version information"`


## FreeMemory

Return the number of bytes immediately available in memory, without performing a
cleanup of temporary values (garbage collection).

See also: [GarbageCollect](#GarbageCollect), [FreeMemory](#FreeMemory)


## AvailableMemory (MEM)

Return the number of bytes available in memory.

*Remark*: The number returned is only a rough indicator of usable memory.
In particular, [recovery features](#LastThingsMenu) consume or release varying
amounts of memory with each operation.

Before it can assess the amount of memory available, `AvailableMemory` removes
objects in temporary memory that are no longer being used. Like on the HP48, you
can therfore use `MEM` `DROP` to force garbage collection. However, there is
also a dedicated command for that, [GarbageCollect](#GarbageCollect).

See also: [FreeMemory](#FreeMemory), [GarbageCollect](#GarbageCollect)


## GarbageCollect

Perform a clean-up of temporary objects and return number of bytes reclaimed.

In order to speed up normal operations, temporaries are only discarded when
necessary to make room. This clean-up process, also called *garbage collection*,
occurs automatically when memory is full. Since garbage collection can slow down
calculator operation at undesired times, you can force it to occur at a desired
time by executing [GarbageCollect](#GarbageCollect).

See also: [FreeMemory](#FreeMemory), [Purge](#Purge)


## Bytes

Return the size of the object and a hash of its value. On classic RPL systems,
teh hash is a 5-nibbles CRC32. On db50x, the hash is a based integer of the
current [wordsize](#stws) corresponding to the binary representation of the
object.

For example, the integer `7` hash will be in the form `#7xx`, where `7` is the
value of the integer, and `xx` represents the integer type, as returned by the
[Type](#type) command.

`X` ▶ `Hash` `Size`


## Type

Return the type of the object as a numerical value. The value is not guaranteed
to be portable across versions of db50x (and pretty much is guarantteed to _not_
be portable), nor to ever match the value returned by the `TYPE` command on the
HP48.

*Note* The [TypeName](#typename) command returns the type as text, and
this is less likely to change from one release to the next.

## TypeName

Return the [type](#Type) of the object as text. For example, `12 type` returns
`"integer"`.


## PEEK
Low-level read memory address


## POKE
Low level write to memory address


## NEWOB
Make a new copy of the given object


## USBFWUPDATE


## PowerOff (OFF)

Turn calculator off programmatically


## SystemSetup

Display the built-in system setup


## SaveState

Save the machine's state to disk, using the current state if one was previously
loaded. This is intended to quickly save the state for example before a system
upgrade.


## Help

Access the built-in help in a contextual way. Bound to __XShift-+__

If the first level of the stack contains a text corresponding to a valid help
topic, this topic will be shown in the help viewer. Otherwise, a help topic
corresponding to the type of data in the stack will be selected.
# Tagged objects

Tagged objects are a way to indicate what a value represents, using a *tag*
between colons and preceding the object. For example, `:X:3` is a tagged
integer, where the tag is `X` and the object is `3`.

When displayed on the stack, tags are shown without the leading colon for
readability. For example, the object above shows as `X:3` on the stack.

## →Tag (ToTag)

Apply a tag to an object. The tag is in level 1 as text or name. The object to
be tagged is in level 2. For example, `"Hello" 1 →Tag` results in `:Hello:1`.
Like on the HP calculators, it is possible to next tags.

## Tag→ (FromTag)

Expand a tagged object in level 1 into its object and tag. The object will be in
level 2, the tag will be in level 1 as a text object.

For example, `:Hello:1 Tag→` results in `"Hello"` in level 1 and `1` in level 2.

## DeleteTag (DTAG)

Remove a tag from an object. For example, `:Hello:1 DeleteTag` results in `1`.
If there is no tag, the object is returned as is.
# Transcendental functions

## SIN
Compute the sine


## COS
Compute the cosine


## TAN
Compute the tangent


## ASIN
Compute the arcsine


## ACOS
Compute the arccosine


## ATAN
Compute the arctangent


## ATAN2
Compute arctangent(y/x)


## LN
Compute natural logarithm


## EXP
Compute exponential function


## SINH
Compute the hyperbolic sine


## COSH
Compute the hyperbolic cosine


## TANH
Compute the hyperbolic tangent


## ASINH
Compute the hyperbolic arcsine


## ACOSH
Compute the hyperbolic arccosine


## ATANH
Compute the hyperbolic arctangent


## LOG
Compute logarithm in base 10


## ALOG
Compute anti-logarithm in base 10


## SQRT
Compute the square root


## EXPM
Compute exp(x)-1


## LNP1
Compute ln(x+1)


## PINUM
Numeric constant π with twice the current system precision

# User Interface

## COPYCLIP
Copy an object to the clipboard


## CUTCLIP
Move an object to the clipboard


## PASTECLIP
Insert the clipboard contents on the stack


## Wait

Wait for a key press or a time lapse.

When the argument is greater than 0, interrupt the program for the given number
of seconds, which can be fractional.

When the argument is 0 or negative, wait indefinitely until a key is
pressed. The key code for the key that was pressed will be pushed in the
stack. If the argument is negative, the current menu will be displayed on the
screen during the wait.


## KEYEVAL
Simulate a keypress from within a program


## KEY
Get instantaneous state of the keyboard


## DOFORM
Take a variable identifier with a form list


## EDINSERT
Insert given text into the editor


## EDREMOVE
Remove characters in the editor at the cursor position


## EDLEFT
Move cursor to the left in the editor


## EDRIGHT
Move cursor to the right in the editor


## EDUP
Move cursor up in the editor


## EDDOWN
Move cursor down in the editor


## EDSTART
Move cursor to the start of text in the editor


## EDEND
Move cursor to the end of text in the editor


## EDLSTART
Move cursor to the start of current line in the editor


## EDLEND
Move cursor to the end of current line in the editor


## EDTOKEN
Extract one full word at the cursor location in the editor


## EDACTOKEN
Extract one word at the left of cursor location (suitable for autocomplete)


## EDMODE
Change the cursor mode in the editor


## SETTHEME
Set system color theme


## GETTHEME
# Operations with Units

## UDEFINE
Create a user-defined unit


## UPURGE
Delete a user-defined unit


## UVAL
Numeric part of a unit object


## UBASE
Expand all unit factors to their base unit


## CONVERT
Convert value from one unit to another


## UFACT
Expose a group of units within a unit object (factor)


## TOUNIT
Apply a unit to an object


## ULIST
List all user-defined units

# USB Communications

## USBSTATUS
Get status of the USB driver


## USBRECV
Receive an object through USB link


## USBSEND
Send an object through the USB link


## USBOFF
Disable USB port


## USBON
Enable USB port


## USBAUTORCV
Receive an object and execute it


## USBARCHIVE
Create a backup on a remote machine


## USBRESTORE
Restore a backup from a remote machine

