# Debugging DFKAA 64022
* `b 64025`
* This all works fine if single stepped in Debugger.

	42: AOSE 0				; Should inc ac0 and not skip
	43: JSP 1,..0130		; Success!

* Correct result is for JSP to lead to ..0130 (64037) with ac0=2.

* When run without stepping:
  * Interrupt PC=042 correct.
  * AC0=2 correct.
  * The `MOVEI 13,1000` at 64033 is skipped.
  * Evidently, the interrupt processing advanced the PC for some reason.

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

