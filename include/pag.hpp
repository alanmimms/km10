#pragma once

#include "word.hpp"
#include "device.hpp"


struct PAGDevice: Device {

  // CONO PAG state bits
  union PAGState {

    struct ATTRPACKED {
      unsigned execBasePage: 13;
      unsigned enablePager: 1;
      unsigned tops2Paging: 1;
      unsigned: 1;
      unsigned cacheStrategyLoad: 1;
      unsigned cacheStrategyLook: 1;
    };

    unsigned u: 18;

    PAGState() :u(0) {};
  } pagState;


  // Constructors
  PAGDevice(KMState &aState):
    Device(3, "PAG", aState)
  {
    pagState.u = 0;
  }


  // Accessors
  bool pagerEnabled() {
    return pagState.enablePager;
  }


  // I/O instruction handlers
  void doCONO(W36 iw, W36 ea) {
    if (logger.mem) logger.s << "; " << ea.fmt18();
    pagState.u = iw.y;
  }

  void doCONI(W36 iw, W36 ea) {
    state.memPutN(W36(state.memGetN(ea).lhu, pagState.u), ea);
  }

  void clearIO() {
    pagState.u = 0;
  }
};
