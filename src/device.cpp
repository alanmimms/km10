#include <sstream>
#include <functional>

#include "device.hpp"
#include "word.hpp"
#include "logger.hpp"


using namespace std;


// Return the device's interrupt function word for its highest
// pending interrupt. Default is to use fixed vector for the level.
// Return zero for level to indicate no pending interrupt from this
// device.
tuple<unsigned,W36> Device::getIntFuncWord() {
  return tuple<unsigned,W36>(0, 0);
}


// Clear the I/O system by calling each device's clearIO() entry
// point.
void Device::clearAll() {
  for (auto [ioDev, devP]: devices) devP->clearIO();
}


// Request an interrupt at this Device's assigned level.
void Device::requestInterrupt()  {
  cerr << " <<< interrupt requested >>>";
  intPending = true;
}


// Handle an I/O instruction by calling the appropriate device
// driver's I/O instruction handler method.
void Device::handleIO(W36 iw, W36 ea, KMState &state, W36 &nextPC) {
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
void Device::clearIO() {	// Default is to do mostly nothing
  intPending = false;
  intLevel = 0;
}


W36 Device::doDATAI(W36 iw, W36 ea) {
  state.memPutN(0, ea);
  return 0;
}
  
void Device::doBLKI(W36 iw, W36 ea, W36 &nextPC) {
  W36 e{state.memGetN(ea)};
  state.memPutN(W36{++e.lhu, ++e.rhu}, ea);
  doDATAI(iw, e.rhu);
  if (e.lhu != 0) ++nextPC.rhu;
}

void Device::doBLKO(W36 iw, W36 ea, W36 &nextPC) {
  W36 e{state.memGetN(ea)};
  state.memPutN(W36{++e.lhu, ++e.rhu}, ea);
  doDATAO(iw, e.rhu);
  if (e.lhu != 0) ++nextPC.rhu;
}


void Device::doDATAO(W36 iw, W36 ea) {
}

void Device::doCONO(W36 iw, W36 ea) {
}

W36 Device::doCONI(W36 iw, W36 ea) {
  state.memPutN(0, ea);
  return 0;
}


void Device::doCONSZ(W36 iw, W36 ea, W36 &nextPC) {
  W36 conditions = doCONI(iw, ea);
  unsigned result = conditions.rhu & ea.rhu;
  bool skip = result == 0;

  cerr << ">>>>>>> CONSZ " << name << "," << W36(ea.rhu).fmt18()
       << ": conditions=" << conditions.fmt36()
       << "  result=" << W36(result).fmt18()
       << "  skip=" << skip
       << logger.endl << flush;

  if (skip) {
    ++nextPC.rhu;
  }
}

void Device::doCONSO(W36 iw, W36 ea, W36 &nextPC) {
  W36 conditions = doCONI(iw, ea);
  if ((conditions.rhu & ea.rhu) != 0) ++nextPC.rhu;
}
