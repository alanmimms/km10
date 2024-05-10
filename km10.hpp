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

  union ProgramFlags {

    struct ATTRPACKED {
      unsigned ndv: 1;
      unsigned fuf: 1;
      unsigned tr1: 1;
      unsigned tr2: 1;
      unsigned afi: 1;
      unsigned pub: 1;
      unsigned pcu: 1;
      unsigned usrIO: 1;
      unsigned usr: 1;
      unsigned fpd: 1;
      unsigned fov: 1;
      unsigned cy0: 1;
      unsigned cy1: 1;
      unsigned ov: 1;
    };

    unsigned u: 13;
  } flags;

  // Pointer to current virtual memory mapping in emulator.
  W36 *memP;

  bool running;
  bool tracePC;
  bool traceAC;
  bool traceMem;

  uint64_t maxInsns;


  // CONO PAG state bits
  union PAGState {

    struct ATTRPACKED {
      unsigned execBasePage: 13;
      unsigned enablePager: 1;
      unsigned tops2Paging: 1;
      unsigned: 1;
      unsigned cacheStrategyLoad: 1;
      unsigned cacheStrategyLook: 1;
    };

    unsigned u: 18;

    PAGState() :u(0) {};
  } pagState;


  // CONI APR interrupt flags
  union APRFlags {

    struct ATTRPACKED {
      unsigned sweepDone: 1;
      unsigned powerFailure: 1;
      unsigned addrParity: 1;
      unsigned cacheDirParity: 1;
      unsigned mbParity: 1;
      unsigned ioPageFail: 1;
      unsigned noMemory: 1;
      unsigned sbusError: 1;
    };

    unsigned u: 8;
  };
  
  // CONI APR status bits (some used in CONO APR)
  union APRState {

    struct ATTRPACKED {
      unsigned intLevel: 3;
      unsigned intRequest: 1;

      APRFlags active;
      unsigned: 4;

      unsigned sweepBusy: 1;
      unsigned: 5;

      APRFlags enabled;

      unsigned: 6;
    };

    uint64_t u: 36;
  } aprState;


  // CONO APR function bits
  union APRFunctions {

    struct ATTRPACKED {
      unsigned intLevel: 3;
      unsigned: 1;

      APRFlags select;

      unsigned set: 1;
      unsigned clear: 1;
      unsigned disable: 1;
      unsigned enable: 1;

      unsigned clearIO: 1;
      unsigned: 1;
    };

    unsigned u: 18;

    APRFunctions(unsigned v) :u(v) {};
  };

  struct APRLevels {
    unsigned sweepDone: 3;
    unsigned powerFailure: 3;
    unsigned addrParity: 3;
    unsigned cacheDirParity: 3;
    unsigned mbParity: 3;
    unsigned ioPageFail: 3;
    unsigned noMemory: 3;
    unsigned sbusError: 3;
  } aprLevels;

  // PI CONO function bits
  union PIFunctions {
    PIFunctions(unsigned v) :u(v) {};

    struct ATTRPACKED {
      unsigned levels: 7;

      unsigned turnPIOn: 1;
      unsigned turnPIOff: 1;

      unsigned levelsOff: 1;
      unsigned levelsOn: 1;
      unsigned levelsInitiate: 1;
      
      unsigned clearPI: 1;
      unsigned dropRequests: 1;
      unsigned: 1;

      unsigned writeEvenParityDir: 1;
      unsigned writeEvenParityData: 1;
      unsigned writeEvenParityAddr: 1;
    };

    unsigned u: 18;
  };

  // PI state and CONI bits
  union PIState {

    struct ATTRPACKED {
      unsigned levelsEnabled: 7;
      unsigned piEnabled: 1;
      unsigned intInProgress: 7;
      unsigned writeEvenParityDir: 1;
      unsigned writeEvenParityData: 1;
      unsigned writeEvenParityAddr: 1;
      unsigned levelsRequested: 7;
      unsigned: 11;
    };

    uint64_t u: 36;

    PIState() :u(0) {};
  } piState;


  // APRID value see 1982_ProcRefMan.pdf p.244
  inline static const union {

    struct ATTRPACKED {
      unsigned serialNumber: 12;

      unsigned: 1;
      unsigned masterOscillator: 1;
      unsigned extendedKL10: 1;
      unsigned channelSupported: 1;
      unsigned cacheSupported: 1;
      unsigned AC50Hz: 1;

      unsigned microcodeVersion: 9;
      unsigned: 6;
      unsigned exoticMicrocode: 1;
      unsigned extendedAddressing: 1;
      unsigned tops20Paging: 1;
    };

    uint64_t u: 36;
  } aprIDValue = {
    04321,			// Processor serial number
    1,				// Master oscillator
    1,				// Extended KL10
    1,				// Channel support
    0,				// No cache support
    0,				// 60Hz,
    0442,			// Microcode version number
    0,				// "Exotic" microcode
    1,				// Microcode handles extended addresses
    1,				// TOPS-20 paging
  };

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


  union FlagsDWord {
    struct ATTRPACKED {
      unsigned processorDependent: 18; // What does KL10 use here?
      unsigned: 6;
      ProgramFlags flags;
      unsigned pc: 30;
      unsigned: 6;
    };

    uint64_t u: 36;
  };

  // Constructors
  KM10(W36 *physicalMemoryP, uint64_t aMaxInsns = UINT64_MAX)
    : memP(physicalMemoryP),
      maxInsns(aMaxInsns)
  { }


  // Accessors
  bool userMode() {return false;}

  W36 flagsWord() {
    return W36((unsigned) flags.u << 4, pc.rhu);
  }

  // Logging
  inline static bool loadLog{true};
  inline static ofstream loadLogS{"load.log"};


  // The instruction emulator
  void emulate() {
    W36 iw{};
    W36 ea{};
    W36 nextPC = pc;
    W36 tmp;

    uint64_t nInsns = 0;

    auto nyi = [&] {cerr << " [not yet implemented]";};

    function<W36(W36)> swap = [&](W36 src) {return W36(src.rhu, src.lhu);};

    function<W36(W36)> negate = [&](W36 src) {
      W36 v(-src.s);
      if (src.u == W36::mostNegative) flags.tr1 = flags.ov = flags.cy1 = 1;
      if (src.u == 0) flags.cy0 = flags.cy1 = 1;
      return v;
    };

    function<W36(W36)> magnitude = [&](W36 src) {
      W36 v(src.s < 0 ? -src.s : src.s);
      if (src.u == W36::mostNegative) flags.tr1 = flags.ov = flags.cy1 = 1;
      return v;
    };

    function<W36(unsigned)> acGetN = [&](unsigned acN) -> W36 {
      W36 value = AC[acN];
      if (traceMem) cerr << " ; ac" << oct << acN << ": " << value.fmt36();
      return value;
    };

    function<W36()> acGet = [&]() -> W36 {
      return acGetN(iw.ac);
    };

    function<W36(unsigned)> acGetEA = [&](unsigned ac) -> W36 {
      W36 value = AC[ac];
      if (traceMem) cerr << " ; ac" << oct << ac << ": " << value.fmt36();
      return value;
    };

    function<W36()> acGetRH = [&]() -> W36 {
      W36 value{0, AC[iw.ac].rhu};
      if (traceMem) cerr << " ; acRH" << oct << iw.ac << ": " << value.fmt36();
      return value;
    };

    function<W36()> acGetLH = [&]() -> W36 {
      W36 value{0, AC[iw.ac].lhu};
      if (traceMem) cerr << " ; acLH" << oct << iw.ac << ": " << value.fmt36();
      return value;
    };

    function<void(W36,unsigned)> acPutN = [&](W36 value, unsigned acN) -> void {
      AC[acN] = value;
      if (traceMem) cerr << " ; ac" << oct << acN << "<-" << value.fmt36();
    };

    function<void(W36)> acPut = [&](W36 value) -> void {
      acPutN(value, iw.ac);
    };

    function<W36()> memGet = [&]() -> W36 {
      W36 value = ea.u < 020 ? acGetEA(ea.u) : memP[ea.u];
      if (traceMem) cerr << " ; " << ea.fmtVMA() << ": " << value.fmt36();
      return value;
    };

    function<W36(W36)> memGetN = [&](W36 a) -> W36 {
      W36 value = a.u < 020 ? acGetEA(a.u) : memP[a.u];
      if (traceMem) cerr << " ; " << a.fmtVMA() << ": " << value.fmt36();
      return value;
    };

    function<W36()> memGetSwapped = [&]() -> W36 {return swap(memGet());};

    function<void(W36,W36)> memPutN = [&](W36 value, W36 a) -> void {

      if (a.u < 020)
	acPut(value);
      else 
	memP[a.u] = value;

      if (traceMem) cerr << " ; " << a.fmtVMA() << "<-" << value.fmt36();
    };

    function<void(W36)> memPut = [&](W36 value) -> void {
      memPutN(value, ea);
    };

    function<void(W36)> doPush = [&](W36 v) -> void {
      W36 ac = acGet();

      if (pc.isSection0() || ac.lhs < 0 || (ac.lhu & 0007777) == 0) {
	ac = W36(ac.lhu + 1, ac.rhu + 1);

	if (ac.lhu == 0)
	  flags.tr2 = 1;
	else			// Correct? Don't access memory for full stack?
	  memPutN(v, ac.rhu);
      } else {
	ac = ac + 1;
	memPutN(ac.vma, v);
      }

      acPut(ac);
    };

    function<W36()> doPop = [&] {
      W36 ac = acGet();
      W36 poppedWord;

      if (pc.isSection0() || ac.lhs < 0 || (ac.lhu & 0007777) == 0) {
	poppedWord = memGetN(ac.rhu);
	ac = W36(ac.lhu - 1, ac.rhu - 1);
	if (ac.lhs == -1) flags.tr2 = 1;
      } else {
	poppedWord = memGetN(ac.vma);
	ac = ac - 1;
      }

      acPut(ac);
      return poppedWord;
    };

    function<void(W36)> selfPut = [&](W36 value) -> void {
      memPut(value);
      if (iw.ac != 0) acPut(value);
    };

    function<void(W36)> bothPut = [&](W36 value) -> void {
      acPut(value);
      memPut(value);
    };

    function<W36()> immediate = [&]() -> W36 {return W36(pc.isSection0() ? 0 : ea.lhu, ea.rhu);};

    // Condition testing predicates
    function<bool(W36)> isLT0  = [&](W36 v) -> bool const {return v.s <  0;};
    function<bool(W36)> isLE0  = [&](W36 v) -> bool const {return v.s <= 0;};
    function<bool(W36)> isGT0  = [&](W36 v) -> bool const {return v.s >  0;};
    function<bool(W36)> isGE0  = [&](W36 v) -> bool const {return v.s >= 0;};
    function<bool(W36)> isNE0  = [&](W36 v) -> bool const {return v.s != 0;};
    function<bool(W36)> isEQ0  = [&](W36 v) -> bool const {return v.s == 0;};
    function<bool(W36)> always = [&](W36 v) -> bool const {return true;};
    function<bool(W36)> never  = [&](W36 v) -> bool const {return false;};

    auto doJUMP = [&](function<bool(W36)> &condF) -> void {
      W36 eaw = memGet();

      if (condF(eaw)) {
	if (traceMem) cerr << " [jump]";
	nextPC.rhu = ea;
      }
    };

    auto doSKIP = [&](function<bool(W36)> &condF) -> void {
      W36 eaw = memGet();

      if (condF(eaw)) {
	if (traceMem) cerr << " [skip]";
	++nextPC.rhu;
      }
      
      if (iw.ac != 0) acPut(eaw);
    };

    function<W36(W36)> noModification = [&](W36 fromSrc) -> auto const {return fromSrc;};
    function<W36(W36)> zeroMask = [&](W36 fromSrc) -> auto const {return fromSrc.u & ~(uint64_t) ea.rhu;};
    function<W36(W36)> onesMask = [&](W36 fromSrc) -> auto const {return fromSrc.u | ea.rhu;};
    function<W36(W36)> compMask = [&](W36 fromSrc) -> auto const {return fromSrc.u ^ ea.rhu;};

    function<W36(W36)> zeroWord = [&](W36 fromSrc) -> auto const {return 0;};
    function<W36(W36)> onesWord = [&](W36 fromSrc) -> auto const {return W36::allOnes;};
    function<W36(W36)> compWord = [&](W36 fromSrc) -> auto const {return ~fromSrc.u;};

    function<void(W36)> noStore = [](W36 toSrc) -> void {};

    auto doTXXXX = [&](function<W36()> &doGetF,
		       function<W36(W36)> &doModifyF,
		       function<bool(W36)> &condF,
		       function<void(W36)> &doStoreF) -> void
    {
      W36 eaw = doGetF() & ea;

      if (condF(eaw)) {
	if (traceMem) cerr << " [skip]";
	++nextPC.rhu;
      }
      
      doStoreF(doModifyF(eaw));
    };

    function<unsigned(unsigned)> extnOf = [&](unsigned v) -> auto const {
      return (v & 04000000) ? W36::halfOnes : 0;
    };

    // doCopyF functions
    function<W36(W36,W36)> copyHRR = [&](W36 src, W36 dst) -> auto const {return W36(dst.lhu, src.rhu);};
    function<W36(W36,W36)> copyHRL = [&](W36 src, W36 dst) -> auto const {return W36(src.rhu, dst.rhu);};
    function<W36(W36,W36)> copyHLL = [&](W36 src, W36 dst) -> auto const {return W36(src.lhu, dst.rhu);};
    function<W36(W36,W36)> copyHLR = [&](W36 src, W36 dst) -> auto const {return W36(dst.rhu, src.lhu);};

    // doModifyF functions
    function<W36(W36)> zeroR = [&](W36 v) -> auto const {return W36(v.lhu, 0);};
    function<W36(W36)> onesR = [&](W36 v) -> auto const {return W36(v.lhu, W36::halfOnes);};
    function<W36(W36)> extnR = [&](W36 v) -> auto const {return W36(v.lhu, extnOf(v.lhu));};
    function<W36(W36)> zeroL = [&](W36 v) -> auto const {return W36(0, v.rhu);};
    function<W36(W36)> onesL = [&](W36 v) -> auto const {return W36(W36::halfOnes, v.rhu);};
    function<W36(W36)> extnL = [&](W36 v) -> auto const {return W36(extnOf(v.rhu), v.rhu);};

    // binary doModifyF functions
    function<W36(W36,W36)> andWord = [&](W36 s1, W36 s2) -> auto const {return s1.u & s2.u;};
    function<W36(W36,W36)> andCWord = [&](W36 s1, W36 s2) -> auto const {return s1.u & ~s2.u;};
    function<W36(W36,W36)> andCBWord = [&](W36 s1, W36 s2) -> auto const {return ~s1.u & ~s2.u;};
    function<W36(W36,W36)> iorWord = [&](W36 s1, W36 s2) -> auto const {return s1.u | s2.u;};
    function<W36(W36,W36)> iorCWord = [&](W36 s1, W36 s2) -> auto const {return s1.u | ~s2.u;};
    function<W36(W36,W36)> iorCBWord = [&](W36 s1, W36 s2) -> auto const {return ~s1.u | ~s2.u;};
    function<W36(W36,W36)> xorWord = [&](W36 s1, W36 s2) -> auto const {return s1.u ^ s2.u;};
    function<W36(W36,W36)> xorCWord = [&](W36 s1, W36 s2) -> auto const {return s1.u ^ ~s2.u;};
    function<W36(W36,W36)> xorCBWord = [&](W36 s1, W36 s2) -> auto const {return ~s1.u ^ ~s2.u;};
    function<W36(W36,W36)> eqvWord = [&](W36 s1, W36 s2) -> auto const {return ~(s1.u ^ s2.u);};
    function<W36(W36,W36)> eqvCWord = [&](W36 s1, W36 s2) -> auto const {return ~(s1.u ^ ~s2.u);};
    function<W36(W36,W36)> eqvCBWord = [&](W36 s1, W36 s2) -> auto const {return ~(~s1.u ^ ~s2.u);};
    
    auto doHXXXX = [&](function<W36()> &doGetSrcF,
		       function<W36()> &doGetDstF,
		       function<W36(W36,W36)> &doCopyF,
		       function<W36(W36)> &doModifyF,
		       function<void(W36)> &doPutDstF) -> void
    {
      doPutDstF(doModifyF(doCopyF(doGetSrcF(), doGetDstF())));
    };


    auto doMOVXX = [&](function<W36()> &doGetSrcF,
		       function<W36(W36)> &doModifyF,
		       function<void(W36)> &doPutDstF) -> void
    {
      doPutDstF(doModifyF(doGetSrcF()));
    };


    auto doSETXX = [&](function<W36()> &doGetSrcF,
		       function<W36(W36)> &doModifyF,
		       function<void(W36)> &doPutDstF) -> void
    {
      doPutDstF(doModifyF(doGetSrcF()));
    };


    auto doBinOpXX = [&](function<W36()> &doGetSrc1F,
			 function<W36()> &doGetSrc2F,
			 function<W36(W36,W36)> &doModifyF,
			 function<void(W36)> &doPutDstF) -> void
    {
      doPutDstF(doModifyF(doGetSrc1F(), doGetSrc2F()));
    };


    // Binary comparison predicates
    function<bool(W36,W36)> isLT    = [&](W36 v1, W36 v2) -> bool const {return v1.s <  v2.s;};
    function<bool(W36,W36)> isLE    = [&](W36 v1, W36 v2) -> bool const {return v1.s <= v2.s;};
    function<bool(W36,W36)> isGT    = [&](W36 v1, W36 v2) -> bool const {return v1.s >  v2.s;};
    function<bool(W36,W36)> isGE    = [&](W36 v1, W36 v2) -> bool const {return v1.s >= v2.s;};
    function<bool(W36,W36)> isNE    = [&](W36 v1, W36 v2) -> bool const {return v1.s != v2.s;};
    function<bool(W36,W36)> isEQ    = [&](W36 v1, W36 v2) -> bool const {return v1.s == v2.s;};
    function<bool(W36,W36)> always2 = [&](W36 v1, W36 v2) -> bool const {return true;};
    function<bool(W36,W36)> never2  = [&](W36 v1, W36 v2) -> bool const {return false;};

    auto doCAXXX = [&](function<W36()> &doGetSrc1F,
		       function<W36()> &doGetSrc2F,
		       function<bool(W36,W36)> &condF) -> void
    {

      if (condF(doGetSrc1F(), doGetSrc2F())) {
	if (traceMem) cerr << " [skip]";
	++nextPC.rhu;
      }
    };


    function<void()> skipAction = [&] {++nextPC.u;};
    function<void()> jumpAction = [&] {nextPC.u = ea;};

    auto doAOSOXX = [&](function<W36()> &doGetF,
			const signed delta,
			function<void(W36)> &doPutF,
			function<bool(W36)> &condF,
			function<void()> &actionF) -> void
    {
      W36 v = doGetF();

      if (delta > 0) {		// Increment

	if (v.u == W36::allOnes >> 1) {
	  flags.tr1 = flags.ov  = flags.cy1 = 1;
	} else if (v.s == -1) {
	  flags.cy0 = flags.cy1 = 1;
	}
      } else {			// Decrement

	if (v.u == W36::mostNegative) {
	  flags.tr1 = flags.ov = flags.cy0 = 1;
	} else if (v.u != 0) {
	  flags.cy0 = flags.cy1 = 1;
	}
      }

      v.s += delta;
      doPutF(v);

      if (condF(v)) actionF();
    };


    // The instruction loop
    do {
      if (nInsns++ > maxInsns) running = false;

      if ((flags.tr1 || flags.tr2) && pagState.enablePager) {
	ExecutiveProcessTable *eptP = (ExecutiveProcessTable *) memP;
	iw = flags.tr1 ? eptP->trap1Insn : eptP->stackOverflowInsn;
      } else {
	iw = memP[pc.vma];
      }

    XCT_ENTRYPOINT:
      W36 eaw{iw};

      nextPC.lhu = pc.lhu;
      nextPC.rhu = pc.rhu + 1;

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
//	     << ": [ea=" << ea.fmtVMA() << "]  "
	     << "  " << setw(20) << left << iw.disasm();
      }

      switch (iw.op) {

      case 0200:		// MOVE
	doMOVXX(memGet, noModification, acPut);
	break;

      case 0201:		// MOVEI
	doMOVXX(immediate, noModification, acPut);
	break;

      case 0202:		// MOVEM
	doMOVXX(acGet, noModification, memPut);
	break;

      case 0203:		// MOVES
	doMOVXX(acGet, noModification, selfPut);
	break;

      case 0204:		// MOVS
	doMOVXX(memGet, swap, acPut);
	break;

      case 0205:		// MOVSI
	doMOVXX(immediate, swap, acPut);
	break;

      case 0206:		// MOVSM
	doMOVXX(acGet, swap, memPut);
	break;

      case 0207:		// MOVSS
	doMOVXX(acGet, swap, selfPut);
	break;

      case 0210:		// MOVN
	doMOVXX(memGet, negate, acPut);
	break;

      case 0211:		// MOVNI
	doMOVXX(immediate, negate, acPut);
	break;

      case 0212:		// MOVNM
	doMOVXX(acGet, negate, memPut);
	break;

      case 0213:		// MOVNS
	doMOVXX(acGet, negate, selfPut);
	break;

      case 0214:		// MOVM
	doMOVXX(memGet, magnitude, acPut);
	break;

      case 0215:		// MOVMI
	doMOVXX(immediate, magnitude, acPut);
	break;

      case 0216:		// MOVMM
	doMOVXX(acGet, magnitude, memPut);
	break;

      case 0217:		// MOVMS
	doMOVXX(acGet, magnitude, selfPut);
	break;

      case 0243: 		// JFFO
	tmp = acGet();

	if (tmp != 0) {
	  unsigned count = 0;

	  while (tmp.s >= 0) {
	    ++count;
	    tmp.u <<= 1;
	  }

	  tmp.u = count;
	}

	acPutN(tmp, iw.ac + 1);
	break;

      case 0250:		// EXCH
	tmp = acGet();
	acPut(memGet());
	memPut(tmp);
	break;

      case 0251: {		// BLT
	W36 ac(acGet());
	bool saveTraceMem = traceMem;

	traceMem = false;

	do {

	  // Note this isn't bug-for-bug compatible with KL10. See
	  // footnote [2] in 1982_ProcRefMan.pdf p.58. We do
	  // wraparound.
	  memPutN(memGetN(W36(ea.lhu, ac.lhu)), W36(ea.lhu, ac.rhu));
	  ac = W36(ac.lhu + 1, ac.rhu + 1);

	  // Put it back for traps or page faults.
	  acPut(ac);
	} while (ac.rhu < ea.rhu);

	traceMem = saveTraceMem;
	break;
      }

      case 0252:		// AOBJP
	tmp = acGet();
	tmp = W36(tmp.lhu + 1, tmp.rhu + 1);
	acPut(tmp);

	if (tmp.s >= 0) {
	  if (traceMem) cerr << " [jump]";
	  nextPC = ea;
	}

	break;

      case 0253:		// AOBJN
	tmp = acGet();
	tmp = W36(tmp.lhu + 1, tmp.rhu + 1);
	acPut(tmp);

	if (tmp.s < 0) {
	  if (traceMem) cerr << " [jump]";
	  nextPC = ea;
	}

	break;

      case 0254:		// JRST family

	if (iw.ac == 0) {	// JRST
	  nextPC.u = ea.u;
	} else {
	  nyi();
	}

	break;

      case 0255:		// JFCL
	if ((iw.ac & 8) && flags.ov)  {flags.ov = 0; nextPC = ea;}
	if ((iw.ac & 4) && flags.cy0) {flags.cy0 = 0; nextPC = ea;}
	if ((iw.ac & 2) && flags.cy1) {flags.cy1 = 0; nextPC = ea;}
	if ((iw.ac & 1) && flags.fov) {flags.fov = 0; nextPC = ea;}
	break;

      case 0256:		// XCT/PXCT

	if (userMode() || iw.ac == 0) {
	  iw = eaw;
	  goto XCT_ENTRYPOINT;
	} else {
	  nyi();
	  break;
	}

      case 0260:		// PUSHJ
	// Note this sets the flags that are cleared by PUSHJ before
	// doPush() since doPush() can set flags.tr2.
	flags.fpd = flags.afi = flags.tr1 = flags.tr2 = 0;
	doPush(flagsWord());
	nextPC = ea;
	break;

      case 0261:		// PUSH
	doPush(memGet());
	break;

      case 0262:		// POP
	memPut(doPop());
	break;

      case 0263:		// POPJ
	nextPC.rhu = doPop().rhu;
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

      case 0266:		// JSA
	memPut(acGet());
	acPut(W36(ea.lhu, pc.rhu));
	pc = ea.u + 1;
	break;

      case 0267:		// JRA
	acPut(memGetN(acGet()));
	pc = ea;
	break;

      case 0300:		// CAI
	doCAXXX(acGet, immediate, never2);
	break;

      case 0301:		// CAIL
	doCAXXX(acGet, immediate, isLT);
	break;

      case 0302:		// CAIE
	doCAXXX(acGet, immediate, isEQ);
	break;

      case 0303:		// CAILE
	doCAXXX(acGet, immediate, isLE);
	break;

      case 0304:		// CAIA
	doCAXXX(acGet, immediate, always2);
	break;

      case 0305:		// CAIGE
	doCAXXX(acGet, immediate, isGE);
	break;

      case 0306:		// CAIN
	doCAXXX(acGet, immediate, isNE);
	break;

      case 0307:		// CAIG
	doCAXXX(acGet, immediate, isGT);
	break;

      case 0310:		// CAM
	doCAXXX(acGet, memGet, never2);
	break;

      case 0311:		// CAML
	doCAXXX(acGet, memGet, isLT);
	break;

      case 0312:		// CAME
	doCAXXX(acGet, memGet, isEQ);
	break;

      case 0313:		// CAMLE
	doCAXXX(acGet, memGet, isLE);
	break;

      case 0314:		// CAMA
	doCAXXX(acGet, memGet, always2);
	break;

      case 0315:		// CAMGE
	doCAXXX(acGet, memGet, isGE);
	break;

      case 0316:		// CAMN
	doCAXXX(acGet, memGet, isNE);
	break;

      case 0317:		// CAMG
	doCAXXX(acGet, memGet, isGT);
	break;

      case 0320:		// JUMP
	doJUMP(never);
	break;

      case 0321:		// JUMPL
	doJUMP(isLT0);
	break;

      case 0322:		// JUMPE
	doJUMP(isEQ0);
	break;

      case 0323:		// JUMPLE
	doJUMP(isLE0);
	break;

      case 0324:		// JUMPA
	doJUMP(always);
	break;

      case 0325:		// JUMPGE
	doJUMP(isGE0);
	break;

      case 0326:		// JUMPN
	doJUMP(isNE0);
	break;

      case 0327:		// JUMPG
	doJUMP(isGT0);
	break;

      case 0330:		// SKIP
	doSKIP(never);
	break;

      case 0331:		// SKIPL
	doSKIP(isLT0);
	break;

      case 0332:		// SKIPE
	doSKIP(isEQ0);
	break;

      case 0333:		// SKIPLE
	doSKIP(isLE0);
	break;

      case 0334:		// SKIPA
	doSKIP(always);
	break;

      case 0335:		// SKIPGE
	doSKIP(isGE0);
	break;

      case 0336:		// SKIPN
	doSKIP(isNE0);
	break;

      case 0337:		// SKIPGT
	doSKIP(isGT0);
	break;

      case 0340:		// AOJ
	doAOSOXX(acGet, 1, acPut, never, jumpAction);
	break;

      case 0341:		// AOJL
	doAOSOXX(acGet, 1, acPut, isLT0, jumpAction);
	break;

      case 0342:		// AOJE
	doAOSOXX(acGet, 1, acPut, isEQ0, jumpAction);
	break;

      case 0343:		// AOJLE
	doAOSOXX(acGet, 1, acPut, isLE0, jumpAction);
	break;

      case 0344:		// AOJA
	doAOSOXX(acGet, 1, acPut, always, jumpAction);
	break;

      case 0345:		// AOJGE
	doAOSOXX(acGet, 1, acPut, isGE0, jumpAction);
	break;

      case 0346:		// AOJN
	doAOSOXX(acGet, 1, acPut, never, jumpAction);
	break;

      case 0347:		// AOJG
	doAOSOXX(acGet, 1, acPut, isGT0, jumpAction);
	break;

      case 0350:		// AOS
	doAOSOXX(acGet, 1, acPut, never, skipAction);
	break;

      case 0351:		// AOSL
	doAOSOXX(acGet, 1, acPut, isLT0, skipAction);
	break;

      case 0352:		// AOSE
	doAOSOXX(acGet, 1, acPut, isEQ0, skipAction);
	break;

      case 0353:		// AOSLE
	doAOSOXX(acGet, 1, acPut, isLE0, skipAction);
	break;

      case 0354:		// AOSA
	doAOSOXX(acGet, 1, acPut, always, skipAction);
	break;

      case 0355:		// AOSGE
	doAOSOXX(acGet, 1, acPut, isGE0, skipAction);
	break;

      case 0356:		// AOSN
	doAOSOXX(acGet, 1, acPut, never, skipAction);
	break;

      case 0357:		// AOSG
	doAOSOXX(acGet, 1, acPut, isGT0, skipAction);
	break;

      case 0360:		// SOJ
	doAOSOXX(acGet, -1, acPut, never, jumpAction);
	break;

      case 0361:		// SOJL
	doAOSOXX(acGet, -1, acPut, isLT0, jumpAction);
	break;

      case 0362:		// SOJE
	doAOSOXX(acGet, -1, acPut, isEQ0, jumpAction);
	break;

      case 0363:		// SOJLE
	doAOSOXX(acGet, -1, acPut, isLE0, jumpAction);
	break;

      case 0364:		// SOJA
	doAOSOXX(acGet, -1, acPut, always, jumpAction);
	break;

      case 0365:		// SOJGE
	doAOSOXX(acGet, -1, acPut, isGE0, jumpAction);
	break;

      case 0366:		// SOJN
	doAOSOXX(acGet, -1, acPut, never, jumpAction);
	break;

      case 0367:		// SOJG
	doAOSOXX(acGet, -1, acPut, isGT0, jumpAction);
	break;

      case 0370:		// SOS
	doAOSOXX(acGet, -1, acPut, never, skipAction);
	break;

      case 0371:		// SOSL
	doAOSOXX(acGet, -1, acPut, isLT0, skipAction);
	break;

      case 0372:		// SOSE
	doAOSOXX(acGet, -1, acPut, isEQ0, skipAction);
	break;

      case 0373:		// SOSLE
	doAOSOXX(acGet, -1, acPut, isLE0, skipAction);
	break;

      case 0374:		// SOSA
	doAOSOXX(acGet, -1, acPut, always, skipAction);
	break;

      case 0375:		// SOSGE
	doAOSOXX(acGet, -1, acPut, isGE0, skipAction);
	break;

      case 0376:		// SOSN
	doAOSOXX(acGet, -1, acPut, never, skipAction);
	break;

      case 0377:		// SOSG
	doAOSOXX(acGet, -1, acPut, isGT0, skipAction);
	break;

      case 0400:		// SETZ
	doSETXX(memGet, zeroWord, acPut);
	break;

      case 0401:		// SETZI
	doSETXX(immediate, zeroWord, acPut);
	break;

      case 0402:		// SETZM
	doSETXX(memGet, zeroWord, memPut);
	break;

      case 0403:		// SETZB
	doSETXX(memGet, zeroWord, bothPut);
	break;

      case 0404:		// AND
	doBinOpXX(memGet, acGet, andWord, acPut);
	break;

      case 0405:		// ANDI
	doBinOpXX(immediate, acGet, andWord, acPut);
	break;

      case 0406:		// ANDM
	doBinOpXX(memGet, acGet, andWord, memPut);
	break;

      case 0407:		// ANDB
	doBinOpXX(memGet, acGet, andWord, bothPut);
	break;

      case 0410:		// ANDCA
	doBinOpXX(memGet, acGet, andCWord, acPut);
	break;

      case 0411:		// ANDCAI
	doBinOpXX(immediate, acGet, andCWord, acPut);
	break;

      case 0412:		// ANDCAM
	doBinOpXX(memGet, acGet, andCWord, memPut);
	break;

      case 0413:		// ANDCAB
	doBinOpXX(memGet, acGet, andCWord, bothPut);
	break;

      case 0414:		// SETM
	doSETXX(acGet, noModification, memPut);
	break;

      case 0415:		// SETMI
	doSETXX(immediate, noModification, memPut);
	break;

      case 0416:		// SETMM
	doSETXX(acGet, noModification, memPut);
	break;

      case 0417:		// SETMB
	doSETXX(acGet, noModification, bothPut);
	break;

      case 0420:		// ANDCM
	doBinOpXX(acGet, memGet, andCWord, acPut);
	break;

      case 0421:		// ANDCMI
	doBinOpXX(acGet, immediate, andCWord, acPut);
	break;

      case 0422:		// ANDCMM
	doBinOpXX(acGet, memGet, andCWord, memPut);
	break;

      case 0423:		// ANDCMB
	doBinOpXX(acGet, memGet, andCWord, bothPut);
	break;

      case 0424:		// SETA
	doSETXX(acGet, noModification, acPut);
	break;

      case 0425:		// SETAI
	doSETXX(immediate, noModification, acPut);
	break;

      case 0426:		// SETAM
	doSETXX(acGet, noModification, memPut);
	break;

      case 0427:		// SETAB
	doSETXX(acGet, noModification, bothPut);
	break;

      case 0430:		// XOR
	doBinOpXX(memGet, acGet, xorWord, acPut);
	break;

      case 0431:		// XORI
	doBinOpXX(immediate, acGet, xorWord, acPut);
	break;

      case 0432:		// XORM
	doBinOpXX(memGet, acGet, xorWord, memPut);
	break;

      case 0433:		// XORB
	doBinOpXX(memGet, acGet, xorWord, bothPut);
	break;

      case 0434:		// IOR
	doBinOpXX(memGet, acGet, iorWord, acPut);
	break;

      case 0435:		// IORI
	doBinOpXX(immediate, acGet, iorWord, acPut);
	break;

      case 0436:		// IORM
	doBinOpXX(memGet, acGet, iorWord, memPut);
	break;

      case 0437:		// IORB
	doBinOpXX(memGet, acGet, iorWord, bothPut);
	break;

      case 0440:		// ANDCBM
	doBinOpXX(acGet, memGet, andCBWord, acPut);
	break;

      case 0441:		// ANDCBMI
	doBinOpXX(acGet, immediate, andCBWord, acPut);
	break;

      case 0442:		// ANDCBMM
	doBinOpXX(acGet, memGet, andCBWord, memPut);
	break;

      case 0443:		// ANDCBMB
	doBinOpXX(acGet, memGet, andCBWord, bothPut);
	break;

      case 0444:		// EQV
	doBinOpXX(memGet, acGet, eqvWord, acPut);
	break;

      case 0445:		// EQVI
	doBinOpXX(immediate, acGet, eqvWord, acPut);
	break;

      case 0446:		// EQVM
	doBinOpXX(memGet, acGet, eqvWord, memPut);
	break;

      case 0447:		// EQVB
	doBinOpXX(memGet, acGet, eqvWord, bothPut);
	break;

      case 0450:		// SETCA
	doSETXX(acGet, compWord, acPut);
	break;

      case 0451:		// SETCAI
	doSETXX(immediate, compWord, acPut);
	break;

      case 0452:		// SETCAM
	doSETXX(acGet, compWord, memPut);
	break;

      case 0453:		// SETCAB
	doSETXX(acGet, compWord, bothPut);
	break;

      case 0454:		// ORCA
	doBinOpXX(memGet, acGet, iorCWord, acPut);
	break;

      case 0455:		// ORCAI
	doBinOpXX(immediate, acGet, iorCWord, acPut);
	break;

      case 0456:		// ORCAM
	doBinOpXX(memGet, acGet, iorCWord, memPut);
	break;

      case 0457:		// ORCAB
	doBinOpXX(memGet, acGet, iorCWord, bothPut);
	break;

      case 0460:		// SETCM
	doSETXX(acGet, compWord, memPut);
	break;

      case 0461:		// SETCMI
	doSETXX(immediate, compWord, memPut);
	break;

      case 0462:		// SETCMM
	doSETXX(acGet, compWord, memPut);
	break;

      case 0463:		// SETCMB
	doSETXX(acGet, compWord, bothPut);
	break;

      case 0464:		// ORCM
	doBinOpXX(acGet, memGet, iorCWord, acPut);
	break;

      case 0465:		// ORCMI
	doBinOpXX(acGet, immediate, iorCWord, acPut);
	break;

      case 0466:		// ORCMM
	doBinOpXX(acGet, memGet, iorCWord, memPut);
	break;

      case 0467:		// ORCMB
	doBinOpXX(acGet, memGet, iorCWord, bothPut);
	break;

      case 0470:		// ORCB
	doBinOpXX(acGet, memGet, iorCBWord, acPut);
	break;

      case 0471:		// ORCBI
	doBinOpXX(acGet, immediate, iorCBWord, acPut);
	break;

      case 0472:		// ORCBM
	doBinOpXX(acGet, memGet, iorCBWord, memPut);
	break;

      case 0473:		// ORCBB
	doBinOpXX(acGet, memGet, iorCBWord, bothPut);
	break;

      case 0474:		// SETO
	doSETXX(memGet, onesWord, acPut);
	break;

      case 0475:		// SETOI
	doSETXX(immediate, onesWord, acPut);
	break;

      case 0476:		// SETOM
	doSETXX(memGet, onesWord, memPut);
	break;

      case 0477:		// SETOB
	doSETXX(memGet, onesWord, bothPut);
	break;

      case 0500:		// HLL
	doHXXXX(memGet, acGet, copyHLL, noModification, acPut);
	break;

      case 0501:		// HLLI/XHLLI
	doHXXXX(immediate, acGet, copyHLL, noModification, acPut);
	break;

      case 0502:		// HLLM
	doHXXXX(acGet, memGet, copyHLL, noModification, memPut);
	break;

      case 0503:		// HLLS
	doHXXXX(memGet, memGet, copyHLL, noModification, selfPut);
	break;

      case 0504:		// HRL
	doHXXXX(memGet, acGet, copyHRL, noModification, acPut);
	break;

      case 0505:		// HRLI
	doHXXXX(immediate, acGet, copyHRL, noModification, acPut);
	break;

      case 0506:		// HRLM
	doHXXXX(acGet, memGet, copyHRL, noModification, memPut);
	break;

      case 0507:		// HRLS
	doHXXXX(memGet, memGet, copyHRL, noModification, selfPut);
	break;

      case 0510:		// HLLZ
	doHXXXX(memGet, acGet, copyHLL, zeroR, acPut);
	break;

      case 0511:		// HLLZI
	doHXXXX(immediate, acGet, copyHLL, zeroR, acPut);
	break;

      case 0512:		// HLLZM
	doHXXXX(acGet, memGet, copyHLL, zeroR, memPut);
	break;

      case 0513:		// HLLZS
	doHXXXX(memGet, memGet, copyHLL, zeroR, selfPut);
	break;

      case 0514:		// HRLZ
	doHXXXX(memGet, acGet, copyHRL, zeroR, acPut);
	break;

      case 0515:		// HRLZI
	doHXXXX(immediate, acGet, copyHRL, zeroR, acPut);
	break;

      case 0516:		// HRLZM
	doHXXXX(acGet, memGet, copyHRL, zeroR, memPut);
	break;

      case 0517:		// HRLZS
	doHXXXX(memGet, memGet, copyHRL, zeroR, selfPut);
	break;

      case 0520:		// HLLO
	doHXXXX(memGet, acGet, copyHLL, onesR, acPut);
	break;

      case 0521:		// HLLOI
	doHXXXX(immediate, acGet, copyHLL, onesR, acPut);
	break;

      case 0522:		// HLLOM
	doHXXXX(acGet, memGet, copyHLL, onesR, memPut);
	break;

      case 0523:		// HLLOS
	doHXXXX(memGet, memGet, copyHLL, onesR, selfPut);
	break;

      case 0524:		// HRLO
	doHXXXX(memGet, acGet, copyHRL, onesR, acPut);
	break;

      case 0525:		// HRLOI
	doHXXXX(immediate, acGet, copyHRL, onesR, acPut);
	break;

      case 0526:		// HRLOM
	doHXXXX(acGet, memGet, copyHRL, onesR, memPut);
	break;

      case 0527:		// HRLOS
	doHXXXX(memGet, memGet, copyHRL, onesR, selfPut);
	break;

      case 0530:		// HLLE
	doHXXXX(memGet, acGet, copyHLL, extnR, acPut);
	break;

      case 0531:		// HLLEI
	doHXXXX(immediate, acGet, copyHLL, extnR, acPut);
	break;

      case 0532:		// HLLEM
	doHXXXX(acGet, memGet, copyHLL, extnR, memPut);
	break;

      case 0533:		// HLLES
	doHXXXX(memGet, memGet, copyHLL, extnR, selfPut);
	break;

      case 0534:		// HRLE
	doHXXXX(memGet, acGet, copyHRL, extnR, acPut);
	break;

      case 0535:		// HRLEI
	doHXXXX(immediate, acGet, copyHRL, extnR, acPut);
	break;

      case 0536:		// HRLEM
	doHXXXX(acGet, memGet, copyHRL, extnR, memPut);
	break;

      case 0537:		// HRLES
	doHXXXX(memGet, memGet, copyHRL, extnR, selfPut);
	break;

      case 0540:		// HRR
	doHXXXX(memGet, acGet, copyHRR, noModification, acPut);
	break;

      case 0541:		// HRRI
	doHXXXX(immediate, acGet, copyHRR, noModification, acPut);
	break;

      case 0542:		// HRRM
	doHXXXX(acGet, memGet, copyHRR, noModification, memPut);
	break;

      case 0543:		// HRRS
	doHXXXX(memGet, memGet, copyHRR, noModification, selfPut);
	break;

      case 0544:		// HLR
	doHXXXX(memGet, acGet, copyHLR, noModification, acPut);
	break;

      case 0545:		// HLRI
	doHXXXX(immediate, acGet, copyHLR, noModification, acPut);
	break;

      case 0546:		// HLRM
	doHXXXX(acGet, memGet, copyHLR, noModification, memPut);
	break;

      case 0547:		// HLRS
	doHXXXX(memGet, memGet, copyHLR, noModification, selfPut);
	break;

      case 0550:		// HRRZ
	doHXXXX(memGet, acGet, copyHRR, zeroL, acPut);
	break;

      case 0551:		// HRRZI
	doHXXXX(immediate, acGet, copyHRR, zeroL, acPut);
	break;

      case 0552:		// HRRZM
	doHXXXX(acGet, memGet, copyHRR, zeroL, memPut);
	break;

      case 0553:		// HRRZS
	doHXXXX(memGet, memGet, copyHRR, zeroL, selfPut);
	break;

      case 0554:		// HLRZ
	doHXXXX(memGet, acGet, copyHLR, zeroL, acPut);
	break;

      case 0555:		// HLRZI
	doHXXXX(immediate, acGet, copyHLR, zeroL, acPut);
	break;

      case 0556:		// HLRZM
	doHXXXX(acGet, memGet, copyHLR, zeroL, memPut);
	break;

      case 0557:		// HLRZS
	doHXXXX(memGet, memGet, copyHLR, zeroL, selfPut);
	break;

      case 0560:		// HRRO
	doHXXXX(memGet, acGet, copyHRR, onesL, acPut);
	break;

      case 0561:		// HRROI
	doHXXXX(immediate, acGet, copyHRR, onesL, acPut);
	break;

      case 0562:		// HRROM
	doHXXXX(acGet, memGet, copyHRR, onesL, memPut);
	break;

      case 0563:		// HRROS
	doHXXXX(memGet, memGet, copyHRR, onesL, selfPut);
	break;

      case 0564:		// HLRO
	doHXXXX(memGet, acGet, copyHLR, onesL, acPut);
	break;

      case 0565:		// HLROI
	doHXXXX(immediate, acGet, copyHLR, onesL, acPut);
	break;

      case 0566:		// HLROM
	doHXXXX(acGet, memGet, copyHLR, onesL, memPut);
	break;

      case 0567:		// HLROS
	doHXXXX(memGet, memGet, copyHLR, onesL, selfPut);
	break;

      case 0570:		// HRRE
	doHXXXX(memGet, acGet, copyHRR, extnL, acPut);
	break;

      case 0571:		// HRREI
	doHXXXX(immediate, acGet, copyHRR, extnL, acPut);
	break;

      case 0572:		// HRREM
	doHXXXX(acGet, memGet, copyHRR, extnL, memPut);
	break;

      case 0573:		// HRRES
	doHXXXX(memGet, memGet, copyHRR, extnL, selfPut);
	break;

      case 0574:		// HLRE
	doHXXXX(memGet, acGet, copyHLR, extnL, acPut);
	break;

      case 0575:		// HLREI
	doHXXXX(immediate, acGet, copyHLR, extnL, acPut);
	break;

      case 0576:		// HLREM
	doHXXXX(acGet, memGet, copyHLR, extnL, memPut);
	break;

      case 0577:		// HLRES
	doHXXXX(memGet, memGet, copyHLR, extnL, selfPut);
	break;

      case 0600:		// TRN
	doTXXXX(acGetRH, noModification, never, memPut);
	break;

      case 0601:		// TLN
	doTXXXX(acGetLH, noModification, never, memPut);
	break;

      case 0602:		// TRNE
	doTXXXX(acGetRH, noModification, isEQ0, memPut);
	break;

      case 0603:		// TLNE
	doTXXXX(acGetLH, noModification, isEQ0, memPut);
	break;

      case 0604:		// TRNA
	doTXXXX(acGetRH, noModification, always, memPut);
	break;

      case 0605:		// TLNA
	doTXXXX(acGetLH, noModification, always, memPut);
	break;

      case 0606:		// TRNN
	doTXXXX(acGetRH, noModification, isNE0, memPut);
	break;

      case 0607:		// TLNN
	doTXXXX(acGetLH, noModification, isNE0, memPut);
	break;

      case 0620:		// TRZ
	doTXXXX(acGetRH, zeroMask, never, memPut);
	break;

      case 0621:		// TLZ
	doTXXXX(acGetLH, zeroMask, never, memPut);
	break;

      case 0622:		// TRZE
	doTXXXX(acGetRH, zeroMask, isEQ0, memPut);
	break;

      case 0623:		// TLZE
	doTXXXX(acGetLH, zeroMask, isEQ0, memPut);
	break;

      case 0624:		// TRZA
	doTXXXX(acGetRH, zeroMask, always, memPut);
	break;

      case 0625:		// TLZA
	doTXXXX(acGetLH, zeroMask, always, memPut);
	break;

      case 0626:		// TRZN
	doTXXXX(acGetRH, zeroMask, isNE0, memPut);
	break;

      case 0627:		// TLZN
	doTXXXX(acGetLH, zeroMask, isNE0, memPut);
	break;

      case 0640:		// TRC
	doTXXXX(acGetRH, compMask, never, memPut);
	break;

      case 0641:		// TLC
	doTXXXX(acGetLH, compMask, never, memPut);
	break;

      case 0642:		// TRCE
	doTXXXX(acGetRH, compMask, isEQ0, memPut);
	break;

      case 0643:		// TLCE
	doTXXXX(acGetLH, compMask, isEQ0, memPut);
	break;

      case 0644:		// TRCA
	doTXXXX(acGetRH, compMask, always, memPut);
	break;

      case 0645:		// TLCA
	doTXXXX(acGetLH, compMask, always, memPut);
	break;

      case 0646:		// TRCN
	doTXXXX(acGetRH, compMask, isNE0, memPut);
	break;

      case 0647:		// TLCN
	doTXXXX(acGetLH, compMask, isNE0, memPut);
	break;

      case 0660:		// TRO
	doTXXXX(acGetRH, onesMask, never, memPut);
	break;

      case 0661:		// TLO
	doTXXXX(acGetLH, onesMask, never, memPut);
	break;

      case 0662:		// TROE
	doTXXXX(acGetRH, onesMask, isEQ0, memPut);
	break;

      case 0663:		// TLOE
	doTXXXX(acGetLH, onesMask, isEQ0, memPut);
	break;

      case 0664:		// TROA
	doTXXXX(acGetRH, onesMask, always, memPut);
	break;

      case 0665:		// TLOA
	doTXXXX(acGetLH, onesMask, always, memPut);
	break;

      case 0666:		// TRON
	doTXXXX(acGetRH, onesMask, isNE0, memPut);
	break;

      case 0667:		// TLON
	doTXXXX(acGetLH, onesMask, isNE0, memPut);
	break;

      case 0610:		// TDN
	doTXXXX(memGet, noModification, never, memPut);
	break;

      case 0611:		// TSN
	doTXXXX(memGetSwapped, noModification, never, noStore);
	break;

      case 0612:		// TDNE
	doTXXXX(memGet, noModification, isEQ0, memPut);
	break;

      case 0613:		// TSNE
	doTXXXX(memGetSwapped, noModification, isEQ0, noStore);
	break;

      case 0614:		// TDNA
	doTXXXX(memGet, noModification, always, memPut);
	break;

      case 0615:		// TSNA
	doTXXXX(memGetSwapped, noModification, always, noStore);
	break;

      case 0616:		// TDNN
	doTXXXX(memGet, noModification, isNE0, memPut);
	break;

      case 0617:		// TSNN
	doTXXXX(memGetSwapped, noModification, isNE0, noStore);
	break;

      case 0630:		// TDZ
	doTXXXX(memGet, zeroMask, never, memPut);
	break;

      case 0631:		// TSZ
	doTXXXX(memGetSwapped, zeroMask, never, noStore);
	break;

      case 0632:		// TDZE
	doTXXXX(memGet, zeroMask, isEQ0, memPut);
	break;

      case 0633:		// TSZE
	doTXXXX(memGetSwapped, zeroMask, isEQ0, noStore);
	break;

      case 0634:		// TDZA
	doTXXXX(memGet, zeroMask, always, memPut);
	break;

      case 0635:		// TSZA
	doTXXXX(memGetSwapped, zeroMask, always, noStore);
	break;

      case 0636:		// TDZN
	doTXXXX(memGet, zeroMask, isNE0, memPut);
	break;

      case 0637:		// TSZN
	doTXXXX(memGetSwapped, zeroMask, isNE0, noStore);
	break;

      case 0700:

	switch ((unsigned) iw.ioAll << 2) {
	case 070000: 		// APRID
	  memPut(aprIDValue.u);
	  break;

	case 070020: {		// CONO APR,
	  APRFunctions func(eaw.u);

	  if (traceMem) cerr << " ; " << oct << setw(6) << eaw.rhu;

	  if (func.clear) {
	    aprState.active.u &= ~func.select.u;
	  } else if (func.set) {
	    aprState.active.u |= func.select.u;

	    // This block argues the APR state needs to be
	    // metaprogrammed with C++17 parameter pack superpowers
	    // instead of this old fashioned mnaual method. This might
	    // be an interesting thing to do in the future. For now,
	    // it's done the 1940s hard way.
	    if (func.intLevel != 0) {
	      if (func.select.sweepDone != 0)      aprLevels.sweepDone = func.intLevel;
	      if (func.select.powerFailure != 0)   aprLevels.powerFailure = func.intLevel;
	      if (func.select.addrParity != 0)     aprLevels.addrParity = func.intLevel;
	      if (func.select.cacheDirParity != 0) aprLevels.cacheDirParity = func.intLevel;
	      if (func.select.mbParity != 0)       aprLevels.mbParity = func.intLevel;
	      if (func.select.ioPageFail != 0)     aprLevels.ioPageFail = func.intLevel;
	      if (func.select.noMemory != 0)       aprLevels.noMemory = func.intLevel;
	      if (func.select.sbusError != 0)      aprLevels.sbusError = func.intLevel;
	    }
	  } else if (func.enable) {
	    aprState.enabled.u |= func.select.u;
	  } else if (func.disable) {
	    aprState.enabled.u &= ~func.select.u;
	  }

	  if (func.clearIO) {
	    nyi();
	  }

	  }
	  break;

	case 070024:		// CONI APR,
	  memPut(aprState.u);
	  break;

	case 070060: {		// CONO PI,
	  PIFunctions pi(ea);

	  if (traceMem) cerr << " ; " << oct << setw(6) << eaw.rhu;

	  if (pi.clearPI) {
	    piState.u = 0;
	  } else {
	    piState.writeEvenParityDir = pi.writeEvenParityDir;
	    piState.writeEvenParityData = pi.writeEvenParityData;
	    piState.writeEvenParityAddr = pi.writeEvenParityAddr;

	    if (pi.turnPIOn) {
	      piState.piEnabled = 1;
	    } else if (pi.turnPIOff) {
	      piState.piEnabled = 0;
	    } else if (pi.dropRequests != 0) {
	      piState.levelsRequested &= ~pi.levels;
	    } else if (pi.levelsInitiate) {
	      piState.levelsRequested |= pi.levels;
	    } else if (pi.levelsOff) {
	      piState.levelsEnabled &= ~pi.levels;
	    } else if (pi.levelsOn) {
	      piState.levelsEnabled |= pi.levels;
	    }
	  }

	  break;
	}

	case 070064:		// CONI PI,
	  memPut(piState.u);
	  break;

	case 070120:		// CONO PAG,
	  if (traceMem) cerr << " ; " << oct << setw(6) << eaw.rhu;
	  pagState.u = iw.y;
	  break;

	case 070124:		// CONI PAG,
	  memPut(W36(memGet().lhu, pagState.u));
	  break;

	default:
	  nyi();
	  break;
	}

	break;

      default:
	nyi();
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
