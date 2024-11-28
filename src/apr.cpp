#include <iostream>

#include "km10.hpp"
#include "instruction-result.hpp"
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
InstructionResult APRDevice::doBLKI(W36 iw, W36 ea) {	// APRID
  km10.memPutN(aprIDValue.u, ea);
  return InstructionResult::iNormal;
}

InstructionResult APRDevice::doBLKO(W36 iw, W36 ea) {	// WRFIL
  // This is essentially a no-op.
  return InstructionResult::iNormal;
}

InstructionResult APRDevice::doDATAI(W36 iw, W36 ea) {
  km10.memPutN(breakState.u, ea);
  return InstructionResult::iNormal;
}

InstructionResult APRDevice::doCONO(W36 iw, W36 ea) {		// WRAPR
  APRFunctions func{ea.rhu};
  unsigned select = func.getSelect().u;

  if (logger.mem) cerr << "; " << ea.fmt18();

  cerr << km10.pc.fmtVMA() << " WRAPR: intLevel=" << oct << func.intLevel;
  intLevel = aprState.intLevel = func.intLevel;

  if (func.disable) {
    cerr << " disable=" << oct << select;
    aprState.setEnabled(aprState.getEnabled().u & ~select);
  }

  if (func.enable) {
    cerr << " enable=" << oct << select;
    aprState.setEnabled(aprState.getEnabled().u | select);
  }

  // Are any newly enabled and set interrupts added by this change?
  // If so, an interrupt is now pending.
  if (func.enable && func.set && (aprState.getEnabled().u & select) > aprState.getActive().u) {
    requestInterrupt();
  }

  if (func.clear) {
    cerr << " clear=" << oct << select;
    aprState.setActive(aprState.getActive().u & ~select);
  }

  if (func.set) {
    cerr << " set=" << oct << select;
    aprState.setActive(aprState.getActive().u | select);
  }

  if (func.clearIO) {
    cerr << " clearIO";
    Device::clearAll();
  }

  cerr << logger.endl;
  return InstructionResult::iNormal;
}

InstructionResult APRDevice::doCONI(W36 iw, W36 ea) {		// RDAPR
  aprState.intLevel = intLevel;	// Refresh our version of the interrupt level
  W36 conditions{(int64_t) aprState.u};
  cerr << km10.pc.fmtVMA() << " RDAPR aprState=" << conditions.fmt36()
       << " ea=" << ea.fmtVMA()
       << logger.endl;
  km10.memPutN(conditions, ea);
  return InstructionResult::iNormal;
}

void APRDevice::clearIO() {
  Device::clearIO();
  aprState.u = 0;
}
