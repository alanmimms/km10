// KL10 Priority Interrupts device.
//
// KL10/KI10 interrupt terminology (in modern terms):
//
// * "Active" means the interrupt is enabled.
//
// * "Held" means the interrupt handler is executing.
//
// * "Level" is the interrupt's priority, where lower numbered levels
//   are serviced ahead of higher numbered levels.
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


#pragma once

#include "word.hpp"
#include "device.hpp"


struct PIDevice: Device {

  // PI CONO function bits
  union ATTRPACKED PIFunctions {

    struct ATTRPACKED {
      unsigned levels: 7;

      unsigned turnPIOn: 1;
      unsigned turnPIOff: 1;

      unsigned levelsTurnOff: 1;
      unsigned levelsTurnOn: 1;
      unsigned initiatePR: 1;
      
      unsigned clearPI: 1;
      unsigned dropPR: 1;
      unsigned: 1;

      unsigned writeEvenParityDir: 1;
      unsigned writeEvenParityData: 1;
      unsigned writeEvenParityAddr: 1;
    };

    unsigned u: 18;

    // Constructors
    PIFunctions(unsigned v=0) : u(v) {}

    // Accessors
    string toString() const {
      stringstream ss;
      ss << " levels=" << levelsToStr(levels);
      if (turnPIOn) ss << " turnPIOn";
      if (turnPIOff) ss << " turnPIOff";
      if (levelsTurnOff) ss << " levelsTurnOff";
      if (levelsTurnOn) ss << " levelsTurnOn";
      if (initiatePR) ss << " initiatePR";
      if (clearPI) ss << " clearPI";
      if (dropPR) ss << " dropPR";
      if (writeEvenParityDir) ss << " writeEvenParityDir";
      if (writeEvenParityData) ss << " writeEvenParityData";
      if (writeEvenParityAddr) ss << " writeEvenParityAddr";
      return ss.str();
    }
  };

  // PI state and CONI bits
  union ATTRPACKED PIState {

    struct ATTRPACKED {
      unsigned levelsOn: 7;
      unsigned piOn: 1;
      unsigned held: 7;
      unsigned writeEvenParityDir: 1;
      unsigned writeEvenParityData: 1;
      unsigned writeEvenParityAddr: 1;
      unsigned prLevels: 7;
      unsigned: 7;

      // When no interrupt is being handled, this is `noLevel`. When
      // an interrupt appears with a larger unsigned value of level,
      // it indicates the interrupt is less important than
      // `currentLevel` and therefore must wait. This is specific to
      // KM10.
      unsigned currentLevel: 4;
    };

    // The `u` bits are just the CONI bits. We initialize and manage
    // `currentLevel` separately.
    struct ATTRPACKED {
      unsigned u: 25;
    };

    // This is the "lowest" level. This is the `currentLevel` value
    // when no interrupts are being serviced.
    static const inline unsigned noLevel = 8;


    // Constructors
    PIState() {
      u = 0;
      currentLevel = noLevel;
    }

    // Accessors
    string toString() const {
      stringstream ss;
      ss << " levelsOn=" << levelsToStr(levelsOn);
      if (piOn) ss << " piOn";
      ss << " held=" << levelsToStr(held);
      if (writeEvenParityDir) ss << " writeEvenParityDir";
      if (writeEvenParityData) ss << " writeEvenParityData";
      if (writeEvenParityAddr) ss << " writeEvenParityAddr";
      ss << " prLevels=" << levelsToStr(prLevels);
      ss << " currentLevel=";

      if (currentLevel == PIState::noLevel)
	ss << "none";
      else
	ss << currentLevel;

      return ss.str();
    }
  } piState;


  // Constructors
  PIDevice(KMState &aState):
    Device(001, "PI", aState)
  {
    clearIO();
  }


  // Formatters
  static inline string levelsToStr(unsigned levels) {
    stringstream ss;
    ss << "{";

    char digit = '1';
    for (unsigned mask=0100; mask; mask >>= 1, ++digit) {
      if (levels & mask) ss << digit;
    }

    ss << "}";
    return ss.str();
  }


  // Configure `state.pc` to handle any pending interrupt by changing
  // the instruction word about to be executed by KM10, or by doing
  // nothing if there is no pending interrupt. Returns true if an
  // interrupt is to be handled.
  W36 setUpInterruptCycleIfPending() {
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
		 << ": pc=" << state.pc.fmtVMA()
		 << " level=" << level
		 << " ifw=" << ifw.fmt36()
		 << logger.endl << flush;
      }

      // Function word is saved here by KL10 microcode. Does anyone
      // look at this? Who knows?
      state.ACbanks[7][3] = ifw;

      switch (ifw.intFunction) {
      case W36::zeroIF:
      case W36::standardIF:
	return state.eptAddressFor(&state.eptP->pioInstructions[2*highestLevel]);

      default:
	cerr << "PI got IFW from '" << highestDevP->name << "' specifying function "
	     << (int) ifw.intFunction << ", which is not implemented yet."
	     << logger.endl;
	break;
      }
    }

    return 0;
  }


  // This ends interrupt service.
  void dismissInterrupt() {
    if (logger.ints) logger.s << "PI dismiss current interrupt" << logger.endl << flush;
    piState.currentLevel = PIState::noLevel;
    piState.held = 0;
    cerr << " <<< dismissInterrupt, end piState=" << W36(piState.u).fmt18() << logger.endl << flush;
  }


  // I/O instruction handlers
  void clearIO() override {
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

  void doCONO(W36 iw, W36 ea) override {
    PIFunctions pif(ea.u);

    if (logger.mem) logger.s << "; " << ea.fmt18();

    cerr << state.pc.fmtVMA() << ": CONO PI,"
	 << pif.toString() << " ea=" << ea.fmt18() << logger.endl;

    if (pif.clearPI) {
      clearIO();
      piState.u = 0;		// Really clear
      piState.currentLevel = PIState::noLevel;
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
	cerr << " <<< CONO PI, has triggered an interrupt on level "
	     << intLevel << " >>>" << logger.endl << flush;
      } else {
	intPending = false;
      }
    }

    cerr << " <<< CONO PI, end piState=" << W36(piState.u).fmt18() << logger.endl << flush;
  }

  virtual W36 doCONI(W36 iw, W36 ea) override {
    W36 conditions{(int64_t) piState.u};
    state.memPutN(conditions, ea);
    return conditions;
  }
};
