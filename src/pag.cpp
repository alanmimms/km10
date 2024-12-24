#include "word.hpp"
#include "device.hpp"
#include "pag.hpp"
#include "km10.hpp"
#include "iresult.hpp"


// Constructors
PAGDevice::PAGDevice(KM10 &cpu):
  Device(003, "PAG", cpu)
{
  processContext.u = 0;
  pagState.u = 0;
}


// Accessors
bool PAGDevice::pagerEnabled() {
  return pagState.enablePager;
}


// This is also what is returned by DATAI PAG, .
W36 PAGDevice::getPCW() const {
  return processContext.u;
}


// I/O instruction handlers
IResult PAGDevice::doDATAO(W36 iw, W36 ea) {
  if (logger.mem) logger.s << "; " << ea.fmt18();
  ProcessContext newContext{km10.memGet()};

  if (newContext.loadUserBase) processContext.userBaseAddress = newContext.userBaseAddress;
  if (newContext.selectPrevContext) processContext.prevSection = newContext.prevSection;

  if (newContext.selectACBlocks) {
    processContext.prevACBlock = newContext.prevACBlock;
    processContext.curACBlock = newContext.curACBlock;
    km10.updateACBlock(newContext.curACBlock);
  }
    
  // XXX We do not implement accounting, so ignore `doNotUpdateAccounts`.

  return iNormal;
}

IResult PAGDevice::doDATAI(W36 iw, W36 ea) {
  ProcessContext result{processContext};
  result.loadUserBase = 0;
  result.selectPrevContext = 0;
  result.selectACBlocks = 0;
  km10.memPut(result.u);
  return iNormal;
}

IResult PAGDevice::doCONO(W36 iw, W36 ea) {
  if (logger.mem) logger.s << "; " << ea.fmt18();
  pagState.u = iw.y;
  return iNormal;
}

IResult PAGDevice::doCONI(W36 iw, W36 ea) {
  W36 conditions{km10.memGetN(ea).lhu, pagState.u};
  km10.memPut(conditions);
  return iNormal;
}

void PAGDevice::clearIO() {
  pagState.u = 0;
}
