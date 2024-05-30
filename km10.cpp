#include <iostream>
#include <assert.h>
using namespace std;

#include <gflags/gflags.h>

#include "kmstate.hpp"
#include "km10.hpp"
#include "debugger.hpp"

#include "logger.hpp"
#include "device.hpp"
#include "dte20.hpp"


Logger logger{};


DEFINE_string(load, "../images/klddt/klddt.a10", ".A10 file to load");
DEFINE_bool(debug, false, "run the build-in debugger instead of starting execution");


int main(int argc, char *argv[]) {
  assert(sizeof(KMState::ExecutiveProcessTable) == 512 * 8);
  assert(sizeof(KMState::UserProcessTable) == 512 * 8);
  gflags::ParseCommandLineFlags(&argc, &argv, true);

  KMState state(4 * 1024 * 1024);

  DTE20 dte{040, "DTE", state};
  Device::devices[040] = &dte;

  if (FLAGS_load != "none") {
    state.loadA10(FLAGS_load.c_str());
    cerr << "[Loaded " << FLAGS_load << "  start=" << state.pc.fmtVMA() << "]" << logger.endl;
  }

  state.maxInsns = 0;
  KM10 km10(state, &dte);

  if (FLAGS_debug) {
    Debugger dbg(km10, state);
    dbg.debug();
  } else {
    state.running = true;
    km10.emulate();
  }

  return 0;
}
