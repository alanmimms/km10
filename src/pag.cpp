#include "word.hpp"
#include "device.hpp"
#include "pag.hpp"
#include "km10.hpp"


// Constructors
PAGDevice::PAGDevice(KM10 &cpu):
  Device(003, "PAG", cpu)
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
  W36 conditions{km10.memGetN(ea).lhu, pagState.u};
  km10.memPutN(conditions, ea);
  return conditions;
}

void PAGDevice::clearIO() {
  pagState.u = 0;
}
