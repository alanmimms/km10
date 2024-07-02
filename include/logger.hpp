#pragma once
#include <iostream>
#include <set>
#include <fstream>
#include <iostream>
#include <vector>

using namespace std;


class KMState;



struct Logger {
  bool running;
  bool ac;
  bool io;
  bool pc;
  bool dte;
  bool mem;
  bool load;
  bool ea;


  Logger()
    : endl("\n")
  {
    logToTTY();
  }

  ofstream s;
  string endl;
  string destination;
  bool loggingToFile;


  inline static const std::set<string> flags{"ac","io","pc","dte","ea","mem","load"};


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

  void nyi(KMState &state);
  void nsd(KMState &state);
};


extern Logger logger;
