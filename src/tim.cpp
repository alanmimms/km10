#include "word.hpp"
#include "device.hpp"
#include "tim.hpp"


// Accessors

// This is called from PI during each interrupt check to see if
// counters are > 1/2 full and service them as interrupts if they
// are.
void TIMDevice::updateCounts() {
}


// I/O instruction handlers
void TIMDevice::doCONO(W36 iw, W36 ea) {
  if (logger.mem) logger.s << "; " << ea.fmt18();
  timState.u = iw.y;
}


W36 TIMDevice::doCONI(W36 iw, W36 ea) {
  W36 conditions{(int64_t) timState.u};
  state.memPutN(conditions, ea);
  return conditions;
}

// RDTIME
W36 TIMDevice::doDATAI(W36 iw, W36 ea) {
  state.memPutN(0, ea);
  return 0;
}
  

void TIMDevice::clearIO() {
  timState.u = 0;
}
