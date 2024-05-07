// This is the KM10 CPU implementation.
#pragma once
#include <string>
#include <cstdint>
#include <sstream>
#include <iomanip>
#include <fstream>
#include <iostream>
#include <limits>
#include <functional>

using namespace std;

#include "w36.hpp"


class KM10 {
public:
  W36 AC[16];
  W36 pc;

  union Flags {
    struct ATTRPACKED {
      unsigned: 23;
      
      unsigned ndv;
      unsigned fuf;
      unsigned tr1;
      unsigned tr2;
      unsigned afi;
      unsigned pub;
      unsigned pcu;
      unsigned usrIO;
      unsigned usr;
      unsigned fpd;
      unsigned fov;
      unsigned cy0;
      unsigned cy1;
      unsigned pcp;
      unsigned ov;
    };

    uint64_t u: 36;
  } flags;

  // Pointer to current virtual memory mapping in emulator.
  W36 *memP;

  bool tops20Paging;
  bool pagingEnabled;

  unsigned intLevel;
  unsigned intLevelsPending;
  unsigned intLevelsRequested;
  unsigned intLevelsInProgress;
  unsigned intLevelsEnabled;
  bool piEnabled;

  bool running;
  bool tracePC;
  bool traceAC;
  bool traceMem;

  uint64_t maxInsns;

  inline static bool loadLog{true};
  inline static ofstream loadLogS{"load.log"};


  // Constructors
  KM10(W36 *physicalMemoryP, uint64_t aMaxInsns = UINT64_MAX)
    : AC(),
      pc(0, 0),
      memP(physicalMemoryP),
      maxInsns(aMaxInsns)
  {
  }


  // See 1982_ProcRefMan.pdf p.262
  struct DTE20ControlBlock {
    W36 to11BP;
    W36 to10BP;
    W36 vectorInsn;
    W36 reserved;
    W36 examineAreaSize;
    W36 examineAreaReloc;
    W36 depositAreaSize;
    W36 depositAreaReloc;
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
  
    DTE20ControlBlock dte20[4];	// 140

    W36 reserved200_420[145];

    W36 trap1Insn;		// 421
    W36 stackOverflowInsn;	// 422
    W36 trap3Insn;		// 423 (not used in KL10?)

    W36 reserved424_507[52];

    W36 timeBase[2];		// 510
    W36 performanceCount[2];	// 512
    W36 intervalCounterIntInsn;	// 514

    W36 reserved515_537[19];

    W36 execSection[32];	// 540

    W36 reserved600_777[128];	// 600
  };


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
  };


  void emulate() {
    W36 iw{};
    W36 ea{};
    W36 nextPC = pc;

    uint64_t nInsns = 0;

    auto memGet = [&]() -> W36 {
      W36 value = memP[ea.u];;
      if (traceMem) cerr << " ; " << ea.fmtVMA() << ": " << value.fmt36();
      return value;
    };

    auto memPut = [&](W36 value) -> void {
      memP[ea.u] = value;
      if (traceMem) cerr << " ; " << ea.fmtVMA() << "<-" << value.fmt36();
    };

    auto acGet = [&]() -> W36 {
      W36 value = AC[iw.ac];
      if (traceMem) cerr << " ; ac" << oct << iw.ac << ": " << value.fmt36();
      return value;
    };

    auto acPut = [&](W36 value) -> void {
      AC[iw.ac] = value;
      if (traceMem) cerr << " ; ac" << oct << iw.ac << "<-" << value.fmt36();
    };

    // Condition testing predicates
    function<bool(W36)> isLT 	= [&](W36 v) -> bool {return v.s <  0;};
    function<bool(W36)> isLE 	= [&](W36 v) -> bool {return v.s <= 0;};
    function<bool(W36)> isGT 	= [&](W36 v) -> bool {return v.s >  0;};
    function<bool(W36)> isGE 	= [&](W36 v) -> bool {return v.s >= 0;};
    function<bool(W36)> isNE 	= [&](W36 v) -> bool {return v.s != 0;};
    function<bool(W36)> isEQ 	= [&](W36 v) -> bool {return v.s == 0;};
    function<bool(W36)> always = [&](W36 v) -> bool {return true;};
    function<bool(W36)> never  = [&](W36 v) -> bool {return false;};

    auto doSKIP = [&](function<bool(W36)> &condF) -> void {
      W36 eaw = memGet();

      if (condF(eaw)) {
	if (traceMem) cerr << " [skip]";
	++nextPC.rhu;
      }
      
      if (iw.ac != 0) acPut(eaw);
    };

    do {

      if (nInsns++ > maxInsns) running = false;

      if ((flags.tr1 || flags.tr2) && pagingEnabled) {
	ExecutiveProcessTable *eptP = (ExecutiveProcessTable *) memP;
	iw = flags.tr1 ? eptP->trap1Insn : eptP->stackOverflowInsn;
      } else {
	iw = memP[pc.vma];
      }

      nextPC.lhu = pc.lhu;
      nextPC.rhu = pc.rhu + 1;

    XCT_ENTRYPOINT:
      W36 eaw{iw};
      bool eaIsLocal = 1;

      // While we keep getting indirection, loop for new EA words.
      // XXX this only works for non-extended addressing.
      for (;;) {
	ea.y = eaw.y;		// Initial assumption

	if (eaw.x != 0) ea.rhu += AC[eaw.x];

	if (eaw.i != 0) {	// Indirection
	  eaw = memP[ea.y];
	} else {		// No indexing or indirection
	  break;
	}
      }

      if (tracePC) {
	cerr << pc.fmtVMA()
	     << " " << iw.fmt36()
	     << ": [ea=" << ea.fmtVMA() << "] "
	     << setw(20) << left << iw.disasm();
      }

      switch (iw.op) {

      case 0330:		// SKIP
	doSKIP(never);
	break;

      case 0331:		// SKIPL
	doSKIP(isLT);
	break;

      case 0332:		// SKIPE
	doSKIP(isEQ);
	break;

      case 0333:		// SKIPLE
	doSKIP(isLE);
	break;

      case 0334:		// SKIPA
	doSKIP(always);
	break;

      case 0335:		// SKIPGE
	doSKIP(isGE);
	break;

      case 0336:		// SKIPN
	doSKIP(isNE);
	break;

      case 0337:		// SKIPGT
	doSKIP(isGT);
	break;

      case 0254:		// JRST
	nextPC.u = ea.u;
	break;

      case 0264:		// JSR
	memPut(pc.isSection0() ? nextPC.u | flags.u : nextPC.vma);
	nextPC.u = ea.u + 1;	// XXX Wrap?
	flags.fpd = flags.afi = flags.tr2 = flags.tr1 = 0;
	break;

      case 0265:		// JSP
	acPut(pc.isSection0() ? nextPC.u | flags.u : nextPC.vma);
	nextPC.u = ea.u + 1;	// XXX Wrap?
	flags.fpd = flags.afi = flags.tr2 = flags.tr1 = 0;
	break;

      default:
	break;
      }

      pc = nextPC;
      if (tracePC) cerr << endl;
    } while (running);
  }


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
      if (loadLog) loadLogS << "getWord[" << whyP << "] ch=" << oct << ch << endl;
      if (ch == EOF || ch == ',' || ch == '\n') break;
      v = (v << 6) | (ch & 077);
    }

    if (loadLog) loadLogS << "getWord[" << whyP << "] returns 0" << oct << v << endl;
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

      if (loadLog) loadLogS << "recType=" << recType << endl;

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

      if (loadLog) loadLogS << "addr=" << setw(6) << setfill('0') << oct << addr << endl;
      if (loadLog) loadLogS << "wc=" << wc << endl;

      unsigned zeroCount;

      switch (recType) {
      case 'Z':
	zeroCount = getWord(inS, "zeroCount");

	if (zeroCount == 0) zeroCount = 64*1024;

	if (loadLog) loadLogS << "zeroCount=0" << oct << zeroCount << endl;

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

	  if (loadLog) {
	    loadLogS << "mem[" << a36.fmtVMA() << "]=" << w36.fmt36() << " " << w36.disasm() << endl;
	  }

	  memP[a].u = w;
	}

	inS.ignore(numeric_limits<streamsize>::max(), '\n');
	break;
      
      default:
	cerr << "ERROR: Unknown record type '" << recType << "' in file '" << fileNameP << "'" << endl;
	break;      
      }
    }
  }
};
