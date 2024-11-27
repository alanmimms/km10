#include "word.hpp"
#include "device.hpp"
#include "mtr.hpp"


// I/O instruction handlers
// WRTIME
void MTRDevice::doCONO(W36 iw, W36 ea) {
  W36 conditions{cpuP->memGetN(ea).rhu};
  if (logger.mem) logger.s << "; " << ea.fmt18();
  mtrState.u = iw.y;

  MTRFunctions funcs{conditions.rhu};

  // XXX do nothing for now . . .
}

W36 MTRDevice::doCONI(W36 iw, W36 ea) {
  cpuP->memPutN(mtrState.u, ea);
  return mtrState.u;
}

// RDEACT
W36 MTRDevice::doDATAI(W36 iw, W36 ea) {
  return 0;
}
  
// RDMACT
void MTRDevice::doBLKI(W36 iw, W36 ea, W36 &nextPC) {
}

void MTRDevice::clearIO() {
  mtrState.u = 0;
}
