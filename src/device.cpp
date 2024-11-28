#include <sstream>
#include <functional>

#include "word.hpp"
#include "logger.hpp"
#include "device.hpp"
#include "km10.hpp"
#include "instruction-result.hpp"

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
InstructionResult Device::handleIO(W36 iw, W36 ea) {
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
    return InstructionResult::iNYI;
  }
}


// I/O instruction handlers
void Device::clearIO() {	// Default is to do mostly nothing
  intPending = false;
  intLevel = 0;
}


InstructionResult Device::doDATAI(W36 iw, W36 ea) {
  km10.memPutN(0, ea);
  return InstructionResult::iNormal;
}
  
InstructionResult Device::doBLKI(W36 iw, W36 ea) {
  W36 e{km10.memGetN(ea)};
  km10.memPutN(W36{++e.lhu, ++e.rhu}, ea);
  doDATAI(iw, e.rhu);

  if (e.lhu != 0) {
    return InstructionResult::iSkip;
  } else {
    return InstructionResult::iNormal;
  }
}

InstructionResult Device::doBLKO(W36 iw, W36 ea) {
  W36 e{km10.memGetN(ea)};
  km10.memPutN(W36{++e.lhu, ++e.rhu}, ea);
  doDATAO(iw, e.rhu);

  if (e.lhu != 0) {
    return InstructionResult::iSkip;
  } else {
    return InstructionResult::iNormal;
  }
}


InstructionResult Device::doDATAO(W36 iw, W36 ea) {
  return InstructionResult::iNoSuchDevice;
}


InstructionResult Device::doCONO(W36 iw, W36 ea) {
  return InstructionResult::iNoSuchDevice;
}


InstructionResult Device::doCONI(W36 iw, W36 ea) {
  km10.memPutN(0, ea);
  return InstructionResult::iNormal;
}


InstructionResult Device::doCONSZ(W36 iw, W36 ea) {
  W36 conditions = doCONI(iw, ea);
  unsigned result = conditions.rhu & ea.rhu;
  bool skip = result == 0;

  cerr << ">>>>>>> CONSZ " << name << "," << W36(ea.rhu).fmt18()
       << ": conditions=" << conditions.fmt36()
       << "  result=" << W36(result).fmt18()
       << "  skip=" << skip
       << logger.endl << flush;

  if (skip) {
    return InstructionResult::iSkip;
  } else {
    return InstructionResult::iNormal;
  }
}


InstructionResult Device::doCONSO(W36 iw, W36 ea) {
  W36 conditions = doCONI(iw, ea);

  if ((conditions.rhu & ea.rhu) != 0) {
    return InstructionResult::iSkip;
  } else {
    return InstructionResult::iNormal;
  }
}
