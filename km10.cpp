#include <iostream>
#include <assert.h>
using namespace std;


#include "km10.hpp"

#include "logging.hpp"
#include "device.hpp"
#include "dte20.hpp"


Logging logging{};


int main(int argc, char *argv[]) {
  assert(sizeof(Memory::ExecutiveProcessTable) == 512 * 8);
  assert(sizeof(Memory::UserProcessTable) == 512 * 8);

  if (argc < 2) {
    cerr << R"(
Usage:
    )" << argv[0] << R"( <filename to load>
)";
    return -1;
  }

  logging.ac = true;
  logging.pc = true;
  logging.mem = true;
  //  logging.load = true;
  logging.maxInsns = 100*1000;

  Memory memory(4 * 1024 * 1024);

  DTE20 dte{040, "DTE", memory};
  Device::devices[040] = &dte;

  KM10 km10(memory);

  km10.loadA10(argv[1]);
  logging.s << "[Loaded " << argv[1]
	    << "  start=" << km10.pc.fmtVMA()
	    << "]" << endl;

  km10.running = true;
  km10.emulate();

  return 0;
}
