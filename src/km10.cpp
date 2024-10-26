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


DEFINE_string(load, "../images/klad/dfkaa.a10", ".A10 or .SAV file to load");
DEFINE_bool(debug, false, "run the built-in debugger instead of starting execution");


//////////////////////////////////////////////////////////////
// This is invoked in a loop to allow the "restart" command to work
// properly. Therefore this needs to clean up the state of the machine
// before it returns. This is mostly done by auto destructors.
static int loopedMain(int argc, char *argv[]) {
  assert(sizeof(KMState::ExecutiveProcessTable) == 512 * 8);
  assert(sizeof(KMState::UserProcessTable) == 512 * 8);
  gflags::ParseCommandLineFlags(&argc, &argv, true);

  KMState state(4 * 1024 * 1024);

  if (FLAGS_load != "none") {

    if (FLAGS_load.ends_with(".a10")) {
      state.loadA10(FLAGS_load.c_str());
    } else if (FLAGS_load.ends_with(".sav")) {
      //      state.loadSAV(FLAGS_load.c_str());
    } else {
      cerr << "ERROR: '-load' option must name a .a10 or .sav file" << logger.endl;
      return -1;
    }

    cerr << "[Loaded " << FLAGS_load << "  start=" << state.pc.fmtVMA() << "]" << logger.endl;
  }

  state.maxInsns = 0;
  KM10 km10(state);

  if (FLAGS_debug) {
    Debugger dbg(km10, state);
    dbg.debug();
    return dbg.restart ? 1 : 0;
  } else {
    state.running = true;
    km10.emulate();
  }

  return 0;
}


////////////////////////////////////////////////////////////////
int main(int argc, char *argv[]) {
  int st;
  
  while ((st = loopedMain(argc, argv)) > 0) ;
  return st;
}


////////////////////////////////////////////////////////////////
void Logger::nyi(KMState &state, const string &context) {
  s << " [not yet implemented: " << context << "]";
  cerr << "Not yet implemented at " << state.pc.fmtVMA() << endl;
}


void Logger::nsd(KMState &state, const string &context) {
  s << " [no such device: " << context << "]";
  cerr << "No such device at " << state.pc.fmtVMA() << endl;
}
