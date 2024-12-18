#include "km10.hpp"

struct UUOsGroup: KM10 {

  InstructionResult doILLEGAL() {
    cerr << "ILLEGAL isn't implemented yet" << logger.endl << flush;
    pcOffset = 0;
    inInterrupt = true;
    logger.nyi(*this);
    return iMUUO;
  }

  InstructionResult doMUUO() {
    cerr << "MUUOs aren't implemented yet" << logger.endl << flush;
    //    exceptionPC = pc + 1;
    inInterrupt = true;
    logger.nyi(*this);
    return iMUUO;
  }

  InstructionResult doLUUO() {

    if (pc.isSection0()) {
      W36 uuoState;
      uuoState.op = iw.op;
      uuoState.ac = iw.ac;
      uuoState.i = 0;
      uuoState.x = 0;
      uuoState.y = ea.rhu;

      // XXX this should select executive virtual space first.
      memPutN(040, uuoState);
      cerr << "LUUO at " << pc.fmtVMA() << " uuoState=" << uuoState.fmt36()
	   << logger.endl << flush;
      pcOffset = 041;
      //	exceptionPC = pc + 1;
      inInterrupt = true;
      return iTrap;
    } else {

      if (flags.usr) {
	W36 uuoA(uptP->luuoAddr);
	memPutN(W36(((uint64_t) flags.u << 23) |
		    ((uint64_t) iw.op << 15) |
		    ((uint64_t) iw.ac << 5)), uuoA.u++);
	memPutN(pc, uuoA.u++);
	memPutN(ea.u, uuoA.u++);
	pcOffset = memGetN(uuoA);
	//	  exceptionPC = pc + 1;
	inInterrupt = true;
	return iTrap;
      } else {	       // Executive mode treats LUUOs as MUUOs
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
  km10.defOp(0000, "ILLEGAL", static_cast<KM10::OpcodeHandler>(&UUOsGroup::doILLEGAL));

  // Install LUUOs and MUUOs
  for(unsigned op=0001; op < 0037; ++op) km10.defOp(op, "LUUO", static_cast<KM10::OpcodeHandler>(&UUOsGroup::doLUUO));
  for(unsigned op=0040; op < 0101; ++op) km10.defOp(op, "MUUO", static_cast<KM10::OpcodeHandler>(&UUOsGroup::doMUUO));
}
