// This is the KM10 CPU implementation.

/*
  Some notes:

  * INVARIANT: Debugger always shows next instruction to execute in
    its prompt.

    * Interrupt vector instruction.
    * Exception vector instruction.
    * XCT next instruction in chain.
    * Normal code flow instruction.

  * INVARIANT: `pc` always points to instruction that is about
    to execute or is executing.

    * Therefore, in exception/interrupt handling, `pc` points to
      interrupt instruction.

  * INVARIANT: `nextPC` always points to next instruction to fetch
    after current one completes.

    * Instructions like JSP/JSR save the initial value of `nextPC` as
      their "return address" and modify it to point to the jump
      destination the same way in both normal and in
      interrupt/exception contexts.
*/

/*
  TODO:

  * Globally use U32,S32, U64,S64, U128,S128 typedefs instead of
    verbose <cstdint> names.
  * Fix the logging stuff. It's seriously broken.
*/

/*
  TODO:

  Reconfigure so that execute1() runs a single instruction and
  returns a value indicating the instruction's effect:

  * Normal
  * Skip
  * MUUO/LUUO
  * Trap
  * HALT
  * XCT
   etc...

  Interrupts are checked in the loop surrounding execute1() and
  dispatched by changing which instruction is executed.
*/

#pragma once
#include <string>
#include <cstdint>
#include <sstream>
#include <iomanip>
#include <fstream>
#include <ostream>
#include <fmt/core.h>
#include <limits>
#include <functional>
#include <assert.h>
#include <unordered_set>

using namespace std;
using namespace fmt;


#include "apr.hpp"
#include "bytepointer.hpp"
#include "cca.hpp"
#include "device.hpp"
#include "dte20.hpp"
#include "logger.hpp"
#include "mtr.hpp"
#include "pag.hpp"
#include "pi.hpp"
#include "tim.hpp"
#include "word.hpp"


class Debugger;


class KM10 {
public:
  APRDevice apr;
  CCADevice cca;
  MTRDevice mtr;
  PIDevice pi;
  PAGDevice pag;
  TIMDevice tim;
  DTE20 dte;
  Device noDevice;
  Debugger *debuggerP;


  // Each instruction method returns this to indicate what type of
  // instruction it was. This affects how PC is updated, whether a
  // trap is to be executed, etc.
  enum InstructionResult {
    iNormal,			// Normal execution with no PC modification or traps.
    iSkip,			// Instruction caused a skip condition.
    iJump,			// Instruction changed the PC.
    iMUUO,			// Instruction is a MUUO.
    iLUUO,			// Instruction is a LUUO.
    iTrap,			// Instruction caused a trap condition.
    iHALT,			// Instruction halted the CPU.
    iXCT,			// Instruction is an XCT.
    iNoSuchDevice,		// Instruction is I/O operation on a non-existent device.
    iNYI,			// Instruction is not yet implemented.
  };


  // Type for the function that implements an opcode.
  using InstructionF = InstructionResult (KM10::*)();


  // This is indexed by opcode, giving the method to call for that
  // opcode. I'm using a C style array so I can use a designated
  // initializer. I make it static and bind the instance "this" at
  // time of call with ".*" since the elements point to instance
  // methods.
  const InstructionF ops[512];


  // I find these a lot less ugly...
  using WFunc = function<W36()>;
  using DFunc = function<W72()>;
  using FuncW = function<void(W36)>;
  using WFuncW = function<W36(W36)>;
  using FuncWW = function<void(W36,W36)>;
  using FuncD = function<void(W72)>;
  using WFuncWW = function<W36(W36,W36)>;
  using DFuncWW = function<W72(W36,W36)>;
  using DFuncDW = function<W72(W72,W36)>;
  using BoolPredW = function<bool(W36)>;
  using BoolPredWW = function<bool(W36,W36)>;
  using VoidFunc = function<void()>;


  // Constructor
  KM10();


  W36 iw;
  W36 ea;


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


  WFunc acGet = [&]() {
    return acGetN(iw.ac);
  };


  WFunc acGetRH = [&]() {
    W36 value{0, acGet().rhu};
    if (logger.mem) logger.s << "; acRH" << oct << iw.ac << ": " << value.fmt18();
    return value;
  };

    // This retrieves LH into the RH of the return value, which is
    // required for things like TLNN to work properly since they use
    // the EA as a mask.
  WFunc acGetLH = [&]() {
    W36 value{0, acGet().lhu};
    if (logger.mem) logger.s << "; acLH" << oct << iw.ac << ": " << value.fmt18();
    return value;
  };


  FuncW acPut = [&](W36 value) {
    acPutN(value, iw.ac);
  };


  FuncW acPutRH = [&](W36 value) {
    acPut(W36(acGet().lhu, value.rhu));
  };


  // This is used to store back in, e.g., TLZE. But the LH of AC is
  // in the RH of the word we're passed because of how the testing
  // logic of these instructions must work. So we put the RH of the
  // value into the LH of the AC, keeping the AC's RH intact.
  FuncW acPutLH = [&](W36 value) {
    acPut(W36(value.rhu, acGet().rhu));
  };


  DFunc acGet2 = [&]() {
    W72 ret{acGetN(iw.ac+0), acGetN(iw.ac+1)};
    return ret;
  };


  FuncD acPut2 = [&](W72 v) {
    acPutN(v.hi, iw.ac+0);
    acPutN(v.lo, iw.ac+1);
  };


  WFunc memGet = [&]() {
    return memGetN(ea);
  };


  FuncW memPut = [&](W36 value) {
    memPutN(value, ea);
  };


  FuncW selfPut = [&](W36 value) {
    memPut(value);
    if (iw.ac != 0) acPut(value);
  };


  FuncW bothPut = [&](W36 value) {
    acPut(value);
    memPut(value);
  };


  FuncD bothPut2 = [&](W72 v) {
    acPutN(v.hi, iw.ac+0);
    acPutN(v.lo, iw.ac+1);
    memPut(v.hi);
  };


  WFuncW swap = [&](W36 src) {return W36{src.rhu, src.lhu};};


  WFuncW negate = [&](W36 src) {
    W36 v(-src.s);
    if (src.u == W36::bit0) flags.tr1 = flags.ov = flags.cy1 = 1;
    if (src.u == 0) flags.cy0 = flags.cy1 = 1;
    return v;
  };


  WFuncW magnitude = [&](W36 src) {
    W36 v(src.s < 0 ? -src.s : src.s);
    if (src.u == W36::bit0) flags.tr1 = flags.ov = flags.cy1 = 1;
    return v;
  };


  WFunc memGetSwapped = [&]() {return swap(memGet());};


  FuncD memPutHi = [&](W72 v) {
    memPut(v.hi);
  };


  WFunc immediate = [&]() {return W36(pc.isSection0() ? 0 : ea.lhu, ea.rhu);};


  // Condition testing predicates
  BoolPredW isLT0  = [&](W36 v) {return v.s  < 0;};
  BoolPredW isLE0  = [&](W36 v) {return v.s <= 0;};
  BoolPredW isGT0  = [&](W36 v) {return v.s  > 0;};
  BoolPredW isGE0  = [&](W36 v) {return v.s >= 0;};
  BoolPredW isNE0  = [&](W36 v) {return v.s != 0;};
  BoolPredW isEQ0  = [&](W36 v) {return v.s == 0;};
  BoolPredW always = [&](W36 v) {return  true;};
  BoolPredW never  = [&](W36 v) {return false;};

  BoolPredWW isNE0T  = [&](W36 a, W36 b) {return (a.u & b.u) != 0;};
  BoolPredWW isEQ0T  = [&](W36 a, W36 b) {return (a.u & b.u) == 0;};
  BoolPredWW alwaysT = [&](W36 a, W36 b) {return  true;};
  BoolPredWW neverT  = [&](W36 a, W36 b) {return false;};


  WFunc getE = [&]() -> W36 {return ea;};
  WFuncW  noMod1 = [&](W36 a) {return a;};
  WFuncWW noMod2 = [&](W36 a, W36 b) {return a;};


  // There is no `zeroMaskL`, `compMaskR`, `onesMaskL` because,
  // e.g., TLZE operates on the LH of the AC while it's in the RH of
  // the value so the testing/masking work properly.
  WFuncWW zeroMaskR = [&](W36 a, W36 b) {return a.u & ~(uint64_t) b.rhu;};
  WFuncWW zeroMask  = [&](W36 a, W36 b) {return a.u & ~b.u;};

  WFuncWW onesMaskR = [&](W36 a, W36 b) {return a.u | b.rhu;};
  WFuncWW onesMask  = [&](W36 a, W36 b) {return a.u | b.u;};

  WFuncWW compMaskR = [&](W36 a, W36 b) {return a.u ^ b.rhu;};
  WFuncWW compMask  = [&](W36 a, W36 b) {return a.u ^ b.u;};

  WFuncW zeroWord = [&](W36 a) {return 0;};
  WFuncW onesWord = [&](W36 a) {return W36::all1s;};
  WFuncW compWord = [&](W36 a) {return ~a.u;};

  FuncW noStore = [](W36 toSrc) {};


  // For a given low halfword, this computes an upper halfword by
  // extending the low halfword's sign.
  function<unsigned(const unsigned)> extnOf = [&](const unsigned v) {
    return (v & 0400'000) ? W36::halfOnes : 0u;
  };


  // doCopyF functions
  WFuncWW copyHRR = [&](W36 src, W36 dst) {return W36(dst.lhu, src.rhu);};
  WFuncWW copyHRL = [&](W36 src, W36 dst) {return W36(src.rhu, dst.rhu);};
  WFuncWW copyHLL = [&](W36 src, W36 dst) {return W36(src.lhu, dst.rhu);};
  WFuncWW copyHLR = [&](W36 src, W36 dst) {return W36(dst.lhu, src.lhu);};


  // doModifyF functions
  WFuncW zeroR = [&](W36 v) {return W36(v.lhu, 0);};
  WFuncW onesR = [&](W36 v) {return W36(v.lhu, W36::halfOnes);};
  WFuncW extnR = [&](W36 v) {return W36(extnOf(v.rhu), v.rhu);};
  WFuncW zeroL = [&](W36 v) {return W36(0, v.rhu);};
  WFuncW onesL = [&](W36 v) {return W36(W36::halfOnes, v.rhu);};
  WFuncW extnL = [&](W36 v) {return W36(v.lhu, extnOf(v.lhu));};


  // binary doModifyF functions
  WFuncWW andWord =   [&](W36 s1, W36 s2) {return s1.u & s2.u;};
  WFuncWW andCWord =  [&](W36 s1, W36 s2) {return s1.u & ~s2.u;};
  WFuncWW andCBWord = [&](W36 s1, W36 s2) {return ~s1.u & ~s2.u;};
  WFuncWW iorWord =   [&](W36 s1, W36 s2) {return s1.u | s2.u;};
  WFuncWW iorCWord =  [&](W36 s1, W36 s2) {return s1.u | ~s2.u;};
  WFuncWW iorCBWord = [&](W36 s1, W36 s2) {return ~s1.u | ~s2.u;};
  WFuncWW xorWord =   [&](W36 s1, W36 s2) {return s1.u ^ s2.u;};
  WFuncWW xorCWord =  [&](W36 s1, W36 s2) {return s1.u ^ ~s2.u;};
  WFuncWW xorCBWord = [&](W36 s1, W36 s2) {return ~s1.u ^ ~s2.u;};
  WFuncWW eqvWord =   [&](W36 s1, W36 s2) {return ~(s1.u ^ s2.u);};
  WFuncWW eqvCWord =  [&](W36 s1, W36 s2) {return ~(s1.u ^ ~s2.u);};
  WFuncWW eqvCBWord = [&](W36 s1, W36 s2) {return ~(~s1.u ^ ~s2.u);};


  WFuncWW addWord = [&](W36 s1, W36 s2) {
    int64_t sum = s1.ext64() + s2.ext64();

    if (sum < -(int64_t) W36::bit0) {
      flags.tr1 = flags.ov = flags.cy0 = 1;
    } else if ((uint64_t) sum >= W36::bit0) {
      flags.tr1 = flags.ov = flags.cy1 = 1;
    } else {

      if (s1.s < 0 && s2.s < 0) {
	flags.cy0 = flags.cy1 = 1;
      } else if ((s1.s < 0) != (s2.s < 0)) {
	const uint64_t mag1 = abs(s1.s);
	const uint64_t mag2 = abs(s2.s);

	if ((s1.s >= 0 && mag1 >= mag2) ||
	    (s2.s >= 0 && mag2 >= mag1)) {
	  flags.cy0 = flags.cy1 = 1;
	}
      }
    }

    return sum;
  };
    

  WFuncWW subWord = [&](W36 s1, W36 s2) {
    int64_t diff = s1.ext64() - s2.ext64();

    if (diff < -(int64_t) W36::bit0) {
      flags.tr1 = flags.ov = flags.cy0 = 1;
    } else if ((uint64_t) diff >= W36::bit0) {
      flags.tr1 = flags.ov = flags.cy1 = 1;
    }

    return diff;
  };
    
    
  DFuncWW mulWord = [&](W36 s1, W36 s2) {
    int128_t prod128 = (int128_t) s1.ext64() * s2.ext64();
    W72 prod = W72::fromMag((uint128_t) (prod128 < 0 ? -prod128 : prod128), prod128 < 0);

    if (s1.u == W36::bit0 && s2.u == W36::bit0) {
      flags.tr1 = flags.ov = 1;
      return W72{W36{1ull << 34}, W36{0}};
    }

    return prod;
  };
    

  WFuncWW imulWord = [&](W36 s1, W36 s2) {
    int128_t prod128 = (int128_t) s1.ext64() * s2.ext64();
    W72 prod = W72::fromMag((uint128_t) (prod128 < 0 ? -prod128 : prod128), prod128 < 0);

    if (s1.u == W36::bit0 && s2.u == W36::bit0) {
      flags.tr1 = flags.ov = 1;
    }


    return W36((prod.s < 0 ? W36::bit0 : 0) | ((W36::all1s >> 1) & prod.u));
  };

    
  DFuncWW idivWord = [&](W36 s1, W36 s2) {

    if ((s1.u == W36::bit0 && s2.s == -1ll) || s2.u == 0ull) {
      flags.ndv = flags.tr1 = flags.ov = 1;
      return W72{s1, s2};
    } else {
      int64_t quo = s1.s / s2.s;
      int64_t rem = abs(s1.s % s2.s);
      if (quo < 0) rem = -rem;
      return W72{W36{quo}, W36{abs(rem)}};
    }
  };

    
  DFuncDW divWord = [&](W72 s1, W36 s2) {
    uint128_t den70 = ((uint128_t) s1.hi35 << 35) | s1.lo35;
    auto dor = s2.mag;
    auto signBit = s1.s < 0 ? 1ull << 35 : 0ull;

    if (s1.hi35 >= s2.mag || s2.u == 0) {
      flags.ndv = flags.tr1 = flags.ov = 1;
      return s1;
    } else {
      int64_t quo = den70 / dor;
      int64_t rem = den70 % dor;
      W72 ret{
	W36{(int64_t) ((quo & W36::magMask) | signBit)},
	W36{(int64_t) ((rem & W36::magMask) | signBit)}};
      return ret;
    }
  };

    
  // Binary comparison predicates.
  BoolPredWW isLT    = [&](W36 v1, W36 v2) {return v1.ext64() <  v2.ext64();};
  BoolPredWW isLE    = [&](W36 v1, W36 v2) {return v1.ext64() <= v2.ext64();};
  BoolPredWW isGT    = [&](W36 v1, W36 v2) {return v1.ext64() >  v2.ext64();};
  BoolPredWW isGE    = [&](W36 v1, W36 v2) {return v1.ext64() >= v2.ext64();};
  BoolPredWW isNE    = [&](W36 v1, W36 v2) {return v1.ext64() != v2.ext64();};
  BoolPredWW isEQ    = [&](W36 v1, W36 v2) {return v1.ext64() == v2.ext64();};
  BoolPredWW always2 = [&](W36 v1, W36 v2) {return true;};
  BoolPredWW never2  = [&](W36 v1, W36 v2) {return false;};


  VoidFunc skipAction = [&] {++nextPC.rhu;};
  VoidFunc jumpAction = [&] {nextPC.rhu = ea.rhu;};


  // Instruction class implementations.
  void doBinOp(auto getSrc1F, auto getSrc2F, auto modifyF, auto putDstF) {
    auto result = modifyF(getSrc1F(), getSrc2F());
    if (!flags.ndv) putDstF(result);
  }


  void doTXXXX(auto get1F, auto get2F, auto modifyF, auto condF, auto storeF) {
    W36 a1 = get1F();
    W36 a2 = get2F();

    if (condF(a1, a2)) {
      logFlow("skip");
      ++nextPC.rhu;
    }
      
    storeF(modifyF(a1, a2));
  }


  void doHXXXX(auto getSrcF, auto getDstF, auto copyF, auto modifyF, auto putDstF) {
    putDstF(modifyF(copyF(getSrcF(), getDstF())));
  }


  void doMOVXX(auto getSrcF, auto modifyF, auto putDstF) {
    putDstF(modifyF(getSrcF()));
  }


  void doSETXX(auto getSrcF, auto modifyF, auto putDstF) {
    putDstF(modifyF(getSrcF()));
  }

  void doCAXXX(auto getSrc1F, auto getSrc2F, auto condF) {

    if (condF(getSrc1F().ext64(), getSrc2F().ext64())) {
      logFlow("skip");
      ++nextPC.rhu;
    }
  }

  void doJUMP(auto condF) {

    if (condF(acGet())) {
      logFlow("jump");
      nextPC.rhu = ea.rhu;
    }
  }


  void doSKIP(auto condF) {
    W36 eaw = memGet();

    if (condF(eaw)) {
      logFlow("skip");
      ++nextPC.rhu;
    }
      
    if (iw.ac != 0) acPut(eaw);
  }


  void doAOSXX(auto getF, const signed delta, auto putF, auto condF, auto actionF) {
    W36 v = getF();

    if (delta > 0) {		// Increment

      if (v.u == W36::all1s >> 1) {
	flags.tr1 = flags.ov = flags.cy1 = 1;
      } else if (v.ext64() == -1) {
	flags.cy0 = flags.cy1 = 1;
      }
    } else {			// Decrement

      if (v.u == W36::bit0) {
	flags.tr1 = flags.ov = flags.cy0 = 1;
      } else if (v.u != 0) {
	flags.cy0 = flags.cy1 = 1;
      }
    }

    v.s += delta;

    if (iw.ac != 0) acPut(v);
    putF(v);

    if (condF(v)) actionF();
  }


  void doPush(W36 v, W36 acN) {
    W36 ac = acGetN(acN);

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

    acPutN(ac, acN);
  }


  W36 doPop(unsigned acN) {
    W36 ac = acGetN(acN);
    W36 poppedWord;

    if (pc.isSection0() || ac.lhs < 0 || (ac.lhu & 0007777) == 0) {
      poppedWord = memGetN(ac.rhu);
      ac = W36(ac.lhu - 1, ac.rhu - 1);
      if (ac.lhs == -1) flags.tr2 = 1;
    } else {
      poppedWord = memGetN(ac.vma);
      ac = ac - 1;
    }

    acPutN(ac, acN);
    return poppedWord;
  }


  void logFlow(const char *msg) {
    if (logger.pc) logger.s << " [" << msg << "]";
  }


  void setDebugger(Debugger *p) { debuggerP = p; }


  InstructionResult doNYI();
  InstructionResult doIO();

  InstructionResult doLUUO();
  InstructionResult doMUUO();
  InstructionResult doDADD();
  InstructionResult doDSUB();
  InstructionResult doDMUL();
  InstructionResult doDDIV();
  InstructionResult doIBP_ADJBP();
  InstructionResult doILBP();
  InstructionResult doLDB();
  InstructionResult doIDPB();
  InstructionResult doDPB();
  InstructionResult doMOVE();
  InstructionResult doMOVEI();
  InstructionResult doMOVEM();
  InstructionResult doMOVES();
  InstructionResult doMOVS();
  InstructionResult doMOVSI();
  InstructionResult doMOVSM();
  InstructionResult doMOVSS();
  InstructionResult doMOVN();
  InstructionResult doMOVNI();
  InstructionResult doMOVNM();
  InstructionResult doMOVNS();
  InstructionResult doMOVM();
  InstructionResult doMOVMI();
  InstructionResult doMOVMM();
  InstructionResult doMOVMS();
  InstructionResult doIMUL();
  InstructionResult doIMULI();
  InstructionResult doIMULM();
  InstructionResult doIMULB();
  InstructionResult doMUL();
  InstructionResult doMULI();
  InstructionResult doMULM();
  InstructionResult doMULB();
  InstructionResult doIDIV();
  InstructionResult doIDIVI();
  InstructionResult doIDIVM();
  InstructionResult doIDIVB();
  InstructionResult doDIV();
  InstructionResult doDIVI();
  InstructionResult doDIVM();
  InstructionResult doDIVB();
  InstructionResult doASH();
  InstructionResult doROT();
  InstructionResult doLSH();
  InstructionResult doJFFO();
  InstructionResult doROTC();
  InstructionResult doLSHC();
  InstructionResult doEXCH();
  InstructionResult doBLT();
  InstructionResult doAOBJP();
  InstructionResult doAOBJN();
  InstructionResult doJRST();
  InstructionResult doJFCL();
  InstructionResult doPXCT();
  InstructionResult doPUSHJ();
  InstructionResult doPUSH();
  InstructionResult doPOP();
  InstructionResult doPOPJ();
  InstructionResult doJSR();
  InstructionResult doJSP();
  InstructionResult doJSA();
  InstructionResult doJRA();
  InstructionResult doADD();
  InstructionResult doADDI();
  InstructionResult doADDM();
  InstructionResult doADDB();
  InstructionResult doSUB();
  InstructionResult doSUBI();
  InstructionResult doSUBM();
  InstructionResult doSUBB();
  InstructionResult doCAI();
  InstructionResult doCAIL();
  InstructionResult doCAIE();
  InstructionResult doCAILE();
  InstructionResult doCAIA();
  InstructionResult doCAIGE();
  InstructionResult doCAIN();
  InstructionResult doCAIG();
  InstructionResult doCAM();
  InstructionResult doCAML();
  InstructionResult doCAME();
  InstructionResult doCAMLE();
  InstructionResult doCAMA();
  InstructionResult doCAMGE();
  InstructionResult doCAMN();
  InstructionResult doCAMG();
  InstructionResult doJUMP();
  InstructionResult doJUMPL();
  InstructionResult doJUMPE();
  InstructionResult doJUMPLE();
  InstructionResult doJUMPA();
  InstructionResult doJUMPGE();
  InstructionResult doJUMPN();
  InstructionResult doJUMPG();
  InstructionResult doSKIP();
  InstructionResult doSKIPL();
  InstructionResult doSKIPE();
  InstructionResult doSKIPLE();
  InstructionResult doSKIPA();
  InstructionResult doSKIPGE();
  InstructionResult doSKIPN();
  InstructionResult doSKIPGT();
  InstructionResult doAOJ();
  InstructionResult doAOJL();
  InstructionResult doAOJE();
  InstructionResult doAOJLE();
  InstructionResult doAOJA();
  InstructionResult doAOJGE();
  InstructionResult doAOJN();
  InstructionResult doAOJG();
  InstructionResult doAOS();
  InstructionResult doAOSL();
  InstructionResult doAOSE();
  InstructionResult doAOSLE();
  InstructionResult doAOSA();
  InstructionResult doAOSGE();
  InstructionResult doAOSN();
  InstructionResult doAOSG();
  InstructionResult doSOJ();
  InstructionResult doSOJL();
  InstructionResult doSOJE();
  InstructionResult doSOJLE();
  InstructionResult doSOJA();
  InstructionResult doSOJGE();
  InstructionResult doSOJN();
  InstructionResult doSOJG();
  InstructionResult doSOS();
  InstructionResult doSOSL();
  InstructionResult doSOSE();
  InstructionResult doSOSLE();
  InstructionResult doSOSA();
  InstructionResult doSOSGE();
  InstructionResult doSOSN();
  InstructionResult doSOSG();
  InstructionResult doSETZ();
  InstructionResult doSETZI();
  InstructionResult doSETZM();
  InstructionResult doSETZB();
  InstructionResult doAND();
  InstructionResult doANDI();
  InstructionResult doANDM();
  InstructionResult doANDB();
  InstructionResult doANDCA();
  InstructionResult doANDCAI();
  InstructionResult doANDCAM();
  InstructionResult doANDCAB();
  InstructionResult doSETM();
  InstructionResult doSETMI();
  InstructionResult doSETMM();
  InstructionResult doSETMB();
  InstructionResult doANDCM();
  InstructionResult doANDCMI();
  InstructionResult doANDCMM();
  InstructionResult doANDCMB();
  InstructionResult doSETA();
  InstructionResult doSETAI();
  InstructionResult doSETAM();
  InstructionResult doSETAB();
  InstructionResult doXOR();
  InstructionResult doXORI();
  InstructionResult doXORM();
  InstructionResult doXORB();
  InstructionResult doIOR();
  InstructionResult doIORI();
  InstructionResult doIORM();
  InstructionResult doIORB();
  InstructionResult doANDCBM();
  InstructionResult doANDCBMI();
  InstructionResult doANDCBMM();
  InstructionResult doANDCBMB();
  InstructionResult doEQV();
  InstructionResult doEQVI();
  InstructionResult doEQVM();
  InstructionResult doEQVB();
  InstructionResult doSETCA();
  InstructionResult doSETCAI();
  InstructionResult doSETCAM();
  InstructionResult doSETCAB();
  InstructionResult doORCA();
  InstructionResult doORCAI();
  InstructionResult doORCAM();
  InstructionResult doORCAB();
  InstructionResult doSETCM();
  InstructionResult doSETCMI();
  InstructionResult doSETCMM();
  InstructionResult doSETCMB();
  InstructionResult doORCM();
  InstructionResult doORCMI();
  InstructionResult doORCMM();
  InstructionResult doORCMB();
  InstructionResult doORCB();
  InstructionResult doORCBI();
  InstructionResult doORCBM();
  InstructionResult doORCBB();
  InstructionResult doSETO();
  InstructionResult doSETOI();
  InstructionResult doSETOM();
  InstructionResult doSETOB();
  InstructionResult doHLL();
  InstructionResult doHLLI();
  InstructionResult doHLLM();
  InstructionResult doHLLS();
  InstructionResult doHRL();
  InstructionResult doHRLI();
  InstructionResult doHRLM();
  InstructionResult doHRLS();
  InstructionResult doHLLZ();
  InstructionResult doHLLZI();
  InstructionResult doHLLZM();
  InstructionResult doHLLZS();
  InstructionResult doHRLZ();
  InstructionResult doHRLZI();
  InstructionResult doHRLZM();
  InstructionResult doHRLZS();
  InstructionResult doHLLO();
  InstructionResult doHLLOI();
  InstructionResult doHLLOM();
  InstructionResult doHLLOS();
  InstructionResult doHRLO();
  InstructionResult doHRLOI();
  InstructionResult doHRLOM();
  InstructionResult doHRLOS();
  InstructionResult doHLLE();
  InstructionResult doHLLEI();
  InstructionResult doHLLEM();
  InstructionResult doHLLES();
  InstructionResult doHRLE();
  InstructionResult doHRLEI();
  InstructionResult doHRLEM();
  InstructionResult doHRLES();
  InstructionResult doHRR();
  InstructionResult doHRRI();
  InstructionResult doHRRM();
  InstructionResult doHRRS();
  InstructionResult doHLR();
  InstructionResult doHLRI();
  InstructionResult doHLRM();
  InstructionResult doHLRS();
  InstructionResult doHRRZ();
  InstructionResult doHRRZI();
  InstructionResult doHRRZM();
  InstructionResult doHRRZS();
  InstructionResult doHLRZ();
  InstructionResult doHLRZI();
  InstructionResult doHLRZM();
  InstructionResult doHLRZS();
  InstructionResult doHRRO();
  InstructionResult doHRROI();
  InstructionResult doHRROM();
  InstructionResult doHRROS();
  InstructionResult doHLRO();
  InstructionResult doHLROI();
  InstructionResult doHLROM();
  InstructionResult doHLROS();
  InstructionResult doHRRE();
  InstructionResult doHRREI();
  InstructionResult doHRREM();
  InstructionResult doHRRES();
  InstructionResult doHLRE();
  InstructionResult doHLREI();
  InstructionResult doHLREM();
  InstructionResult doHLRES();
  InstructionResult doTRN();
  InstructionResult doTLN();
  InstructionResult doTRNE();
  InstructionResult doTLNE();
  InstructionResult doTRNA();
  InstructionResult doTLNA();
  InstructionResult doTRNN();
  InstructionResult doTLNN();
  InstructionResult doTRZ();
  InstructionResult doTLZ();
  InstructionResult doTRZE();
  InstructionResult doTLZE();
  InstructionResult doTRZA();
  InstructionResult doTLZA();
  InstructionResult doTRZN();
  InstructionResult doTLZN();
  InstructionResult doTRC();
  InstructionResult doTLC();
  InstructionResult doTRCE();
  InstructionResult doTLCE();
  InstructionResult doTRCA();
  InstructionResult doTLCA();
  InstructionResult doTRCN();
  InstructionResult doTLCN();
  InstructionResult doTRO();
  InstructionResult doTLO();
  InstructionResult doTROE();
  InstructionResult doTLOE();
  InstructionResult doTROA();
  InstructionResult doTLOA();
  InstructionResult doTRON();
  InstructionResult doTLON();
  InstructionResult doTDN();
  InstructionResult doTSN();
  InstructionResult doTDNE();
  InstructionResult doTSNE();
  InstructionResult doTDNA();
  InstructionResult doTSNA();
  InstructionResult doTDNN();
  InstructionResult doTSNN();
  InstructionResult doTDZ();
  InstructionResult doTSZ();
  InstructionResult doTDZE();
  InstructionResult doTSZE();
  InstructionResult doTDZA();
  InstructionResult doTSZA();
  InstructionResult doTDZN();
  InstructionResult doTSZN();
  InstructionResult doTDC();
  InstructionResult doTSC();
  InstructionResult doTDCE();
  InstructionResult doTSCE();
  InstructionResult doTDCA();
  InstructionResult doTSCA();
  InstructionResult doTDCN();
  InstructionResult doTSZCN();
  InstructionResult doTDO();
  InstructionResult doTSO();
  InstructionResult doTDOE();
  InstructionResult doTSOE();
  InstructionResult doTDOA();
  InstructionResult doTSOA();
  InstructionResult doTDON();
  InstructionResult doTSON();


  ////////////////////////////////////////////////////////////////////////////////
  // The instruction emulator. Call this to start, step, or continue
  // running.
  void emulate(Debugger *debugger);

protected:
  uint16_t getWord(ifstream &inS, [[maybe_unused]] const char *whyP);
};
