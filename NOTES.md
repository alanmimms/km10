# Interrupt/Exception Requirements:
  * *INVARIANT:* Debugger always shows next instruction to execute in
    its prompt.
    * Interrupt vector instruction.
	* Exception vector instruction.
	* XCT next instruction in chain.
	* Normal code flow instruction.
  * *INVARIANT:* `state.pc` always points to instruction that is about
    to execute or is executing.
    * Therefore, in exception/interrupt handling, `state.pc` points to
      interrupt instruction.
  * *INVARIANT:* `nextPC` always points to next instruction to fetch
    after current one completes.
	* Interrupt/exception instructions like JSP/JSR save `nextPC` as
	  their "return address" before modifying it just like in any other
	  situation.

## Thoughts
  * Real KL10 has a gate on incrementing the PC based on instruction type.
    * Is KM10's solution better - at least for KM10?

  * It looks like incrementing the PC should be the _last_ thing done
    by many instructions so they can trap, page fault, etc. with PC
    pointing to the faulting instruction.

  * Exceptions to this are any sort of jump that changes PC, and any
    sort of skip that needs to increment by two instead of one.

  * The handler for an instruction that causes a trap or page fault
    should see the PC pointing to the instruction that caused the
    fault.

  * An interruptable instructions needs an interrupt handler to see
    the current instruction's PC and not PC+1 so the instruction will
    continue when the interrupt is dismissed. This is much like a
    fault.

  * JSR/JSA/PUSHJ should just save current instruction's PC+1 as their
    return address.

  * Summary:
    * `pc` is always pointing at the instruction being executed, so
      XCT, traps, page faults, etc. work properly. Instructions that
      save the PC like JSR/JSP/JSA/PUSHJ should save the PC+1 value.
    * Interrupts and traps and faults are handled by fetching the
      appropriate vector instruction(s) and executing them _without
      changing `pc`_.
    * By the end of instruction execution, if PC needs to be
	  incremented (the default), `pcIncrement` is nonzero.
	  Instructions (e.g., JRST) that change PC and want no PC
	  increment will set this value to zero. Each time through the
	  instruction execution loop (outside of `execute1()` we set
	  `pcIncrement` back to one.


// NOTE From EK-EBOX-all-OCR2.pdf (PDF p.111):
// If the instruction at 40 + 2n is a BLKX instruction, a specified
// number of transfers are performed, one transfer at a time, each
// time returning to the interrupted program or to a higher level
// subroutine. On the last transfer, the return to the interrupted
// program is “NOT SKIPPED” and an instruction is fetched from 41 +
// 2n. In a similar fashion, if 40 + 2n contains a SKIP class
// instruction; when the skip condition is satisfied, a return to the
// interrupted program takes place. If the skip is not satisfied, the
// instruction in 41 + 2n is executed instead of the return.

