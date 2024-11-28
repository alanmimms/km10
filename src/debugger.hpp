#pragma once
#include <map>
#include <string>

using namespace std;

#include "word.hpp"
#include "logger.hpp"


struct Debugger {

  Debugger(KM10 &aKM10)
    : km10(aKM10),
      prevLine("help"),
      lastAddr(0),
      globalSymbols{},
      localSymbols{},
      localInvisibleSymbols{},
      valueToSymbol{},
      verboseLoad(false)
  {}

  KM10 &km10;
  string prevLine;
  W36 lastAddr;

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
    step,			// Step nSteps instructions
    run,			// Just continue from current PC
    quit,			// Exit from the emulator entirely
    restart,			// Restart emulator as if reboot of PDP10
    pcChanged,			// Change of PC - emulator must fetch again
  };

  DebugAction debug();

  string dump(W36 w, W36 pc, bool showCharForm=false);
  string symbolicForm(W36 w);

  void loadREL(const char *fileNameP);
  void loadWord(unsigned addr, W36 value);
};
