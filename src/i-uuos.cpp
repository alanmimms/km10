#include "km10.hpp"

struct UUOsGroup: KM10 {

  InstructionResult doMUUO() {
    cout << "MUUO pc=" << pc.fmtVMA() << logger.endl << flush;
    /*
      Kernel	No trap	430
      Kernel	Trap	431

      Superv	No trap	432
      Superv	Trap	433

      Concealed	No trap	434
      Concealed	Trap	435

      Public	No trap	436
      Public	Trap	437
     */
    
    // XXX We don't handle traps in MUUOs yet.
    uint64_t contextBits;

    if (flags.usr) {
      contextBits = 0;

      if (flags.pub) {		// Public
	fetchPC = uptGetN(0436);
      } else {			// Concealed
	fetchPC = uptGetN(0434);
      }
    } else {
      contextBits = pag.getPCW();

      if (flags.pub) {		// Supervisor
	fetchPC = uptGetN(0432);
      } else {			// Kernel
	fetchPC = uptGetN(0430);
      }
    }

    ProgramFlags flagsBits{flags.u};

    // It appears DFKAA at 70013 depends on these flags to be zero.
    flagsBits.cy0 = flagsBits.cy1 = flagsBits.ov = 0;

    uptPutN(W36(contextBits |
		((uint64_t) flagsBits.u << 23) |
		((uint64_t) iw.op << 15) |
		((uint64_t) iw.ac << 5)), 0424);
    uptPutN(pc, 0425);
    uptPutN(ea, 0426);
    uptPutN(pag.getPCW(), 0427);

    // Enter kernel mode to handle MUUO and keep on going from there.
    flags.u = 0;
    pc = fetchPC;

    cout << "    fetchPC=" << fetchPC.fmtVMA() << logger.endl << flush;
    return iMUUO;
  }


  InstructionResult doLUUO() {
    cout << "LUUO pc=" << pc.fmtVMA() << logger.endl << flush;

    if (pc.isSection0()) {
      W36 uuoState = iw;
      uuoState.x = 0;		// Always zero I,X.
      uuoState.i = 0;

      // XXX this should select executive virtual space first.
      memPutN(uuoState, 040);
      cerr << "LUUO at " << pc.fmtVMA() << " uuoState=" << uuoState.fmt36()
	   << logger.endl << flush;
      pcOffset = 0;
      fetchPC = 041;
      // Do NOT set inInterrupt here since JSP, etc. need to save PC+1.
      return iLUUO;
    } else {

      if (flags.usr) {
	W36 uuoA(uptP->luuoAddr);
	memPutN(W36(((uint64_t) flags.u << 23) |
		    ((uint64_t) iw.op << 15) |
		    ((uint64_t) iw.ac << 5)), uuoA.u++);
	memPutN(pc, uuoA.u++);
	memPutN(ea.u, uuoA.u++);
	pcOffset = 0;
	fetchPC = memGetN(uuoA.u++);
	// Do NOT set inInterrupt here since we loaded PC from 420
	// vector and JSP, etc. don't have to be weird.
	return iLUUO;
      } else {	       // Executive mode treats LUUOs as MUUOs.
	return doMUUO();
      }
    }
  }

  // This might someday be special - we'll see.
  InstructionResult doJSYS() {
    cerr << "JSYS isn't implemented yet" << logger.endl << flush;
    //    exceptionPC = pc + 1;
    inInterrupt = true;
    logger.nyi(*this);
    return iMUUO;
  }
};


void InstallUUOsGroup(KM10 &km10) {
  // Install each instruction group's handlers in the ops array. We
  // default every opcode to MUUO until we have an implementation for
  // it.
  km10.ops.fill(static_cast<KM10::OpcodeHandler>(&UUOsGroup::doMUUO));

  // Install LUUOs and MUUOs
  for(unsigned op=0001; op <= 0037; ++op) {
    km10.defOp(op, "LUUO", static_cast<KM10::OpcodeHandler>(&UUOsGroup::doLUUO));
  }
  
  km10.defOp(0104, "JSYS", static_cast<KM10::OpcodeHandler>(&UUOsGroup::doJSYS));
}
