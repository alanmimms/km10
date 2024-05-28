#pragma once
#include <iostream>


struct Logger {
  bool running;
  bool pc;
  bool ac;
  bool mem;
  bool load;
  uint64_t maxInsns;


  Logger(ostream &aStream = cout)
    : s(aStream)
  {}

  ostream &s;

  // Logger
  void nyi() {
    s << " [not yet implemented]";
  }


  void nsd() {
    s << " [no such device]";
  }
};


extern Logger logger;
