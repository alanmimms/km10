#pragma once

using namespace std;


#include "km10.hpp"
#include "kmstate.hpp"


struct Debugger {

  Debugger(KM10 &aKM10, KMState &aState)
    : km10(aKM10),
      state(aState),
      prevLine("help"),
      lastAddr(0),
      globalSymbols{},
      localSymbols{},
      localInvisibleSymbols{},
      valueToSymbol{},
      verboseLoad(false)
  {}

  KM10 &km10;
  KMState &state;
  string prevLine;
  unsigned lastAddr;

  map<string, W36> globalSymbols;
  map<string, W36> localSymbols;
  map<string, W36> localInvisibleSymbols;
  map<W36, string> valueToSymbol;

  bool verboseLoad;

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

  void loadSEQ(const char *fileNameP);
  void loadREL(const char *fileNameP);

  void loadWord(unsigned addr, W36 value);
};
