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


// Load a listing file (*.SEQ) to get symbol definitions for symbolic
// debugging.
void Debugger::loadSEQ(const char *fileNameP) {
  static const int WIDEST_NORMAL_LINE = 122;
  ifstream inS(fileNameP);
  map<int, string> lines;	// Each listing line from SEQ file is placed in here
  bool scanningAssembly{false};
  bool scanningSymbols{false};

  string symbolName;	   // Symbol we're defining at the moment
  W36 value;		   // Line number where the symbol is DEFINED ("#")

  while (!inS.eof()) {
    string scanningLine;
    getline(inS, scanningLine);
    if (scanningLine.length() == 0) continue;

    // If we are skipping the front matter before the assembly listing
    // begins, check for the heading that indicates the assembly
    // listing has begun. This is indicated by a line that starts with
    // '\f' and has the MACRO program's heading format. If this is
    // true, we can start scanning the assembly listing.
    if (!scanningAssembly) {

      if (scanningLine[0] == '\f' && scanningLine.find("MACRO %") != string::npos) {
	scanningAssembly = true;
	getline(inS, scanningLine);		// Discard second line of page header
      }

      continue;
    }

    // Ignore listing page headers (two lines) if we're not yet to the
    // symbol CREF part of the listing.
    if (!scanningSymbols && scanningLine.front() == '\f') {
      getline(inS, scanningLine);
      continue;
    }

    istringstream lineS(scanningLine);
    string word;
    int lineNumber;

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
    } else if (!scanningSymbols) {

      // If we see a formfeed we have to ignore the line and the one following it.
      if (lineS.peek() == '\f') {
	getline(inS, scanningLine); // Grab second line of page heading and throw it away.
	continue;
      }

      // If we get here it's a normal listing line. Grab the line
      // number from the first "word". Save the line for back
      // referencing from symbol CREF.
      lineS >> word;
      lineNumber = stoi(word);
      lines[lineNumber] = scanningLine;
      continue;
    }

    // Everything we see from this point is a symbol definition line
    // or continuation thereof until EOF. Note EOF ends with a FF and
    // some NULs, which we ignore.


    // Throw the SEQ xxxxx at column 123 if present.
    if (scanningLine.length() > WIDEST_NORMAL_LINE) {
      scanningLine.resize(WIDEST_NORMAL_LINE);
    }

    // Discard ^L when we encounter it.
    if (lineS.peek() == '\f') lineS.get();

    // If there's no leading whitespace, we're defining a new symbol.
    // Grab the name of the symbol from the leading word on the line.
    if (!isspace(lineS.peek())) {
      lineS >> symbolName;
    }

    // We're now gobbling values for the definition of symbolName,
    // looking for the one with "#" indicating the line number the
    // symbol is defined on.
    while (lineS >> word) {

      // If the last character of the "word" is "#" it means this word
      // contains the line number in the listing file where the symbol
      // is defined. Get that line and find its address and save that
      // as the symbol's value.
      if (!word.empty() && word.back() == '#') {
	// Strip trailing '#' and scan the integer line number.
	lineNumber = stoi(word.substr(0, word.size() - 1));
	istringstream listingS(lines[lineNumber]);

	// Examine the listing line to determine if it's an assembly
	// of a word (one tab char after the line number) or a 36-bit
	// constant valued symbol (TWO tabs after the line number)
	// that is being defined there.
	listingS >> word;	// Grab the line number

	if (listingS.get() == '\t' && listingS.peek() == '\t') {
	  listingS.get();	// Consume the second tab
	  unsigned lh = 0;
	  unsigned rh = 0;

	  // Test whether we have fullword (two tabs) or halfword
	  // (three tabs) constant.
	  if (listingS.peek() == '\t') {
	    // A halfword symbol - THREE tabs and just one octal value.
	    // 42927			000774			LAST=774	...
	    listingS >> word;	// Grab the symbol's halfword octal value
	    rh = stoi(word, nullptr, 8);
	  } else {
	    // 36-bit constant flavor (two tabs), value is LH\tRH.
	    // 42937		700600	031577			OPDEF	CLRPI	...
	    listingS >> word;	// Grab the symbol's LH octal value
	    lh = stoi(word, nullptr, 8);
	    listingS >> word;	// Grab the symbol's RH octal value
	    rh = stoi(word, nullptr, 8);
	  }

	  value = W36(lh, rh);
	} else {
	  // Assembly word flavor (one tab), where the value is just a halfword:
	  //  42914	057634	402 00 0 00 030037 	BEGIOT:	...
	  listingS >> word;	// Grab the symbol's octal value
	  value = W36(stoi(word, nullptr, 8));
	}

	// Given the value, save the symbol's definition. We cannot
	// get a symbol definition that already exists in
	// `symbolToValue[]`, but we may have many symbols with the
	// same value. The first part of this means we don't ever have
	// to worry about inserting multiple entries in
	// `valueToSymbol[]` with the same `symbolName`.
	symbolToValue[symbolName] = value;
	valueToSymbol.insert(pair(value, symbolName));
      }
    }
  }
}
