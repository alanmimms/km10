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


  Logger(ostream &aStream = cout)
    : s(aStream),
      endl("\n")
  {}

  ostream &s;
  string endl;

  inline static const std::set<string> flags{"ac","io","pc","dte","mem","load"};

  // Logger
  void nyi() {
    s << " [not yet implemented]";
  }


  void nsd() {
    s << " [no such device]";
  }
};


extern Logger logger;
