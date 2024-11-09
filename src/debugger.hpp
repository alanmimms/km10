#pragma once

using namespace std;


#include "km10.hpp"
#include "kmstate.hpp"


struct Debugger {

  Debugger(KM10 &aKM10, KMState &aState)
    : km10(aKM10),
      state(aState)
  {}

  KM10 &km10;
  KMState &state;

  // This is how debugger tells our emulator loop what to do when it
  // returns.
  enum DebugAction {
    // Debugger NEVER returns `noop` to emulator - it's for internal
    // use only.
    noop,
    step,			// Step state.nSteps instructions
    run,			// Just continue from current PC
    quit,			// Exit from the emulator entirely
    restart,			// Restart emulator as if reboot of PDP10
    pcChanged,			// Change of PC - emulator must fetch again
  };

  DebugAction debug();
};
