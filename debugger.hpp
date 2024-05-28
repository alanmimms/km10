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
    string line;
    bool done{false};
    bool cmdMatch;

    auto cmd = [&](const char *s1, const char *s2, auto handler) {

      if (line.rfind(s1, 0) == 0 || (s2 != nullptr && line.rfind(s2, 0) == 0)) {
	handler();
	cmdMatch = true;
      }
    };

    auto doHelp = [&]() {
      cout << R"(Command line options:
  --help	This help.
  --debug	Start the debugger instead of running the loaded program (if any).
  --load=<file>	Load the specified .A10 file or if 'none' is specified, load nothing.
)";
    };

    cout << "[KM-10 debugger]" << endl;

    const string prompt{"km10> "};

    while (!done) {
      cmdMatch = false;
      cout << prompt << flush;
      getline(cin, line);

      cmd("quit", "q", [&]() {done = true;});

      cmd("acs", nullptr, [&]() {

	for (int k=0; k < 020; ++k) {
	  cout << "ac" << oct << setw(2) << k << ": " << state.AC[k].fmt36() << endl;
	}
      });

      cmd("help", nullptr, doHelp);

      if (!cmdMatch) doHelp();
    }

    cout << "[exiting]" << endl;
  }
};
