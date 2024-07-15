#pragma once
#include <cstdint>
#include <map>
#include <assert.h>

using namespace std;

#include "word.hpp"
#include "logger.hpp"
#include "kmstate.hpp"


struct Device {
  unsigned ioAddress;
  string name;
  KMState &state;
  unsigned intLevel;
  bool intPending;

  // Set this in DTE20 device so it can cause interrupts on level #0.
  bool canIntLevel0;

  static inline map<unsigned, Device *> devices{};

  // Constructors
  Device(unsigned anAddr, string aName, KMState &aState, bool aCanIntLevel0 = false)
    : ioAddress(anAddr),
      name(aName),
      state(aState),
      intLevel(0),
      intPending(false),
      canIntLevel0(aCanIntLevel0)
  {
    devices[ioAddress] = this;
  }


  // Return the device's interrupt function word for its highest
  // pending interrupt. Default is to use fixed vector for the level.
  // Return zero for level to indicate no pending interrupt from this
  // device.
  virtual tuple<unsigned,W36> getIntFuncWord() {
    return tuple<unsigned,W36>(0, 0);
  }


  // Clear the I/O system by calling each device's clearIO() entry
  // point.
  static void clearAll() {
    for (auto [ioDev, devP]: devices) devP->clearIO();
  }


  // Request an interrupt at this Device's assigned level.
  void requestInterrupt() {
    // Do nothing if interrupt is disabled
    if (!canIntLevel0 && intLevel == 0) return;

    cerr << " <<< interrupt requested >>>";
    intPending = true;
  }


  // Handle an I/O instruction by calling the appropriate device
  // driver's I/O instruction handler method.
  static void handleIO(W36 iw, W36 ea, KMState &state, W36 &nextPC) {
    Device *devP;

    try {
      devP = devices.at((unsigned) iw.ioDev);
    } catch (const exception &e) {
      devP = devices[0777777ul];
    }

    assert(devP != nullptr);

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
      logger.nyi(state, ss.str());
      break;
      }
    }
  }


  // I/O instruction handlers
  virtual void clearIO() {	// Default is to do mostly nothing
    cerr << state.pc.fmtVMA() << " Device " << name << " clearIO" << logger.endl << flush;
    intPending = false;
    intLevel = 0;
  }


  virtual W36 doDATAI(W36 iw, W36 ea) {
    state.memPutN(0, ea);
    return 0;
  }
  
  virtual void doBLKI(W36 iw, W36 ea, W36 &nextPC) {
    W36 e{state.memGetN(ea)};
    state.memPutN(W36{++e.lhu, ++e.rhu}, ea);
    doDATAI(iw, e.rhu);
    if (e.lhu != 0) ++nextPC.rhu;
  }

  virtual void doBLKO(W36 iw, W36 ea, W36 &nextPC) {
    W36 e{state.memGetN(ea)};
    state.memPutN(W36{++e.lhu, ++e.rhu}, ea);
    doDATAO(iw, e.rhu);
    if (e.lhu != 0) ++nextPC.rhu;
  }

  virtual void doDATAO(W36 iw, W36 ea) {
  }

  virtual void doCONO(W36 iw, W36 ea) {
  }

  virtual W36 doCONI(W36 iw, W36 ea) {
    state.memPutN(0, ea);
    return 0;
  }

  virtual void doCONSZ(W36 iw, W36 ea, W36 &nextPC) {
    W36 conditions = doCONI(iw, ea);
    if ((conditions.rhu & ea.rhu) == 0) ++nextPC.rhu;
  }

  virtual void doCONSO(W36 iw, W36 ea, W36 &nextPC) {
    W36 conditions = doCONI(iw, ea);
    if ((conditions.rhu & ea.rhu) != 0) ++nextPC.rhu;
  }
};
