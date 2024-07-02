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
  static void handleIO(W36 iw, W36 ea, KMState &state, W36 &nextPC) {
    auto devP = devices[(unsigned) iw.ioDev];
    string opName;

    if (devP == nullptr) devP = devices[0777777ul];

    switch (iw.ioOp) {
    case W36::BLKI:
      devP->doBLKI(iw, ea, nextPC);
      return;

    case W36::DATAI:
      devP->doDATAI(iw, ea);
      return;

    case W36::BLKO:
      devP->doBLKO(iw, ea, nextPC);
      return;

    case W36::DATAO:
      devP->doDATAO(iw, ea);
      return;

    case W36::CONO:
      devP->doCONO(iw, ea);
      return;

    case W36::CONI:
      devP->doCONI(iw, ea);
      return;

    case W36::CONSZ:
      devP->doCONSZ(iw, ea, nextPC);
      return;

    case W36::CONSO:
      devP->doCONSO(iw, ea, nextPC);
      return;

    default: {
      stringstream ss;
      ss << "???" << oct << iw.ioOp << "???";
      opName = ss.str();
      logger.nyi(state, ss.str());
      break;
      }
    }
  }


  // I/O instruction handlers
  virtual void clearIO() {	// Default is to do nothing
  }


  virtual void doDATAI(W36 iw, W36 ea) {
  }
  
  virtual void doBLKI(W36 iw, W36 ea, W36 &nextPC) {
    W36 e{state.memGetN(ea)};
    state.memPutN(W36{++e.lhu, ++e.rhu}, ea);
    this->doDATAI(iw, e.rhu);
    if (e.lhu != 0) ++nextPC.rhu;
  }

  virtual void doBLKO(W36 iw, W36 ea, W36 &nextPC) {
    W36 e{state.memGetN(ea)};
    state.memPutN(W36{++e.lhu, ++e.rhu}, ea);
    this->doDATAO(iw, e.rhu);
    if (e.lhu != 0) ++nextPC.rhu;
  }

  virtual void doDATAO(W36 iw, W36 ea) {
  }

  virtual void doCONO(W36 iw, W36 ea) {
  }

  virtual void doCONI(W36 iw, W36 ea) {
  }

  virtual void doCONSZ(W36 iw, W36 ea, W36 &nextPC) {
    ++nextPC.rhu;		// ALWAYS SKIP
  }

  virtual void doCONSO(W36 iw, W36 ea, W36 &nextPC) {
  }
};
