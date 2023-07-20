# Overview

## DB48X on DM42

The DB48X project intends to rebuild and improve the user experience of the
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
* [Differences with the HP48](#differences-with-the-hp48)
* [Built-in help](#help)
* [Acknowledgements and credits](#acknowledgements-and-credits)


## State of the project

This is currently **UNSTABLE** software. Please only consider installing this if
you are a developer and interested in contributing. Please refer to the web site
of the project on GitHub for details and updates.

## Design overview

The objective is to re-create an RPL-like experience, but to optimize it for the
existing DM42 physical hardware.

Compared to the original HP48, the DM42 has a much larger screen, but no
annunciators (it is a fully bitmap screen). It has a keyboard with dedicated
soft-menu (function) keys, but only one shift key (whereas the HP48 has two),
lacks a dedicated alpha key, does not provides left or right arrow keys (only up
and down), and has no space key (_SPC_ on the HP48).


### Keyboard interaction

The keyboard differences force us to revisit the user interaction with the
calculator compared to the HP48:

* The single _Shift_ key cycles between three states, *Shift*, *Right Shift* and
  no shift.  This double-shift shortcut appears necessary because RPL RPL
  calculators like the HP48 have a rather full keyboard even with two shift
  keys.

* Since RPL uses Alpha entry a lot more than the HP42, making it quickly
  accessible seems important, so holding the *Shift* key enters Alpha mode.

* It is also possible to enter Alpha mode with _Shift_ _ ENTER _ to match
  the labelling on the DM42. _Shift_ _ ENTER _ also cycles between *uppercase*,
  *lowercase* and *non-alpha* entry. The first two are shown as _ABC_ and _abc_
  in the annunciator area.

* Alpha mode is always sticky. In other words, there is no equivalent of the
  HP48's "single-Alpha" mode. Alpha mode is also exited when pressing _ ENTER _
  or _EXIT_.

* Since the DM42's alphabetic keys overlap with the numeric keys, as well as
  with and _ × _ and _ ÷ _, using _Shift_ in Alpha mode brings back
  numbers. This means it cannot be used for lowercase, hence the need for the
  _Shift_ _ ENTER _ sequence.

* The less-frequently used *Right Shift* functions can be accessed with a
  long-press on _Shift_.

* The _ ▲ _ and _ ▼ _ keys move the cursor _left_ and _right_ while editing
  instead of _up_ and _down_. These cursor movements are much more useful for a
  text-based program editing as found in RPL.
  Using _Shift_ _ ▲ _ and _Shift_ _ ▼ _ moves the cursor up and down.
  When not editing, _ ▲ _ and _ ▼ _  behave like on the HP48, i.e. _ ▲ _
  enters the *interactive stack* and _ ▼ _ edits the object on the first level
  of the stack.

* Long-pressing arrow keys, the _ ← _ (also known as *Backspace*) or text entry
  keys in Alpha mode activates auto-repeat.

* Long-pressing keys that would directly trigger a function (e.g. _SIN_),
  including function keys associated with a soft-menu, will show up the built-in
  help for the corresponding function.


### Key mapping

Some keys that have little use or no direct equivalent for RPL are remapped
as follows (**Note**: most of this is **not implemented yet**):

* _Σ+_ is used to call [MathMenu](#MathMenu), which includes submenus for sums
  and statistics among others

* _Σ-_ (i.e. _Shift_ _Σ+_) will select the [TopMenu](#TopMenu), i.e. the
  top-level menu giving access to all other menus and features in DB48X.

* _XEQ_ opens an algebraic expression, i.e. it shows `''` on the
  command-line and switches to equation entry. It can be remembered as
  *Execute Equation* and can be used to evaluate expressions in
  [algebraic mode](#algebraic-mode)
  instead of RPN. It also activates a menu facilitating algebraic entry,
  e.g. with shortcuts for parentheses, and symbolic manipulation of
  sub-expressions.

* _Gto_ opens the [BranchesMenu](#BranchesMenu), with RPL branches and loops,
  e.g. `IF` `THEN` or `DO` `WHILE`, as well as conditional tests.

* _Complex_ will open the [ComplexMenu](#ComplexMenu), not just build a complex
  like on the DM42. The [ComplexMenu](#ComplexMenu) includes features to enter
  complex numbers, as well as complex-specific functions like
  [Conjugate](#Conjugate).

* _RCL_ will open the [VariablesMenu](#VariablesMenu) menu (user variables),
  except if that menu is already open, in which case it will perform a
  [Recall (RCL)](#Recall) function.

* _ % _ will open the [FractionsMenu](#FractionsMenu), which contains operations
  on fractions.

* _ R↓ _ will open the [StackMenu](#StackMenu), containing operations on the
  stack.

* _ π _ will open the [ConstantsMenu](#ConstantsMenu) (π being one of
  them). Just like in the [VariablesMenu](#VariablesMenu), Each constant can
  either be evaluated by pressing the corresponding function key, or simply
  named using Shift with the function key. For example, pressing _F1_ shows an
  approximate value of π beginning with `3.1415926`, whereas Shift _F1_ shows
  `'π'`. The value of constants in this menu come from a file `CONSTANTS.CSV` on
  disk, which makes it possible to add user-defined constants with arbitrary
  precision.

* _X⇆Y_ executes the matching [Swap](#swap) function

* _LAST x_ will open a [LastThingsMenu](#LastThingsMenu), with options to Undo
  the last operation, functions like `LastX`, `LastStack`, `LastArgs`, `LastKey`
  and [LastMenu](#LastMenu).

* _+/-_ executes the equivalent RPL `Negate` function
* _Modes_ calls the [ModesMenu](#ModesMenu), with submenus for various settings,
  including computation precision, display modes, etc.

* _Disp_ calls the [DisplayMenu](#DisplayMenu), with submenus for graphic
  operations, plotting, shapes, and forms.

* _Clear_ calls a [ClearThingsMenu](#ClearThingsMenu) with options to clear
  various items, including [ClearStack` and `ClearMenu](#ClearStack` and
  `ClearMenu).

* _Bst_ and _Sst_ are remapped to moving the cursor Up and Down in text editing
  mode. In direct mode, _Bst_ selects the _Best_ editor for the object, and
  _Sst_ selects single-step evaluation.

* _Solver_ shows the [SolverMenu](#SolverMenu), with submenus for numerical and
  symbolic solvers.

* _∫f(x)_ shows the [SymbolicMenu](#SymbolicMenu), with symbolic and numerical
  integration and derivation features.

* _Matrix_ shows the [MatrixMenu](#MatrixMenu) with operations on vectors,
  matrices and tensors.

* _Stat_ shows the [StatisticsMenu](#StatisticsMenu)

* _Base_ shows the [BasedNumbersMenu](#BasedNumbersMenu), with operations on
  based numbers, including facilities for entering hexadecimal numbers,
  temporarily remapping the second row of the DM42 keyboard to enter `A`, `B`,
  `C`, `D`, `E` and `F`.

* _Convert_ shows a [UnitsMenu](#UnitsMenu) with units and and conversion
  functions.

* _Flags_ shows the [FlagsMenu](#FlagsMenu) with operations on user and system
  flags.

* _Prob_ shows the [PrombabilitiesMenu](#PrombabilitiesMenu), with functions
  such as [Factorial](#factorial), [Combinations](#combinations) or\
  [Random](#random).

* _Assign_ makes it possible to assign any function to any key. These
  special functions are then selected by using _Custom_ before the
  function. This is the equivalent of the HP48 "User" mode. Selecting
  _Custom_ twice makes the custom-keys mode sticky. It is also possible to
  store and evaluate complete keymaps, to match special environment
  cases. The current keymap is stored in special variable `Keymap` for the
  current directory.

* _Pgm.Fcn_ shows the [ProgrammingMenu](#ProgrammingMenu), with all
  general-purpose programming functions, categorized as sub-menus

* _Print_ shows the [DevicesMenu](#DevicesMenu), which includes submenus like
  [PrintMenu](#PrintMenu), [FlashStorageMenu](#FlashStorageMenu),
  [TimeMenu](#TimeMenu), [DateMenu](#DateMenu), and [AlarmMenu](#AlarmMenu).

* _Exit_ corresponds to what the HP48 manual calls _Attn_, and typically
  cancels the current activity. It can also be used to interrupt a running
  program.

* _Off_ shuts down the calculator

* _Setup_ calls the DM42's built-in [SystemMenu](#SystemMenu), for example to
  load the original DM42 program, activate USB disk, and a menu-based access to
  calculator preferences.

* _Show_ selects the [ShowMenu](#ShowMenu), with various ways to display objects
  on the stack, such as [ShowBest](#ShowBest), [ShowSymbolic](#ShowSymbolic),
  [ShowGraphical](#ShowGraphical), [ShowCompact](#ShowCompact).

* The _R/S_ keys inserts a space in Alpha mode, and maps to
 [Evaluate](#evaluate) in direct mode.

* _Prgm_ inserts the delimiters for an RPL program, corresponding to `«` and
  `»`.

* _Catalog_ shows a complete context-sensitive catalog of all available
  functions, and enables auto-competion using the soft-menu keys.


### Soft menus

The DM42 has 6 dedicated soft-menu keys at the top of the keyboard. Most of the
advanced features of DB48X can be accessed through these soft menus.

Menus are organized internally as a hierarchy, where menus can refer to other
menus. A special menu, [TopMenu](#TopMenu), accessile via the _Σ-_ key label
(_Shift_ _Σ+_), contains all other menus.

Menus can contain up to 12 entries at once, 6 being directly accessible, and 6
more being shown when using the Shift key. Since function keys are designed for
rapiid access to features, a right-shift access rarely makes sense, since that
would require a long press of the shift key. There are exceptions, like the
[VariablesMenu](#VariablesMenu), where dangerous operations (overwriting a
variable) are associated to right-shift.

A long press on a function key invokes the on-line help for the associated
function.

When a menu contains more than 12 entries, then the _F6_ function key turns into
a `▶︎`. When shifted, then _F6_ turns into `◀`︎. These keys can be used to
navigate across the available menu entries.



### Differences with the HP48

There are a number of intentional differences in design compared to the HP48:

* DB48X features an extensive built-in help system, which you are presently
  using. Information for that help system is stored using a regular *markdown*
  file named `/HELP/DB48X.md`, stored in the calculator's flash storage.

* DB48X will feature auto-completion for commands while typing, through
  the  _Catalog_ key ([CatalogMenu](#CatalogMenu)).

* Many RPL words exist in short and long form, and a user preference selects how
  a program shows. For example, the [Negate](#negate) command, which the HP48
  calls `NEG`, can display, based on user preferences, as `NEG`, `neg`, `Neg` or
  `Negate`. In the help, it will be shown as **Negate (NEG)**.

* The DB48X dialect of RPL is not case sensitive, but it is case-respecting.
  For example, if your preference is to display built-in functions in long form,
  typing `inv` or `INV` will show up as `Invert` in the resulting program. If
  you have a named variable called `FooBar`, typing `foobar` will still show
  `FooBar` in the program.

* Internally, the calculator deals with various representations for
  numbers. Notably, it keeps integer values and fractions in exact form for
  as long as possible to optimize both performance and memory usage.

* The calculator features at least 3 floating-point precisions using 32-bit,
  64-bit and 128-bit respectively, provided by the DMCP's existing Intel Binary
  Decimal Floating-Point library. The 128-bit format gives the calculator 34
  significant digits of precision, like the DM42. DB48X may support other
  formats in the future, like the extended floating-point found in newRPL.

* Based numbers like `#123h` keep their base, which makes it possible to show on
  stack binary and decimal numbers side by side. Mixed operations convert to the
  base in stack level X, so that `#10d #A0h +` evaluates as `#AAh`.

* The storage of data in memory uses a denser format than on the HP48.
  Therefore, objects will almost always use less space on DB48X. Notably, the
  most frequently used functions and data types consume only one byte on DB48X,
  as opposed to 5 nibbles (2.5 bytes) on the HP48.

* Numerical equality can be tested with `=`,  whereas object equality is tested
  using `==`. For example, `0=0.0` is true, but `0==0.0` is false, because `0`
  is an integer whereas `0.0` is a floating-point.

* DB48X borrows to the DM42 the idea of _special variables_, which are variables
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

* The DB48X also provides full-screen setup menus, taking advantage of the DM42
  existing system menus. It is likely that the same menu objects used for
  softkey menus will be able to control system menus, with a different function
  to start the interaction.

* The whole banking and flash access storage mechanism of the HP48 will be
  replaced with a system that works well with FAT USB storage. It should be
  possible to directly use a part of the flash storage to store RPL programs,
  either in source or compiled form.


## Help

The DB48X project includes an extensive built-in help, which you are presently
reading. This help is stored as a `HELP/DB48X.md` file on the calculator. You
can also read it from a web browser directly on the GitHub page of the project.

The DB48X help viewer works roughly simiilarly to the DM42's, but with history
tracking and the ability to directly access help about a given function by
holding a key for more than half a second.

To navigate the help on the calculator, use the following keys:

* The soft menu keys at the top of the keyboard, references as _F1_ through
  _F6_, correspond to the functions shown in the six labels at the bottom of the
  screen.

* While the help is shown, the keys _ ▼ _ and _ ▲ _ on the keyboard scroll
  through the text.

* The _F1_ key returns to the [Home](#overview) (overview).

* The _F2_ and _F3_ keys (labels `Page▲` and `Page▼`) scroll the text one full
  page at a time.

* The _F4_ and _F5_ keys (labels `Link▲` and `Link▼`) select the previous and
  next link respectively. The keys _ ÷ _ and _ 9 _ also select the previous
  link, while the keys _ × _ and _ 3 _ can also be used to select the next link.

* The _F6_ key correspond to the `←Menu` label, and returns one step back in
  the help history. The _ ← _ key achieves the same effect.

* To follow a highlighted link, click on the _ ENTER _ key.


## Acknowledgements and credits

DB48X is Free Software, see the LICENSE file for details.
You can obtain the source code for this software at the following URL:
https://github.com/c3d/DB48X-on-DM42
(C) 2022-2023 Christophe de Dinechin and the DB48X team

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

Christophe de Dinechin, the primary author of DB48X, was lucky enough to meet
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

DB48X inherits many ideas from newRPL, including, but not limited to:

* Implementing RPL natively for ARM CPUs
* Adding indicators in the cursor to indicate current status
* Integrating a catalog of functions to the command line

A first iteration of DB48X started as a
[branch of newRPL](https://github.com/c3d/db48x/), although the
current implementation had to restart from scratch due to heavy space
constraints on the DM42.


### WP43 and C47 projects

The DB48X took several ideas and some inspiration from the
[WP43](https://gitlab.com/rpncalculators/wp43) and
[C47](https://47calc.com) projects.

Walter Bonin initiated the WP43 firwmare for the DM42 as a "superset of the
legendary HP42S RPN Scientific".

C47 (initially called C43) is a variant of that firmware initiated by Jaco
Mostert, which focuses on compatibility with the existing DM42, notably with
respect to keyboard layout.

DB48X borrowed at least the following from these projects:

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
which is currently the primary target for the DB48X firmware.

Special thanks and kudos to Michael Steinmann and his team for keeping
the shining spirit of HP RPN calculators alive.

The DM42 version of the DB48X software relies on
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

Floating-point computations in DB48X take advantage of
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
