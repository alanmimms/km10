// TODO:
// * Add command capability to change ACs, memory, PC, flags.
// * Add display of APR state and program flags
// * Add history ring buffer for PC and a way to dump it.
#include <string>
#include <iostream>
#include <string_view>
#include <set>

#include <signal.h>

using namespace std;


#include "debugger.hpp"
#include "word.hpp"
#include "km10.hpp"
#include "kmstate.hpp"


static KMState *stateForHandlerP;

static void sigintHandler(int signum) {
  cerr << "[SIGINT handler]" << logger.endl << flush;
  stateForHandlerP->running = false;
}


Debugger::DebugAction Debugger::debug() {
  string line;
  vector<string> words;


  auto COMMAND = [&](const char *s1, const char *s2, auto handler) {

    if (words[0] == s1 || (s2 != nullptr && words[0] == s2)) {
      handler();
    }
  };


  auto dumpAC = [&](int k) {
    cout << ">>>"
	 << oct << setfill(' ') << setw(2) << k << ": "
	 << state.AC[k].dump(true) << logger.endl;
  };


  auto handleBPCommand = [&](unordered_set<unsigned> &s) {

    if (words.size() == 1) {

      for (auto bp: s) {
	cout << ">>>" << W36(bp).fmtVMA() << logger.endl;
      }

      cout << flush;
    } else if (words.size() > 1) {

      if (words[1] == "clear") {
	s.clear();
      } else {

	try {
	  long a = stol(words[1], 0, 8);

	  if (a < 0) {
	    s.erase((unsigned) -a);
	  } else {
	    s.insert((unsigned) a);
	  }

	} catch (exception &e) {
	}
      }
    }
  };


  ////////////////////////////////////////////////////////////////
  // Put console back into normal mode
  km10.dte.disconnect();

  static bool firstTime{true};
  if (firstTime) {
    stateForHandlerP = &state;
    signal(SIGINT, sigintHandler);

    cout << "[KM-10 debugger]" << logger.endl << flush;
    firstTime = false;
  }

  const string prefix{">>>"};
  const string prompt{prefix + " "};


  // While this stays `noop` we keep looping, reading, and executing
  // debugger commands.
  DebugAction action{noop};

  do {
    // Show next instruction to execute.
    cout << state.pc.fmtVMA() << ": " << km10.iw.dump();
    if (state.inInterrupt) cout << " [EXCEPTION] ";

    cout << prompt << flush;
    getline(cin, line);

    if (line.length() == 0) {
      line = prevLine;
    } else {
      prevLine = line;
    }

    words.clear();

    for (stringstream ss(line); !ss.eof(); ) {
      string s;
      ss >> s;
      if (s.length() > 0) words.push_back(s);
    }

    COMMAND("quit", "q", [&]() {
      state.running = false;
      state.restart = false;
      action = quit;
    });

    COMMAND("abp", nullptr, [&]() {handleBPCommand(state.addressBPs);});

    COMMAND("ac", "acs", [&]() {

      if (words.size() > 1) {

	try {
	  int k = stoi(words[1], 0, 8);
	  dumpAC(k);
	} catch (exception &e) {
	}
      } else {
	for (int k=0; k < 020; ++k) dumpAC(k);
      }
    });

    COMMAND("bp", "b", [&]() {handleBPCommand(state.executeBPs);});

    COMMAND("memory", "m", [&]() {

      try {
	W36 a;
	int n = words.size() > 2 ? stoi(words[2], 0, 8) : 1;

	if (words.size() > 1) {
	  string lo1(words[1]);

	  for (auto& c: lo1) c = tolower(c);

	  if (lo1 == "pc")
	    a = state.pc;
	  else
	    a = stoll(words[1], 0, 8);
	} else {
	  a.u = lastAddr;
	}

	for (int k=0; k < n; ++k) {
	  W36 w(state.memGetN(a));
	  cout << prefix << a.fmtVMA() << ": " << w.dump(true) << logger.endl << flush;
	  a = a + 1;
	}

	lastAddr = a.u;
	prevLine = "m";
      } catch (exception &e) {
      }
    });

    COMMAND("restart", nullptr, [&]() {
      state.restart = true;
      state.running = false;
      action = restart;
    });

    COMMAND("step", "s", [&]() {

      if (words.size() == 1) {
	state.nSteps = 1;
      } else {

	try {
	  state.nSteps = words.size() > 1 ? stoi(words[1], 0, 8) : 1;
	} catch (exception &e) {
	  state.nSteps = 1;
	}
      }

      state.running = true;
      action = step;
    });

    COMMAND("show", nullptr, [&]() {

      if (words.size() == 1) {
	cout << "Must specify apr or flags" << logger.endl;
      } else if (words.size() == 2) {

	if (words[1] == "apr") {
	  cout << prefix << km10.apr.aprState.toString() << logger.endl;
	} else if (words[1] == "pi") {
	  cout << prefix << km10.pi.piState.toString() << logger.endl;
	} else if (words[1] == "flags") {
	  cout << prefix << state.flags.toString() << logger.endl;
	} else if (words[1] == "devs") {

	  for (auto [ioDev, devP]: Device::devices) {
	    if (devP->ioAddress == 0777777) continue;
	    cout << setw(10) << devP->name
		 << " ioAddr=" << W36(devP->ioAddress).fmt18()
		 << " intLevel=" << devP->intLevel
		 << " intPending=" << devP->intPending
		 << logger.endl;
	  }
	} else {
	  cout << "Must specify apr or flags" << logger.endl;
	}
      }

      cout << flush;
    });

    COMMAND("log", "l", [&]() {

      if (words.size() == 1) {
	cout << prefix << "Logging to " << logger.destination << ": ";
	if (logger.ac) cout << " ac";
	if (logger.io) cout << " io";
	if (logger.pc) cout << " pc";
	if (logger.dte) cout << " dte";
	if (logger.ea) cout << " ea";
	if (logger.mem) cout << " mem";
	if (logger.load) cout << " load";
	if (logger.ints) cout << " ints";
	cout << logger.endl;
      } else if (words.size() >= 2) {

	if (words[1] == "off") {
	  logger.ac = logger.io = logger.pc = logger.dte = logger.mem = logger.load = logger.ints = false;
	} else if (words[1] == "file") {
	  logger.logToFile(words.size() == 3 ? words[2] : "km10.log");
	} else if (words[1] == "tty") {
	  logger.logToTTY();
	} else if (words[1] == "all") {
	  logger.ac = logger.io = logger.pc = logger.dte = logger.mem = logger.load = logger.ints = true;
	} else {
	  if (words[1] == "ac") logger.ac = !logger.ac;
	  if (words[1] == "io") logger.io = !logger.io;
	  if (words[1] == "pc") logger.pc = !logger.pc;
	  if (words[1] == "dte") logger.dte = !logger.dte;
	  if (words[1] == "mem") logger.mem = !logger.mem;
	  if (words[1] == "load") logger.load = !logger.load;
	  if (words[1] == "ints") logger.ints = !logger.ints;
	}
      }

      cout << flush;
    });

    COMMAND("pc", nullptr, [&]() {

      if (words.size() == 1) {
	cout << prefix
	     << "Flags:  " << state.flags.toString() << logger.endl
	     << "   PC: " << state.pc.fmtVMA() << logger.endl;

	if (state.inXCT) {
	  cout << " [XCT]" << logger.endl;
	}
      } else {

	try {
	  state.pc.u = stoll(words[1], 0, 8);
	} catch (exception &e) {
	}
      }

      cout << flush;
      action = pcChanged;
    });

    COMMAND("continue", "c", [&]() {
      state.nSteps = 0;
      state.running = true;
      action = run;
    });

    COMMAND("stats", nullptr, [&]() {
      cout << prefix << "Instructions: " << state.nInsns << logger.endl << flush;
    });

    COMMAND("help", "?", [&]() {
      cout << R"(
  abp [A]       Set address breakpoint after any access to address A or list breakpoints.
                Use -A to remove an existing address breakpoint or 'clear' to clear all.
  ac,acs [N]    Dump a single AC N (octal) or all of them.
  b,bp [A]      Set breakpoint before execution of address A or display list of breakpoints.
                Use -A to remove existing breakpoint or 'clear' to clear all breakpoints.
  c,continue    Continue execution at current PC.
  ?,help        Display this help.
  l,log [ac|io|pc|dte|ea|mem|load|ints|off|all]
                Display logging flags, toggle one, or turn all on or off.
  l,log file [FILENAME]
                Log to FILENAME or 'km10.log' if not specified (overwriting).
  l,log tty     Log to console.
  m,memory A N  Dump N (octal) words of memory starting at A (octal). A can be 'pc'.
  pc [N]        Dump PC and flags, or if N is specified set PC to N (octal).
  restart       Reset and reload as if started from scratch again.
  s,step N      Step N (octal) instructions at current PC.
  show apr|pi|flags|devs
                Display APR, PI state, program flags, or device list.
  stats         Display emulator statistics.
  q,quit        Quit the KM10 simulator.
)"sv.substr(1);	// "" (this helps Emacs parse the syntax properly)
    });
  } while (action == noop);

  // Restore console to raw mode for FE input/output.
  km10.dte.connect();
  return action;
}


// Grab the next non-whitespace "word" in the istringstream, skipping
// any leading whitesapce and return it in word. Return true if the
// lineS stream is exhausted before any non-whitespace characters are
// found.
static bool grabNextWord(istringstream &lineS, string &word) {
}


// Load a listing file (*.SEQ) to get symbol definitions for symbolic
// debugging.
void Debugger::loadSEQ(const char *fileNameP) {
  static const int WIDEST_NORMAL_LINE = 122;
  ifstream inS(fileNameP);
  bool scanningSymbols{false};

  string symbolName;		// Symbol we're defining at the moment
  vector<W36> values;		// List of values for current symbol so far

  bool havePreviousSymbol{false};

  while (!ioS.eof()) {
    string scaningLine;
    getline(inS, scanningLine);

    // When we find the end of the assembly listing, the symbols
    // follow after a summary:
    //
    // 001 NO ERRORS DETECTED
    // 002
    // 003 PROGRAM BREAK IS 000000
    // 004 ABSLUTE BREAK IS 071356
    // 005 CPU TIME USED 03:37.242
    // 006
    // 007 18K CORE USED
    // 008
    // 009A00	   902#					[first symbol definition is here]
    if (!scanningSymbols && scanningLine == "NO ERRORS DETECTED") {
      getline(inS, scanningLine);	// Gobble 002
      getline(inS, scanningLine);	// Gobble 003
      getline(inS, scanningLine);	// Gobble 004
      getline(inS, scanningLine);	// Gobble 005
      getline(inS, scanningLine);	// Gobble 006
      getline(inS, scanningLine);	// Gobble 007
      getline(inS, scanningLine);	// Gobble 008
      scanningSymbols = true;
      continue;
    } else if (!scanningSymbols || scanningLine.empty()) {
      continue;			// Just skip all assembly and blank lines
    }

    // Everything we see from this point is a symbol definition line
    // or continuation thereof until EOF. Note EOF ends with a FF and
    // some NULs, which we ignore.
    istringstream lineS(scanningLine);

    // Throw the SEQ xxxxx at column 123 if present.
    if (scanningLine.length() > WIDEST_NORMAL_LINE) {
      scanningLine.resize(WIDEST_NORMAL_LINE);
    }

    // Discard ^L when we encounter it.
    if (lineS.peek() == '\f') lineS.get();

    // If there's no leading whitespace, we're defining a new symbol
    // name. Finish previous symbol definition (if there is one). Then
    // grab the new symbol name and start parsing values.
    if (!isspace(lineS.peek())) {

      // This is really a "first time through" check. Save the symbol
      // values for the symbol that has just been defined.
      if (havePreviousSymbol) {
	symbolToValue[symbolName] = values
      }

      havePreviousSymbol = true;
      values = vector<W36>{};	// Empty values list out for this new symbol

      // Grab the next of the symbol from the leading word on the line.
      lineS >> symbolName;
    }

    // We're now gobbling values for the definition of symbolName.
  }
}
