#pragma once

#include <iostream>

#include "logger.hpp"
#include "word.hpp"
#include "device.hpp"

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
#define APRFLAGS(PREFIX)	\
  APRFLAG(PREFIX,sweepDone);	\
  APRFLAG(PREFIX,powerFailure);	\
  APRFLAG(PREFIX,addrParity);	\
  APRFLAG(PREFIX,cacheDirParity);\
  APRFLAG(PREFIX,mbParity);	\
  APRFLAG(PREFIX,ioPageFail);	\
  APRFLAG(PREFIX,noMemory);	\
  APRFLAG(PREFIX,sbusError)
#define APRFLAG(PREFIX,FLAG)	unsigned PREFIX ## _ ## FLAG : 1
#else
#define APRFLAGS(FIELDNAME)	APRFlags FIELDNAME
#endif

struct APRDevice: Device {

  // CONI APR interrupt flags
  union ATTRPACKED APRFlags {

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

    // Constructors
    APRFlags(unsigned a) :u(a) {}

    // Accessors
    string toString() const {
      stringstream ss;
      if (sbusError)	  ss << " sbusError";
      if (noMemory)	  ss << " noMemory";
      if (ioPageFail)	  ss << " ioPageFail";
      if (mbParity)	  ss << " mbParity";
      if (cacheDirParity) ss << " cacheDirParity";
      if (addrParity)	  ss << " addrParity";
      if (powerFailure)	  ss << " powerFailure";
      if (sweepDone)	  ss << " sweepDone";
      return ss.str();
    };
  };
  
  // CONI APR status bits (some used in CONO APR)
  union ATTRPACKED APRState {

    struct ATTRPACKED {
      unsigned intLevel: 3;
      unsigned intRequest: 1;

      APRFLAGS(active);

      unsigned: 4;

      unsigned sweepBusy: 1;
      unsigned: 5;

      APRFLAGS(enabled);

      unsigned: 6;
    };

#if GCC_BUG
    struct ATTRPACKED {
      unsigned: 4;
      unsigned active: 8;
      unsigned: 10;
      unsigned enabled: 8;
      unsigned: 6;
    };
#endif

    uint64_t u: 36;

    APRState(W36 v=0) :u(v.u) {}

    APRFlags getActive() const { return APRFlags{active}; }
    void setActive(const APRFlags a) { active = a.u; }

    APRFlags getEnabled() const { return APRFlags{enabled}; }
    void setEnabled(const APRFlags a) { enabled = a.u; }

    string toString() const {
      stringstream ss;
      ss << " enabled={" << getEnabled().toString() << "}";
      if (sweepBusy) ss << " sweepBusy";
      ss << " active={" << getActive().toString() << "}";
      ss << " intRequest=" << intRequest;
      ss << " intLevel=" << intLevel;
      ss << " u=" << W36(u).fmt36();
      return ss.str();
    }
  } aprState;


  union ATTRPACKED APRBreakState {

    struct ATTRPACKED {
      unsigned vma: 23;
      unsigned user: 1;
      unsigned write: 1;
      unsigned read: 1;
      unsigned fetch: 1;
    };

    uint64_t u: 36;

    APRBreakState(W36 v = 0) :u(v.u) {}

    string toString() const {
      stringstream ss;
      ss << " " << W36(u).fmt36() << " ";
      if (fetch) ss << " fetch";
      if (write)  ss << " write";
      if (read) ss << " read";
      ss << " vma=" << W36(vma).fmtVMA();
      return ss.str();
    }
  } breakState;


  // CONO APR function bits
  union ATTRPACKED APRFunctions {

    struct ATTRPACKED {
      unsigned intLevel: 3;
      unsigned: 1;

      APRFLAGS(select);

      unsigned set: 1;
      unsigned clear: 1;
      unsigned disable: 1;
      unsigned enable: 1;

      unsigned clearIO: 1;
      unsigned: 1;
    };

#if GCC_BUG
    struct ATTRPACKED {
      unsigned: 4;
      unsigned select: 8;
    };
#endif

    unsigned u: 18;

    APRFunctions(unsigned v=0) :u(v) {}

    APRFlags getSelect() const { return APRFlags{select}; }
    void setSelect(const APRFlags a) { select = a.u; }

    string toString() const {
      stringstream ss;
      if (clearIO) ss << " clearIO";
      if (enable)  ss << " enable";
      if (disable) ss << " disable";
      if (clear)   ss << " clear";
      if (set)     ss << " set";
      ss << getSelect().toString() << " ";
      ss << "intLevel: " << intLevel;
      return ss.str();
    }
  };

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


  // Constructors
  APRDevice(KMState &aState)
    : Device(0000, "APR", aState)
  { }


  // Interrupt handling
  void requestInterrupt() override {
    aprState.intRequest = 1;
    Device::requestInterrupt();
  }

  // Interface for CCA to set and clear our sweep status.
  void startSweep() {
    aprState.sweepBusy = 1;
  }

  void endSweep() {
    aprState.sweepBusy = 0;
    aprState.active_sweepDone = 1;
    if (aprState.intLevel != 0 && aprState.enabled_sweepDone) requestInterrupt();
  }

  // I/O instruction handlers
  virtual void doBLKI(W36 iw, W36 ea, W36 &nextPC) override {	// APRID
    state.memPutN(aprIDValue.u, ea);
  }

  virtual void doBLKO(W36 iw, W36 ea, W36 &nextPC) override {	// WRFIL
    // This is essentially a no-op.
  }

  virtual W36 doDATAI(W36 iw, W36 ea) override {
    state.memPutN(breakState.u, ea);
    return breakState.u;
  }

  virtual void doCONO(W36 iw, W36 ea) override {		// WRAPR
    APRFunctions func{ea.rhu};
    unsigned select = func.getSelect().u;

    if (logger.mem) cerr << "; " << ea.fmt18();

    cerr << state.pc.fmtVMA() << " WRAPR: intLevel=" << oct << func.intLevel;
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

  virtual W36 doCONI(W36 iw, W36 ea) override {		// RDAPR
    aprState.intLevel = intLevel;	// Refresh our version of the interrupt level
    W36 conditions{(int64_t) aprState.u};
    cerr << state.pc.fmtVMA() << " RDAPR aprState=" << conditions.fmt36()
	 << " ea=" << ea.fmtVMA()
	 << logger.endl;
    state.memPutN(conditions, ea);
    return conditions;
  }

  virtual void clearIO() override {
    Device::clearIO();
    aprState.u = 0;
  }
};