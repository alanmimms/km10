#pragma once

#include "word.hpp"
#include "device.hpp"


struct PIDevice: Device {

  // PI CONO function bits
  union PIFunctions {
    PIFunctions(unsigned v) :u(v) {};

    struct ATTRPACKED {
      unsigned levels: 7;

      unsigned turnPIOn: 1;
      unsigned turnPIOff: 1;

      unsigned levelsOff: 1;
      unsigned levelsOn: 1;
      unsigned levelsInitiate: 1;
      
      unsigned clearPI: 1;
      unsigned dropRequests: 1;
      unsigned: 1;

      unsigned writeEvenParityDir: 1;
      unsigned writeEvenParityData: 1;
      unsigned writeEvenParityAddr: 1;
    };

    unsigned u: 18;
  };

  // PI state and CONI bits
  union PIState {

    struct ATTRPACKED {
      unsigned levelsEnabled: 7;
      unsigned piEnabled: 1;
      unsigned intInProgress: 7;
      unsigned writeEvenParityDir: 1;
      unsigned writeEvenParityData: 1;
      unsigned writeEvenParityAddr: 1;
      unsigned levelsRequested: 7;
      unsigned: 11;
    };

    uint64_t u: 36;

    PIState() :u(0) {};
  } piState;


  // Constructors
  PIDevice(KMState &aState):
    Device(001, "PI", aState)
  {
    piState.u = 0;
  }


  // I/O instruction handlers
  void clearIO() {
    piState.u = 0;
  }

  void doCONO(W36 iw, W36 ea) {
    PIFunctions pi(ea);

    if (logger.mem) logger.s << "; " << ea.fmt18();

    if (pi.clearPI) {
      clearIO();
    } else {
      piState.writeEvenParityDir = pi.writeEvenParityDir;
      piState.writeEvenParityData = pi.writeEvenParityData;
      piState.writeEvenParityAddr = pi.writeEvenParityAddr;

      if (pi.turnPIOn) {
	piState.piEnabled = 1;
      } else if (pi.turnPIOff) {
	piState.piEnabled = 0;
      } else if (pi.dropRequests != 0) {
	piState.levelsRequested &= ~pi.levels;
      } else if (pi.levelsInitiate) {
	piState.levelsRequested |= pi.levels;
      } else if (pi.levelsOff) {
	piState.levelsEnabled &= ~pi.levels;
      } else if (pi.levelsOn) {
	piState.levelsEnabled |= pi.levels;
      }
    }

  }

  void doCONI(W36 iw, W36 ea) {
    state.memPutN(piState.u, ea);
  }
};
