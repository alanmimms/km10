#include "word.hpp"
#include "device.hpp"
#include "tim.hpp"
#include "km10.hpp"
#include "iresult.hpp"

// Accessors

// This is called from PI during each interrupt check to see if
// counters are > 1/2 full and service them as interrupts if they
// are.
void TIMDevice::updateCounts() {
}


// I/O instruction handlers
IResult TIMDevice::doCONO(W36 iw, W36 ea) {
  if (logger.mem) logger.s << "; " << ea.fmt18();
  timState.u = iw.y;
  return iNormal;
}


IResult TIMDevice::doCONI(W36 iw, W36 ea) {
  W36 conditions{(int64_t) timState.u};
  km10.memPut(conditions);
  return iNormal;
}

// RDTIME
IResult TIMDevice::doDATAI(W36 iw, W36 ea) {
  km10.memPut(0);
  return iNormal;
}
  

void TIMDevice::clearIO() {
  timState.u = 0;
}
