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

// We keep these breakpoint sets outside of the looped main so they
// stick across restart.
static unordered_set<unsigned> aBPs;
static unordered_set<unsigned> eBPs;


// Definitions for our command line options
DEFINE_string(load, "../images/klad/dfkaa.a10", ".A10 or .SAV file to load");
DEFINE_string(seq, "../images/klad/dfkaa.seq", ".SEQ file to load symbols from");
DEFINE_bool(debug, false, "run the built-in debugger instead of starting execution");


////////////////////////////////////////////////////////////////
void KM10::emulate(Debugger *debuggerP) {
  W36 exceptionPC{0};

  ////////////////////////////////////////////////////////////////
  // Connect our DTE20 (put console into raw mode)
  dte.connect();

  // The instruction loop
  for (;;) {
    // Keep the cache sweep timer ticking until it goes DING.
    cca.handleSweep();

    // Handle execution (PC) breakpoints
    if (state.executeBPs.contains(state.pc.vma)) state.running = false;

    // Prepare to fetch next iw and remember if it's an interrupt or
    // trap.
    if ((state.flags.tr1 || state.flags.tr2) && pag.pagerEnabled()) {
      // We have a trap.
      state.exceptionPC = state.pc;
      state.pc = state.eptAddressFor(state.flags.tr1 ?
				     &state.eptP->trap1Insn :
				     &state.eptP->stackOverflowInsn);
      state.inInterrupt = true;
      cerr << ">>>>> trap cycle PC now=" << state.pc.fmtVMA()
	   << "  exceptionPC=" << state.exceptionPC.fmtVMA()
	   << logger.endl << flush;
    } else if (W36 vector = pi.setUpInterruptCycleIfPending(); vector != W36(0)) {
      // We have an active interrupt.
      state.exceptionPC = state.pc;
      state.pc = vector;
      state.inInterrupt = true;
      cerr << ">>>>> interrupt cycle PC now=" << state.pc.fmtVMA()
	   << "  exceptionPC=" << state.exceptionPC.fmtVMA()
	   << logger.endl << flush;
    } else {
      state.inInterrupt = false;
    }

    // Now fetch the instruction at our normal, exception, or interrupt PC.
    iw = state.memGetN(state.pc);

    // Capture next PC AFTER we possibly set up to handle an exception or interrupt.
    if (!state.inXCT) {
      state.nextPC.rhu = state.pc.rhu + 1;
      state.nextPC.lhu = state.pc.lhu;
    }

    // If we're debugging, this is where we pause to let the user
    // inspect and change things. The debugger tells us what our next
    // action should be based on its return value.
    if (!state.running) {

      switch (debuggerP->debug()) {
      case Debugger::step:	// Debugger has set step count in state.nSteps.
	break;

      case Debugger::run:	// Continue from current PC (state.nSteps set by debugger to 0).
	break;

      case Debugger::quit:	// Quit from emulator.
	return;

      case Debugger::restart:	// Restart emulator like a PDP10 reboot
	return;

      case Debugger::pcChanged:	// PC changed by debugger - go fetch again
	continue;

      default:			// This should never happen...
	assert("Debugger returned unknown action" == nullptr);
	return;
      }
    }

    // Handle nSteps so we don't keep running if we run out of step
    // count. THIS instruction is our single remaining step. If
    // state.nSteps is zero we just keep going "forever".
    if (state.nSteps > 0) {
      if (--state.nSteps <= 0) state.running = false;
    }

    if (logger.loggingToFile && logger.pc) logger.s << state.pc.fmtVMA() << ": " << iw.dump();

    // Unless we encounter ANOTHER XCT we're not in one now.
    state.inXCT = false;

    // Execute the instruction in `iw`. If `execute1()` returns true,
    // we're executing an XCT instruction, so don't update PC. It was
    // already set to the XCT instruction's EA.
    if (!execute1()) {
      state.pc = state.nextPC;
    }

    if (logger.pc || logger.mem || logger.ac || logger.io || logger.dte)
      logger.s << logger.endl << flush;
  }

  // Restore console to normal
  dte.disconnect();
}


//////////////////////////////////////////////////////////////
// This is invoked in a loop to allow the "restart" command to work
// properly. Therefore this needs to clean up the state of the machine
// before it returns. This is mostly done by auto destructors.
static int loopedMain(int argc, char *argv[]) {
  assert(sizeof(KMState::ExecutiveProcessTable) == 512 * 8);
  assert(sizeof(KMState::UserProcessTable) == 512 * 8);
  gflags::ParseCommandLineFlags(&argc, &argv, true);

  KMState state(4 * 1024 * 1024, aBPs, eBPs);

  if (FLAGS_load != "none") {

    if (FLAGS_load.ends_with(".a10")) {
      state.loadA10(FLAGS_load.c_str());
    } else if (FLAGS_load.ends_with(".sav")) {
      cerr << "ERROR: For now, '-load' option must name a .a10 file" << logger.endl;
      return -1;
      //      state.loadSAV(FLAGS_load.c_str());
    } else {
      cerr << "ERROR: '-load' option must name a .a10 or .sav file" << logger.endl;
      return -1;
    }

    cerr << "[Loaded " << FLAGS_load << "  start=" << state.pc.fmtVMA() << "]" << logger.endl;
  }

  KM10 km10(state);
  Debugger debugger(km10, state);

  if (FLAGS_seq != "none") {
    debugger.loadSEQ(FLAGS_seq.c_str());
  }

  state.running = !FLAGS_debug;
  km10.emulate(&debugger);

  return state.restart ? 1 : 0;
}


////////////////////////////////////////////////////////////////
int main(int argc, char *argv[]) {
  int st;
  
  while ((st = loopedMain(argc, argv)) > 0) {
    cerr << endl << "[restarting]" << endl;
  }

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
