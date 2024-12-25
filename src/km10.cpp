#include <iostream>
#include <assert.h>
#include <unordered_map>
#include <sys/mman.h>
#include <cstdint>
#include <sstream>
#include <iomanip>
#include <fstream>
#include <ostream>
#include <limits>
#include <string>
#include <ctime>
using namespace std;

#if 0
#include <fmt/core.h>
using namespace fmt;
#endif

#include <CLI/CLI.hpp>

#include "km10.hpp"
#include "logger.hpp"
#include "iresult.hpp"
#include "bytepointer.hpp"


Logger logger{};


// We keep these breakpoint sets outside of the looped main and not
// part of KM10 or Debugger object so they stick across restart.
static KM10::BreakpointTable aOBPs;
static KM10::BreakpointTable aGBPs;
static KM10::BreakpointTable aPBPs;
static KM10::BreakpointTable eBPs;


extern void InstallAOxSOxGroup(KM10 &km10);
extern void InstallBitRotGroup(KM10 &km10);
extern void InstallByteGroup(KM10 &km10);
extern void InstallCmpAndGroup(KM10 &km10);
extern void InstallDWordGroup(KM10 &km10);
extern void InstallHalfGroup(KM10 &km10);
extern void InstallIncJSGroup(KM10 &km10);
extern void InstallIntBinGroup(KM10 &km10);
extern void InstallIOGroup(KM10 &km10);
extern void InstallJumpGroup(KM10 &km10);
extern void InstallMoveGroup(KM10 &km10);
extern void InstallMulDivGroup(KM10 &km10);
extern void InstallTstSetGroup(KM10 &km10);
extern void InstallUUOsGroup(KM10 &km10);


////////////////////////////////////////////////////////////////
// Constructor
KM10::KM10(unsigned nMemoryWords,
	   KM10::BreakpointTable &aOBPs,
	   KM10::BreakpointTable &aGBPs,
	   KM10::BreakpointTable &aPBPs,
	   KM10::BreakpointTable &eBPs)
  : apr{*this},
    cca{*this},
    mtr{*this},
    pag{*this},
    pi {*this},
    tim{*this},
    dte{040, *this},
    noDevice{0777777ul, "?NoDevice?", *this},
    debugger{*this},
    pc(0),
    iw(0),
    ea(0),
    fetchPC(0),
    pcOffset(0),
    running(false),
    restart(false),
    ACBlocks{},
    flags(0u),
    inInterrupt(false),
    era(0u),
    AC(ACBlocks[0]),
    memorySize(nMemoryWords),
    nSteps(0),
    opBPs(aOBPs),
    addressGBPs(aGBPs),
    addressPBPs(aPBPs),
    executeBPs(eBPs),
    instructionCounter(0),
    runNS(0)
{
  // THIS MUST BE FIRST so that all UUOs are MUUOs by default.
  InstallUUOsGroup(*this);

  InstallAOxSOxGroup(*this);
  InstallBitRotGroup(*this);
  InstallByteGroup(*this);
  InstallCmpAndGroup(*this);
  InstallDWordGroup(*this);
  InstallHalfGroup(*this);
  InstallIncJSGroup(*this);
  InstallIntBinGroup(*this);
  InstallIOGroup(*this);
  InstallJumpGroup(*this);
  InstallMoveGroup(*this);
  InstallMulDivGroup(*this);
  InstallTstSetGroup(*this);

  // Note this anonymous mmap() implicitly zeroes the virtual memory.
  physicalP = (W36 *) mmap(nullptr,
			   memorySize * sizeof(uint64_t),
			   PROT_READ | PROT_WRITE,
			   MAP_PRIVATE | MAP_ANONYMOUS,
			   0, 0);
  assert(physicalP != nullptr);

  // Initially we have no virtual addressing, so virtual == physical
  // and EPT and UPT are mapped to physical addresses based at zero.
  memP = physicalP;
  eptP = (ExecutiveProcessTable *) memP;
  uptP = (UserProcessTable *) memP;
}


void KM10::updateACBlock(unsigned newBlock) {
  AC = ACBlocks[newBlock];
}


string KM10::ProgramFlags::toString() {
  ostringstream ss;
  ss << oct << setw(6) << setfill('0') << ((unsigned) u << 5);
  if (ndv) ss << " NDV";
  if (fuf) ss << " FUF";
  if (tr1) ss << " TR1";
  if (tr2) ss << " TR2";
  if (afi) ss << " AFI";
  if (pub) ss << " PUB";
  if (uio) ss << " UIO/PCU";
  if (usr) ss << " USR";
  if (fpd) ss << " FPD";
  if (fov) ss << " FOV";
  if (cy1) ss << " CY1";
  if (cy0) ss << " CY0";
  if (ov ) ss << " OV/PCP";
  return ss.str();
}


// Implement the helpers used by the various instruction emulator
// functions.
W36 KM10::acGet() {
  return acGetN(iw.ac);
}


W36 KM10::acGetRH() {
  W36 v{0, acGet().rhu};
  if (logger.mem) logger.s << "; acRH" << oct << iw.ac << ": " << v.fmt18();
  return v;
}


// This retrieves LH into the RH of the return value, which is
// required for things like TLNN to work properly since they use
// the EA as a mask.
W36 KM10::acGetLH() {
  W36 v{0, acGet().lhu};
  if (logger.mem) logger.s << "; acLH" << oct << iw.ac << ": " << v.fmt18();
  return v;
}


void KM10::acPut(W36 v) {
  acPutN(v, iw.ac);
}


void KM10::acPutRH(W36 v) {
  acPut(W36(acGet().lhu, v.rhu));
}


// This is used to store back in, e.g., TLZE. But the LH of AC is in
// the RH of the word we're passed because of how the testing logic of
// these instructions must work. So we put the RH of the value into
// the LH of the AC, keeping the AC's RH intact.
void KM10::acPutLH(W36 v) {
  acPut(W36(v.rhu, acGet().rhu));
}


W72 KM10::acGet2() {
  W72 ret{acGetN(iw.ac+0), acGetN(iw.ac+1)};
  return ret;
}


void KM10::acPut2(W72 v) {
  acPutN(v.hi, iw.ac+0);
  acPutN(v.lo, iw.ac+1);
}


W36 KM10::memGet() {
  return memGetN(ea);
}


void KM10::memPut(W36 v) {
  memPutN(v, ea);
}


void KM10::bothPut2(W72 v) {
  acPutN(v.hi, iw.ac+0);
  acPutN(v.lo, iw.ac+1);
  memPut(v.hi);
}


void KM10::memPutHi(W72 v) {
  memPut(v.hi);
}


W36 KM10::immediate() {
  return W36(pc.isSection0() ? 0 : ea.lhu, ea.rhu);
}


////////////////////////////////////////////////////////////////
KM10::~KM10() {
  // XXX this eventually must tear down kernel and user virtual
  // mappings as well.
  if (physicalP) munmap(physicalP, memorySize * sizeof(uint64_t));
}


////////////////////////////////////////////////////////////////
// Return the KM10 memory VIRTUAL address (EPT is in kernel virtual
// space) for the specified pointer into the EPT.
W36 KM10::eptAddressFor(const W36 *eptEntryP) {
  return W36(eptEntryP - (W36 *) eptP);
}


W36 KM10::acGetN(unsigned n) {
  assert(n < 16);
  W36 value = AC[n];
  if (logger.mem) logger.s << "; ac" << oct << n << ":" << value.fmt36();
  return value;
}


W36 KM10::acGetEA(unsigned n) {
  assert(n < 16);
  W36 value = AC[n];
  if (logger.mem) logger.s << "; ac" << oct << n << ":" << value.fmt36();
  return value;
}


void KM10::acPutN(W36 value, unsigned n) {
  assert(n < 16);
  AC[n] = value;
  if (logger.mem) logger.s << "; ac" << oct << n << "=" << value.fmt36();
}


W36 KM10::memGetN(W36 a) {
  W36 value = a.rhu < 020 ? acGetEA(a.rhu) : memP[a.rhu];
  if (logger.mem) logger.s << "; " << a.fmtVMA() << ":" << value.fmt36();
  if (addressGBPs.contains(a.vma)) running = false;
  return value;
}


void KM10::memPutN(W36 value, W36 a) {

  if (a.rhu < 020)
    acPutN(value, a.rhu);
  else 
    memP[a.rhu] = value;

  if (logger.mem) logger.s << "; " << a.fmtVMA() << "=" << value.fmt36();
  if (addressPBPs.contains(a.vma)) running = false;
}


void KM10::uptPutN(W36 value, unsigned uptWordOffset) {
  W36 memWordAddr = ((W36 *) uptP - (W36 *) memP) + uptWordOffset;
  memPutN(value, memWordAddr);
}


W36 KM10::uptGetN(unsigned uptWordOffset) {
  W36 memWordAddr = (W36 *) uptP - (W36 *) memP;
  W36 result = memGetN(memWordAddr + uptWordOffset);
  return result;
}


// Effective address calculation
uint64_t KM10::getEA(unsigned i, unsigned x, uint64_t y) {

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
bool KM10::userMode() {return !!flags.usr;}


// This builds and returns a flags word with the specified PC in the
// RH or VMA.
W36 KM10::flagsWord(unsigned pc) {
  W36 v(pc);
  v.pcFlags = flags.u;
  return v;
}


// Used by JRSTF and JEN
void KM10::restoreFlags(W36 ea) {
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

static uint16_t getWord(ifstream &inS, [[maybe_unused]] const char *whyP) {
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
void KM10::loadA10(const char *fileNameP) {
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
	  logger.s << "mem[" << a36.fmtVMA() << "]=" << w36.fmt36()
		   << " " << w36.disasm(nullptr)
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


static uint64_t getCPUTimeNS() {
  struct timespec ts;
  if (clock_gettime(CLOCK_PROCESS_CPUTIME_ID, &ts) != 0) throw runtime_error("Failed to get CPU time");
  return (uint64_t) ts.tv_sec * 1000ll*1000ll*1000ll + ts.tv_nsec;
}


////////////////////////////////////////////////////////////////
void KM10::emulate() {
  uint64_t startNS = getCPUTimeNS();	// Beginning of Time

  ////////////////////////////////////////////////////////////////
  // Connect our DTE20 (put console into raw mode)
  dte.connect();

  // The instruction loop.
  fetchPC = pc;

  for (;;) {

    // Keep the cache sweep timer ticking until it goes DING.
    cca.handleSweep();

    // Handle execution breakpoints.
    if (executeBPs.contains(fetchPC.vma)) running = false;

    // Prepare to fetch next iw and remember if it's an interrupt or
    // trap.
    if ((flags.tr1 || flags.tr2) && pag.pagerEnabled()) {
      // We have a trap.
      fetchPC = eptAddressFor(flags.tr1 ? &eptP->trap1Insn : &eptP->stackOverflowInsn);
      inInterrupt = true;
      if (logger.ints) logger.s << ">>>>> trap cycle PC now=" << pc.fmtVMA()
				<< logger.endl << flush;
    } else if (W36 vec = pi.setUpInterruptCycleIfPending(); vec != W36(0)) {
      // We have an active interrupt.
      fetchPC = vec;
      inInterrupt = true;
      if (logger.ints) logger.s << ">>>>> interrupt cycle PC=" << pc.fmtVMA()
				<< "  vector=" << fetchPC.fmtVMA()
				<< logger.endl << flush;
    }

    // Fetch the instruction and save PC (fetch, really) history.
    ++instructionCounter;
    iw = memGetN(fetchPC);
    debugger.pcRing.add(fetchPC);

    if (opBPs.contains(iw.op)) running = false;

    // If we're debugging, this is where we pause to let the user
    // inspect and change things. The debugger tells us what our next
    // action should be based on its return value.
    if (!running) {
      runNS += getCPUTimeNS() - startNS;

      switch (debugger.debug()) {
      case Debugger::step:		// Debugger has set step count in nSteps.
	break;

      case Debugger::run:		// Continue from current PC.
	break;

      case Debugger::quit:		// Quit from emulator.
	return;

      case Debugger::restart:		// Restart emulator - total reboot
	return;

      case Debugger::pcChanged:		// PC changed by debugger - go fetch again
	fetchPC = pc;
	startNS = getCPUTimeNS();	// Restart time - debugger is exiting.
	continue;

      default:				// This should never happen...
	assert("Debugger returned unknown action" == nullptr);
	return;
      }

      startNS = getCPUTimeNS();		// Restart time - debugger is exiting.
    }


    // Handle nSteps so we don't keep running if we run out of step
    // count. THIS instruction is our single remaining step. If
    // nSteps is zero we just keep going "forever".
    if (nSteps > 0) {
      if (--nSteps <= 0) running = false;
    }

    if (logger.loggingToFile && logger.pc) {
      logger.s << fetchPC.fmtVMA() << ": " << debugger.dump(iw, fetchPC);
    }

    // Compute effective address.
    ea.u = getEA(iw.i, iw.x, iw.y);

    // Execute the instruction in `iw`.
    IResult result = (this->*ops[iw.op])();

    // If we "continue" we have to set up `fetchPC` to point to the
    // instruction to fetch and execute next. If we "break" (from the
    // switch case) we use `fetchPC` + pcOffset.
    switch (result) {
    case iNormal:

      // If we're in an interrupt vector and we get iNormal, just
      // fetch next word in the vector.
      if (inInterrupt) {
	fetchPC = fetchPC.vma + 1; // Increment to second word in vector.
	pcOffset = 0;
	continue;
      } else {
	pcOffset = 1;
      }

      break;

    case iSkip:
      // Any skip instruction that skips.

      // If we're in an interrupt vector and we get iSkip, vector is
      // done so resume normal flow.
      if (inInterrupt) {
	pcOffset = 1;
	inInterrupt = false;
      } else {
	pcOffset = 2;
      }

      break;

    case iJump:
      // In this case, jump instructions put destination in `ea` for
      // "free". Make sure we don't stay in interrupt vector if we're
      // in one.
      inInterrupt = false;
      pcOffset = 0;
      pc = ea;
      break;

    case iTrap:			// Advance and THEN handle trap.
      break;

    case iMUUO:
    case iLUUO:
      // All of these cases require that we fetch the next instruction
      // from a specified location (already in `fetchPC`) and loop
      // back to execute that instruction with PC left pointing to
      // next word for the monitor to use to figure out the UUO.
      pcOffset = 1;
      continue;

    case iXCT:
      fetchPC = ea;		// Move forward in XCT chain.
      continue;

    case iHALT:
      pcOffset = 0;		// Leave PC at HALT instruction.
      running = false;
      break;

    case iNoSuchDevice:
    case iNYI:
      pcOffset = 1;		// Should treat like iNormal?
      break;
    }

    if (logger.pc || logger.mem || logger.ac || logger.io || logger.dte)
      logger.s << logger.endl << flush;

    // If we get here we just offset the PC by `pcOffset` and loop to
    // fetch next instruction.
    pc.vma = fetchPC.vma = pc.vma + pcOffset;
  }

  // Restore console to normal
  dte.disconnect();
}


//////////////////////////////////////////////////////////////
// This is invoked in a loop to allow the "restart" command to work
// properly. Therefore this needs to clean up the state of the machine
// before it returns. This is mostly done by auto destructors.
static int loopedMain(int argc, char *argv[]) {
  CLI::App app;

  // Definitions for our command line options
  app.option_defaults()->always_capture_default();

  bool dVal{true};
  //    app.add_option("-d,--debug", dVal, "run the built-in debugger instead of starting execution");

  unsigned mVal{4096};
  app.add_option("-m", mVal, "Size (in Kwords) of KM10 main memory")
    ->check([](const string &str) {
      unsigned size = atol(str.c_str());

      if (size >= 256 && size <= 4096 && (size & 0xFF) == 0) {
	return string{};	// Good value!
      } else {
	return string{"The '-m' size in Kwords must be a multiple of 256 from 256K to 4096K words."};
      }
    });
  
  string lVal{"../images/klad/dfkaa.a10"};
  app.add_option("-l,--load", lVal, ".A10 or .SAV file to load")
    ->check(CLI::ExistingFile);

  string rVal{"../images/klad/dfkaa.rel"};
  app.add_option("-r,--rel", rVal, ".REL file to load symbols from")
    ->check(CLI::ExistingFile);

  try {
    app.parse(argc, argv);
  } catch(const CLI::Error &e) {
    cerr << "Command line error: " << endl << flush;
    return app.exit(e);
  }

  KM10 km10(mVal*1024, aOBPs, aGBPs, aPBPs, eBPs);
  assert(sizeof(*km10.eptP) == 512 * 8);
  assert(sizeof(*km10.uptP) == 512 * 8);

  if (lVal.ends_with(".a10")) {
    km10.loadA10(lVal.c_str());
  } else if (lVal.ends_with(".sav")) {
    cerr << "ERROR: For now, '--load' option must name a .a10 file" << logger.endl;
    return -1;
  } else {
    cerr << "ERROR: '--load' option must name a .a10 file" << logger.endl;
    return -1;
  }

  cerr << "[Loaded " << lVal << "  start=" << km10.pc.fmtVMA() << "]" << logger.endl;

  if (rVal != "none") {
    km10.debugger.loadREL(rVal.c_str());
  }

  // Make sure we defined very ops entry.
  if (dVal) for (unsigned op=0; op < 512; ++op) assert(km10.ops[op] != nullptr);

  km10.running = !dVal;
  km10.emulate();

  return km10.restart ? 1 : 0;
}


////////////////////////////////////////////////////////////////
int main(int argc, char *argv[]) {
  int st;
  
  while ((st = loopedMain(argc, argv)) > 0) {
    cerr << endl << "[restarting]" << endl;
  }

  return st;
}
