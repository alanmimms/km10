#pragma once

#include "word.hpp"
#include "device.hpp"
#include "iresult.hpp"


class KM10;

struct PAGDevice: Device {

  // CONO PAG state bits
  union ProcessContext {

    struct ATTRPACKED {
      unsigned userBaseAddress: 13;
      unsigned doNotUpdateAccounts: 1;
      unsigned prevSection: 5;
      unsigned: 1;
      unsigned prevACBlock: 3;
      unsigned curACBlock: 3;
      unsigned: 3;
      unsigned loadUserBase: 1;
      unsigned selectPrevContext: 1;
      unsigned selectACBlocks: 1;
    };

    uint64_t u: 36;

    ProcessContext(uint64_t v = 0) :u(v) {};
  } processContext;

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
  PAGDevice(KM10 &cpu);


  // Accessors
  bool pagerEnabled();


  // I/O instruction handlers
  virtual IResult doDATAI(W36 iw, W36 ea) override;
  virtual IResult doDATAO(W36 iw, W36 ea) override;
  virtual IResult doCONO(W36 iw, W36 ea) override;
  virtual IResult doCONI(W36 iw, W36 ea) override;
  virtual void clearIO();

  W36 getPCW() const;
};
