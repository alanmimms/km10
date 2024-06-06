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


# Instruction Execution Sequence

* `noIncPC` is set when PC should not increment before next fetch.


* `NEXT`:
When we get here we have the instruction in IR.

* Check for pending interrupt, HALTed, meter interrupt, trap. If any of these:
  * Set `noIncPC` flag.
  * If meter interrupt:
	* TBD: Handle meter interrupt.
	* Goto NEXT
  * If interrupt:
	* IR = 40 + n*2.
	* Goto NEXT
  * If HALTed:
	* Goto NEXT
  * If trap:
	* IR = 420+trap.
	* Goto NEXT


* `COMPUTE_EA`:
This is used when PC is in section zero.
Initially IW = IR.

  * Extract I, X, Y from IW.
  * If X != 0, E = RH(AC[X] + Y) else E = Y.
  * If I != 0, IW = fetch(E); goto `COMPUTE_EA`; else done


`EXTENDED_COMPUTE_EA`:
This is used when PC is in a nonzero section.
Initially EAIR = IR.




FETCH:
* IR = *PC.
* If !noIncPC ++PC;

