#pragma once
#include <iostream>
#include <fstream>
#include <limits>
#include <atomic>
#include <unordered_set>
#include <sys/mman.h>
#include <assert.h>

using namespace std;

#include "word.hpp"
#include "logger.hpp"


struct KMState {
  KMState(unsigned nWords, unordered_set<unsigned> &aBPs, unordered_set<unsigned> &eBPs)
    : running(false),
      restart(false),
      nextPC(0),
      exceptionPC(0),
      pc(0),
      ACbanks{},
      flags(0u),
      inInterrupt(false),
      era(0u),
      AC(ACbanks[0]),
      memorySize(nWords),
      nSteps(0),
      inXCT(false),
      addressBPs(aBPs),
      executeBPs(eBPs)
  {
    // Note this anonymous mmap() implicitly zeroes the virtual memory.
    physicalP = (W36 *) mmap(nullptr,
			     memorySize * sizeof(uint64_t),
			     PROT_READ | PROT_WRITE,
			     MAP_PRIVATE | MAP_ANONYMOUS,
			     0, 0);
    assert(physicalP != nullptr);

    // Initially we have no virtual addressing, so virtual == physical.
    memP = physicalP;
    eptP = (ExecutiveProcessTable *) memP;

    // Initially, we have no user mode mapping.
    uptP = nullptr;
  }

  ~KMState() {
    if (physicalP) munmap(physicalP, memorySize * sizeof(uint64_t));
  }
  
  // Pointer to physical memory.
  W36 *physicalP;

  // Pointer to current virtual memory mapping.
  W36 *memP;

  // Pointer to kernel mode virtual memory mapping.
  W36 *kernelMemP;

  // Pointer to user mode virtual memory mapping.
  W36 *userMemP;
  
  // The "RUN flop"
  volatile atomic<bool> running;

  // The "REBOOT flop"
  bool restart;

  // PC of instruction to execute AFTER current one.
  W36 nextPC;

  // PC of instruction that was interrupted or caused a trap.
  W36 exceptionPC;

  // The processor's program counter.
  W36 pc;

  // KL10 has 8 banks of 16 ACs.
  W36 ACbanks[8][16];

  union ATTRPACKED ProgramFlags {

    // 13-bit field LEFTMOST-ALIGNED in the LH.
    struct ATTRPACKED {
      unsigned ndv: 1;

      unsigned fuf: 1;
      unsigned tr1: 1;
      unsigned tr2: 1;

      unsigned afi: 1;
      unsigned pub: 1;
      unsigned uio: 1;

      unsigned usr: 1;
      unsigned fpd: 1;
      unsigned fov: 1;

      unsigned cy1: 1;
      unsigned cy0: 1;
      unsigned ov: 1;
    };

    // Two flags have different name/usage, depending on context.
    struct ATTRPACKED {
      unsigned: 6;
      unsigned pcu: 1;
      unsigned: 5;
      unsigned pcp: 1;
    };

    unsigned u: 13;

    ProgramFlags(unsigned newFlags) {
      u = newFlags;
    }

    string toString() {
      ostringstream ss;
      ss << oct << setw(6) << setfill('0') << ((unsigned) u << 5);
      if (ndv) ss << " NDV";
      if (fuf) ss << " FUF";
      if (tr1) ss << " TR1";
      if (tr2) ss << " TR2";
      if (afi) ss << " AFI";
      if (pub) ss << " PUB";
      if (uio) ss << (usr ? " UIO" : " PCU");
      if (usr) ss << " USR";
      if (fpd) ss << " FPD";
      if (fov) ss << " FOV";
      if (cy1) ss << " CY1";
      if (cy0) ss << " CY0";
      if (ov ) ss << " OV";
      return ss.str();
    }
  } flags;

  union FlagsDWord {
    struct ATTRPACKED {
      unsigned processorDependent: 18; // What does KL10 use here?
      unsigned: 1;
      unsigned flags: 18;
      unsigned pc: 30;
      unsigned: 6;
    };

    uint64_t u: 36;
  };

  // True if we are running in an interrupt state or trap state.
  bool inInterrupt;

  W36 era;

  // See 1982_ProcRefMan.pdf p.230
  struct ExecutiveProcessTable {

    struct {
      W36 initialCommand;
      W36 statusWord;
      W36 lastUpdatedCommand;
      W36 reserved;
    } channelLogout[8];

    W36 pioInstructions[16];	// 040
    W36 channelBlockFill[4];	// 060

    W36 reserved64_137[44];
  
    // See 1982_ProcRefMan.pdf p.262
    struct DTEControlBlock {
      W36 to11BP;
      W36 to10BP;
      W36 vectorInsn;
      W36 reserved;
      W36 examineAreaSize;
      W36 examineAreaReloc;
      W36 depositAreaSize;
      W36 depositAreaReloc;
    } dte[4];			// 140

    W36 reserved200_420[145];

    W36 trap1Insn;		// 421
    W36 stackOverflowInsn;	// 422
    W36 trap3Insn;		// 423 (not used in KL10?)

    W36 reserved424_443[16];

    W36 DTEMonitorOpComplete;	// 444
    W36 reserved445_447[3];
    W36 DTEto10Arg;		// 450
    W36 DTEto11Arg;		// 451
    W36 reserved452;
    W36 DTEOpInProgress;	// 453
    W36 reserved454;
    W36 DTEOutputDone;		// 455
    W36 DTEKLNotReadyForChar;	// 456

    W36 reserved457_507[25];

    W36 timeBase[2];		// 510
    W36 performanceCount[2];	// 512
    W36 intervalCounterIntInsn;	// 514

    W36 reserved515_537[19];

    W36 execSection[32];	// 540

    W36 reserved600_777[128];	// 600
  } *eptP;


  struct UserProcessTable {
    W36 reserved000_417[0420];	// 000
    W36 luuoAddr;		// 420
    W36 trap1Insn;		// 421
    W36 stackOverflowInsn;	// 422
    W36 trap3Insn;		// 423 (not used in KL10?)
    W36 muuoFlagsOpAC;		// 424
    W36 muuoOldPC;		// 425
    W36 muuoE;			// 426
    W36 muuoContext;		// 427
    W36 kernelNoTrapMUUOPC;	// 430
    W36 kernelTrapMUUOPC;	// 431
    W36 supvNoTrapMUUOPC;	// 432
    W36 supvTrapMUUOPC;		// 433
    W36 concNoTrapMUUOPC;	// 434
    W36 concTrapMUUOPC;		// 435
    W36 publNoTrapMUUOPC;	// 436
    W36 publTrapMUUOPC;		// 437

    W36 reserved440_477[32];

    W36 pfWord;			// 500
    W36 pfFlags;		// 501
    W36 pfOldPC;		// 502
    W36 pfNewPC;		// 503

    W36 userExecTime[2];	// 504
    W36 userMemRefCount[2];	// 506

    W36 reserved510_537[24];

    W36 userSection[32];	// 540

    W36 reserved600_777[128];	// 600
  } *uptP;


  W36 *AC;
  unsigned memorySize;
  uint64_t nSteps;
  bool inXCT;
  uint64_t nInsns;
  unordered_set<unsigned> &addressBPs;
  unordered_set<unsigned> &executeBPs;


  // Return the KM10 memory VIRTUAL address (EPT is in kernel virtual
  // space) for the specified pointer into the EPT.
  W36 eptAddressFor(const W36 *eptEntryP);

  // AC and memory accessors.
  W36 acGetN(unsigned n);
  W36 acGetEA(unsigned n);
  void acPutN(W36 value, unsigned n);
  W36 memGetN(W36 a);
  void memPutN(W36 value, W36 a);

  // Effective address calculation.
  uint64_t getEA(unsigned i, unsigned x, uint64_t y);

  // Accessors
  bool userMode();
  W36 flagsWord(unsigned pc);

  // Used by JRSTF and JEN
  void restoreFlags(W36 ea);

  void loadA10(const char *fileNameP);

protected:
  uint16_t getWord(ifstream &inS, [[maybe_unused]] const char *whyP);

};
