This directory is the compiler for an encoded PDP10 instruction set.
The use of a compiler allows a simple expression of each instruction's
operation, including flags it changes, side effects, etc. The compiler
reads this list of instruction definitions and produces C code from
it, effectively providing a simple C pre-processor for defining the
cases in the giant `switch()` statement that makes up the emulator.

If I ever decide to change this up so it generates machine code for
the emulator, I could emit assembly language for the host machine or
LLVM IR or something. For now, C is my target.


# Definition of the Language

One useful characteristic of this technique is that the 
