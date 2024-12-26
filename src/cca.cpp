#include "word.hpp"
#include "device.hpp"
#include "cca.hpp"
#include "apr.hpp"
#include "km10.hpp"
#include "iresult.hpp"


// Set a "sweep" going, waiting this many instruction cycles before
// handleSweep triggers the end of the cycle and possibly an
// interrupt.
void CCADevice::startSweep() {
  cerr << "CCA: startSweep" << logger.endl;
  sweepCountDown = 10;
  km10.apr.startSweep();
}


// This is called each instruction cycle to manage the sweep counter
// and for APR to trigger the optional interrupt when it's done.
void CCADevice::handleSweep() {

  if (sweepCountDown && --sweepCountDown == 0) {
    cerr << "CCA: apr.endSweep" << logger.endl << flush;
    km10.apr.endSweep();
  }
}


unsigned CCADevice::getConditions() {
  return genericConditions;
}


void CCADevice::putConditions(unsigned v) {
  genericConditions = v;
}


// I/O instruction handlers
IResult CCADevice::doCONO(W36 iw, W36 ea) {
  startSweep();
  return Device::doCONO(iw, ea);
}

// SWPIA
IResult CCADevice::doDATAI(W36 iw, W36 ea) {
  startSweep();
  return Device::doDATAI(iw, ea);
}

// SWPVA
IResult CCADevice::doBLKO(W36 iw, W36 ea) {
  startSweep();
  return Device::doBLKO(iw, ea);
}


// SWPUA
IResult CCADevice::doDATAO(W36 iw, W36 ea) {
  startSweep();
  return Device::doDATAO(iw, ea);
}


// SWPIO
IResult CCADevice::doCONI(W36 iw, W36 ea) {
  startSweep();
  return Device::doCONI(iw, ea);
}

// SWPVO
IResult CCADevice::doCONSZ(W36 iw, W36 ea) {
  startSweep();
  return Device::doCONSZ(iw, ea);
}

// SWPUO
IResult CCADevice::doCONSO(W36 iw, W36 ea) {
  startSweep();
  return Device::doCONSO(iw, ea);
}

void CCADevice::clearIO() {
  if (sweepCountDown) km10.apr.endSweep();
  sweepCountDown = 0;
}
