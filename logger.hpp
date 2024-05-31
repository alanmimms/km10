#pragma once
#include <iostream>
#include <set>


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


  void nyi() {
    s << " [not yet implemented]";
  }


  void nsd() {
    s << " [no such device]";
  }
};


extern Logger logger;
