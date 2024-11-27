#include <iostream>

#include "logger.hpp"
#include "word.hpp"
#include "device.hpp"
#include "apr.hpp"

#define GCC_BUG 1

// TODO: File this as a bug report.
//
// This is necessitated by an apparent bug in G++ 11.4.0 for embedded
// structs in side packed unions - they have an alignment gap.
// Harrumph.
//
// Instead of using the APRFlags union inside other unions/structs, we
// use this hack of declaring `prefix_field` for each field and use
// the accessor functions to get and set the value of the entire
// APRFlags bitfield.
#if GCC_BUG
#define APRFLAGS(PREFIX)			\
  APRFLAG(PREFIX,sweepDone);			\
  APRFLAG(PREFIX,powerFailure);			\
  APRFLAG(PREFIX,addrParity);			\
  APRFLAG(PREFIX,cacheDirParity);		\
  APRFLAG(PREFIX,mbParity);			\
  APRFLAG(PREFIX,ioPageFail);			\
  APRFLAG(PREFIX,noMemory);			\
  APRFLAG(PREFIX,sbusError)
#define APRFLAG(PREFIX,FLAG)	unsigned PREFIX ## _ ## FLAG : 1
#else
#define APRFLAGS(FIELDNAME)	APRFlags FIELDNAME
#endif

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
void APRDevice::doBLKI(W36 iw, W36 ea, W36 &nextPC) {	// APRID
  cpuP->memPutN(aprIDValue.u, ea);
}

void APRDevice::doBLKO(W36 iw, W36 ea, W36 &nextPC) {	// WRFIL
  // This is essentially a no-op.
}

W36 APRDevice::doDATAI(W36 iw, W36 ea) {
  cpuP->memPutN(breakState.u, ea);
  return breakState.u;
}

void APRDevice::doCONO(W36 iw, W36 ea) {		// WRAPR
  APRFunctions func{ea.rhu};
  unsigned select = func.getSelect().u;

  if (logger.mem) cerr << "; " << ea.fmt18();

  cerr << cpuP->pc.fmtVMA() << " WRAPR: intLevel=" << oct << func.intLevel;
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
}

W36 APRDevice::doCONI(W36 iw, W36 ea) {		// RDAPR
  aprState.intLevel = intLevel;	// Refresh our version of the interrupt level
  W36 conditions{(int64_t) aprState.u};
  cerr << cpuP->pc.fmtVMA() << " RDAPR aprState=" << conditions.fmt36()
       << " ea=" << ea.fmtVMA()
       << logger.endl;
  cpuP->memPutN(conditions, ea);
  return conditions;
}

void APRDevice::clearIO() {
  Device::clearIO();
  aprState.u = 0;
}
