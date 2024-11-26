#pragma once

#include "word.hpp"
#include "device.hpp"


struct MTRDevice: Device {

  // CONO MTR state bits
  union ATTRPACKED MTRState {

    struct ATTRPACKED {
      unsigned intLevel: 3;
      unsigned: 7;
      unsigned timeBaseOn: 1;
      unsigned: 1;
      unsigned accountingOn: 1;
      unsigned execNonPIAccount: 1;
      unsigned execPIAccount: 1;
      unsigned: 3;
    };

    unsigned u: 18;

    MTRState(unsigned v=0) :u(v) {}
  } mtrState;


  union ATTRPACKED MTRFunctions {

    struct ATTRPACKED {
      unsigned intLevel: 3;
      unsigned: 6;
      unsigned clear: 1;
      unsigned turnOn: 1;
      unsigned turnOff: 1;
      unsigned turnAcctOn: 1;
      unsigned execNonPIAccount: 1;
      unsigned execPIAccount: 1;
      unsigned: 2;
      unsigned setUpAccounts: 1;
    };

    unsigned u: 18;

    MTRFunctions(unsigned v=0) : u(v) {}
  };


  // Constructors
  MTRDevice(KMState &aState):
    Device(024, "MTR", aState),
    mtrState(0)
  { }


  // I/O instruction handlers
  // WRTIME
  virtual void doCONO(W36 iw, W36 ea) override;
  virtual W36 doCONI(W36 iw, W36 ea) override;
  virtual W36 doDATAI(W36 iw, W36 ea);
  virtual void doBLKI(W36 iw, W36 ea, W36 &nextPC);
  virtual void clearIO();
};
