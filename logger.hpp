#pragma once
#include <iostream>
#include <set>

#include "kmstate.hpp"


struct Logger {
  bool running;
  bool ac;
  bool io;
  bool pc;
  bool dte;
  bool mem;
  bool load;


  Logger()
    : endl("\n")
  {
    logToTTY();
  }

  ofstream s;
  string endl;
  string destination;
  bool loggingToFile;


  inline static const std::set<string> flags{"ac","io","pc","dte","mem","load"};


  // Logger
  void logToFile(string name) {
    if (s.is_open()) s.close();
    s.open(name);
    destination = name;
    endl = "\n";
    loggingToFile = true;
  }


  void logToTTY() {
    if (s.is_open()) s.close();
    s.open("/dev/tty");
    destination = "tty";
    endl = "\r\n";
    loggingToFile = false;
  }


  void nyi(KMState &state) {
    s << " [not yet implemented]";
    cerr << "Not yet implemented at " << state.pc.fmtVMA() << endl;
  }


  void nsd(KMState &state) {
    s << " [no such device]";
    cerr << "No such device " << state.pc.fmtVMA() << endl;
  }
};


extern Logger logger;
