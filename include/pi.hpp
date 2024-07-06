#pragma once

#include "word.hpp"
#include "device.hpp"


struct PIDevice: Device {

  // PI CONO function bits
  union ATTRPACKED PIFunctions {

    struct ATTRPACKED {
      unsigned levels: 7;

      unsigned turnPIOn: 1;
      unsigned turnPIOff: 1;

      unsigned levelsTurnOff: 1;
      unsigned levelsTurnOn: 1;
      unsigned levelsInitiate: 1;
      
      unsigned clearPI: 1;
      unsigned dropRequests: 1;
      unsigned: 1;

      unsigned writeEvenParityDir: 1;
      unsigned writeEvenParityData: 1;
      unsigned writeEvenParityAddr: 1;
    };

    unsigned u: 18;

    // Constructors
    PIFunctions(unsigned v=0) : u(v) {}

    // Accessors
    string toString() const {
      stringstream ss;
      ss << " levels=" << levels;
      if (turnPIOn) ss << " turnPIOn";
      if (turnPIOff) ss << " turnPIOff";
      if (levelsTurnOff) ss << " levelsTurnOff";
      if (levelsTurnOn) ss << " levelsTurnOn";
      if (levelsInitiate) ss << " levelsInitiate";
      if (clearPI) ss << " clearPI";
      if (dropRequests) ss << " dropRequests";
      if (writeEvenParityDir) ss << " writeEvenParityDir";
      if (writeEvenParityData) ss << " writeEvenParityData";
      if (writeEvenParityAddr) ss << " writeEvenParityAddr";
      return ss.str();
    }
  };

  // PI state and CONI bits
  union ATTRPACKED PIState {

    struct ATTRPACKED {
      unsigned levelsOn: 7;
      unsigned piEnabled: 1;
      unsigned intInProgress: 7;
      unsigned writeEvenParityDir: 1;
      unsigned writeEvenParityData: 1;
      unsigned writeEvenParityAddr: 1;
      unsigned levelsRequested: 7;
      unsigned: 11;
    };

    uint64_t u: 36;

    // Constructors
    PIState() :u(0) {}

    // Accessors
    string toString() const {
      stringstream ss;
      ss << " levelsOn=" << levelsOn;
      if (piEnabled) ss << " piEnabled";
      ss << " intInProgress=" << intInProgress;
      if (writeEvenParityDir) ss << " writeEvenParityDir";
      if (writeEvenParityData) ss << " writeEvenParityData";
      if (writeEvenParityAddr) ss << " writeEvenParityAddr";
      ss << " levelsRequested=" << levelsRequested;
      return ss.str();
    }
  } piState;

  // This is the "lowest" level - when no interrupts are being
  // serviced.
  static const inline unsigned noLevel = ~0u;

  // When no interrupt is being handled, this is `noLevel`. A larger
  // unsigned value indicates a level less important than
  // `currentLevel` and therefore must wait.
  unsigned currentLevel;


  // Constructors
  PIDevice(KMState &aState):
    Device(001, "PI", aState)
  {
    piState.u = 0;
    currentLevel = noLevel;
  }


  // Handle any pending interrupt by changing the instruction word
  // about to be executed by KM10, or by doing nothing if there is no
  // pending interrupt.
  void handleInterrupts(W36 &iw) {
    if (!piState.piEnabled) return;

    // XXX for now just return until PI interrupt handling is written.
    return;

    // Find highest pending interrupt and its mask. If none is found
    // or the highest pending is at same or lower level than
    // currentLevel, just exit.
    unsigned levelMask = 1u<<7;
    unsigned highestPending = 1;
    for (; (levelsOn & levelMask) == 0; ++highestPending, levelMask>>=1) ;
    if (levelMask == 0 || highestPending >= currentLevel) return;

    // We have a pending interrupt at `highestPending` level that isn't being serviced.
    piState.intInProgress |= levelMask;		// Mark this level as in progress.
  }


  // I/O instruction handlers
  void clearIO() {
    piState.u = 0;
  }

  void doCONO(W36 iw, W36 ea) {
    PIFunctions pif(ea.u);

    if (logger.mem) logger.s << "; " << ea.fmt18();

    cerr << "CONO PI," << pif.toString() << " ea=" << ea.fmt18() << logger.endl;

    if (pif.clearPI) {
      clearIO();
    } else {
      piState.writeEvenParityDir = pif.writeEvenParityDir;
      piState.writeEvenParityData = pif.writeEvenParityData;
      piState.writeEvenParityAddr = pif.writeEvenParityAddr;

      if (pif.turnPIOn) {
	piState.piEnabled = 1;
      } else if (pif.turnPIOff) {
	piState.piEnabled = 0;
      } else if (pif.dropRequests != 0) {
	piState.levelsRequested &= ~pif.levels;
      } else if (pif.levelsInitiate) {
	piState.levelsRequested |= pif.levels;
      } else if (pif.levelsTurnOff) {
	piState.levelsOn &= ~pif.levels;
      } else if (pif.levelsTurnOn) {
	piState.levelsOn |= pif.levels;
      }
    }

  }

  virtual W36 doCONI(W36 iw, W36 ea) override {
    W36 conditions{(int64_t) piState.u};
    state.memPutN(conditions, ea);
    return conditions;
  }
};
