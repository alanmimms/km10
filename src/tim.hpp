#pragma once

#include "word.hpp"
#include "device.hpp"
#include "iresult.hpp"


struct TIMDevice: Device {

  // CONO TIM state bits
  union TIMState {

    struct ATTRPACKED {
      unsigned period: 13;
      unsigned overflow: 1;
      unsigned done: 1;
      unsigned on: 1;
      unsigned: 3;
      unsigned counter: 12;
      unsigned: 6;
    };

    uint64_t u: 36;

    TIMState(uint64_t v=0ull) : u(v) {}
  } timState;


  union ATTRPACKED TIMFunctions {

    struct ATTRPACKED {
    };

    unsigned u: 18;

    TIMFunctions(unsigned v=0) : u(v) {}
  };


  // Constructors
  TIMDevice(KM10 &cpu):
    Device(020, "TIM", cpu),
    timState(0)
  { }


  // Accessors

  // This is called from PI during each interrupt check to see if
  // counters are > 1/2 full and service them as interrupts if they
  // are.
  void updateCounts();

  virtual unsigned getConditions();
  virtual void putConditions(unsigned v);
};
