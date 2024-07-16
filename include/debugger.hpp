// TODO:
// * Add command capability to change ACs, memory, PC, flags.
// * Add display of APR state and program flags
// * Add history ring buffer for PC and a way to dump it.
#pragma once

#include <string>
#include <iostream>
#include <string_view>
#include <set>

#include <signal.h>

using namespace std;


#include "word.hpp"
#include "km10.hpp"


static KMState *stateForHandlerP;


static void sigintHandler(int signum) {
  cerr << "[SIGINT handler]" << logger.endl << flush;
  stateForHandlerP->running = false;
}


struct Debugger {

  Debugger(KM10 &aKM10, KMState &aState)
    : km10(aKM10),
      state(aState)
  {}

  KM10 &km10;
  KMState &state;


  void debug() {
    string line;
    vector<string> words;
    bool done{false};
    bool cmdMatch;

    auto COMMAND = [&](const char *s1, const char *s2, auto handler) {

      if (words[0] == s1 || (s2 != nullptr && words[0] == s2)) {
	handler();
	cmdMatch = true;
      }
    };

    auto doHelp = [&]() {
      cout << R"(
  abp [A]       Set address breakpoint after any access to address A or list breakpoints.
                Use -A to remove an existing address breakpoint or 'clear' to clear all.
  ac,acs [N]    Dump a single AC N (octal) or all of them.
  b,bp [A]      Set breakpoint before execution of address A or display list of breakpoints.
                Use -A to remove existing breakpoint or 'clear' to clear all breakpoints.
  c,continue    Continue execution at current PC.
  ?,help        Display this help.
  l,log [ac|io|pc|dte|ea|mem|load|ints|off|all]  Display logging flags, toggle one, or turn all on or off.
  l,log file [FILENAME]  Log to FILENAME or 'km10.log' if not specified (overwriting).
  l,log tty     Log to console.
  m,memory A N  Dump N (octal) words of memory starting at A (octal). A can be 'pc'.
  pc [N]        Dump PC and flags, or if N is specified set PC to N (octal).
  s,step N      Step N (octal) instructions at current PC.
  show apr|pi|flags|devs  Display APR, PI state, program flags, or device list.
  stats         Display emulator statistics.
  q,quit        Quit the KM10 simulator.
)"sv.substr(1);	// ""
    };

    auto dumpAC = [&](int k) {
      cout << oct << setfill(' ') << setw(2) << k << ": " << state.AC[k].dump(true) << logger.endl;
    };

    auto dumpACs = [&]() {
      for (int k=0; k < 020; ++k) dumpAC(k);
    };


    auto handleBPCommand = [&](unordered_set<unsigned> &s) {

      if (words.size() == 1) {

	for (auto bp: s) {
	  cout << W36(bp).fmtVMA() << logger.endl;
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


    auto doContinue = [&]() {
      state.running = true;
      state.debugging = true;
      km10.emulate();
      state.debugging = false;
    };


    ////////////////////////////////////////////////////////////////
    cout << "[KM-10 debugger]" << logger.endl << flush;

    stateForHandlerP = &state;
    signal(SIGINT, sigintHandler);

    const string prompt{"km10> "};
    string prevLine{" "};

    while (!done) {
      W36 iw(state.memGetN(state.pc));
      // Show next instruction to execute.
      cout << state.pc.fmtVMA() << ": " << iw.dump();

      cout << prompt << flush;
      getline(cin, line);
      cmdMatch = false;

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

      COMMAND("quit", "q", [&]() {done = true;});

      COMMAND("abp", nullptr, [&]() {handleBPCommand(state.addressBPs);});

      COMMAND("ac", "acs", [&]() {

	if (words.size() > 1) {

	  try {
	    int k = stoi(words[1], 0, 8);
	    dumpAC(k);
	  } catch (exception &e) {
	  }
	} else {
	  dumpACs();
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
	  }

	  for (int k=0; k < n; ++k) {
	    W36 w(state.memGetN(a));
	    cout << a.fmtVMA() << ": " << w.dump(true) << logger.endl << flush;
	    a = a + 1;
	  }
	} catch (exception &e) {
	}
      });

      COMMAND("step", "s", [&]() {

	if (words.size() == 1) {
	  state.maxInsns = 1;
	} else {

	  try {
	    state.maxInsns = words.size() > 1 ? stoi(words[1], 0, 8) : 1;
	  } catch (exception &e) {
	    state.maxInsns = 1;
	  }
	}

	doContinue();
      });

      COMMAND("show", nullptr, [&]() {

	if (words.size() == 1) {
	  cout << "Must specify apr or flags" << logger.endl;
	} else if (words.size() == 2) {

	  if (words[1] == "apr") {
	    cout << km10.apr.aprState.toString() << logger.endl;
	  } else if (words[1] == "pi") {
	    cout << km10.pi.piState.toString() << logger.endl;
	  } else if (words[1] == "flags") {
	    cout << state.flags.toString() << logger.endl;
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
	  cout << "Logging to " << logger.destination << ": ";
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
	  cout << "Flags:  " << state.flags.toString() << logger.endl
		   << "   PC: " << state.pc.fmtVMA() << logger.endl;
	} else {

	  try {
	    state.pc.u = stoll(words[1], 0, 8);
	  } catch (exception &e) {
	  }
	}

	cout << flush;
      });

      COMMAND("continue", "c", [&]() {
	state.maxInsns = 0;
	doContinue();
      });

      COMMAND("stats", nullptr, [&]() {
	cout << "Instructions: " << state.nInsns << logger.endl << flush;
      });

      COMMAND("help", "?", doHelp);

      if (!cmdMatch) doHelp();
    }

    cout << "[exiting]" << logger.endl << flush;
  }
};
