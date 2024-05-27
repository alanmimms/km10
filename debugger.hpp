#pragma once

#include <string>
#include <iostream>

using namespace std;

#include "w36.hpp"
#include "km10.hpp"


struct Debugger {

  Debugger(KMState &aState)
    : state(aState)
  {}

  KMState &state;


  void debug() {
    cerr << "[debugger starts]" << endl;
  }
};
