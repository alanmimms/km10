#include <iostream>
#include <assert.h>
using namespace std;

#include <gflags/gflags.h>

#include "word.hpp"
#include "kmstate.hpp"
#include "km10.hpp"
#include "debugger.hpp"

#include "logger.hpp"


Logger logger{};


DEFINE_string(load, "../images/klddt/klddt.a10", ".A10 file to load");
DEFINE_bool(debug, false, "run the build-in debugger instead of starting execution");


//////////////////////////////////////////////////////////////
int main(int argc, char *argv[]) {
  assert(sizeof(KMState::ExecutiveProcessTable) == 512 * 8);
  assert(sizeof(KMState::UserProcessTable) == 512 * 8);
  gflags::ParseCommandLineFlags(&argc, &argv, true);

  KMState state(4 * 1024 * 1024);

  if (FLAGS_load != "none") {
    state.loadA10(FLAGS_load.c_str());
    cerr << "[Loaded " << FLAGS_load << "  start=" << state.pc.fmtVMA() << "]" << logger.endl;
  }

  state.maxInsns = 0;
  KM10 km10(state);

  if (FLAGS_debug) {
    Debugger dbg(km10, state);
    dbg.debug();
  } else {
    state.running = true;
    km10.emulate();
  }

  return 0;
}


////////////////////////////////////////////////////////////////
void Logger::nyi(KMState &state) {
  s << " [not yet implemented]";
  cerr << "Not yet implemented at " << state.pc.fmtVMA() << endl;
}


void Logger::nsd(KMState &state) {
  s << " [no such device]";
  cerr << "No such device at " << state.pc.fmtVMA() << endl;
}
