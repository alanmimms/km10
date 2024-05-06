#include "km10.hpp"
#include <iostream>


int main(int argc, char *argv[]) {
  static std::array<W36, 4 * 1024 * 1024> memory;

  if (argc != 2) {
    std::cerr << R"(
Usage:
    )" << argv[0] << R"( <filename to load>
)";
    return -1;
  }

  KM10 km10(4 * 1024 * 1024, 20);

  km10.loadA10(argv[1]);
  std::cerr << "[Loaded " << argv[1]
	    << "  start=" << km10.pc.fmtVMA()
	    << "]" << std::endl;

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
