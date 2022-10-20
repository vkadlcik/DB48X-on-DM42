# Building the DB48X project

The DB48X project can be built in two variants:

* A simulator that uses Qt to approximately simulate the DM42 platform. There is
  no guarantee that the DMCP functions will behave as they do on the real thing,
  but it seems to be sufficient for software development. The benefit of using
  the simulator is that it facilitates debugging. The simulator also contains a
  [test suite](https://www.youtube.com/watch?v=vT-I3UlROtA) that can be invoked
  by running the simulator with the `-T` option.

* A firmware for the DM42, which is designed to run on top of SwissMicro's DMCP
  platform, and takes advantage of it.

Each variant can be built in `debug` or `release` mode.


## Prerequisites

The project has the following pre-requisites:

* `make`, `bash`, `dd`, `sed`, `tac`, `printf` and `find`

* Firmware builds require the `arm-none-eabi-gcc` GNU toolchain, which can be
  downloaded [from the ARM site](https://developer.arm.com/open-source/gnu-toolchain/gnu-rm/downloads)
  or installed directly on many platforms.

* Simulator builds require `g++` or `clang`, as well as Qt6 (Qt5 is likely to
  work as well).



## Build

To build the simulator, simply use `make`.

To build the firmware, use `make fw`. If the build complains about the QSPI
contents having changed, something most likely went wrong.

Note: QSPI is an area of flash memory in the DM42 which is presenty used to
store, among other things, some important numerical processing routines. In
theory, the QSPI has additional space that could be used by your program, but
the built-in QSPI loader in the DM42 only accepts QSPI contents that matches
the default DM42 QSPI image. While it is possible to work around this, the
project currently attempts to retain the original QSPI content to make it easier
to install the program, and to ensure you can switch back and forth between DM42
stock firmware and DB48X.


## Testing

There is a test suite integrated in the simulator. Run the simulator with the
`-T` option to test changes before submitting patches or pull requests.


## SDKdemo repository

This code is a distance ancestor of SwissMicro's SDKDemo. The latest version of
SDKdemo is available on [SwissMicro's GitHub account](https://github.com/swissmicros/SDKdemo).

The [db48x.html](help/db48x.html) help file can be copied to the DM42's `/HELP`
directory, although it is presently devoid of useful content. Ultimately, it is
likely that the DB48X's built-in help will use another help format that makes it
easier to have per-function on-line help.
