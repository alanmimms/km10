#include <sstream>
#include <functional>

#include "word.hpp"
#include "logger.hpp"
#include "device.hpp"
#include "km10.hpp"
#include "iresult.hpp"

using namespace std;


map<unsigned, Device *> Device::devices = {};


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
  if (logger.ints) logger.s << " <<< interrupt requested >>>";
  intPending = true;
}


// Handle an I/O instruction by calling the appropriate device
// driver's I/O instruction handler method.
IResult Device::handleIO(W36 iw, W36 ea) {
  Device *devP;

  if (auto it = devices.find((unsigned) iw.ioDev); it != devices.end()) {
    devP = it->second;
  } else {
    devP = devices.at(0777777ul);
  }

  assert(devP != nullptr);

  switch (iw.ioOp) {
  case W36::BLKI:
    return devP->doBLKI(iw, ea);

  case W36::DATAI:
    return devP->doDATAI(iw, ea);

  case W36::BLKO:
    return devP->doBLKO(iw, ea);

  case W36::DATAO:
    return devP->doDATAO(iw, ea);

  case W36::CONO:
    return devP->doCONO(iw, ea);

  case W36::CONI:
    return devP->doCONI(iw, ea);

  case W36::CONSZ:
    return devP->doCONSZ(iw, ea);

  case W36::CONSO:
    return devP->doCONSO(iw, ea);

  default:
    return IResult::iNYI;
  }
}


// I/O instruction handlers
void Device::clearIO() {	// Default is to do mostly nothing
  intPending = false;
  intLevel = 0;
  putConditions(0);
}


IResult Device::doBLKI(W36 iw, W36 ea) {
  W36 e{km10.memGetN(ea)};
  km10.memPut(W36{++e.lhu, ++e.rhu});
  doDATAI(iw, e.rhu);

  if (e.lhu != 0) {
    return IResult::iSkip;
  } else {
    return IResult::iNormal;
  }
}


IResult Device::doBLKO(W36 iw, W36 ea) {
  W36 e{km10.memGetN(ea)};
  km10.memPut(W36{++e.lhu, ++e.rhu});
  doDATAO(iw, e.rhu);

  if (e.lhu != 0) {
    return IResult::iSkip;
  } else {
    return IResult::iNormal;
  }
}


IResult Device::doDATAI(W36 iw, W36 ea) {
  km10.memPut(0);
  return IResult::iNormal;
}
  

IResult Device::doDATAO(W36 iw, W36 ea) {
  return IResult::iNoSuchDevice;
}


IResult Device::doCONO(W36 iw, W36 ea) {
  putConditions(ea.rhu);
  return IResult::iNoSuchDevice;
}


IResult Device::doCONI(W36 iw, W36 ea) {
  km10.memPut(getConditions());
  return IResult::iNormal;
}


IResult Device::doCONSZ(W36 iw, W36 ea) {

  if ((getConditions() & ea.rhu) == 0) {
    return IResult::iSkip;
  } else {
    return IResult::iNormal;
  }
}


IResult Device::doCONSO(W36 iw, W36 ea) {

  if ((getConditions() & ea.rhu) != 0) {
    return IResult::iSkip;
  } else {
    return IResult::iNormal;
  }
}
