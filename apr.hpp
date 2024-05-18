#pragma once

#include "logging.hpp"
#include "w36.hpp"
#include "device.hpp"


struct APRDevice: Device {

  // CONI APR interrupt flags
  union APRFlags {

    struct ATTRPACKED {
      unsigned sweepDone: 1;
      unsigned powerFailure: 1;
      unsigned addrParity: 1;
      unsigned cacheDirParity: 1;
      unsigned mbParity: 1;
      unsigned ioPageFail: 1;
      unsigned noMemory: 1;
      unsigned sbusError: 1;
    };

    unsigned u: 8;
  };
  
  // CONI APR status bits (some used in CONO APR)
  union APRState {

    struct ATTRPACKED {
      unsigned intLevel: 3;
      unsigned intRequest: 1;

      APRFlags active;
      unsigned: 4;

      unsigned sweepBusy: 1;
      unsigned: 5;

      APRFlags enabled;

      unsigned: 6;
    };

    uint64_t u: 36;
  } state;


  // CONO APR function bits
  union APRFunctions {

    struct ATTRPACKED {
      unsigned intLevel: 3;
      unsigned: 1;

      APRFlags select;

      unsigned set: 1;
      unsigned clear: 1;
      unsigned disable: 1;
      unsigned enable: 1;

      unsigned clearIO: 1;
      unsigned: 1;
    };

    unsigned u: 18;

    APRFunctions(unsigned v) :u(v) {};
  };

  struct APRLevels {
    unsigned sweepDone: 3;
    unsigned powerFailure: 3;
    unsigned addrParity: 3;
    unsigned cacheDirParity: 3;
    unsigned mbParity: 3;
    unsigned ioPageFail: 3;
    unsigned noMemory: 3;
    unsigned sbusError: 3;
  } aprLevels;


  // APRID value see 1982_ProcRefMan.pdf p.244
  inline static const union {

    struct ATTRPACKED {
      unsigned serialNumber: 12;

      unsigned: 1;
      unsigned masterOscillator: 1;
      unsigned extendedKL10: 1;
      unsigned channelSupported: 1;
      unsigned cacheSupported: 1;
      unsigned AC50Hz: 1;

      unsigned microcodeVersion: 9;
      unsigned: 6;
      unsigned exoticMicrocode: 1;
      unsigned extendedAddressing: 1;
      unsigned tops20Paging: 1;
    };

    uint64_t u: 36;
  } aprIDValue = {
    04321,			// Processor serial number
    1,				// Master oscillator
    1,				// Extended KL10
    1,				// Channel support
    0,				// No cache support
    0,				// 60Hz,
    0442,			// Microcode version number
    0,				// "Exotic" microcode
    1,				// Microcode handles extended addresses
    1,				// TOPS-20 paging
  };


  // I/O instruction handlers
  void doBLKI() {		// APRID
    memPut(aprIDValue.u);
  }

  void doBLKO() {		// WRFIL
    Logging::nyi();
  }

  void doCONO(W36 ea) {		// WRAPR
    APRFunctions func(ea.u);

    if (logging.traceMem) cerr << " ; " << ea.fmt18();

    if (func.clear) {
      state.active.u &= ~func.select.u;
    } else if (func.set) {
      state.active.u |= func.select.u;

      // This block argues the APR state needs to be
      // metaprogrammed with C++17 parameter pack superpowers
      // instead of this old fashioned mnaual method. This might
      // be an interesting thing to do in the future. For now,
      // it's done the 1940s hard way.
      if (func.intLevel != 0) {
	if (func.select.sweepDone != 0)      aprLevels.sweepDone = func.intLevel;
	if (func.select.powerFailure != 0)   aprLevels.powerFailure = func.intLevel;
	if (func.select.addrParity != 0)     aprLevels.addrParity = func.intLevel;
	if (func.select.cacheDirParity != 0) aprLevels.cacheDirParity = func.intLevel;
	if (func.select.mbParity != 0)       aprLevels.mbParity = func.intLevel;
	if (func.select.ioPageFail != 0)     aprLevels.ioPageFail = func.intLevel;
	if (func.select.noMemory != 0)       aprLevels.noMemory = func.intLevel;
	if (func.select.sbusError != 0)      aprLevels.sbusError = func.intLevel;
      }
    } else if (func.enable) {
      state.enabled.u |= func.select.u;
    } else if (func.disable) {
      state.enabled.u &= ~func.select.u;
    }

    if (func.clearIO) {
      for (auto *devP: Device::devices) devP->clearIO();
    }
  }

  void doCONI() {		// RDAPR
    Logging::nyi();
  }
};
