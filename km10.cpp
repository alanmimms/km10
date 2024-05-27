#include <iostream>
#include <assert.h>
using namespace std;

#include <gflags/gflags.h>

#include "km10.hpp"

#include "logging.hpp"
#include "device.hpp"
#include "dte20.hpp"
#include "debugger.hpp"


Logging logging{};


DEFINE_string(load, "../images/klddt/klddt.a10", ".A10 file to load");
DEFINE_bool(debug, false, "run the build-in debugger instead of starting execution");


int main(int argc, char *argv[]) {
  assert(sizeof(KMState::ExecutiveProcessTable) == 512 * 8);
  assert(sizeof(KMState::UserProcessTable) == 512 * 8);
  gflags::ParseCommandLineFlags(&argc, &argv, true);

  logging.ac = true;
  logging.pc = true;
  logging.mem = true;
  logging.maxInsns = 100*1000;

  KMState state(4 * 1024 * 1024);

  DTE20 dte{040, "DTE", state};
  Device::devices[040] = &dte;

  KM10 km10(state);

  km10.loadA10(FLAGS_load.c_str());
  cerr << "[Loaded " << FLAGS_load << "  start=" << state.pc.fmtVMA() << "]" << endl;

  if (FLAGS_debug) {
    Debugger dbg(state);
  } else {
    km10.running = true;
    km10.emulate();
  }

  return 0;
}
