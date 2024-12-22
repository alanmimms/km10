#include "km10.hpp"

struct UUOsGroup: KM10 {

  InstructionResult doMUUO() {
    cerr << "MUUOs aren't implemented yet" << logger.endl << flush;
    //    exceptionPC = pc + 1;
    inInterrupt = true;
    logger.nyi(*this);
    return iMUUO;
  }

  InstructionResult doLUUO() {

    if (pc.isSection0()) {
      W36 uuoState = iw;
      uuoState.x = 0;		// Always zero X.

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
  // Install LUUOs and MUUOs
  for(unsigned op=0001; op <= 0037; ++op) {
    km10.defOp(op, "LUUO", static_cast<KM10::OpcodeHandler>(&UUOsGroup::doLUUO));
  }
  
  for(unsigned op=0040; op <= 0101; ++op) {
    km10.defOp(op, "MUUO", static_cast<KM10::OpcodeHandler>(&UUOsGroup::doMUUO));
  }

  km10.defOp(0247, "MUUO", static_cast<KM10::OpcodeHandler>(&UUOsGroup::doMUUO));

  km10.defOp(0104, "JSYS", static_cast<KM10::OpcodeHandler>(&UUOsGroup::doJSYS));
}
