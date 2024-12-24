// KL10 Priority Interrupts device.
//
// KL10/KI10 interrupt terminology (in modern terms):
//
// * "Active" means the interrupt is enabled.
//
// * "Held" means the interrupt handler is executing.
//
// * "Level" is the interrupt's priority, where numerically lower
//   levels are serviced ahead of numerically higher levels.
//
// * A "PR" is a Programmed Requested interrupt.
//
// Note: only higher priority levels can interrupt lower priority
// levels. An interrupt at a given level cannot be interrupted by a
// same or lower priority level interrupt.

// TODO XXX:
//
// * Force PRs even on disabled interrupt channels as stated in
//   1982_ProcRefMan.pdf p.208.

// NOTE From EK-EBOX-all-OCR2.pdf (PDF p.111): If the instruction at
// 40 + 2n is a BLKX instruction, a specified number of transfers are
// performed, one transfer at a time, each time returning to the
// interrupted program or to a higher level subroutine. On the last
// transfer, the return to the interrupted program is “NOT SKIPPED”
// and an instruction is fetched from 41 + 2n. In a similar fashion,
// if 40 + 2n contains a SKIP class instruction, when the skip
// condition is satisfied a return to the interrupted program takes
// place. If the skip is not satisfied, the instruction in 41 + 2n is
// executed instead of the return.


#include "word.hpp"
#include "device.hpp"
#include "pi.hpp"
#include "km10.hpp"
#include "iresult.hpp"


// Constructors
PIDevice::PIDevice(KM10 &cpu):
  Device(001, "PI", cpu)
{
  clearIO();
}


// Formatters
string PIDevice::levelsToStr(unsigned levels) {
  stringstream ss;
  ss << "{";

  char digit = '1';
  for (unsigned mask=0100; mask; mask >>= 1, ++digit) {
    if (levels & mask) ss << digit;
  }

  ss << "}";
  return ss.str();
}


// Configure `pc` to handle any pending interrupt by changing
// the instruction word about to be executed by KM10, or by doing
// nothing if there is no pending interrupt. Returns true if an
// interrupt is to be handled.
W36 PIDevice::setUpInterruptCycleIfPending() {
  if (!piState.piOn || piState.levelsOn == 0) return 0;

  // Find highest pending interrupt and its mask.
  unsigned highestLevel = 99;
  Device *highestDevP = nullptr;
  unsigned highestMask = 0;

  // Search device list for highest leveled device that has an
  // interrupt pending.
  for (auto [ioDev, devP]: Device::devices) {

    // We don't bother ordering the devices by physical ID as KL10
    // does. Instead, we just service the first one we find that has
    // an interrupt pending.
    if (devP->intPending) {
      const unsigned levelMask = 1 << (7 - devP->intLevel);

      if ((levelMask & piState.levelsOn) && devP->intLevel < highestLevel) {
	highestLevel = devP->intLevel;
	highestMask = levelMask;
	highestDevP = devP;
      }
    }
  }

  if (highestLevel < piState.currentLevel) {

    // We have a pending interrupt at `highestLevel` level that isn't being serviced.
    piState.held |= highestMask;		// Mark this level as held - i.e., ACTIVELY RUNNING.
    piState.currentLevel = highestLevel;	// Remember we are running at this level.

    // Ask device about its interrupt.
    auto [level, ifw] = highestDevP->getIntFuncWord();

    if (logger.ints) {
      logger.s << "<<<<INTERRUPT>>>> by " << highestDevP->name
	       << ": pc=" << km10.pc.fmtVMA()
	       << " level=" << level
	       << " ifw=" << ifw.fmt36()
	       << logger.endl << flush;
    }

    // Function word is saved here by KL10 microcode. Does anyone
    // look at this? Who knows?
    km10.ACBlocks[7][3] = ifw;

    switch (ifw.intFunction) {
    case W36::zeroIF:
    case W36::standardIF:
      return km10.eptAddressFor(&km10.eptP->pioInstructions[2*highestLevel]);

    default:
      if (logger.ints) logger.s << "PI got IFW from '" << highestDevP->name
				<< "' specifying function "
				<< (int) ifw.intFunction << ", which is not implemented yet."
				<< logger.endl;
      break;
    }
  }

  return 0;
}


// This ends interrupt service.
void PIDevice::dismissInterrupt() {
  if (logger.ints) logger.s << "PI dismiss current interrupt" << logger.endl << flush;
  piState.currentLevel = PIState::noLevel;
  piState.held = 0;
  km10.inInterrupt = false;
  if (logger.ints) logger.s << " <<< dismissInterrupt, end piState="
			    << W36(piState.u).fmt18() << logger.endl << flush;
}


// I/O instruction handlers
void PIDevice::clearIO() {
  // This is apparently only supposed to be a partial reset based on
  // DFKAA test at 064756.
  Device::clearIO();

  // Preserve the parts of piState that aren't cleared by clearIO().
  unsigned levelsOn = piState.levelsOn;
  unsigned piOn = piState.piOn;
  piState.u = 0;
  piState.currentLevel = PIState::noLevel;
  piState.levelsOn = levelsOn;
  piState.piOn = piOn;
}

IResult PIDevice::doCONO(W36 iw, W36 ea) {
  PIFunctions pif(ea.u);

  if (logger.mem) logger.s << "; " << ea.fmt18();

  if (logger.ints) logger.s << km10.pc.fmtVMA() << ": CONO PI,"
			    << pif.toString() << " ea=" << ea.fmt18() << logger.endl;

  if (pif.clearPI) {
    clearIO();
    piState.u = 0;		// Really clear
    piState.currentLevel = PIState::noLevel;
    km10.inInterrupt = false;
  } else {
    piState.writeEvenParityDir = pif.writeEvenParityDir;
    piState.writeEvenParityData = pif.writeEvenParityData;
    piState.writeEvenParityAddr = pif.writeEvenParityAddr;

    if (pif.turnPIOn)	     piState.piOn = 1;
    if (pif.turnPIOff)     piState.piOn = 0;
    if (pif.levelsTurnOff) piState.levelsOn &= ~pif.levels;
    if (pif.levelsTurnOn)  piState.levelsOn |= pif.levels;
    if (pif.dropPR)	     piState.prLevels &= ~pif.levels;

    if (pif.initiatePR) {

      if ((piState.prLevels & pif.levels) == 0) {
	// Find highest requested interrupt and its mask.
	unsigned levelMask = 0100;
	unsigned thisLevel = 1;
	for (; (pif.levels & levelMask) == 0; ++thisLevel, levelMask>>=1) ;

	if (levelMask == 0 || thisLevel < piState.currentLevel) intLevel = thisLevel;
      }

      piState.prLevels |= pif.levels;
      requestInterrupt();
      if (logger.ints) logger.s << " <<< CONO PI, has triggered an interrupt on level "
				<< intLevel << " >>>" << logger.endl << flush;
    } else {
      intPending = false;
    }
  }

  if (logger.ints) logger.s << " <<< CONO PI, end piState="
			    << W36(piState.u).fmt18() << logger.endl << flush;
  return iNormal;
}


IResult PIDevice::doCONI(W36 iw, W36 ea) {
  W36 conditions{(int64_t) piState.u};
  km10.memPut(conditions);
  return iNormal;
}
