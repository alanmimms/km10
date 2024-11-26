#include "word.hpp"
#include "device.hpp"
#include "cca.hpp"
#include "apr.hpp"


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
void CCADevice::doCONO(W36 iw, W36 ea) {
  startSweep();
}

// SWPIA
W36 CCADevice::doDATAI(W36 iw, W36 ea) {
  startSweep();
  return 0;
}

// SWPVA
void CCADevice::doBLKO(W36 iw, W36 ea, W36 &nextPC) {
  startSweep();
}


// SWPUA
void CCADevice::doDATAO(W36 iw, W36 ea) {
  startSweep();
}


// SWPIO
W36 CCADevice::doCONI(W36 iw, W36 ea) {
  startSweep();
  return 0;
}

// SWPVO
void CCADevice::doCONSZ(W36 iw, W36 ea, W36 &nextPC) {
  startSweep();
}

// SWPUO
void CCADevice::doCONSO(W36 iw, W36 ea, W36 &nextPC) {
  startSweep();
}

void CCADevice::clearIO() {
  if (sweepCountDown) apr.endSweep();
  sweepCountDown = 0;
}
