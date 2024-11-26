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
  PAGDevice(KMState &aState);


  // Accessors
  bool pagerEnabled();


  // I/O instruction handlers
  virtual void doCONO(W36 iw, W36 ea) override;
  virtual W36 doCONI(W36 iw, W36 ea) override;
  virtual void clearIO();
};
