#pragma once
#include <map>
#include <string>
#include <optional>

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

  static constexpr size_t pcHistorySize = 1024;

  struct RingBuffer {
    RingBuffer();

    void add(const W36& element);
    std::optional<W36> peek() const;
    std::vector<W36> mostRecent(size_t n) const;
    void print(std::ostream& s, size_t count) const;

    std::array<W36, pcHistorySize> buffer;
    size_t head;
    bool full;
  } pcRing;


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
