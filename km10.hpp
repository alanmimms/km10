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
    W36 tmp;

    uint64_t nInsns = 0;

    function<W36(W36)> swap = [&](W36 src) {return W36(src.rhu, src.lhu);};

    function<W36(W36)> negate = [&](W36 src) {
      W36 v(-src.s);
      if (src.u == W36::mostNegative) flags.tr1 = flags.ov = flags.cy1 = 1;
      return v;
    };

    function<W36()> memGet = [&]() -> W36 {
      W36 value = memP[ea.u];;
      if (traceMem) cerr << " ; " << ea.fmtVMA() << ": " << value.fmt36();
      return value;
    };

    function<W36()> memGetSwapped = [&]() -> W36 {return swap(memGet());};

    function<void(W36)> memPut = [&](W36 value) -> void {
      memP[ea.u] = value;
      if (traceMem) cerr << " ; " << ea.fmtVMA() << "<-" << value.fmt36();
    };

    function<W36()> acGet = [&]() -> W36 {
      W36 value = AC[iw.ac];
      if (traceMem) cerr << " ; ac" << oct << iw.ac << ": " << value.fmt36();
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

    function<void(W36)> acPut = [&](W36 value) -> void {
      AC[iw.ac] = value;
      if (traceMem) cerr << " ; ac" << oct << iw.ac << "<-" << value.fmt36();
    };

    function<void(W36)> selfPut = [&](W36 value) -> void {
      memPut(value);
      if (iw.ac != 0) acPut(value);
    };

    function<W36()> immediate = [&]() -> W36 {return W36(0, ea.rhu);};

    // For the likes of XHLLI.
    function<W36()> immediateX = [&]() -> W36 {return ea;};

    // Condition testing predicates
    function<bool(W36 toTest)> isLT0  = [&](W36 v) -> bool const {return v.s <  0;};
    function<bool(W36 toTest)> isLE0  = [&](W36 v) -> bool const {return v.s <= 0;};
    function<bool(W36 toTest)> isGT0  = [&](W36 v) -> bool const {return v.s >  0;};
    function<bool(W36 toTest)> isGE0  = [&](W36 v) -> bool const {return v.s >= 0;};
    function<bool(W36 toTest)> isNE0  = [&](W36 v) -> bool const {return v.s != 0;};
    function<bool(W36 toTest)> isEQ0  = [&](W36 v) -> bool const {return v.s == 0;};
    function<bool(W36 toTest)> always = [&](W36 v) -> bool const {return true;};
    function<bool(W36 toTest)> never  = [&](W36 v) -> bool const {return false;};

    auto doJUMP = [&](function<bool(W36 toTest)> &condF) -> void {
      W36 eaw = memGet();

      if (condF(eaw)) {
	if (traceMem) cerr << " [jump]";
	nextPC.rhu = ea;
      }
    };

    auto doSKIP = [&](function<bool(W36 toTest)> &condF) -> void {
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

    function<void(W36)> noStore = [](W36 toSrc) -> void {};

    auto doTxxxx = [&](function<W36()> &doGetF,
		       function<W36(W36 fromSrc)> &doModifyF,
		       function<bool(W36 toTest)> &condF,
		       function<void(W36 toSrc)> &doStoreF) -> void
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
    function<W36(W36, W36)> copyHRR = [&](W36 src, W36 dst) -> auto const {return W36(dst.lhu, src.rhu);};
    function<W36(W36, W36)> copyHRL = [&](W36 src, W36 dst) -> auto const {return W36(src.rhu, dst.rhu);};
    function<W36(W36, W36)> copyHLL = [&](W36 src, W36 dst) -> auto const {return W36(src.lhu, dst.rhu);};
    function<W36(W36, W36)> copyHLR = [&](W36 src, W36 dst) -> auto const {return W36(dst.rhu, src.lhu);};

    // doModifyF functions
    function<W36(W36)> zeroR = [&](W36 dst) -> auto const {return W36(dst.lhu,               0);};
    function<W36(W36)> onesR = [&](W36 dst) -> auto const {return W36(dst.lhu,   W36::halfOnes);};
    function<W36(W36)> extnR = [&](W36 dst) -> auto const {return W36(dst.lhu, extnOf(dst.lhu));};
    function<W36(W36)> zeroL = [&](W36 dst) -> auto const {return W36(              0, dst.rhu);};
    function<W36(W36)> onesL = [&](W36 dst) -> auto const {return W36(  W36::halfOnes, dst.rhu);};
    function<W36(W36)> extnL = [&](W36 dst) -> auto const {return W36(extnOf(dst.rhu), dst.rhu);};

    auto doHxxxx = [&](function<W36()> &doGetSrcF,
		       function<W36()> &doGetDstF,
		       function<W36(W36 src, W36 dst)> &doCopyF,
		       function<W36(W36 dst)> &doModifyF,
		       function<void(W36 v)> &doPutDstF) -> void
    {
      doPutDstF(doModifyF(doCopyF(doGetSrcF(), doGetDstF())));
    };


    auto doMOVxx = [&](function<W36()> &doGetSrcF,
		       function<W36(W36 dst)> &doModifyF,
		       function<void(W36 v)> &doPutDstF) -> void
    {
      doPutDstF(doModifyF(doGetSrcF()));
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
	     << ": [ea=" << ea.fmtVMA() << "]  "
	     << setw(20) << left << iw.disasm();
      }

      switch (iw.op) {

      case 0200:		// MOVE
	doMOVxx(memGet, noModification, acPut);
	break;

      case 0201:		// MOVEI
	doMOVxx(immediate, noModification, acPut);
	break;

      case 0202:		// MOVEM
	doMOVxx(acGet, noModification, memPut);
	break;

      case 0203:		// MOVES
	doMOVxx(acGet, noModification, selfPut);
	break;

      case 0204:		// MOVS
	doMOVxx(memGet, swap, acPut);
	break;

      case 0205:		// MOVSI
	doMOVxx(immediate, swap, acPut);
	break;

      case 0206:		// MOVSM
	doMOVxx(acGet, swap, memPut);
	break;

      case 0207:		// MOVSS
	doMOVxx(acGet, swap, selfPut);
	break;

      case 0210:		// MOVN
	doMOVxx(memGet, negate, acPut);
	break;

      case 0211:		// MOVNI
	doMOVxx(immediate, negate, acPut);
	break;

      case 0212:		// MOVNM
	doMOVxx(acGet, negate, memPut);
	break;

      case 0213:		// MOVNS
	doMOVxx(acGet, negate, selfPut);
	break;

      case 0250:		// EXCH
	tmp = acGet();
	acPut(memGet());
	memPut(tmp);
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

      case 0500:		// HLL
	doHxxxx(memGet, acGet, copyHLL, noModification, acPut);
	break;

      case 0501:		// HLLI/XHLLI

	if (pc.isSection0()) {
	  doHxxxx(acGet, immediate, copyHLL, noModification, acPut);
	} else {
	  doHxxxx(acGet, immediateX, copyHLL, noModification, acPut);
	}
	
	break;

      case 0502:		// HLLM
	doHxxxx(acGet, memGet, copyHLL, noModification, memPut);
	break;

      case 0503:		// HLLS
	doHxxxx(memGet, memGet, copyHLL, noModification, selfPut);
	break;

      case 0504:		// HRL
	doHxxxx(memGet, acGet, copyHRL, noModification, acPut);
	break;

      case 0505:		// HRLI
	doHxxxx(acGet, immediate, copyHRL, noModification, acPut);
	break;

      case 0506:		// HRLM
	doHxxxx(acGet, memGet, copyHRL, noModification, memPut);
	break;

      case 0507:		// HRLS
	doHxxxx(memGet, memGet, copyHRL, noModification, selfPut);
	break;

      case 0510:		// HLLZ
	doHxxxx(memGet, acGet, copyHLL, zeroR, acPut);
	break;

      case 0511:		// HLLZI
	doHxxxx(acGet, immediate, copyHLL, zeroR, acPut);
	break;

      case 0512:		// HLLZM
	doHxxxx(acGet, memGet, copyHLL, zeroR, memPut);
	break;

      case 0513:		// HLLZS
	doHxxxx(memGet, memGet, copyHLL, zeroR, selfPut);
	break;

      case 0514:		// HRLZ
	doHxxxx(memGet, acGet, copyHRL, zeroR, acPut);
	break;

      case 0515:		// HRLZI
	doHxxxx(acGet, immediate, copyHRL, zeroR, acPut);
	break;

      case 0516:		// HRLZM
	doHxxxx(acGet, memGet, copyHRL, zeroR, memPut);
	break;

      case 0517:		// HRLZS
	doHxxxx(memGet, memGet, copyHRL, zeroR, selfPut);
	break;

      case 0520:		// HLLO
	doHxxxx(memGet, acGet, copyHLL, onesR, acPut);
	break;

      case 0521:		// HLLOI
	doHxxxx(acGet, immediate, copyHLL, onesR, acPut);
	break;

      case 0522:		// HLLOM
	doHxxxx(acGet, memGet, copyHLL, onesR, memPut);
	break;

      case 0523:		// HLLOS
	doHxxxx(memGet, memGet, copyHLL, onesR, selfPut);
	break;

      case 0524:		// HRLO
	doHxxxx(memGet, acGet, copyHRL, onesR, acPut);
	break;

      case 0525:		// HRLOI
	doHxxxx(acGet, immediate, copyHRL, onesR, acPut);
	break;

      case 0526:		// HRLOM
	doHxxxx(acGet, memGet, copyHRL, onesR, memPut);
	break;

      case 0527:		// HRLOS
	doHxxxx(memGet, memGet, copyHRL, onesR, selfPut);
	break;

      case 0530:		// HLLE
	doHxxxx(memGet, acGet, copyHLL, extnR, acPut);
	break;

      case 0531:		// HLLEI
	doHxxxx(acGet, immediate, copyHLL, extnR, acPut);
	break;

      case 0532:		// HLLEM
	doHxxxx(acGet, memGet, copyHLL, extnR, memPut);
	break;

      case 0533:		// HLLES
	doHxxxx(memGet, memGet, copyHLL, extnR, selfPut);
	break;

      case 0534:		// HRLE
	doHxxxx(memGet, acGet, copyHRL, extnR, acPut);
	break;

      case 0535:		// HRLEI
	doHxxxx(acGet, immediate, copyHRL, extnR, acPut);
	break;

      case 0536:		// HRLEM
	doHxxxx(acGet, memGet, copyHRL, extnR, memPut);
	break;

      case 0537:		// HRLES
	doHxxxx(memGet, memGet, copyHRL, extnR, selfPut);
	break;

      case 0540:		// HRR
	doHxxxx(memGet, acGet, copyHRR, noModification, acPut);
	break;

      case 0541:		// HRRI
	doHxxxx(acGet, immediate, copyHRR, noModification, acPut);
	break;

      case 0542:		// HRRM
	doHxxxx(acGet, memGet, copyHRR, noModification, memPut);
	break;

      case 0543:		// HRRS
	doHxxxx(memGet, memGet, copyHRR, noModification, selfPut);
	break;

      case 0544:		// HLR
	doHxxxx(memGet, acGet, copyHLR, noModification, acPut);
	break;

      case 0545:		// HLRI
	doHxxxx(acGet, immediate, copyHLR, noModification, acPut);
	break;

      case 0546:		// HLRM
	doHxxxx(acGet, memGet, copyHLR, noModification, memPut);
	break;

      case 0547:		// HLRS
	doHxxxx(memGet, memGet, copyHLR, noModification, selfPut);
	break;

      case 0550:		// HRRZ
	doHxxxx(memGet, acGet, copyHRR, zeroL, acPut);
	break;

      case 0551:		// HRRZI
	doHxxxx(acGet, immediate, copyHRR, zeroL, acPut);
	break;

      case 0552:		// HRRZM
	doHxxxx(acGet, memGet, copyHRR, zeroL, memPut);
	break;

      case 0553:		// HRRZS
	doHxxxx(memGet, memGet, copyHRR, zeroL, selfPut);
	break;

      case 0554:		// HLRZ
	doHxxxx(memGet, acGet, copyHLR, zeroL, acPut);
	break;

      case 0555:		// HLRZI
	doHxxxx(acGet, immediate, copyHLR, zeroL, acPut);
	break;

      case 0556:		// HLRZM
	doHxxxx(acGet, memGet, copyHLR, zeroL, memPut);
	break;

      case 0557:		// HLRZS
	doHxxxx(memGet, memGet, copyHLR, zeroL, selfPut);
	break;

      case 0560:		// HRRO
	doHxxxx(memGet, acGet, copyHRR, onesL, acPut);
	break;

      case 0561:		// HRROI
	doHxxxx(acGet, immediate, copyHRR, onesL, acPut);
	break;

      case 0562:		// HRROM
	doHxxxx(acGet, memGet, copyHRR, onesL, memPut);
	break;

      case 0563:		// HRROS
	doHxxxx(memGet, memGet, copyHRR, onesL, selfPut);
	break;

      case 0564:		// HLRO
	doHxxxx(memGet, acGet, copyHLR, onesL, acPut);
	break;

      case 0565:		// HLROI
	doHxxxx(acGet, immediate, copyHLR, onesL, acPut);
	break;

      case 0566:		// HLROM
	doHxxxx(acGet, memGet, copyHLR, onesL, memPut);
	break;

      case 0567:		// HLROS
	doHxxxx(memGet, memGet, copyHLR, onesL, selfPut);
	break;

      case 0570:		// HRRE
	doHxxxx(memGet, acGet, copyHRR, extnL, acPut);
	break;

      case 0571:		// HRREI
	doHxxxx(acGet, immediate, copyHRR, extnL, acPut);
	break;

      case 0572:		// HRREM
	doHxxxx(acGet, memGet, copyHRR, extnL, memPut);
	break;

      case 0573:		// HRRES
	doHxxxx(memGet, memGet, copyHRR, extnL, selfPut);
	break;

      case 0574:		// HLRE
	doHxxxx(memGet, acGet, copyHLR, extnL, acPut);
	break;

      case 0575:		// HLREI
	doHxxxx(acGet, immediate, copyHLR, extnL, acPut);
	break;

      case 0576:		// HLREM
	doHxxxx(acGet, memGet, copyHLR, extnL, memPut);
	break;

      case 0577:		// HLRES
	doHxxxx(memGet, memGet, copyHLR, extnL, selfPut);
	break;

      case 0600:		// TRN
	doTxxxx(acGetRH, noModification, never, memPut);
	break;

      case 0601:		// TLN
	doTxxxx(acGetLH, noModification, never, memPut);
	break;

      case 0602:		// TRNE
	doTxxxx(acGetRH, noModification, isEQ0, memPut);
	break;

      case 0603:		// TLNE
	doTxxxx(acGetLH, noModification, isEQ0, memPut);
	break;

      case 0604:		// TRNA
	doTxxxx(acGetRH, noModification, always, memPut);
	break;

      case 0605:		// TLNA
	doTxxxx(acGetLH, noModification, always, memPut);
	break;

      case 0606:		// TRNN
	doTxxxx(acGetRH, noModification, isNE0, memPut);
	break;

      case 0607:		// TLNN
	doTxxxx(acGetLH, noModification, isNE0, memPut);
	break;

      case 0620:		// TRZ
	doTxxxx(acGetRH, zeroMask, never, memPut);
	break;

      case 0621:		// TLZ
	doTxxxx(acGetLH, zeroMask, never, memPut);
	break;

      case 0622:		// TRZE
	doTxxxx(acGetRH, zeroMask, isEQ0, memPut);
	break;

      case 0623:		// TLZE
	doTxxxx(acGetLH, zeroMask, isEQ0, memPut);
	break;

      case 0624:		// TRZA
	doTxxxx(acGetRH, zeroMask, always, memPut);
	break;

      case 0625:		// TLZA
	doTxxxx(acGetLH, zeroMask, always, memPut);
	break;

      case 0626:		// TRZN
	doTxxxx(acGetRH, zeroMask, isNE0, memPut);
	break;

      case 0627:		// TLZN
	doTxxxx(acGetLH, zeroMask, isNE0, memPut);
	break;

      case 0640:		// TRC
	doTxxxx(acGetRH, compMask, never, memPut);
	break;

      case 0641:		// TLC
	doTxxxx(acGetLH, compMask, never, memPut);
	break;

      case 0642:		// TRCE
	doTxxxx(acGetRH, compMask, isEQ0, memPut);
	break;

      case 0643:		// TLCE
	doTxxxx(acGetLH, compMask, isEQ0, memPut);
	break;

      case 0644:		// TRCA
	doTxxxx(acGetRH, compMask, always, memPut);
	break;

      case 0645:		// TLCA
	doTxxxx(acGetLH, compMask, always, memPut);
	break;

      case 0646:		// TRCN
	doTxxxx(acGetRH, compMask, isNE0, memPut);
	break;

      case 0647:		// TLCN
	doTxxxx(acGetLH, compMask, isNE0, memPut);
	break;

      case 0660:		// TRO
	doTxxxx(acGetRH, onesMask, never, memPut);
	break;

      case 0661:		// TLO
	doTxxxx(acGetLH, onesMask, never, memPut);
	break;

      case 0662:		// TROE
	doTxxxx(acGetRH, onesMask, isEQ0, memPut);
	break;

      case 0663:		// TLOE
	doTxxxx(acGetLH, onesMask, isEQ0, memPut);
	break;

      case 0664:		// TROA
	doTxxxx(acGetRH, onesMask, always, memPut);
	break;

      case 0665:		// TLOA
	doTxxxx(acGetLH, onesMask, always, memPut);
	break;

      case 0666:		// TRON
	doTxxxx(acGetRH, onesMask, isNE0, memPut);
	break;

      case 0667:		// TLON
	doTxxxx(acGetLH, onesMask, isNE0, memPut);
	break;

      case 0610:		// TDN
	doTxxxx(memGet, noModification, never, memPut);
	break;

      case 0611:		// TSN
	doTxxxx(memGetSwapped, noModification, never, noStore);
	break;

      case 0612:		// TDNE
	doTxxxx(memGet, noModification, isEQ0, memPut);
	break;

      case 0613:		// TSNE
	doTxxxx(memGetSwapped, noModification, isEQ0, noStore);
	break;

      case 0614:		// TDNA
	doTxxxx(memGet, noModification, always, memPut);
	break;

      case 0615:		// TSNA
	doTxxxx(memGetSwapped, noModification, always, noStore);
	break;

      case 0616:		// TDNN
	doTxxxx(memGet, noModification, isNE0, memPut);
	break;

      case 0617:		// TSNN
	doTxxxx(memGetSwapped, noModification, isNE0, noStore);
	break;

      case 0630:		// TDZ
	doTxxxx(memGet, zeroMask, never, memPut);
	break;

      case 0631:		// TSZ
	doTxxxx(memGetSwapped, zeroMask, never, noStore);
	break;

      case 0632:		// TDZE
	doTxxxx(memGet, zeroMask, isEQ0, memPut);
	break;

      case 0633:		// TSZE
	doTxxxx(memGetSwapped, zeroMask, isEQ0, noStore);
	break;

      case 0634:		// TDZA
	doTxxxx(memGet, zeroMask, always, memPut);
	break;

      case 0635:		// TSZA
	doTxxxx(memGetSwapped, zeroMask, always, noStore);
	break;

      case 0636:		// TDZN
	doTxxxx(memGet, zeroMask, isNE0, memPut);
	break;

      case 0637:		// TSZN
	doTxxxx(memGetSwapped, zeroMask, isNE0, noStore);
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
