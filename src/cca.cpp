#include "word.hpp"
#include "device.hpp"
#include "cca.hpp"
#include "apr.hpp"
#include "instruction-result.hpp"


// Set a "sweep" going, waiting this many instruction cycles before
// handleSweep triggers the end of the cycle and possibly an
// interrupt.
void CCADevice::startSweep() {
  cerr << "CCA: startSweep" << logger.endl;
  sweepCountDown = 10;
  apr.startSweep();
}


// This is called each instruction cycle to manage the sweep counter
// and for APR to trigger the optional interrupt when it's done.
void CCADevice::handleSweep() {

  if (sweepCountDown && --sweepCountDown == 0) {
    cerr << "CCA: apr.endSweep" << logger.endl << flush;
    apr.endSweep();
  }
}


// I/O instruction handlers
InstructionResult CCADevice::doCONO(W36 iw, W36 ea) {
  startSweep();
  return InstructionResult::iNormal;
}

// SWPIA
InstructionResult CCADevice::doDATAI(W36 iw, W36 ea) {
  startSweep();
  return InstructionResult::iNormal;
}

// SWPVA
InstructionResult CCADevice::doBLKO(W36 iw, W36 ea) {
  startSweep();
  return InstructionResult::iNormal;
}


// SWPUA
InstructionResult CCADevice::doDATAO(W36 iw, W36 ea) {
  startSweep();
  return InstructionResult::iNormal;
}


// SWPIO
InstructionResult CCADevice::doCONI(W36 iw, W36 ea) {
  startSweep();
  return InstructionResult::iNormal;
}

// SWPVO
InstructionResult CCADevice::doCONSZ(W36 iw, W36 ea) {
  startSweep();
  return InstructionResult::iNormal; // XXX
}

// SWPUO
InstructionResult CCADevice::doCONSO(W36 iw, W36 ea) {
  startSweep();
  return InstructionResult::iNormal; // XXX
}

void CCADevice::clearIO() {
  if (sweepCountDown) apr.endSweep();
  sweepCountDown = 0;
}
