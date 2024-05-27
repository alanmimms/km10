#pragma once
#include <assert.h>
#include <sys/mman.h>

using namespace std;

#include "w36.hpp"


struct KMState {
  KMState(unsigned nWords) {
    physicalP = (W36 *) mmap(nullptr,
			     nWords * sizeof(uint64_t),
			     PROT_READ | PROT_WRITE,
			     MAP_SHARED | MAP_ANONYMOUS,
			     0, 0);
    assert(physicalP != nullptr);

    // Initially we have no virtual addressing, so virtual == physical.
    memP = physicalP;
    eptP = (ExecutiveProcessTable *) memP;

    // Initially, we have no user mode mapping.
    uptP = nullptr;
  }
  
  // Pointer to physical memory.
  W36 *physicalP;

  // Pointer to current virtual memory mapping.
  W36 *memP;

  // Pointer to kernel mode virtual memory mapping.
  W36 *kernelMemP;

  // Pointer to user mode virtual memory mapping.
  W36 *userMemP;
  

  W36 pc;

  union ATTRPACKED ProgramFlags {

    struct ATTRPACKED {
      unsigned: 5;
      unsigned ndv: 1;
      unsigned fuf: 1;
      unsigned tr1: 1;
      unsigned tr2: 1;
      unsigned afi: 1;
      unsigned pub: 1;
      unsigned usrIO: 1;
      unsigned usr: 1;
      unsigned fpd: 1;
      unsigned fov: 1;
      unsigned cy1: 1;
      unsigned cy0: 1;
      unsigned ov: 1;
    };

    struct ATTRPACKED {
      unsigned: 5;
      unsigned: 1;
      unsigned: 1;
      unsigned: 1;
      unsigned: 1;
      unsigned: 1;
      unsigned: 1;
      unsigned pcu: 1;
      unsigned: 1;
      unsigned: 1;
      unsigned: 1;
      unsigned: 1;
      unsigned: 1;
      unsigned pcp: 1;
    };

    unsigned u: 18;
  } flags;


  union FlagsDWord {
    struct ATTRPACKED {
      unsigned processorDependent: 18; // What does KL10 use here?
      unsigned: 1;
      ProgramFlags flags;
      unsigned pc: 30;
      unsigned: 6;
    };

    uint64_t u: 36;
  };


  // See 1982_ProcRefMan.pdf p.230
  struct ExecutiveProcessTable {

    struct {
      W36 initialCommand;
      W36 statusWord;
      W36 lastUpdatedCommand;
      W36 reserved;
    } channelLogout[8];

    W36 reserved40_41[2];	// 040

    W36 pioInstructions[14];

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

  W36 acGetN(unsigned n) {
    W36 value = AC[n];
    if (logging.mem) logging.s << " ; ac" << oct << n << ": " << value.fmt36();
    return value;
  }


  W36 acGetEA(unsigned ac) {
    W36 value = AC[ac];
    if (logging.mem) logging.s << " ; ac" << oct << ac << ": " << value.fmt36();
    return value;
  }


  void acPutN(W36 value, unsigned acN) {
    AC[acN] = value;
    if (logging.mem) logging.s << " ; ac" << oct << acN << "<-" << value.fmt36();
  }

  W36 memGetN(W36 a) {
    W36 value = a.u < 020 ? acGetEA(a.u) : memP[a.u];
    if (logging.mem) logging.s << " ; " << a.fmtVMA() << ": " << value.fmt36();
    return value;
  }

  void memPutN(W36 value, W36 a) {

    if (a.u < 020)
      acPutN(value, a.u);
    else 
      memP[a.u] = value;

    if (logging.mem) logging.s << " ; " << a.fmtVMA() << "<-" << value.fmt36();
  }


  // Effective address calculation
  unsigned getEA(unsigned i, unsigned x, unsigned y) {
    W36 ea(0);
    W36 eaw(0);
    eaw.i = i;
    eaw.x = x;
    eaw.y = y;

    // While we keep getting indirection, loop for new EA words.
    // XXX this only works for non-extended addressing.
    for (;;) {
      ea.y = eaw.y;	// Initial assumption

      if (eaw.x != 0) ea.rhu += acGetN(eaw.x);

      if (eaw.i != 0) {	// Indirection
	eaw = memGetN(ea.y);
      } else {		// No indexing or indirection
	return ea.u;
      }
    }
  }


  // Accessors
  bool userMode() {return !!flags.usr;}

  W36 flagsWord(unsigned pc) {
    return W36(flags.u, pc);
  }
};
