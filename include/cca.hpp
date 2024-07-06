#pragma once

#include "word.hpp"
#include "device.hpp"


struct CCADevice: Device {
  // Counts down to zero when sweep is busy, causing interrupt when
  // sweep is done.
  int sweepCountDown;

  // We need this to call APR for start and end of sweep.
  APRDevice &apr;

  // Constructors
  CCADevice(KMState &aState, APRDevice &anAPR)
    : Device(014, "CCA", aState),
      sweepCountDown(0),
      apr(anAPR)
  { }


  // Set a "sweep" going, waiting this many instruction cycles before
  // handleSweep triggers the end of the cycle and possibly an
  // interrupt.
  void startSweep() {
    cerr << "CCA: startSweep" << logger.endl;
    sweepCountDown = 50;
    apr.startSweep();
  }


  // This is called each instruction cycle to manage the sweep counter
  // and for APR to trigger the optional interrupt when it's done.
  void handleSweep() {

    if (sweepCountDown && --sweepCountDown == 0) {
      cerr << "CCA: apr.endSweep" << logger.endl;
      apr.endSweep();
    }
  }


  // I/O instruction handlers
  virtual void doCONO(W36 iw, W36 ea) override {
    startSweep();
  }

  // SWPIA
  virtual W36 doDATAI(W36 iw, W36 ea) {
    startSweep();
    return 0;
  }
  
  // SWPVA
  virtual void doBLKO(W36 iw, W36 ea, W36 &nextPC) {
    startSweep();
  }

  // SWPUA
  virtual void doDATAO(W36 iw, W36 ea) {
    startSweep();
  }


  // SWPIO
  virtual W36 doCONI(W36 iw, W36 ea) {
    startSweep();
    return 0;
  }

  // SWPVO
  virtual void doCONSZ(W36 iw, W36 ea, W36 &nextPC) {
    startSweep();
  }

  // SWPUO
  virtual void doCONSO(W36 iw, W36 ea, W36 &nextPC) {
    startSweep();
  }


  virtual void clearIO() {
    apr.endSweep();
  }
};
