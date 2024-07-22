# What Is This KM10 thing?

This is a new implementation to emulate the KL-10 PDP-10 CPU. It
emulates:

* KL-10 CPU with full extended address functionality and without
  cache.

* Paging support matches what the hardware and microcode do in a real
  KL-10 for TOPS-20 paging (only).
  
* Supports the maximum physical memory of 4MW, pretending to have an
  MG20 that implements this just enough so the SBDIAG functions that
  are required work properly.

* Supports RH20 for Massbus peripherals.
  * RP07 or something.
  * TU78 or something.

* Supports one DTE20 attaching an emulated console and "front end"
  bootloader.
  
* Doesn't support:
  * DIA20.
  * Any Unibus peripherals other than those (e.g., console) whose
    function is provided through RSX-20m to the KL-10 via DTE20
    interactions.
  * Multiprocessing with additional KL-10 processors. This might
    change in the future.
  * TOPS-10 paging.
  * PUBLIC mode. This is pretty useless anyway.
  * Address break functionality.


# Building

First, clone the project using
```
git clone --recurse-submodules https://github.com/alanmimms/km10.git
```

Install `cmake` at least version 3.14.

Create a `build` directory and change directory to it and run `cmake`.
```
cd km10 ; mkdir -p build ; cd build ; cmake -DCMAKE_BUILD_TYPE=Debug ..
```

Now you can build the project.
```
make
```

When this completes, run `./km10 --help` to see built in help for the
command line.


# PDP-10 is More Than a Little Strange
The XCT instruction changes a lot about how the CPU determines what
instruction to run next. XCTing a skip instruction and or a
jump-and-save instruction will skip or save the PC after an arbitrary
number of sequential XCTs.

The interrupt and exception handling acts like an XCT was issued from
the point of the interrupted execution to run the interrupt/exception
vector instruction. This means that an interrupt/exception vector
instruction that skips skips the instruction after the interrupted
instruction or the instruction of the exception-causing instruction,
and a JSR or JSP (or similar) instruction in an interrupt/exception
vector saves the PC after the interrupted or the PC of the
exception-causing instruction.
