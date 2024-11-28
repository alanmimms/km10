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
#include <atomic>

using namespace std;
using namespace fmt;

#include "word.hpp"
#include "apr.hpp"
#include "cca.hpp"
#include "mtr.hpp"
#include "pag.hpp"
#include "pi.hpp"
#include "tim.hpp"
#include "dte20.hpp"
#include "device.hpp"
#include "debugger.hpp"
#include "instruction-result.hpp"


class KM10 {
public:
  APRDevice apr;
  CCADevice cca;
  MTRDevice mtr;
  PAGDevice pag;
  PIDevice pi;
  TIMDevice tim;
  DTE20 dte;
  Device noDevice;
  Debugger debugger;


  using BreakpointTable = unordered_set<unsigned>;


  // Type for the function that implements an opcode.
  using InstructionF = InstructionResult (KM10::*)();


  // This is indexed by opcode, giving the method to call for that
  // opcode. I'm using a C style array so I can use a designated
  // initializer. I make it static and bind the instance "this" at
  // time of call with ".*" since the elements point to instance
  // methods.
  static InstructionF ops[512];


  // Constructor and destructor
  KM10(unsigned nMemoryWords, BreakpointTable &aBPs, BreakpointTable &eBPs);
  ~KM10();


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


  W36 acGet();
  W36 acGetRH();

  // This retrieves LH into the RH of the return value, which is
  // required for things like TLNN to work properly since they use
  // the EA as a mask.
  W36 acGetLH();
  void acPut(W36 w);
  void acPutRH(W36 w);

  // This is used to store back in, e.g., TLZE. But the LH of AC is
  // in the RH of the word we're passed because of how the testing
  // logic of these instructions must work. So we put the RH of the
  // value into the LH of the AC, keeping the AC's RH intact.
  void acPutLH(W36 w);
  W72 acGet2();
  void acPut2(W72 d);
  W36 memGet();
  void memPut(W36 w);
  void selfPut(W36 w);
  void bothPut(W36 w);
  void bothPut2(W72 d);
  W36 swap(W36 w);

  W36 negate(W36 w);
  W36 magnitude(W36 w);
  W36 memGetSwapped();
  void memPutHi(W72 d);
  W36 immediate();


  // Condition testing predicates
  bool isLT0(W36 w);
  bool isLE0(W36 w);
  bool isGT0(W36 w);
  bool isGE0(W36 w);
  bool isNE0(W36 w);
  bool isEQ0(W36 w);
  bool always(W36 w);
  bool never(W36 w);

  bool isNE0T(W36 a, W36 b);
  bool isEQ0T(W36 a, W36 b);
  bool alwaysT(W36 a, W36 b);
  bool neverT(W36 a, W36 b);


  W36 getE();
  W36 noMod1(W36 w);
  W36 noMod2(W36 a, W36 b);


  // There is no `zeroMaskL`, `compMaskR`, `onesMaskL` because,
  // e.g., TLZE operates on the LH of the AC while it's in the RH of
  // the value so the testing/masking work properly.
  W36 zeroMaskR(W36 a, W36 b);
  W36 zeroMask(W36 a, W36 b);

  W36 onesMaskR(W36 a, W36 b);
  W36 onesMask(W36 a, W36 b);

  W36 compMaskR(W36 a, W36 b);
  W36 compMask(W36 a, W36 b);

  W36 zeroWord(W36 a);
  W36 onesWord(W36 a);
  W36 compWord(W36 a);

  void noStore(W36 w);


  // For a given low halfword, this computes an upper halfword by
  // extending the low halfword's sign.
  unsigned extnOf(const unsigned);


  // doCopyF functions
  W36 copyHRR(W36 a, W36 b);
  W36 copyHRL(W36 a, W36 b);
  W36 copyHLL(W36 a, W36 b);
  W36 copyHLR(W36 a, W36 b);


  // doModifyF functions
  W36 zeroR(W36 a);
  W36 onesR(W36 a);
  W36 extnR(W36 a);
  W36 zeroL(W36 a);
  W36 onesL(W36 a);
  W36 extnL(W36 a);


  // binary doModifyF functions
  W36 andWord(W36 a, W36 b);
  W36 andCWord(W36 a, W36 b);
  W36 andCBWord(W36 a, W36 b);
  W36 iorWord(W36 a, W36 b);
  W36 iorCWord(W36 a, W36 b);
  W36 iorCBWord(W36 a, W36 b);
  W36 xorWord(W36 a, W36 b);
  W36 xorCWord(W36 a, W36 b);
  W36 xorCBWord(W36 a, W36 b);
  W36 eqvWord(W36 a, W36 b);
  W36 eqvCWord(W36 a, W36 b);
  W36 eqvCBWord(W36 a, W36 b);


  W36 addWord(W36 a, W36 b);
  W36 subWord(W36 a, W36 b);
  W72 mulWord(W36 a, W36 b);
  W36 imulWord(W36 a, W36 b);
  W72 idivWord(W36 a, W36 b);
  W72 divWord(W72 a, W36 b);

    
  // Binary comparison predicates.
  bool isLT(W36 a, W36 b);
  bool isLE(W36 a, W36 b);
  bool isGT(W36 a, W36 b);
  bool isGE(W36 a, W36 b);
  bool isNE(W36 a, W36 b);
  bool isEQ(W36 a, W36 b);
  bool always2(W36 a, W36 b);
  bool never2(W36 a, W36 b);


  void skipAction();
  void jumpAction();


  // Genericized instruction class implementations.
  void doBinOp(auto getSrc1F, auto getSrc2F, auto modifyF, auto putDstF);
  void doTXXXX(auto get1F, auto get2F, auto modifyF, auto condF, auto storeF);
  void doHXXXX(auto getSrcF, auto getDstF, auto copyF, auto modifyF, auto putDstF);
  void doMOVXX(auto getSrcF, auto modifyF, auto putDstF);
  void doSETXX(auto getSrcF, auto modifyF, auto putDstF);
  void doCAXXX(auto getSrc1F, auto getSrc2F, auto condF);

  void doJUMP(auto condF);
  void doSKIP(auto condF);
  void doAOSXX(auto getF, const signed delta, auto putF, auto condF, auto actionF);

  void doPush(W36 v, W36 acN);
  W36 doPop(unsigned acN);
  void logFlow(const char *msg);

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
  void emulate();


  // static singleton with constructor to populate the ops[] static table.
  struct OpsInitializer {
    OpsInitializer();
  };

  static OpsInitializer opsInitializer;
};
