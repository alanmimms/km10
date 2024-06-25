#pragma once
#include <cstdint>
#include <map>

using namespace std;

#include "word.hpp"
#include "logger.hpp"
#include "kmstate.hpp"


struct Device {
  unsigned ioAddress;
  string &name;
  KMState &state;

  static inline map<unsigned, Device *> devices{};

  // Constructors
  Device(unsigned anAddr, string aName, KMState &aState)
    : ioAddress(anAddr),
      name(aName),
      state(aState)
  {
    devices[ioAddress] = this;
  }


  // Clear the I/O system by calling each device's clearIO() entry
  // point.
  static void clearAll() {
    for (auto [ioDev, devP]: devices) devP->clearIO();
  }


  // Handle an I/O instruction by calling the appropriate device
  // driver's I/O instruction handler method.
  static void handleIO(W36 iw, W36 ea, KMState &state) {
    auto devP = devices[(unsigned) iw.ioDev];


    if (!devP) {
      cerr << "handleIO at " << state.pc.fmtVMA()
	   << " ioDev=" << oct << (int) iw.ioDev
	   << logger.endl;
      logger.nsd(state);
      return;
    }

    switch (iw.ioOp) {
    case W36::BLKI:
      devP->doBLKI(iw, ea);
      break;

    case W36::DATAI:
      devP->doDATAI(iw, ea);
      break;

    case W36::BLKO:
      devP->doBLKO(iw, ea);
      break;

    case W36::DATAO:
      devP->doDATAO(iw, ea);
      break;

    case W36::CONO:
      devP->doCONO(iw, ea);
      break;

    case W36::CONI:
      devP->doCONI(iw, ea);
      break;

    case W36::CONSZ:
      devP->doCONSZ(iw, ea);
      break;

    case W36::CONSO:
      devP->doCONSZ(iw, ea);
      break;

    default:
      logger.nyi(state);
    }
  }


  // I/O instruction handlers
  virtual void clearIO() {	// Default is to do nothing
  }


  virtual void doDATAI(W36 iw, W36 ea) {
    logger.nyi(state);
  }
  
  virtual void doBLKI(W36 iw, W36 ea) {
    logger.nyi(state);
  }

  virtual void doBLKO(W36 iw, W36 ea) {
    logger.nyi(state);
  }

  virtual void doDATAO(W36 iw, W36 ea) {
    logger.nyi(state);
  }

  virtual void doCONO(W36 iw, W36 ea) {
    logger.nyi(state);
  }

  virtual void doCONI(W36 iw, W36 ea) {
    logger.nyi(state);
  }

  virtual void doCONSZ(W36 iw, W36 ea) {
    logger.nyi(state);
  }
};
