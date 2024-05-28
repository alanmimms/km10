#pragma once

#include <string>
#include <iostream>

using namespace std;

#include "w36.hpp"
#include "km10.hpp"
#include "replxx.hxx"

using Replxx = replxx::Replxx;


struct Debugger {

  Debugger(KMState &aState)
    : state(aState)
  {}

  KMState &state;


  void debug() {
    Replxx rx;

    rx.install_window_change_handler();

    string historyPath{"./.km10-history"};
    ifstream historyFile(historyPath.c_str());

    rx.history_load(historyFile);
    rx.set_max_history_size(1000);
    rx.set_max_hint_rows(3);
    rx.set_prompt("km10>");

    cout << "[KM-10 debugger]" << endl;

    for (;;) {
      char const *cin{nullptr};

      do {
      } while (cin == nullptr && errno == EAGAIN);

      if (cin == nullptr) break;

      string sin{cin};

      if (sin.empty()) continue;
      
      if (sin.rfind(".quit", 0) == 0 || sin.rfind(".exit", 0) == 0) {
	rx.history_add(sin);
	break;
      }
    }
  }
};
