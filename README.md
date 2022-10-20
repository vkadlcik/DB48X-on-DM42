# DB48X on DM42

The DB48X project intends to rebuild and improve the user experience of the
legendary HP48 family of calculators, notably their "Reverse Polish Lisp" (RPL)
language with its rich set of data types and built-in functions.

The project in this repository is presently targeting the [SwissMicro DM42
calculator](https://www.swissmicros.com/product/dm42), and leveraging its
built-in software platform, known as
[DMCP](https://technical.swissmicros.com/dmcp/doc/DMCP-ifc-html/).

In the long-term, the vision is to be able to port DB48X on a number of
[different physical calculator platforms](https://www.youtube.com/watch?v=34pPycq8ia8),
like the ARM-based
[HP50 and related machines (HP49, HP48Gii, etc)](https://en.wikipedia.org/wiki/HP_49/50_series),
and the [HP Prime](https://en.wikipedia.org/wiki/HP_Prime)
(at least the G1, since the G2 seems a bit more locked down), maybe others.
The basis for that work can be found in the [DB48X](../db48x) project.


## Why name the project DB48X?

DB stands for "Dave and Bill", who are more commonly known as Hewlett and
Packard. The order is reversed compared to HP, since they reportedly chose the
order at random, and it's about time Dave Packard was given preeminence.

One of Dave and Bill's great legacy is a legendary series of calculators.
The [HP48](https://en.wikipedia.org/wiki/HP_48_series) remains one of my
favorites, notably for its rich built-in programming language, known as [Reverse
Polish Lisp (RPL)](https://en.wikipedia.org/wiki/RPL_(programming_language)).
This project aims at recreating a decent successor to the HP48, at least in
spirit.


## State of the project

This is currently **UNSTABLE** software. Please only consider installing this if
you are a developer and interested in contributing.

The detailed current status is described in the [STATUS file](STATUS.md).

[![Self-test in the simulator](http://img.youtube.com/vi/vT-I3UlROtA/0.jpg)](https://www.youtube.com/watch?v=vT-I3UlROtA "Self-test demo")


## How to build this project

There is a separate document explaining [how to build this project](BUILD.md).
The simulator includes a test suite, which you should run before submitting
patches. To run these tests, pass the `-T` option to the simulator, or hit the
**F12** key in the simulator.


## Design overview

The objective is to re-create an RPL-like experience, but to optimize it for the
existing DM42 physical hardware.

Compared to the original HP48, the DM42 has a much larger screen but all bitmap
(no annunciators). It has a keyboard with dedicated soft-menu (function) keys,
but has only one shift key, lacks a dedicated alpha key, has no left or right
arrow keys (only up and down), and no `SPC` (space) key.


### Keyboard interaction

The keyboard differences force us to revisit the user interaction with the
calculator compared to the HP48:

* The single *Shift* key cycles between **Shift**, **Alpha** and no shift.
  This double-shift shortcut appears necessary because RPL uses Alpha entry a
  lot more than the HP42, so making it quickly accessible seems important.

* It is also possible to enter Alpha mode with **Shift `ENTER`** to match
  the labelling on the DM42. Furthermore, **Shift `ENTER`** also cycles between
  uppercase and lowercase alphabetical entry.

* Alpha mode is always sticky. In other words, there is no equivalent of the
  HP48's "single-Alpha" mode. Alpha mode is also exited when pressing `ENTER` or
  `EXIT`.

* Since the DM42's Alpha keys overlap with the numeric keys, as well as with and
  `×` and `÷`, using **Shift** in Alpha mode brings back numbers. This means it
  cannot be used for lowercase, hence the **Shift `ENTER`** shortcut.

* The less-frequently used _Right Shift_ functions can be accessed with a
  long-press on Shift.

* The `🔼` and `🔽` keys are generally interpreted as `◀️` and `▶️` instead.
  To get `🔼` and `🔽` functionality, use **Shift**. When not editing, `🔼` and
  `🔽` behave like they do on the HP48, where `🔼` enters the _Interactive
  stack_ mode, and `🔽` edits an object.

* Long-pressing arrow keys, the **⭠** (**Backspace**) or text entry keys in
  Alpha mode activates auto-repeat.

* Long-pressing keys that would directly trigger a function (e.g. `SIN`),
  including function keys associated with a soft-menu, will show up the built-in
  help for the corresponding function.

* Some keys that have little use or no direct equivalent for RPL are remapped
  as follows (note: most of this is _not implemented yet_):

  * The **Σ+** key is used to call `MathMenu`, which includes submenus for sums
    and statistics among others
  * **Σ-** will select the `TopMenu`, i.e. the top-level menu containing all
    other menus.
  * **XEQ** opens an algebraic expression, i.e. it shows `''` on the
    command-line and switches to equation entry. It can be remembered as
    _Execute Equation_ and can be used to evaluate expressions in algebraic mode
    instead of RPN. It also activates a menu facilitating algebraic entry,
    e.g. with shortcuts for parentheses, and symbolic manipulation of
    sub-expressions.
  * **Gto** opens the `BranchesMenu`, with RPL branches and loops, e.g. `IF
    THEN` or `DO WHILE`, as well as conditional tests.
  * **Complex** will open the `ComplexMenu`, not just build a complex like on
    the DM42. The `ComplexMenu` includes features to enter complex numbers, as
    well as complex-specific functions like `Conjugate`.
  * **RCL** will open the `VariablesMenu` menu (user variables), except if that
    menu is already open, in which case it will perform a `Recall` (`RCL`)
    function.
  * **%** will open the `FractionsMenu`
  * **R↓** will open the `StackMenu`
  * **π** will open the `ConstantsMenu` (π being one of them). Just like in the
    `VariablesMenu`, Each constant can either be evaluated by pressing the
    corresponding function key, or simply named using Shift with the function
    key. For example, pressing **F1** shows an approximate value of π beginning
    with `3.1415926`, whereas Shift **F1** shows `'π'`. The value of constants
    in this menu come from a file `CONSTANTS.CSV` on disk, which makes it
    possible to add user-defined constants with arbitrary precision.
  * **X⇆Y** executes the matching `Swap` function
  * **LAST x** will open a `LastThingsMenu`, with options to Undo the last
    operation, functions like `LastX`, `LastStack`, `LastArgs`, `LastKey` and
    `LastMenu`.
  * **+/-** executes the equivalent RPL `Negate` function
  * **Modes** calls the `ModesMenu`, with submenus for various settings,
    including computation precision, display modes, etc.
  * **Disp** calls the `DisplayMenu`, with submenus for graphic operations,
    plotting, shapes, and forms.
  * **Clear** calls a `ClearThingsMenu` with options to clear various items,
    including `ClearStack` and `ClearMenu`.
  * **Bst** and **Sst** are remapped to moving the cursor Up and Down in text
    editing mode. In direct mode, **Bst** selects the _Best_ editor for the
    object, and **Sst** selects single-step evaluation.
  * **Solver** shows the `SolverMenu`, with submenus for numerical and symbolic
    solvers.
  * **∫f(x)** shows the `SymbolicMenu`, with symbolic and numerical integration
    and derivation features.
  * **Matrix** shows the `MatrixMenu` with operations on vectors, matrices and
    tensors.
  * **Stat** shows the `StatisticsMenu`
  * **Base** shows the `BasedNumbersMenu`, with operations on based numbers,
    including facilities for entering hexadecimal numbers, temporarily remapping
    the second row of the DM42 keyboard to enter `A`, `B`, `C`, `D`, `E` and
    `F`.
  * **Convert** shows a `UnitsMenu` with units and and conversion functions.
  * **Flags** shows the `FlagsMenu` with operations on user and system flags.
  * **Prob** shows the `PrombabilitiesMenu`, with functions such as `Factorial`,
    `Combinations` or `Random`.
  * **Assign** makes it possible to assign any function to any key. These
    special functions are then selected by using **Custom** before the
    function. This is the equivalent of the HP48 "User" mode. Selecting
    **Custom** twice makes the custom-keys mode sticky. It is also possible to
    store and evaluate complete keymaps, to match special environment
    cases. The current keymap is stored in special variable `Keymap` for the
    current directory.
  * **Pgm.Fcn** shows the `ProgrammingMenu`, with all general-purpose
    programming functions, categorized as sub-menus
  * **Print** shows the `DevicesMenu`, which includes submenus like `PrintMenu`,
    `FlashStorageMenu`. `TimeMenu`, `DateMenu`, and `AlarmMenu`.
  * **Exit** corresponds to what the HP48 manual calls **Attn**, and typically
    cancels the current activity. It can also be used to interrupt a running
    program.
  * **Off** shuts down the calculator
  * **Setup** calls the DM42's built-in `SystemMenu`, for example to load the
    original DM42 program, activate USB disk, and a menu-based access to
    calculator preferences.
  * **Show** selects the `ShowMenu`, with various ways to display objects on the
    stack, such as `ShowBest`, `ShowSymbolic`, `ShowGraphical`, `ShowCompact`.
  * The **R/S** keys is used for `SPC` (inserting a space) in alpha mode, and
    maps to `Evaluate` in direct mode.
  * **Prgm** inserts the delimiters for an RPL program, corresponding to `«` and
    `»`. Since these characters are missing from the standard DM42 character
    set, we will need a temporary workaround.
  * **Catalog** shows a complete context-sensitive catalog of all available
    functions, for auto-completion.

### Soft menus

The DM42 has 6 dedicated soft-menu keys at the top of the keyboard. Most of the
advanced features of DB48X can be accessed through these soft menus.

Menus are organized internally as a hierarchy, where menus can refer to other
menus. A special menu, `TopMenu`, accessile via the **Σ-** key label
(Shift-**Σ+**), contains all other menus.

Menus can contain up to 12 entries at once, 6 being directly accessible, and 6
more being shown when using the Shift key. Since function keys are designed for
rapiid access to features, a right-shift access does not really make sense,
since that would require a long press of the shift key. A long press on a
function key invokes the on-line help for the associated function.

When a menu contains more than 12 entries (which rarely happens, built-in menus
being designed to avoid that situation, but can happen for example for the
`VariablesMenu` listing user variables), then the F6 function key turns into a
`▶︎`, and into `◀`︎ when shifted. These keys can be used to navigate across the
available menu entries.

For example:

* A softkey menu containing 6 entries or less, such as the `DirectoryPathMenu`,
  which contains `CurrentPath`, `CreateDir`, `PurgeDir`, `VariableList`,
  `TypedVariables` and `OrderVariables`, would show (using short names for menu
  labels) as `Path` `CrDir` `PgDir`, `Vars`, `TVars`, `Order`. Since there are
  6 entries or less, Shift has no effect.

* A softkey menu containing between 6 and 12 entries, such as the `MathMenu`,
  would show the first six entries, `VectorMenu`, `MatrixMenu`, `ListMenu`,
  `HyperbolicMenu`, `RealMenu`, `BasedNumbersMenu`, showing in short form as
  `Vectr`, `Matrx`, `List`, `Hyp`, `Real`, `Base`. Hitting the Shift key would
  have the same effect as the **NXT** key on the HP48, showing the four
  remaining entries, `ProbabilitiesMenu`, `FastFourierTransformMenu`,
  `ComplexMenu`, `ConstantsMenu`, showing in short form as `Proba`, `Fourier`,
  `Complex`, `Constants`.

* An `ExampleMenu` containing the spelling of thirty numbers in English would
  show `One`, `Two`, `Three`, `Four` and `Five` for keys **F1** through **F5**,
  and `▶` for key **F6**. With Shift, it would show `Six`, `Seven`, `Eigh`,
  `Nine`, `Ten` and `◀`︎︎. Hitting `▶` with **F6** would switch to the next ten
  elements, `Eleven`, `Twelve`, `Thirteen`, `Fourteen`, `Fifteen` and `▶`,
  with Shift showing `Sixteen`, `Seventeen`, `Eighteen`m `Nineteen`, `Tzenty`
  and `◀`. The Shift **F6** key would return to the previous ten entries
  beginning with `One`, the **F6** key would go to the next ten entries
  beginning with `TwentyOne`.


The `Variables` menu is special in the sense that selecting an entry evaluates
that menu entry, while Shift recalls its name without evaluating it. This is one
case where extended shift applies to a function key, and is used to directly
store a value in the associated variable. As a result, at most 5 variables are
shown at a time, the Shift state showing the variable name with tick marks, like
`'X'`, and the long-shift state  showing the variable name with a `Store`
indicator, as in `▶︎X`.

For example, if the first variable is called `X` and contains the program
`« 1 + »`, then

1. typing `3` and hitting **F1** with show `4`, having evaluted the program in
   `X` which adds one to the value on the stack. The associated label is `X`.

2. After that, typing Shift-**F1** will show `'X'` in the menu, and put `'X'` on
   the stack (if the text editor was active, it would type `X` in the editor
   instead).

3. If you then type  **RCL**, since the `VariablesMenu` is already active, that
   key will `Recall` the value of `X`, bringing the program `« 1 + »` on the
   stack.

4. Typing `3`, Shift-**F1**, then **STO** will store the value `3` in `X`,
   leaving only the program `« 1 + »` on the stack.

5. Hitting **F1** at that point will evaluate `3`, leaving `3` on the stack.

6. The **X⇆Y** key will then put `3` at level 2 on the stack, and the
   program ``« 1 + »` at level 1.

7. A long-shift followed by **F1** will store the program in `X`. The menu label
   changes to `▶︎X`

8. Typing **F1** once more will evaluate `X` now containing the incrementing
   program again, leaving `4` on the stack.



### Some design differences with the original HP48

There are a number of intentional differences in design compared to the HP48:

* DB48X will feature an extensive built-in help system. While this is not
  implemented yet in this repository, the code was already developed in a
  [sibling newRPL-based project](https://www.youtube.com/watch?v=34pPycq8ia8).
  The built-in help will combine the use of HTML files, rendered using DMCP's
  built-in help viewer, which will be used for high-level overview (and anything
  where links and advanced styling are useful), and a markdown-based
  [per-command help system](doc/calc-help).

* Inspired by [newRPL](../db48x), DB48X will feature auto-completion. Available
  auto-completions will show up in the **Catalog** menu. For example, typing
  `LAST` with the **Catalog** menu active will show all functions starting with
  `Last`, like `LastX` or `LastMenu`. The **Catalog** menu updates with each
  keystroke.

* Many RPL words exist in short and long form, and a user preference selects how
  a program shows. For example, what the HP48 calls `NEG` can display, based on
  user preferences, as `NEG`, `neg`, `Neg` or `Negate`.

* The DB48X dialect of RPL will not be case sensitive, but it will be
  case-respecting. For example, if your preference is to display built-in
  functions in long form, typing `inv` or `INV` will show up as `Invert` in the
  resulting program. If you have a named variable called `FooBar`, typing
  `foobar` will still show `FooBar` in the program.

* Internally, the calculator will deal with various representations for
  numbers. Notably, it will keep integer values and fractions in exact form for
  as long as possible.

  * When adding `1` and `2`, the internal representation of
    the result is an integer, which uses less memory and allows faster
    computations.

  * And integer will be promoted to a floating-point value only if you use a
    function that requires it, like `exp`.

  * Performing a division like `8` divided by `4` will result in an integer.

  * Performing a division between integers with a remainder will keep the result
    in reduced fraction form, for example `3/4` or `7/31`.

* The calculator will feature at least 3 floating-point precisions using 32-bit,
  64-bit and 128-bit respectively, as featured by the DM42 existing Intel Binary
  Decimal Floating-Point library.

  * Long-term, variable floating-point precision as in newRPL will probably be
    implemented.

  * The selection of which precision to use for floating-point numbers will be
    decided by the `SetPrecision` function when converting from integer or
    fraction.

  * The precision of numbers stored in a program will be based on user
    input. For example, storing `1.3` in a program will always use the 32-bit
    floating-point format, since it's sufficient for that number.

* Based numbers like `#123h` keep their base, which makes it possible to show on
  stack binary and decimal numbers side by side. Mixed operations convert to the
  base in stack level X, so that `#10d #A0h +` evaluates as `#AAh`.

* The storage of data in memory uses a denser format than on the HP48.
  Therefore, objects will almost always use less space on DB48X.

  * The most frequently used RPL functions and data types consume only one byte,
    as opposed to 5 nibbles, or 2.5 bytes, on HP's implementation.

  * No built-in command requires more than two bytes, as opposed to 2.5 bytes on
    HP's implementation.

  * Lengths are also coded more efficiently. For example, a string like
  `"Hello"` requires only 7 bytes on DB48X, as opposed to 10 bytes on an HP48.

* The garbage collector is very simple, and moves objects in use only once per
  collection cycle. Memory allocation is also streamlined.

* Support for algebraic-style programs, using a general parser derived from the
  [XL programming language](https://github.com/c3d/xl).

* Several minor changes to RPL, e.g. using `=` for equality test in addition to
  `==`, where `==` means "same object representation", and `=` means
  "mathematically equal" (thus, `0=-0` is true, but `0==-0` is false). In
  general, such changes will be documented in the help associated with each
  command.

* Support for extended-precision integer and floating-point numbers, with at
  least 30+ digits decimal 128 (already working), ideally newRPL-style
  variable-precision floating-point computations.

* DB48X borrows to the DM42 the idea of _special variables_, which are variables
  with a special meaning. For example, the `Precision` special variable is the
  current operating precision for floating point, in number of digits. While
  there is a `SetPrecision` command, it is also possible to use `'Precision'
  STO`. This does not imply that there is an internal `Precision` variable
  somewhere. This is used for a number of settings.

* All built-in soft-key menus are named, with names ending in `Menu`. For
  example, the `VariablesMenu` is the menu listing global variables in the
  current directory. There is no menu number, but the `Menu` special variable
  holds the name of the current menu, and `LastMenu` the name of the previous
  one.

* The DB48X will also have provide full-screen setup menus, taking advantage of
  the DM42 existing system menus. It is likely that the same menu objects used
  for softkey menus will be able to control system menus, with a different
  function to start the interaction.

* The whole banking and flash access storage mechanism of the HP48 will be
  replaced with a system that works well with FAT USB storage. It should be
  possible to directly use a part of the flash storage to store RPL programs
  directly, either in source or compiled form.


## Other documentation

There is DMCP interface doc in progress see [DMCP IFC doc](http://technical.swissmicros.com/dmcp/doc/DMCP-ifc-html/)
(or you can download html zip from [doc directory](http://technical.swissmicros.com/dmcp/doc/)).

The [source code of the `DM42PGM` program](https://github.com/swissmicros/DM42PGM)
is also quite informative about the capabilities of the DMCP.
