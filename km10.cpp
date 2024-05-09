#include "km10.hpp"
#include <iostream>
#include <cassert>

using namespace std;


int main(int argc, char *argv[]) {
  assert(sizeof(W36) == 8);
  static W36 memory[4 * 1024 * 1024];

  if (argc != 2) {
    cerr << R"(
Usage:
    )" << argv[0] << R"( <filename to load>
)";
    return -1;
  }

  KM10 km10(memory, 50);

  km10.loadA10(argv[1]);
  cerr << "[Loaded " << argv[1]
	    << "  start=" << km10.pc.fmtVMA()
	    << "]" << endl;

  km10.tracePC = 1;
  km10.traceAC = 1;
  km10.traceMem = 1;
  km10.running = 1;

  km10.emulate();

  return 0;
}
