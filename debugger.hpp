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

    auto cmd = [&](const char *s1, const char *s2, auto handler) {

      if (words[0].rfind(s1, 0) == 0 || (s2 != nullptr && words[0].rfind(s2, 0) == 0)) {
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

      cout << "Words: ";
      for (auto &&w: words) cout << w << " ";
      cout << logger.endl;

      cmd("quit", "q", [&]() {done = true;});

      cmd("acs", nullptr, [&]() {

	for (int k=0; k < 020; ++k) {
	  cout << "ac" << oct << setw(2) << k << ": " << state.AC[k].fmt36() << logger.endl;
	}
      });

      cmd("step", "s", [&]() {
	int n = words.size() > 1 ? stoi(words[1]) : 1;
	cout << "Step " << n << " instructions" << logger.endl;
	logger.maxInsns = n;
	state.running = true;
	km10.emulate();
      });

      cmd("help", nullptr, doHelp);

      if (!cmdMatch) doHelp();
    }

    cout << "[exiting]" << logger.endl;
  }
};
