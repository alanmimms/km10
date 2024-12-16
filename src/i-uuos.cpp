#include "km10.hpp"

void KM10::installUUOsGroup() {

  defOp(0000, "ILLEGAL", [this]() {
    cerr << "ILLEGAL isn't implemented yet" << logger.endl << flush;
    pcOffset = 0;
    inInterrupt = true;
    logger.nyi(c);
    return iMUUO;
  });


  auto doMUUO = [this]() {
    cerr << "MUUOs aren't implemented yet" << logger.endl << flush;
    //    exceptionPC = pc + 1;
    inInterrupt = true;
    logger.nyi(c);
    return iMUUO;
  };

  auto doLUUO = [this]() {

    if (pisSection0()) {
      W36 uuoState;
      uuoState.op = iw.op;
      uuoState.ac = iw.ac;
      uuoState.i = 0;
      uuoState.x = 0;
      uuoState.y = ea.rhu;

      // XXX this should select executive virtual space first.
      memPutN(040, uuoState);
      cerr << "LUUO at " << pfmtVMA() << " uuoState=" << uuoState.fmt36()
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
  };

  // Install LUUOs and MUUOs
  for(unsigned op=0001; op < 0037; ++op) defOp(op, "LUUO", doLUUO);
  for(unsigned op=0040; op < 0101; ++op) defOp(op, "MUUO", doMUUO);

  // This might someday be special - we'll see.
  defOp(0104, "JSYS", [this]() {
    cerr << "JSYS isn't implemented yet" << logger.endl << flush;
    //    exceptionPC = pc + 1;
    inInterrupt = true;
    logger.nyi(c);
    return iMUUO;
  });
}
