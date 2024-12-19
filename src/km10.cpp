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
using namespace std;

#include <fmt/core.h>
using namespace fmt;

#include <CLI/CLI.hpp>

#include "km10.hpp"
#include "logger.hpp"
#include "instruction-result.hpp"
#include "bytepointer.hpp"


Logger logger{};


// We keep these breakpoint sets outside of the looped main and not
// part of KM10 or Debugger object so they stick across restart.
static KM10::BreakpointTable aBPs;
static KM10::BreakpointTable eBPs;


extern void InstallBitRotGroup(KM10 &km10);
extern void InstallByteGroup(KM10 &km10);
extern void InstallCmpAndGroup(KM10 &km10);
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
KM10::KM10(unsigned nMemoryWords, KM10::BreakpointTable &aBPs, KM10::BreakpointTable &eBPs)
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
    ACbanks{},
    flags(0u),
    era(0u),
    AC(ACbanks[0]),
    memorySize(nMemoryWords),
    nSteps(0),
    addressBPs(aBPs),
    executeBPs(eBPs)
{
  // Install each instruction group's handlers in the ops array.
  ops.fill(nullptr);
  InstallBitRotGroup(*this);
  InstallByteGroup(*this);
  InstallCmpAndGroup(*this);
  InstallHalfGroup(*this);
  InstallIncJSGroup(*this);
  InstallIntBinGroup(*this);
  InstallIOGroup(*this);
  InstallJumpGroup(*this);
  InstallMoveGroup(*this);
  InstallMulDivGroup(*this);
  InstallTstSetGroup(*this);
  InstallUUOsGroup(*this);

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


string KM10::ProgramFlags::toString() {
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
#if 0
InstructionResult KM10::doNYI() {
  cerr << "Not yet implemented: " << oct << iw.op << logger.endl << flush;
  return iNormal;
}


// XXX do we need to implement the skipping IO instructions somehow
// here? I think so.
InstructionResult KM10::doIO() {
  if (logger.io) logger.s << "; ioDev=" << oct << iw.ioDev << " ioOp=" << oct << iw.ioOp;
  return Device::handleIO(iw, ea);
}


InstructionResult KM10::doDADD() {
  auto a1 = W72{memGetN(ea.u+0), memGetN(ea.u+1)};
  auto a2 = W72{acGetN(iw.ac+0), acGetN(iw.ac+1)};

  int128_t s1 = a1.toS70();
  int128_t s2 = a2.toS70();
  uint128_t u1 = a1.toU70();
  uint128_t u2 = a2.toU70();
  auto isNeg1 = s1 < 0;
  auto isNeg2 = s2 < 0;
  int128_t sum128 = s1 + s2;
  InstructionResult result = iNormal;

  if (sum128 >= W72::sBit1) {
    flags.cy1 = flags.tr1 = flags.ov = 1;
    result = iTrap;
  } else if (sum128 < -W72::sBit1) {
    flags.cy0 = flags.tr1 = flags.ov = 1;
    result = iTrap;
  } else if ((s1 < 0 && s2 < 0) ||
	     (isNeg1 != isNeg2 &&
	      (u1 == u2 || ((!isNeg1 && u1 > u2) || (!isNeg2 && u2 > u1)))))
    {
      flags.cy0 = flags.cy1 = flags.tr1 = flags.ov = 1;
      result = iTrap;
    }

  auto [hi36, lo36] = W72::toDW(sum128);
  acPutN(hi36, iw.ac+0);
  acPutN(lo36, iw.ac+1);
  return result;
}


InstructionResult KM10::doDSUB() {
  auto a1 = W72{memGetN(ea.u+0), memGetN(ea.u+1)};
  auto a2 = W72{acGetN(iw.ac+0), acGetN(iw.ac+1)};

  int128_t s1 = a1.toS70();
  int128_t s2 = a2.toS70();
  uint128_t u1 = a1.toU70();
  uint128_t u2 = a2.toU70();
  auto isNeg1 = s1 < 0;
  auto isNeg2 = s2 < 0;
  int128_t diff128 = s1 - s2;
  InstructionResult result = iNormal;

  if (diff128 >= W72::sBit1) {
    flags.cy1 = flags.tr1 = flags.ov = 1;
    result = iTrap;
  } else if (diff128 < -W72::sBit1) {
    flags.cy0 = flags.tr1 = flags.ov = 1;
    result = iTrap;
  } else if ((isNeg1 && isNeg2 && u2 >= u1) ||
	     (isNeg1 != isNeg2 && s2 < 0))
    {
      flags.cy0 = flags.cy1 = flags.tr1 = flags.ov = 1;
      result = iTrap;
    }

  auto [hi36, lo36] = W72::toDW(diff128);
  acPutN(hi36, iw.ac+0);
  acPutN(lo36, iw.ac+1);
  return result;
}


InstructionResult KM10::doDMUL() {
  auto a = W72{memGetN(ea.u+0), memGetN(ea.u+1)};
  auto b = W72{acGetN(iw.ac+0), acGetN(iw.ac+1)};
  const uint128_t a70 = a.toU70();
  const uint128_t b70 = b.toU70();

  if (a.isMaxNeg() && b.isMaxNeg()) {
    const W36 big1{0400000,0};
    flags.tr1 = flags.ov = 1;
    acPutN(big1, iw.ac+0);
    acPutN(big1, iw.ac+1);
    acPutN(big1, iw.ac+2);
    acPutN(big1, iw.ac+3);
    return iTrap;
  }

  W144 prod{W144::product(a70, b70, (a.s < 0) ^ (b.s < 0))};
  auto [r0, r1, r2, r3] = prod.toQuadWord();
  acPutN(r0, iw.ac+0);
  acPutN(r1, iw.ac+1);
  acPutN(r2, iw.ac+2);
  acPutN(r3, iw.ac+3);
  return iNormal;
}


InstructionResult KM10::doDDIV() {
  const W144 den{
    acGetN(iw.ac+0),
    acGetN(iw.ac+1),
    acGetN(iw.ac+2),
    acGetN(iw.ac+3)};
  const W72 div72{memGetN(ea.u+0), memGetN(ea.u+1)};
  auto const div = div72.toU70();

  if (den >= div) {
    flags.tr1 = flags.ov = flags.ndv = 1;
    return iTrap;
  }

  int denNeg = den.sign;
  int divNeg = div72.hiSign;

  /*
    Divide 192 bit n2||n1||n0 by d, returning remainder in rem.
    performs : (n2||n1||0) = ((n2||n1||n0) / d)
    d : a 128bit unsigned integer
  */
  const uint128_t lo70 = den.lowerU70();
  const uint128_t hi70 = den.upperU70();
  uint64_t n0 = lo70;
  uint64_t n1 = hi70 | (lo70 >> 64);
  uint64_t n2 = hi70 >> 6;

  uint128_t remainder = n2 % div;
  n2 = n2 / div;
  uint128_t partial = (remainder << 64) | n1;
  n1 = partial / div;
  remainder = partial % div;
  partial = (remainder << 64) | n0;
  n0 = partial / div;

  const auto quo72 = W72::fromMag(((uint128_t) n1 << 64) | n0, denNeg ^ divNeg);
  const auto rem72 = W72::fromMag(remainder, denNeg);

  acPutN(quo72.hi, iw.ac+0);
  acPutN(quo72.lo, iw.ac+1);
  acPutN(rem72.hi, iw.ac+2);
  acPutN(rem72.lo, iw.ac+3);
  return iNormal;
}


// Instruction class implementations.
void KM10::doBinOp(auto getSrc1F, auto getSrc2F, auto modifyF, auto putDstF) {
  auto result = modifyF(getSrc1F(), getSrc2F());
  if (!flags.ndv) putDstF(result);
}


InstructionResult KM10::doSKIPXX(auto condF) {
  W36 eaw = memGet();
  const bool doSkip = condF(eaw);
  if (iw.ac != 0) acPut(eaw);
  return doSkip ? iSkip : iNormal;
}


void KM10::doAOSXX(auto getF, const signed delta, auto putF, auto condF, auto actionF) {
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

  return condF(v) ? actionF() : iNormal;
}


void KM10::logFlow(const char *msg) {
  if (logger.pc) logger.s << " [" << msg << "]";
}


InstructionResult KM10::doASH() {
  int n = ea.rhs % 36;
  W36 a(acGet());
  auto aSigned{a.ext64()};

  W36 lostBits;

  if (n > 0) {
    lostBits.u = a.u & ((1ull << n) - 1);
    a.s = aSigned >> n;
  } else if (n < 0) {
    n = -n;
    lostBits.u = a.u & (W36::all1s >> n);
    a.s = aSigned << n;
  }

  // Set flags. XXX not sure if these should be set for negative
  // shift count. 1982_ProcRefMan.pdf p.97 is not clear.
  if ((a.ext64() > 0 && lostBits.u != 0) || (a.ext64() < 0 && lostBits.u == 0))
    flags.tr1 = flags.ov = 1;

  // Restore sign bit from before shift.
  a.u = (aSigned & W36::bit0) | (a.u & ~W36::bit0);
  acPut(a);
  return iNormal;
}

InstructionResult KM10::doROT() {
  int n = ea.rhs % 36;
  W36 a(acGet());
  W36 prev(a);

  if (n > 0) {
    a.u <<= n;
    a.u |= prev >> (36 - n);
  } else if (n < 0) {
    n = -n;
    a.u >>= n;
    a.u |= (prev << (36 - n)) & W36::all1s;
  }

  acPut(a);
  return iNormal;
}

InstructionResult KM10::doLSH() {
  int n = ea.rhs % 36;
  W36 a(acGet());

  if (n > 0)
    a.u <<= n;
  else if (n < 0)
    a.u >>= -n;

  acPut(a);
  return iNormal;
}

InstructionResult KM10::doJFFO() {
  W36 tmp = acGet();

  if (tmp.ext64() != 0) {
    unsigned count = 0;

    while (tmp.ext64() >= 0) {
      ++count;
      tmp.u <<= 1;
    }

    tmp.u = count;
  }

  acPutN(tmp, iw.ac+1);
  return iNormal;
}

InstructionResult KM10::doROTC() {
  int n = ea.rhs % 72;
  uint128_t a = ((uint128_t) acGetN(iw.ac+0) << 36) | acGetN(iw.ac+1);

  if (n > 0) {
    uint128_t newLSBs = a >> (72-n);
    a <<= n;
    a |= newLSBs;
  } else if (n < 0) {
    n = -n;
    uint128_t newMSBs = a << (72-n);
    a >>= n;
    a |= newMSBs;
  }

  acPutN((a >> 36) & W36::all1s, iw.ac+0);
  acPutN(a & W36::all1s, iw.ac+1);
  return iNormal;
}

InstructionResult KM10::doLSHC() {
  W72 a(acGet(), acGetN(iw.ac+1));

  if (ea.rhs > 0)
    a.u <<= ea.rhs & 0377;
  else if (ea.rhs < 0)
    a.u >>= -(ea.rhs & 0377);

  acPutN(a.hi, iw.ac+0);
  acPutN(a.lo, iw.ac+1);
  return iNormal;
}

InstructionResult KM10::doEXCH() {
  W36 tmp = acGet();
  acPut(memGet());
  memPut(tmp);
  return iNormal;
}

InstructionResult KM10::doBLT() {
  W36 ac(acGet());
  bool mem = logger.mem;

  logger.mem = false;
  const string prefix{logger.endl + "                                                 ; "};

  do {
    W36 srcA(ea.lhu, ac.lhu);
    W36 dstA(ea.lhu, ac.rhu);

    if (logger.mem) logger.s << prefix << "BLT src=" << srcA.vma << "  dst=" << dstA.vma;

    // Note this isn't bug-for-bug compatible with KL10. See
    // footnote [2] in 1982_ProcRefMan.pdf p.58. We do
    // wraparound.
    memPutN(memGetN(srcA), dstA);
    ac = W36(ac.lhu + 1, ac.rhu + 1);

    // Put it back for traps or page faults.
    acPut(ac);
  } while (ac.rhu <= ea.rhu);

  if (logger.mem) logger.s << prefix << "BLT at end ac=" << ac.fmt36();
  logger.mem = mem;
  return iNormal;
}

  InstructionResult KM10::doADD() {
    doBinOp(acGet, memGet, addWord, acPut);
    return iNormal;
  }

  InstructionResult KM10::doADDI() {
    doBinOp(acGet, immediate, addWord, acPut);
    return iNormal;
  }

  InstructionResult KM10::doADDM() {
    doBinOp(acGet, memGet, addWord, memPut);
    return iNormal;
  }

  InstructionResult KM10::doADDB() {
    doBinOp(acGet, memGet, addWord, bothPut);
    return iNormal;
  }

  InstructionResult KM10::doSUB() {
    doBinOp(acGet, memGet, subWord, acPut);
    return iNormal;
  }

  InstructionResult KM10::doSUBI() {
    doBinOp(acGet, immediate, subWord, acPut);
    return iNormal;
  }

  InstructionResult KM10::doSUBM() {
    doBinOp(acGet, memGet, subWord, memPut);
    return iNormal;
  }

  InstructionResult KM10::doSUBB() {
    doBinOp(acGet, memGet, subWord, bothPut);
    return iNormal;
  }

  InstructionResult KM10::doAOJ() {
    return doAOSXX(acGet, 1, acPut, never, jumpAction);
  }

  InstructionResult KM10::doAOJL() {
    return doAOSXX(acGet, 1, acPut, isLT0, jumpAction);
  }

  InstructionResult KM10::doAOJE() {
    return doAOSXX(acGet, 1, acPut, isEQ0, jumpAction);
  }

  InstructionResult KM10::doAOJLE() {
    return doAOSXX(acGet, 1, acPut, isLE0, jumpAction);
  }

  InstructionResult KM10::doAOJA() {
    return doAOSXX(acGet, 1, acPut, always, jumpAction);
  }

  InstructionResult KM10::doAOJGE() {
    return doAOSXX(acGet, 1, acPut, isGE0, jumpAction);
  }

  InstructionResult KM10::doAOJN() {
    return doAOSXX(acGet, 1, acPut, isNE0, jumpAction);
  }

  InstructionResult KM10::doAOJG() {
    return doAOSXX(acGet, 1, acPut, isGT0, jumpAction);
  }

  InstructionResult KM10::doAOS() {
    return doAOSXX(memGet, 1, memPut, never, skipAction);
  }

  InstructionResult KM10::doAOSL() {
    return doAOSXX(memGet, 1, memPut, isLT0, skipAction);
  }

  InstructionResult KM10::doAOSE() {
    return doAOSXX(memGet, 1, memPut, isEQ0, skipAction);
  }

  InstructionResult KM10::doAOSLE() {
    return doAOSXX(memGet, 1, memPut, isLE0, skipAction);
  }

  InstructionResult KM10::doAOSA() {
    return doAOSXX(memGet, 1, memPut, always, skipAction);
  }

  InstructionResult KM10::doAOSGE() {
    return doAOSXX(memGet, 1, memPut, isGE0, skipAction);
  }

  InstructionResult KM10::doAOSN() {
    return doAOSXX(memGet, 1, memPut, isNE0, skipAction);
  }

  InstructionResult KM10::doAOSG() {
    return doAOSXX(memGet, 1, memPut, isGT0, skipAction);
  }

  InstructionResult KM10::doSOJ() {
    return doAOSXX(acGet, -1, acPut, never, jumpAction);
  }

  InstructionResult KM10::doSOJL() {
    return doAOSXX(acGet, -1, acPut, isLT0, jumpAction);
  }

  InstructionResult KM10::doSOJE() {
    return doAOSXX(acGet, -1, acPut, isEQ0, jumpAction);
  }

  InstructionResult KM10::doSOJLE() {
    return doAOSXX(acGet, -1, acPut, isLE0, jumpAction);
  }

  InstructionResult KM10::doSOJA() {
    return doAOSXX(acGet, -1, acPut, always, jumpAction);
  }

  InstructionResult KM10::doSOJGE() {
    return doAOSXX(acGet, -1, acPut, isGE0, jumpAction);
  }

  InstructionResult KM10::doSOJN() {
    return doAOSXX(acGet, -1, acPut, isNE0, jumpAction);
  }

  InstructionResult KM10::doSOJG() {
    return doAOSXX(acGet, -1, acPut, isGT0, jumpAction);
  }

  InstructionResult KM10::doSOS() {
    return doAOSXX(memGet, -1, memPut, never, skipAction);
  }

  InstructionResult KM10::doSOSL() {
    return doAOSXX(memGet, -1, memPut, isLT0, skipAction);
  }

  InstructionResult KM10::doSOSE() {
    return doAOSXX(memGet, -1, memPut, isEQ0, skipAction);
  }

  InstructionResult KM10::doSOSLE() {
    return doAOSXX(memGet, -1, memPut, isLE0, skipAction);
  }

  InstructionResult KM10::doSOSA() {
    return doAOSXX(memGet, -1, memPut, always, skipAction);
  }

  InstructionResult KM10::doSOSGE() {
    return doAOSXX(memGet, -1, memPut, isGE0, skipAction);
  }

  InstructionResult KM10::doSOSN() {
    return doAOSXX(memGet, -1, memPut, isNE0, skipAction);
  }

  InstructionResult KM10::doSOSG() {
    return doAOSXX(memGet, -1, memPut, isGT0, skipAction);
  }

  InstructionResult KM10::doAND() {
    doBinOp(memGet, acGet, andWord, acPut);
    return iNormal;
  }

  InstructionResult KM10::doANDI() {
    doBinOp(immediate, acGet, andWord, acPut);
    return iNormal;
  }

  InstructionResult KM10::doANDM() {
    doBinOp(memGet, acGet, andWord, memPut);
    return iNormal;
  }

  InstructionResult KM10::doANDB() {
    doBinOp(memGet, acGet, andWord, bothPut);
    return iNormal;
  }

  InstructionResult KM10::doANDCA() {
    doBinOp(memGet, acGet, andCWord, acPut);
    return iNormal;
  }

  InstructionResult KM10::doANDCAI() {
    doBinOp(immediate, acGet, andCWord, acPut);
    return iNormal;
  }

  InstructionResult KM10::doANDCAM() {
    doBinOp(memGet, acGet, andCWord, memPut);
    return iNormal;
  }

  InstructionResult KM10::doANDCAB() {
    doBinOp(memGet, acGet, andCWord, bothPut);
    return iNormal;
  }

  InstructionResult KM10::doANDCM() {
    doBinOp(acGet, memGet, andCWord, acPut);
    return iNormal;
  }

  InstructionResult KM10::doANDCMI() {
    doBinOp(acGet, immediate, andCWord, acPut);
    return iNormal;
  }

  InstructionResult KM10::doANDCMM() {
    doBinOp(acGet, memGet, andCWord, memPut);
    return iNormal;
  }

  InstructionResult KM10::doANDCMB() {
    doBinOp(acGet, memGet, andCWord, bothPut);
    return iNormal;
  }

  InstructionResult KM10::doORCA() {
    doBinOp(memGet, acGet, iorCWord, acPut);
    return iNormal;
  }

  InstructionResult KM10::doORCAI() {
    doBinOp(immediate, acGet, iorCWord, acPut);
    return iNormal;
  }

  InstructionResult KM10::doORCAM() {
    doBinOp(memGet, acGet, iorCWord, memPut);
    return iNormal;
  }

  InstructionResult KM10::doORCAB() {
    doBinOp(memGet, acGet, iorCWord, bothPut);
    return iNormal;
  }

  InstructionResult KM10::doORCM() {
    doBinOp(acGet, memGet, iorCWord, acPut);
    return iNormal;
  }

  InstructionResult KM10::doORCMI() {
    doBinOp(acGet, immediate, iorCWord, acPut);
    return iNormal;
  }

  InstructionResult KM10::doORCMM() {
    doBinOp(acGet, memGet, iorCWord, memPut);
    return iNormal;
  }

  InstructionResult KM10::doORCMB() {
    doBinOp(acGet, memGet, iorCWord, bothPut);
    return iNormal;
  }

  InstructionResult KM10::doORCB() {
    doBinOp(memGet, acGet, iorCBWord, acPut);
    return iNormal;
  }

  InstructionResult KM10::doORCBI() {
    doBinOp(immediate, acGet, iorCBWord, acPut);
    return iNormal;
  }

  InstructionResult KM10::doORCBM() {
    doBinOp(memGet, acGet, iorCBWord, memPut);
    return iNormal;
  }

  InstructionResult KM10::doORCBB() {
    doBinOp(memGet, acGet, iorCBWord, bothPut);
    return iNormal;
  }
#endif


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
    if (addressBPs.contains(a.vma)) running = false;
    return value;
  }


  void KM10::memPutN(W36 value, W36 a) {

    if (a.rhu < 020)
      acPutN(value, a.rhu);
    else 
      memP[a.rhu] = value;

    if (logger.mem) logger.s << "; " << a.fmtVMA() << "=" << value.fmt36();
    if (addressBPs.contains(a.vma)) running = false;
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


  ////////////////////////////////////////////////////////////////
  void KM10::emulate() {
    ////////////////////////////////////////////////////////////////
    // Connect our DTE20 (put console into raw mode)
    dte.connect();

    // The instruction loop.
    fetchPC = pc;

    for (;;) {

      // Keep the cache sweep timer ticking until it goes DING.
      cca.handleSweep();

      // Handle execution breakpoints.
      if (executeBPs.contains(ea.vma)) running = false;

      // XXX do we handle traps and interrupts here? Or some other place?

      // Fetch the instruction.
      iw = memGetN(fetchPC);

      // If we're debugging, this is where we pause to let the user
      // inspect and change things. The debugger tells us what our next
      // action should be based on its return value.
      if (!running) {

	switch (debugger.debug()) {
	case Debugger::step:	// Debugger has set step count in nSteps.
	  break;

	case Debugger::run:	// Continue from current PC (nSteps set by debugger to 0).
	  break;

	case Debugger::quit:	// Quit from emulator.
	  return;

	case Debugger::restart:	// Restart emulator - total reboot
	  return;

	case Debugger::pcChanged:	// PC changed by debugger - go fetch again
	  ea = pc;
	  continue;

	default:			// This should never happen...
	  assert("Debugger returned unknown action" == nullptr);
	  return;
	}
      }

      // Handle nSteps so we don't keep running if we run out of step
      // count. THIS instruction is our single remaining step. If
      // nSteps is zero we just keep going "forever".
      if (nSteps > 0) {
	if (--nSteps <= 0) running = false;
      }

      if (logger.loggingToFile && logger.pc) {
	logger.s << ea.fmtVMA() << ": " << debugger.dump(iw, ea);
      }

      // Compute effective address. Note this wipes out `ea` so we no
      // longer know what location the instruction we're running was
      // fetched from.
      ea.u = getEA(iw.i, iw.x, iw.y);

      // Execute the instruction in `iw`.
      InstructionResult result = (this->*ops[iw.op])();

      // If we "continue" we have to set up `fetchPC` to point to the
      // instruction to fetch and execute next. If we "break" (from the
      // switch case) we use `fetchPC` + pcOffset.
      switch (result) {
      case iNormal:
	pcOffset = 1;
	break;

      case iSkip:
	// Any skip instruction that skips.
	pcOffset = 2;
	break;

      case iJump:
	// In this case, jump instructions put destination in `ea` for
	// "free".
	pc = ea;
	continue;

      case iMUUO:
      case iLUUO:
      case iTrap:
      case iXCT:
	// All of these cases require that we fetch the next instruction
	// from a specified location (contained in `fetchPC` and already
	// set) and loop back to execute that instruction WITHOUT
	// changing PC.
	continue;

      case iHALT:
	pcOffset = 0;		// Leave PC at HALT instruction.
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
      pc.vma = fetchPC.vma = fetchPC.vma + pcOffset;
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
    app.add_option("-d,--debug", dVal, "run the built-in debugger instead of starting execution");

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

    KM10 km10(mVal*1024, aBPs, eBPs);
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
