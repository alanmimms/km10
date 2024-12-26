#include <iostream>

#include "km10.hpp"
#include "iresult.hpp"
#include "apr.hpp"
#include "device.hpp"
#include "logger.hpp"


// Interrupt handling
void APRDevice::requestInterrupt() {
  aprState.intRequest = 1;
  Device::requestInterrupt();
}

// Interface for CCA to set and clear our sweep status.
void APRDevice::startSweep() {
  aprState.sweepBusy = 1;
}

void APRDevice::endSweep() {
  aprState.sweepBusy = 0;
  aprState.active_sweepDone = 1;
  if (aprState.intLevel != 0 && aprState.enabled_sweepDone) requestInterrupt();
}

// I/O instruction handlers
IResult APRDevice::doBLKI(W36 iw, W36 ea) {	// APRID
  km10.memPut(aprIDValue.u);
  return IResult::iNormal;
}

IResult APRDevice::doBLKO(W36 iw, W36 ea) {	// WRFIL
  // This is essentially a no-op.
  return IResult::iNormal;
}


// XXX implement address breaks under program control.
IResult APRDevice::doDATAI(W36 iw, W36 ea) {
  km10.memPut(breakState.u);
  return IResult::iNormal;
}


IResult APRDevice::doDATAO(W36 iw, W36 ea) {
  breakState.u = km10.memGet().rhu;
  return IResult::iNormal;
}


void APRDevice::putConditions(unsigned v) {
  APRFunctions func{v};
  unsigned select = func.getSelect().u;

  if (logger.mem) cerr << "; " << km10.ea.fmt18();

  if (logger.ints) logger.s << km10.pc.fmtVMA() << " WRAPR: intLevel="
			    << oct << func.intLevel;
  intLevel = aprState.intLevel = func.intLevel;

  if (func.disable) {
    if (logger.ints) logger.s << " disable=" << oct << select;
    aprState.setEnabled(aprState.getEnabled().u & ~select);
  }

  if (func.enable) {
    if (logger.ints) logger.s << " enable=" << oct << select;
    aprState.setEnabled(aprState.getEnabled().u | select);
  }

  // Are any newly enabled and set interrupts added by this change?
  // If so, an interrupt is now pending.
  if (func.enable && func.set &&
      (aprState.getEnabled().u & select) > aprState.getActive().u)
  {
    requestInterrupt();
  }

  if (func.clear) {
    if (logger.ints) logger.s << " clear=" << oct << select;
    aprState.setActive(aprState.getActive().u & ~select);
  }

  if (func.set) {
    if (logger.ints) logger.s << " set=" << oct << select;
    aprState.setActive(aprState.getActive().u | select);
  }

  if (func.clearIO) {
    if (logger.ints) logger.s << " clearIO";
    Device::clearAll();
  }

  if (logger.ints) logger.s << logger.endl;
}


IResult APRDevice::doCONO(W36 iw, W36 ea) {		// WRAPR
  putConditions(ea.rhu);
  return IResult::iNormal;
}


unsigned APRDevice::getConditions() {
  aprState.intLevel = intLevel;	// Refresh our version of the interrupt level
  return aprState.u;
}


IResult APRDevice::doCONI(W36 iw, W36 ea) {		// RDAPR
  unsigned conditions = getConditions();
  if (logger.ints) logger.s << km10.pc.fmtVMA() << " RDAPR aprState="
			    << conditions
			    << " ea=" << ea.fmtVMA()
			    << logger.endl;
  km10.memPut(conditions);
  return IResult::iNormal;
}


void APRDevice::clearIO() {
  Device::clearIO();
  aprState.u = 0;
}
