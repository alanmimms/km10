#pragma once
#include <iostream>
#include <fstream>
#include <limits>
#include <atomic>
#include <unordered_set>

#include <assert.h>
#include <sys/mman.h>
#include <signal.h>

using namespace std;

#include "word.hpp"
#include "logger.hpp"


struct KMState {
  KMState(unsigned nWords = 4 * 1024 * 1024)
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
      addressBPs{},
      executeBPs{}
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
  uint64_t nInsns;
  unordered_set<unsigned> addressBPs;
  unordered_set<unsigned> executeBPs;


  // Return the KM10 memory VIRTUAL address (EPT is in kernel virtual
  // space) for the specified pointer into the EPT.
  inline W36 eptAddressFor(const W36 *eptEntryP) {
    return W36(eptEntryP - (W36 *) eptP);
  }


  W36 acGetN(unsigned n) {
    assert(n < 16);
    W36 value = AC[n];
    if (logger.mem) logger.s << "; ac" << oct << n << ":" << value.fmt36();
    return value;
  }


  W36 acGetEA(unsigned n) {
    assert(n < 16);
    W36 value = AC[n];
    if (logger.mem) logger.s << "; ac" << oct << n << ":" << value.fmt36();
    return value;
  }


  void acPutN(W36 value, unsigned n) {
    assert(n < 16);
    AC[n] = value;
    if (logger.mem) logger.s << "; ac" << oct << n << "=" << value.fmt36();
  }

  W36 memGetN(W36 a) {
    W36 value = a.rhu < 020 ? acGetEA(a.rhu) : memP[a.rhu];
    if (logger.mem) logger.s << "; " << a.fmtVMA() << ":" << value.fmt36();
    if (addressBPs.contains(a.vma)) running = false;
    return value;
  }

  void memPutN(W36 value, W36 a) {

    if (a.rhu < 020)
      acPutN(value, a.rhu);
    else 
      memP[a.rhu] = value;

    if (logger.mem) logger.s << "; " << a.fmtVMA() << "=" << value.fmt36();
    if (addressBPs.contains(a.vma)) running = false;
  }


  // Effective address calculation
  uint64_t getEA(unsigned i, unsigned x, uint64_t y) {

    // While we keep getting indirection, loop for new EA words.
    // XXX this only works for non-extended addressing.
    for (;;) {

      // XXX there are some significant open questions about how much
      // the address wraps and how much is included in these addition
      // and indirection steps. For now, this can work for section 0
      // or unextended code.

      if (x != 0) {
	if (logger.ea) logger.s << "EA (" << oct << x << ")=" << acGetN(x).fmt36() << logger.endl;
	y += acGetN(x).u;
      }

      if (i != 0) {		// Indirection
	W36 eaw(memGetN(y));
	if (logger.ea) logger.s << "EA @" << W36(y).fmt36() << "=" << eaw.fmt36() << logger.endl;
	y = eaw.y;
	x = eaw.x;
	i = eaw.i;
      } else {			// No indexing or indirection
	if (logger.ea) logger.s << "EA=" << W36(y).fmt36() << logger.endl;
	return y;
      }
    }
  }


  // Accessors
  bool userMode() {return !!flags.usr;}

  W36 flagsWord(unsigned pc) {
    W36 v(pc);
    v.pcFlags = flags.u;
    return v;
  }


  // Used by JRSTF and JEN
  void restoreFlags(W36 ea) {
    ProgramFlags newFlags{(unsigned) ea.pcFlags};

    // User mode cannot clear USR. User mode cannot set UIO.
    if (flags.usr) {
      newFlags.uio = 0;
      newFlags.usr = 1;
    }

    // A program running in PUB mode cannot clear PUB mode.
    if (flags.pub) newFlags.pub = 1;

    flags.u = newFlags.u;
  }


  // Loaders
  /*
    PDP-10 ASCIIZED FILE FORMAT
    ---------------------------

    PDP-10 ASCIIZED FILES ARE COMPOSED OF THREE TYPES OF
    FILE LOAD LINES.  THEY ARE:

    A.      CORE ZERO LINE

    THIS LOAD FILE LINE SPECIFIES WHERE AND HOW MUCH PDP-10 CORE
    TO BE ZEROED.  THIS IS NECESSARY AS THE PDP-10 FILES ARE
    ZERO COMPRESSED WHICH MEANS THAT ZERO WORDS ARE NOT INCLUDED
    IN THE LOAD FILE TO CONSERVE FILE SPACE.

    CORE ZERO LINE

    Z WC,ADR,COUNT,...,CKSUM

    Z = PDP-10 CORE ZERO
    WORD COUNT = 1 TO 4
    ADR = ZERO START ADDRESS
    DERIVED FROM C(JOBSA)
    COUNT = ZERO COUNT, 64K MAX
    DERIVED FROM C(JOBFF)

    IF THE ADDRESSES ARE GREATER THAN 64K THE HI 2-BITS OF
    THE 18 BIT PDP-10 ADDRESS ARE INCLUDED AS THE HI-BYTE OF
    THE WORD COUNT.

    B.      LOAD FILE LINES

    AS MANY OF THESE TYPES OF LOAD FILE LINES ARE REQUIRED AS ARE
    NECESSARY TO REPRESENT THE BINARY SAVE FILE.

    LOAD FILE LINE

    T WC,ADR,DATA 20-35,DATA 4-19,DATA 0-3, - - - ,CKSUM

    T = PDP-10 TYPE FILE
    WC = PDP-10 DATA WORD COUNT TIMES 3, 3 PDP-11 WORDS
    PER PDP-10 WORD.
    ADR = PDP-10 ADDRESS FOR THIS LOAD FILE LINE
    LOW 16 BITS OF THE PDP-10 18 BIT ADDRESS, IF
    THE ADDRESS IS GREATER THAN 64K, THE HI 2-BITS
    OF THE ADDRESS ARE INCLUDED AS THE HI-BYTE OF
    THE WORD COUNT.

    UP TO 8 PDP-10 WORDS, OR UP TO 24 PDP-11 WORDS

    DATA 20-35
    DATA  4-19      ;PDP-10 EQUIV DATA WORD BITS
    DATA  0-3

    CKSUM = 16 BIT NEGATED CHECKSUM OF WC, ADR & DATA

    C.      TRANSFER LINE

    THIS LOAD FILE LINE CONTAINS THE FILE STARTING ADDRESS.

    TRANSFER LINE

    T 0,ADR,CKSUM

    0 = WC = SIGNIFIES TRANSFER, EOF
    ADR = PROGRAM START ADDRESS

  */


  // This takes a "word" from the comma-delimited A10 format and
  // converts it from its ASCIIized form into an 16-bit integer value.
  // On entry, inS must be at the first character of a token. On exit,
  // inS is at the start of the next token or else the NUL at the end
  // of the string.
  //
  // Example:
  //     |<---- inS is at the 'A' on entry
  //     |   |<---- and at the 'E' four chars later at exit.
  // T ^,AEh,E,LF@,E,O?m,FC,E,Aru,Lj@,F,AEv,F@@,E,,AJB,L,AnT,F@@,E,Arz,Lk@,F,AEw,F@@,E,E,ND@,K,B,NJ@,E,B`K

  static auto getWord(ifstream &inS, [[maybe_unused]] const char *whyP) -> uint16_t {
    unsigned v = 0;

    for (;;) {
      char ch = inS.get();
      if (logger.load) logger.s << "getWord[" << whyP << "] ch=" << oct << ch << logger.endl;
      if (ch == EOF || ch == ',' || ch == '\n') break;
      v = (v << 6) | (ch & 077);
    }

    if (logger.load) logger.s << "getWord[" << whyP << "] returns 0" << oct << v << logger.endl;
    return v;
  }


  // Load the specified .A10 format file into memory.
  void loadA10(const char *fileNameP) {
    ifstream inS(fileNameP);
    unsigned addr = 0;
    unsigned highestAddr = 0;
    unsigned lowestAddr = 0777777;

    for (;;) {
      char recType = inS.get();

      if (recType == EOF) break;

      if (logger.load) logger.s << "recType=" << recType << logger.endl;

      if (recType == ';') {
	// Just ignore comment lines
	inS.ignore(numeric_limits<streamsize>::max(), '\n');
	continue;
      }

      // Skip the blank after the record type
      inS.get();

      // Count of words on this line.
      uint16_t wc = getWord(inS, "wc");

      addr = getWord(inS, "addr");
      addr |= wc & 0xC000;
      wc &= ~0xC000;

      if (logger.load) logger.s << "addr=" << setw(6) << setfill('0') << oct << addr << logger.endl;
      if (logger.load) logger.s << "wc=" << wc << logger.endl;

      unsigned zeroCount;

      switch (recType) {
      case 'Z':
	zeroCount = getWord(inS, "zeroCount");

	if (zeroCount == 0) zeroCount = 64*1024;

	if (logger.load) logger.s << "zeroCount=0" << oct << zeroCount << logger.endl;

	inS.ignore(numeric_limits<streamsize>::max(), '\n');

	for (unsigned offset = 0; offset < zeroCount; ++offset) {
	  unsigned a = addr + offset;

	  if (a > highestAddr) highestAddr = a;
	  if (a < lowestAddr) lowestAddr = a;
	  memP[a].u = 0;
	}

	break;

      case 'T':
	if (wc == 0) {pc.lhu = 0; pc.rhu = addr;}

	for (unsigned offset = 0; offset < wc/3; ++offset) {
	  uint64_t w0 = getWord(inS, "w0");
	  uint64_t w1 = getWord(inS, "w1");
	  uint64_t w2 = getWord(inS, "w2");
	  uint64_t w = ((w2 & 0x0F) << 32) | (w1 << 16) | w0;
	  uint64_t a = addr + offset;
	  W36 w36(w);
	  W36 a36(a);

	  if (a > highestAddr) highestAddr = a;
	  if (a < lowestAddr) lowestAddr = a;

	  if (logger.load) {
	    logger.s << "mem[" << a36.fmtVMA() << "]=" << w36.fmt36() << " " << w36.disasm()
		     << logger.endl;
	  }

	  memP[a].u = w;
	}

	inS.ignore(numeric_limits<streamsize>::max(), '\n');
	break;
      
      default:
	logger.s << "ERROR: Unknown record type '" << recType << "' in file '" << fileNameP << "'"
		 << logger.endl;
	break;      
      }
    }
  }
};
