#pragma once

#include "w36.hpp"
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
  } state;


  // Constructors
  PIDevice() {
    state.u = 0;
  }


  // Accessors



  // I/O instruction handlers
  void clearIO() {
    state.u = 0;
  }

  void doCONO(W36 ea) {
    PIFunctions pi(ea);

    if (km10.traceMem) cerr << " ; " << oct << eaw.fmt18();

    if (pi.clearPI) {
      clearIO();
    } else {
      state.writeEvenParityDir = pi.writeEvenParityDir;
      state.writeEvenParityData = pi.writeEvenParityData;
      state.writeEvenParityAddr = pi.writeEvenParityAddr;

      if (pi.turnPIOn) {
	state.piEnabled = 1;
      } else if (pi.turnPIOff) {
	state.piEnabled = 0;
      } else if (pi.dropRequests != 0) {
	state.levelsRequested &= ~pi.levels;
      } else if (pi.levelsInitiate) {
	state.levelsRequested |= pi.levels;
      } else if (pi.levelsOff) {
	state.levelsEnabled &= ~pi.levels;
      } else if (pi.levelsOn) {
	state.levelsEnabled |= pi.levels;
      }
    }

  }

  void doCONI() {
    km10.memPut(state.u);
  }
};
