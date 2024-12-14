#include "km10.hpp"

void installUUOs(KM10 &c) {

  c.defOp(0000, "ILLEGAL", [&]() {
    cerr << "ILLEGAL isn't implemented yet" << logger.endl << flush;
    c.pcOffset = 0;
    c.inInterrupt = true;
    logger.nyi(c);
    return iMUUO;
  });

  auto doMUUO = [&]() {
    cerr << "MUUOs aren't implemented yet" << logger.endl << flush;
    //    exceptionPC = pc + 1;
    c.inInterrupt = true;
    logger.nyi(c);
    return iMUUO;
  };

  auto doLUUO = [&]() {

    if (c.pc.isSection0()) {
      W36 uuoState;
      uuoState.op = c.iw.op;
      uuoState.ac = c.iw.ac;
      uuoState.i = 0;
      uuoState.x = 0;
      uuoState.y = c.ea.rhu;

      // XXX this should select executive virtual space first.
      c.memPutN(040, uuoState);
      cerr << "LUUO at " << c.pc.fmtVMA() << " uuoState=" << uuoState.fmt36()
	   << logger.endl << flush;
      c.pcOffset = 041;
      //	exceptionPC = pc + 1;
      c.inInterrupt = true;
      return iTrap;
    } else {

      if (c.flags.usr) {
	W36 uuoA(c.uptP->luuoAddr);
	c.memPutN(W36(((uint64_t) c.flags.u << 23) |
		      ((uint64_t) c.iw.op << 15) |
		      ((uint64_t) c.iw.ac << 5)), uuoA.u++);
	c.memPutN(c.pc, uuoA.u++);
	c.memPutN(c.ea.u, uuoA.u++);
	c.pcOffset = c.memGetN(uuoA);
	//	  exceptionPC = pc + 1;
	c.inInterrupt = true;
	return iTrap;
      } else {	       // Executive mode treats LUUOs as MUUOs
	return doMUUO();
      }
    }
  };

  // Install LUUOs and MUUOs
  for(unsigned op=0001; op < 0037; ++op) c.defOp(op, "LUUO", doLUUO);
  for(unsigned op=0040; op < 0101; ++op) c.defOp(op, "MUUO", doMUUO);

  c.defOp(0104, "JSYS", [&]() {
    cerr << "JSYS isn't implemented yet" << logger.endl << flush;
    //    c.exceptionPC = c.pc + 1;
    c.inInterrupt = true;
    logger.nyi(c);
    return iMUUO;
  });
}
