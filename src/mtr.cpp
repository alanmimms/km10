#include "word.hpp"
#include "km10.hpp"
#include "device.hpp"
#include "iresult.hpp"
#include "mtr.hpp"

// I/O instruction handlers
// WRTIME
IResult MTRDevice::doCONO(W36 iw, W36 ea) {
  W36 conditions{km10.memGetN(ea).rhu};
  if (logger.mem) logger.s << "; " << ea.fmt18();
  mtrState.u = iw.y;

  MTRFunctions funcs{conditions.rhu};

  // XXX do nothing for now . . .
  return iNormal;
}

IResult MTRDevice::doCONI(W36 iw, W36 ea) {
  km10.memPut(mtrState.u);
  return iNormal;
}

// RDEACT
IResult MTRDevice::doDATAI(W36 iw, W36 ea) {
  km10.memPut(0);
  return iNormal;
}
  
// RDMACT
IResult MTRDevice::doBLKI(W36 iw, W36 ea) {
  return iNormal;
}

void MTRDevice::clearIO() {
  mtrState.u = 0;
}
