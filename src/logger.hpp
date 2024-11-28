#pragma once
#include <iostream>
#include <set>
#include <fstream>
#include <iostream>
#include <vector>

using namespace std;

class KM10;


struct Logger {
  bool running;
  bool ac;
  bool io;
  bool pc;
  bool dte;
  bool mem;
  bool load;
  bool ea;
  bool ints;


  Logger()
    : endl("\n")
  {
    logToTTY();
  }

  ofstream s;
  string endl;
  string destination;
  bool loggingToFile;


  inline static const std::set<string> flags{"ac","io","pc","dte","ea","mem","load","ints"};


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

  void nyi(KM10 &cpu, const string &context = "");
  void nsd(KM10 &cpu, const string &context = "");
};


extern Logger logger;
