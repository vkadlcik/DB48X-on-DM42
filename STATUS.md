### System and user interface

- [x] Functional graphical simulator for the DMCP platform
- [x] Test suite on simulator
- [x] Main calculator loop, on/off, accessing DMCP Setup menu
- [x] RPL-style command-line text editor
- [x] Key processing
- [x] DM42-adapted shift logic
- [x] Virtual annunciators
- [x] Alpha mode and lowercases
- [x] Garbage collector
- [x] Low-memory stress testing
- [x] Built-in commands lookup and parsing
- [ ] Soft menus navigation
- [ ] Global variables
- [ ] Local variables
- [ ] Home directory and subdirectories
- [ ] Variables menu accessible with **RCL** key
- [ ] RPL program evaluattion
- [ ] RPL program single-stepping / debugging
- [ ] Stack undo / redo
- [ ] Accelerated [blitter code with 8x8 patterns](https://github.com/c3d/db48x/blob/dm42/firmware/include/ggl.h)
- [ ] Unicode-capable [text drawing replacement](https://github.com/c3d/db48x/blob/dm42/firmware/sys/graphics.c)
- [ ] Unicode / UTF-8 support
- [ ] [Unicode fonts](https://github.com/c3d/db48x/blob/dm42/firmware/include/unifont.h)
      (e.g. derived from [WP43S](https://gitlab.com/wpcalculators/wp43) project)
- [ ] Larger font for editor (old geezer mode)
- [ ] Objects in USB-accessible flash storage
- [ ] Calculator state load/save
- [ ] Global help from [HTML help file](help/db48x.html)
- [ ] [Per-function help](https://github.com/c3d/db48x/blob/dm42/firmware/hal_keyboard.c#L116)
      from [markdown files](doc/calc-help)
- [ ] Character menu / table
- [ ] Brezenham lines, circles and ellipses
- [ ] Polygon fill
- [ ] RPL-controlled soft-menu system
- [ ] Domain-specific features / customizaton from files on USB disk


### Object system

- [x] Object system with compact storage and dynamic dispatch
- [x] Stack management
- [x] Convenient allocation, initialization and recycling of temporaries
- [x] Object evaluation
- [x] Object parsing
- [x] Object text rendering
- [ ] Object graphical rendering
- [ ] Object graphical editor
- [x] Errors and error display
- [ ] Error trapping / handling, e.g `DOERR`.

### RPL data types

- [x] Built-in commands (e.g. `Neg`)
- [x] Integer data type, e.g. `123`
- [x] Hexadecimal (`#123h`), decimal (`#123d`), octal (`#123o`) and binary
      (`#1001b`) based numbers
- [ ] Based integers (arbitrary base between 2 and 36, e.g. `13#10231AC`)
- [ ] Arbitrary-size integers e.g. `123456789012345678901234567890`
- [ ] Fractions, e.g. `1/7`
- [ [ Fraction formats, e.g. `22/7` vs `3 1/7` vs. `3,1428571429`
- [x] Decimal floating-point with fixed size (32, 64 and 128 bits)
- [ ] Decimal floating-point with [arbitrary size](https://github.com/c3d/db48x/blob/dm42/newrpl/decimal.h)
- [ ] Binary floating-point with arbitrary size
- [ ] Hardware-accelerated 32-bit binary floating-point
- [x] Text / strings, e.g. `"ABC"`
- [x] Name / symbols, e.g. `ABC`
- [ ] Complex numbers in rectangular form, e.g. `2 + 3i`
- [ ] Complex numbers in polar form, e.g. `2 ∡ 30°`
- [ ] Quaternions
- [ ] Intervals / ranges, e.g. `1..5`
- [ ] Angles, e.g. `∡30°`
- [ ] Lists, e.g. `{ A 1 "Hello" }`
- [ ] Arrays, vectors, matrices and tensors, e.g. `[ [ 1 2 3] [ 4 5 6] ]`
- [ ] Program objects, e.g `« IF 1 = THEN "Equal to one" END »`
- [ ] Algebraic programs, e.g. `'if x = 1 then "Equal to one"` based on
      [XL-style parser](https://github.com/c3d/XL)
- [ ] Bitmaps, e.g. `GROB 8 8 00000000000000FF`
- [ ] Fonts, e.g. `FONT 000000`
- [ ] Vector graphics
- [ ] Tagged objects
- [ ] Algebraic expressions
- [ ] Unit objects
- [ ] Directories
- [ ] Software libraries
- [ ] Keymaps
- [ ] Softkey menus
- [ ] Full-screen menus
- [ ] Forms
- [ ] Object handler (can replace eval, parse, render, graphical editor)
- [ ] HP41/HP42-style key-entry program
- [ ] ARM PIC code


### Basic Arithmetic

- [x] Integer arithmetic
- [x] Floating-point arithmetic
- [ ] Detection of precise vs. imprecise results, like newRPL
- [x] Automatic integer to floating-point promotion
- [x] Automatic selection of entered floating-point size based on user input
- [x] Automatic selection of computed floating-point size based on `Precision`
- [ ] Fraction arithmetic
- [ ] Automatic fraction promotion
- [ ] Correct rounding (a la [crlibm](https://github.com/taschini/crlibm))
- [ ] Range arithmetic


### Computations on real numbers

- [x] floating-point arithmetic
- [ ] Circular functions, e.g. `sin`, `cos`, `atan`, ...
- [ ] Exponentials and logarithms, e.g. `exp`, `ln`, ...
- [ ] Hyperbolic functions, e.g. `sinh`, `atanh`, ...
- [ ] Probabilities functions, like `Factorial`, `Combinations`, etc.
- [ ] Statistics functions, like `Σ+`, `Mean` or `Median`
- [ ] Special functions, like `Bessel`, `FastFourierTransforma`, etc
- [ ] Constant tables with arbitrary precision from files on USB disk
- [ ] Equation tables with explanatory graphics from files on USB disk


### Computations on complex numbers

- [ ] Complex arithmetic
- [ ] Circular functions, e.g. `sin`, `cos`, `atan`, ...
- [ ] Exponentials and logarithms, e.g. `exp`, `ln`, ...
- [ ] Hyperbolic functions, e.g. `sinh`, `atanh`, ...
- [ ] Probabilities functions, like `Factorial`, `Combinations`, etc.
- [ ] Statistics functions, like `Σ+`, `Mean` or `Median`
- [ ] Special functions, like `Bessel`, `FastFourierTransforma`, etc
- [ ] Equation tables with explanatory graphics from files on USB disk


### Computations on quaternions

- [ ] Arithmetic
- [ ] Circular functions, e.g. `sin`, `cos`, `atan`, ...
- [ ] Exponentials and logarithms, e.g. `exp`, `ln`, ...
- [ ] Hyperbolic functions, e.g. `sinh`, `atanh`, ...
- [ ] Probabilities functions, like `Factorial`, `Combinations`, etc.
- [ ] Statistics functions, like `Σ+`, `Mean` or `Median`
- [ ] Special functions, like `Bessel`, `FastFourierTransforma`, etc
- [ ] Equation tables with explanatory graphics from files on USB disk

### Computations on vectors, matrices and tensors

- [ ] Arithmetic
- [ ] Determinant, inverse, transpose
- [ ] Eigenvalues
- [ ] Tables with special matrices, e.g. Pauli matrices, General Relativity


### Computer Algebra system

- [ ] Symbolic operations
- [ ] Integrate existing CAS like [GIAC](https://github.com/geogebra/giac)
- [ ] Rules engine
- [ ] Symbolic differentiation
- [ ] Symbolic integration


### Other RPL functional areas

- [ ] Real-time clock
- [ ] Alarms
- [ ] File system access
- [ ] Printing
- [ ] Plotting
- [ ] Equation writer
- [ ] Matrix writer
