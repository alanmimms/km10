#pragma once

#include "word.hpp"
#include "device.hpp"


struct CCADevice: Device {
  unsigned genericConditions: 18;

  // Counts down to zero when sweep is busy, causing interrupt when
  // sweep is done.
  int sweepCountDown;

  // Constructors
  CCADevice(KM10 &cpu)
    : Device(014, "CCA", cpu),
      genericConditions(0),
      sweepCountDown(0)
  { }


  // Set a "sweep" going, waiting this many instruction cycles before
  // handleSweep triggers the end of the cycle and possibly an
  // interrupt.
  void startSweep();


  // This is called each instruction cycle to manage the sweep counter
  // and for APR to trigger the optional interrupt when it's done.
  void handleSweep();


  virtual unsigned getConditions();
  virtual void putConditions(unsigned v);

  // I/O instruction handlers
  virtual IResult doCONO(W36 iw, W36 ea) override;
  virtual IResult doDATAI(W36 iw, W36 ea); // SWPIA
  virtual IResult doBLKO(W36 iw, W36 ea);  // SWPVA
  virtual IResult doDATAO(W36 iw, W36 ea); // SWPUA
  virtual IResult doCONI(W36 iw, W36 ea);  // SWPIO
  virtual IResult doCONSZ(W36 iw, W36 ea); // SWPVO
  virtual IResult doCONSO(W36 iw, W36 ea); // SWPUO

  virtual void clearIO();
};
