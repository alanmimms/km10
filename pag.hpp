#pragma once

#include "w36.hpp"
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
  } state;


  // Constructors
  PAGDevice() {
    state.u = 0;
  }


  // Accessors
  bool pagerEnabled() {
    return state.enablePager;
  }


  // I/O instruction handlers
  void doCONO(W36 ea) {
    if (km10.traceMem) cerr << " ; " << eaw.fmt18();
    state.u = iw.y;
  }

  void doCONI() {
    km10.memPut(W36(km10.memGet().lhu, state.u));
  }
};
