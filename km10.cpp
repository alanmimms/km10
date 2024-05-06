#include "km10.hpp"
#include <iostream>
#include <cassert>

using namespace std;


int main(int argc, char *argv[]) {
  assert(sizeof(W36) == 8);
  static array<W36, 4 * 1024 * 1024> memory;

  cerr << oct;
  W36 testY{}; testY.y = ~0ull; cerr << "testY:  " << setw(12) << testY.u << endl;
  W36 testX{}; testX.x = ~0ull; cerr << "testX:  " << setw(12) << testX.u << endl;
  W36 testI{}; testI.i = ~0ull; cerr << "testI:  " << setw(12) << testI.u << endl;
  W36 testAC{}; testAC.ac = ~0ull; cerr << "testAC: " << setw(12) << testAC.u << endl;
  W36 testOP{}; testOP.op = ~0ull; cerr << "testOP: " << setw(12) << testOP.u << endl;
  cerr << dec;

  if (argc != 2) {
    cerr << R"(
Usage:
    )" << argv[0] << R"( <filename to load>
)";
    return -1;
  }

  KM10 km10(4 * 1024 * 1024, 20);

  km10.loadA10(argv[1]);
  cerr << "[Loaded " << argv[1]
	    << "  start=" << km10.pc.fmtVMA()
	    << "]" << endl;

  km10.tracePC = 1;
  km10.traceAC = 1;
  km10.traceMem = 1;
  km10.running = 1;

  // Initially paging is OFF
  km10.tops20Paging = 0;
  km10.pagingEnabled = 0;

  km10.emulate();

  return 0;
}
