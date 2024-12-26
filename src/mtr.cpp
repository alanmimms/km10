#include "word.hpp"
#include "km10.hpp"
#include "device.hpp"
#include "iresult.hpp"
#include "mtr.hpp"


unsigned MTRDevice::getConditions() {
  return mtrState.u;
}


void MTRDevice::putConditions(unsigned v) {
  mtrState.u =  v;
}
