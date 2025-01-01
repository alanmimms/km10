// This is the KM10 CPU implementation.

/*
  Some notes:

  * INVARIANT: Debugger always shows next instruction to execute in
    its prompt.

    * Interrupt vector instruction.
    * Exception vector instruction.
    * XCT next instruction in chain.
    * Normal code flow instruction.

  * `pc` always points to instruction that is about to execute in
    normal non-trap/non-interrupt/non-XCT-chain case. PC is
    incremented so that if instruction saves the PC it saves the
    incremented value.

  * In XCT chain, PC points to instruction after the chain's
    initiating XCT no matter how long the chain is. So for

    FOO:   XCT -> XCT -> ... -> JSP

    the JSP will save FOO+1 as its return address.

  * When handling an interrupt, PC points to the instruction that
    didn't execute so the interrupt handler could be invoked instead.
    When handling a trap/page fault/xUUO, PC points to the instruction
    that caused the trap.
*/

/*
  TODO:

  * Globally use U32,S32, U64,S64, U128,S128 typedefs instead of
    verbose <cstdint> names.

  * Fix the logging stuff. It's seriously broken/redundant/wrong.
*/

/*
  TODO:

  Reconfigure so that execute1() runs a single instruction and
  returns a value indicating the instruction's effect:

  * Normal
  * Skip
  * MUUO/LUUO/JSYS
  * Trap
  * HALT
  * XCT
   etc...

  Interrupts are checked in the loop surrounding execute1() and
  dispatched by changing which instruction is executed.
*/

#pragma once
#include <array>
#include <assert.h>
#include <unordered_set>
#include <atomic>
#include <string>

using namespace std;

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
#include "iresult.hpp"


class KM10 {

public:
  APRDevice apr;
  CCADevice cca;
  MTRDevice mtr;
  PAGDevice pag;
  PIDevice pi;
  TIMDevice tim;
  DTE20 dte;

  struct NoDevice: Device {
    NoDevice(unsigned anAddr, KM10 &cpu)
      : Device(anAddr, "?no-device?", cpu)
    {}

    unsigned genericConditions: 18;
    virtual unsigned getConditions();
    virtual void putConditions(unsigned v);
  } noDevice;

  Debugger debugger;


  using BreakpointTable = unordered_set<unsigned>;


  // This is an implementation of an opcode to be saved in the ops[].
  using OpcodeHandler = IResult (KM10::*)();


  // This is indexed by opcode, giving the method to call for that
  // opcode. I'm using a C style array so I can use a designated
  // initializer. I make it static and bind the instance "this" at
  // time of call with ".*" since the elements point to instance
  // methods.
  array<OpcodeHandler, 512> ops;

  // Constructor and destructor
  KM10(unsigned nMemoryWords,
       BreakpointTable &aOBPs,
       BreakpointTable &aGBPs,
       BreakpointTable &aPBPs,
       BreakpointTable &eBPs);

  ~KM10();


  W36 pc;	      // PC of instr we fetched before trap,int,XCT-chain.
  W36 iw;	      // Instruction word we're executing.
  W36 ea;	      // Effective address (always calculated whether used or not).
  W36 fetchPC;	      // Addr cur instr came from, target for skip/jump/XCT/xUUO/trap.

  // Offset to add to PC at end of instruction: zero for JUMPs, two
  // for SKIPs, one for normal. For traps, the trap vector address is
  // placed here.
  unsigned pcOffset;

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

  // KL10 has 8 blocks of 16 ACs.
  W36 ACBlocks[8][16];

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

    string toString();
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
    W36 kernNoTrapMUUOPC;	// 430
    W36 kernTrapMUUOPC;		// 431
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
  int64_t nSteps;
  uint64_t nInsns;
  unordered_set<unsigned> &opBPs;	// Opcode breakpoints
  unordered_set<unsigned> &addressGBPs; // Address GET breakpoints
  unordered_set<unsigned> &addressPBPs; // Address PUT breakpoints
  unordered_set<unsigned> &executeBPs;	// Execution breakpoints
  uint64_t instructionCounter;
  uint64_t runNS;

  // Call by PAG when DATAO changes current AC block number.
  void updateACBlock(unsigned acBlock);


  // Return the KM10 memory VIRTUAL address (EPT is in kernel virtual
  // space) for the specified pointer into the EPT.
  W36 eptAddressFor(const W36 *eptEntryP);

  // AC and memory accessors.
  W36 acGet();
  W36 acGetRH();
  W36 acGetLH();
  void acPut(W36 v);
  void acPutRH(W36 v);
  void acPutLH(W36 v);
  W72 acGet2();
  void acPut2(W72 v);
  W36 memGet();
  void memPut(W36 value);
  void bothPut2(W72 v);
  void memPutHi(W72 v);
  W36 immediate();

  W36 acGetN(unsigned n);
  W36 acGetEA(unsigned n);
  void acPutN(W36 value, unsigned n);
  W36 memGetN(W36 a);
  W36 uptGetN(unsigned uptWordOffset);
  void memPutN(W36 value, W36 a);
  void uptPutN(W36 value, unsigned uptWordOffset);

  // Effective address calculation.
  uint64_t getEA(unsigned i, unsigned x, uint64_t y);

  // Accessors
  bool userMode();
  W36 flagsWord(unsigned pc);

  // Fixups for flags for add and subtract operations.
  IResult setADDFlags(W36 a, W36 b, W36 sum);
  IResult setSUBFlags(W36 a, W36 b, W36 diff);

  // Used by JRSTF and JEN
  void restoreFlags(W36 ea);

  tuple<unsigned, unsigned> loadA10(const char *fileNameP);


  // This is how our subclasses (separately compiled) install their
  // OpcodeHandlers.
  inline void defOp(unsigned op, const char *mneP, OpcodeHandler impl) {
    ops[op] = impl;
  }

  IResult doILLEGAL();

  void logFlow(const char *msg);


  ////////////////////////////////////////////////////////////////////////////////
  // The instruction emulator. Call this to start, step, or continue
  // running.
  void emulate();
};
