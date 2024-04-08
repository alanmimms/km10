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

