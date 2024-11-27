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
  PIDevice(KM10 *aCPU);


  // Formatters
  static string levelsToStr(unsigned levels);

  // Configure `pc` to handle any pending interrupt by changing
  // the instruction word about to be executed by KM10, or by doing
  // nothing if there is no pending interrupt. Returns true if an
  // interrupt is to be handled.
  W36 setUpInterruptCycleIfPending();

  // This ends interrupt service.
  void dismissInterrupt();

  // I/O instruction handlers
  virtual void clearIO() override;
  virtual void doCONO(W36 iw, W36 ea) override;
  virtual W36 doCONI(W36 iw, W36 ea) override;
};
