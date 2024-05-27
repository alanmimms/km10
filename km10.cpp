#include <iostream>
#include <assert.h>
using namespace std;

#include "cmdlime/commandlinereader.h"


#include "km10.hpp"

#include "logging.hpp"
#include "device.hpp"
#include "dte20.hpp"
#include "debugger.hpp"


Logging logging{};


struct CmdlimeConfig: cmdlime::Config {
  CMDLIME_PARAM(load, string)("../images/klddt/klddt.a10") << ".A10 file to load";
  CMDLIME_FLAG(debug) << "run the built-in debugger instead of starting execution";
};


int run(const CmdlimeConfig& cmd) {
  logging.ac = true;
  logging.pc = true;
  logging.mem = true;
  logging.maxInsns = 100*1000;

  KMState state(4 * 1024 * 1024);

  DTE20 dte{040, "DTE", state};
  Device::devices[040] = &dte;

  KM10 km10(state);

  km10.loadA10(cmd.load.c_str());
  cerr << "[Loaded " << cmd.load << "  start=" << state.pc.fmtVMA() << "]" << endl;

  if (cmd.debug) {
    Debugger dbg(state);
  } else {
    km10.running = true;
    km10.emulate();
  }

  return 0;
}


int main(int argc, char *argv[]) {
  assert(sizeof(KMState::ExecutiveProcessTable) == 512 * 8);
  assert(sizeof(KMState::UserProcessTable) == 512 * 8);

  auto cmdlineReader = cmdlime::CommandLineReader("km10");
  cmdlineReader.setVersionInfo("km10 0.1");
  return cmdlineReader.exec<CmdlimeConfig>(argc, argv, run);
}
