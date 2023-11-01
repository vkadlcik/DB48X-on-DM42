# Release notes

Release 0.4.9 - Full support for units (All Saints Edition)

This release focuses on support for units, but also adds a large number of other
fixes and improvements.

## New features

* Power-off message indicating low-battery situation (#521)
* Add `ConvertToUnixPrefix` command and SI prefix menu keys (#513)
* Recognize all units that exist in the HP48, and a few more (#491)
* `UFACT` (`FactorUnit`) command (#512)
* Unit simplification, e.g. turn `1_m^2*s/s/m` into `1_m` (#506)
* Converting unity units to numbers (#502)
* `→Unit` command (#501)
* `UnitValue` (`UVAL`) command (#493)
* Implement "kibibytes" (`KiB`) and power-of-two SI prefixes (#492)
* Unit arithmetic (#481)
* Add `B->R` and `R->B` to `BasesMenu` (#488)
* Implement term reordering capability in `rewrite` (#484)
* `BaseUnits` (`UBase`) command (#483)
* Unit parsing for complex units, e.g. `1_cm^2` (#482)
* Unit arithmetic (#481) including automatic conversions (#480)
* `Convert` command (#480)
* Implement the `Cycle` command for unit objects
* Add `Å` character for angstroem (#477)
* Add `Merge state` to `State` system menu (#475)
* Use Unicode font to display the name of a program when executing it (#469)
* Allow incremental search to find digits and Unicode (#468)
* Add tool glyph to user interface font


## Bug fixes

* Do not parse symbols beyond input buffer (#524)
* Parse unit menu entries as expressions, not symbols (#523)
* Fix reduced-precision arithmetic (#521)
* Do not parse empty denominator as zero, e.g. `2/s` (#520)
* Do not parse a fraction inside a power, e.g. `X^2/3` (#519)
* Convert fractions to decimal in numeric mode (#516)
* Do not emit `mantissa_error` for valid numbers (#515)
* Apply negation correctly on unit objects (#500)
* Do not emit separator after trailing 0 in integer decimals (#489)
* Do not emit extra spacing before decimal separator (#485)
* Fix stack depth in one error case of `evaluate_function()`
* Fix display of next/previous icons for large menus (#478)
* Clear settings when loading a state (#474)
* Fix separators in whole part of decimal numbers when setting is not 3 (#464)
* Parse `(sin x)²+(cos x)²` correctly, as well as HP67 Mach example (#427)


## Improvements

* Rename `equation` as `expression` (#518) and `labelText` as `label_text`
* Do not update `LastArg` except for command line (#511)
* ToolsMenu: Connect units to the `UnitsConversionMenu` (#514)
* Display unit using `/` and `·`, e.g. `1_m·s^2/A` (#507)
* Show units menu for inverse units as `mm⁻¹` (#503)
* Display battery level more accurately, i.e. consider 2.6V "low" (#476)
* No longer acccept empty equations or parentheses, e.g. `1+()` (#487)
* Make `StandardDisplay` mode obey `MinimumSignificantDigits` (#462)
* Add algebraic evaluation function for easier evaluation in C++ code
* Reimplement `unit` type as a derivative of `complex` (#471)
* documentation: Use `🟨` and `🟦` for more commands (#467)
* Swap `Search` and `Copy` commands in `EditorMenu` (#466)
* `STO` stores variables at beginning of directory (#462)
* documentation: Add quickstart guide, used for video recording
* documentation: Add links to YouTube videos
* documentation: Add release notes
* documentation: Some typos and other improvements
* documentation: Rework section on keyboard mappings
