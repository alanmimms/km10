#pragma once
#include <iostream>


struct Logging {
  bool running;
  bool pc;
  bool ac;
  bool mem;
  bool load;
  uint64_t maxInsns;


  static Logging &getInstance() {
    static Logging instance;
    return instance;
  }

  Logging() = default;
  Logging(const Logging&) = delete;
  Logging& operator=(const Logging&) = delete;

  inline static ofstream s{"km10.log"};

  // Logging
  static void nyi() {
    s << " [not yet implemented]";
  }


  static void nsd() {
    s << " [no such device]";
  }
};


extern Logging logging;
