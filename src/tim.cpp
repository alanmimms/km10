#include "word.hpp"
#include "device.hpp"
#include "tim.hpp"
#include "km10.hpp"
#include "iresult.hpp"

// Accessors

// This is called from PI during each interrupt check to see if
// counters are > 1/2 full and service them as interrupts if they
// are.
void TIMDevice::updateCounts() {
}


unsigned TIMDevice::getConditions() {
  return timState.u;
}


void TIMDevice::putConditions(unsigned v) {
  timState.u = v;
}
