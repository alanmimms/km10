#pragma once

#include <string>
#include <iostream>
#include <string_view>

using namespace std;


#include "w36.hpp"
#include "km10.hpp"


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
  ac #          Dump a single AC (octal).
  acs           Dump all 16 ACs.
  c,continue    Continue execution at current PC.
  ?,help        Display this help.
  l,log [ac|io|pc|dte|mem|load|off] Display logging flags or toggle a logging flag or turn all off.
  m,memory A N  Dump N (octal) words of memory starting at A (octal).
  pc [N]        Dump PC and flags, or if N is specified set PC to N (octal).
  s,step N      Step N (octal) instructions at current PC.
  q,quit        Quit the KM10 simulator.
)"sv.substr(1);	// ""
    };

    auto dumpAC = [&](int k) {
      cout << "ac" << oct << setfill('0') << setw(2) << k << ": "
	   << state.AC[k].fmt36() << logger.endl;
    };

    cout << "[KM-10 debugger]" << logger.endl;

    const string prompt{"km10> "};
    string prevLine{" "};

    while (!done) {
      cmdMatch = false;
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

      COMMAND("quit", "q", [&]() {done = true;});

      COMMAND("ac", nullptr, [&]() {

	try {
	  int k = stoi(words[1], 0, 8);
	  dumpAC(k);
	} catch (exception &e) {
	}
      });

      COMMAND("acs", nullptr, [&]() {

	for (int k=0; k < 020; ++k) {
	  dumpAC(k);
	}
      });

      COMMAND("memory", "m", [&]() {

	try {
	  W36 a(stoll(words[1], 0, 8));

	  for (unsigned k=0, n=words.size() > 2 ? stoi(words[2], 0, 8) : 1;
	       k < n;
	       ++k)
	    {
	      W36 w(state.memGetN(a));
	      cout << w.dump(true) << logger.endl;
	      a = a + 1;
	    }

	} catch (exception &e) {
	}
      });

      COMMAND("step", "s", [&]() {

	try {
	  state.maxInsns = words.size() > 1 ? stoi(words[1], 0, 8) : 1;
	} catch (exception &e) {
	  state.maxInsns = 1;
	}

	state.running = true;
	km10.emulate();
	cout << logger.endl;
      });

      COMMAND("log", "l", [&]() {

	if (words.size() == 1) {
	  cout << "Logging: ";
	  if (logger.ac) cout << " ac";
	  if (logger.io) cout << " io";
	  if (logger.pc) cout << " pc";
	  if (logger.dte) cout << " dte";
	  if (logger.mem) cout << " mem";
	  if (logger.load) cout << " load";
	  cout << logger.endl;
	} else {

	  if (words[1] == "off") {
	    logger.ac = logger.io = logger.pc = logger.dte = logger.mem = logger.load = false;
	  } else {
	    if (words[1] == "ac") logger.ac = !logger.ac;
	    if (words[1] == "io") logger.ac = !logger.io;
	    if (words[1] == "pc") logger.pc = !logger.pc;
	    if (words[1] == "dte") logger.mem = !logger.dte;
	    if (words[1] == "mem") logger.mem = !logger.mem;
	    if (words[1] == "load") logger.load = !logger.load;
	  }
	}
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
      });

      COMMAND("continue", "c", [&]() {
	state.maxInsns = 0;
	state.running = true;
	km10.emulate();
	cout << logger.endl;
      });

      COMMAND("help", "?", doHelp);

      if (!cmdMatch) doHelp();
    }

    cout << "[exiting]" << logger.endl;
  }
};
