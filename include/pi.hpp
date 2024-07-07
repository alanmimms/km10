// KL10 Priority Interrupts device.
//
// KL10/KI10 interrupt terminology (in modern terms):
//
// * "Active" means the interrupt is enabled.
//
// * "Held" means the interrupt is pending.
//
// * "Level" is the interrupt's priority, where lower numbered levels
//   are serviced ahead of higher numbered levels.
//
// Note: only higher priority levels can interrupt lower priority
// levels. An interrupt at a given level cannot be interrupted by a
// same or lower priority level interrupt.

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
      unsigned initiatePR: 1;
      
      unsigned clearPI: 1;
      unsigned dropPR: 1;
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
      if (initiatePR) ss << " initiatePR";
      if (clearPI) ss << " clearPI";
      if (dropPR) ss << " dropPR";
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
      unsigned held: 7;
      unsigned writeEvenParityDir: 1;
      unsigned writeEvenParityData: 1;
      unsigned writeEvenParityAddr: 1;
      unsigned prLevels: 7;
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
      ss << " held=" << held;
      if (writeEvenParityDir) ss << " writeEvenParityDir";
      if (writeEvenParityData) ss << " writeEvenParityData";
      if (writeEvenParityAddr) ss << " writeEvenParityAddr";
      ss << " prLevels=" << prLevels;
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

    // Find highest pending interrupt and its mask.
    unsigned levelMask = 1u<<7;
    unsigned highestPending = 1;
    for (; (piState.levelsOn & levelMask) == 0; ++highestPending, levelMask>>=1) ;

    // If none is found, or if the highest pending is at same or lower
    // level than currentLevel, just exit.
    if (levelMask == 0 || highestPending >= currentLevel) return;

    // We have a pending interrupt at `highestPending` level that isn't being serviced.
    piState.held |= levelMask;		// Mark this level as held.
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
      } else if (pif.dropPR != 0) {
	piState.prLevels &= ~pif.levels;
      } else if (pif.initiatePR) {
	piState.prLevels |= pif.levels;
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
