#include "word.hpp"
#include "device.hpp"
#include "pag.hpp"
#include "km10.hpp"


// Constructors
PAGDevice::PAGDevice(KM10 *aCPU):
  Device(003, "PAG", aCPU)
{
  pagState.u = 0;
}


// Accessors
bool PAGDevice::pagerEnabled() {
  return pagState.enablePager;
}


// I/O instruction handlers
void PAGDevice::doCONO(W36 iw, W36 ea) {
  if (logger.mem) logger.s << "; " << ea.fmt18();
  pagState.u = iw.y;
}

W36 PAGDevice::doCONI(W36 iw, W36 ea) {
  W36 conditions{cpuP->memGetN(ea).lhu, pagState.u};
  cpuP->memPutN(conditions, ea);
  return conditions;
}

void PAGDevice::clearIO() {
  pagState.u = 0;
}
