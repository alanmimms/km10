#pragma once

using namespace std;

#include "w36.hpp"
#include "device.hpp"
#include "logging.hpp"


struct DTE20: Device {

  DTE20(unsigned anAddr, string aName)
    : Device(anAddr, aName)
  { }


  // See 1982_ProcRefMan.pdf p.262
  struct ControlBlock {
    W36 to11BP;
    W36 to10BP;
    W36 vectorInsn;
    W36 reserved;
    W36 examineAreaSize;
    W36 examineAreaReloc;
    W36 depositAreaSize;
    W36 depositAreaReloc;
  };


  union CONOMask {

    struct ATTRPACKED {
      unsigned: 3;
      unsigned pi0Enable: 1;
      unsigned piEnable: 1;
      unsigned clearTo10: 1;
      unsigned clearTo11: 1;
      unsigned clear11PI: 1;
      unsigned setReload11: 1;
      unsigned clearReload11: 1;
      unsigned to11Doorbell: 1;

      unsigned: 14;
    };

    uint64_t u: 36;
  };

  unsigned piAssigned;
  bool pi0Enabled;


  // I/O instruction handlers
  virtual void clearIO() {
    logging.s << " ; DTE CLEAR IO";
  }

  virtual void doCONO(W36 iw, W36 ea) {
    logging.s << " ; DTE CONO " << oct << ea;
    Logging::nyi();
  }

  virtual void doCONI(W36 iw, W36 ea) {
    logging.s << " ; DTE CONI";
  }
};
