#pragma once

#include "word.hpp"
#include "device.hpp"


struct CCADevice: Device {
  // Counts down to zero when sweep is busy, causing interrupt when
  // sweep is done.
  int sweepCountDown;

  // Constructors
  CCADevice(KM10 &cpu)
    : Device(014, "CCA", cpu),
      sweepCountDown(0)
  { }


  // Set a "sweep" going, waiting this many instruction cycles before
  // handleSweep triggers the end of the cycle and possibly an
  // interrupt.
  void startSweep();


  // This is called each instruction cycle to manage the sweep counter
  // and for APR to trigger the optional interrupt when it's done.
  void handleSweep();


  // I/O instruction handlers
  virtual InstructionResult doCONO(W36 iw, W36 ea) override;
  virtual InstructionResult doDATAI(W36 iw, W36 ea); // SWPIA
  virtual InstructionResult doBLKO(W36 iw, W36 ea);  // SWPVA
  virtual InstructionResult doDATAO(W36 iw, W36 ea); // SWPUA
  virtual InstructionResult doCONI(W36 iw, W36 ea);  // SWPIO
  virtual InstructionResult doCONSZ(W36 iw, W36 ea); // SWPVO
  virtual InstructionResult doCONSO(W36 iw, W36 ea); // SWPUO

  virtual void clearIO();
};
