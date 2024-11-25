#include <iostream>
#include <assert.h>
using namespace std;

#include <gflags/gflags.h>

#include "word.hpp"
#include "kmstate.hpp"
#include "km10.hpp"
#include "debugger.hpp"

#include "logger.hpp"


Logger logger{};

// We keep these breakpoint sets outside of the looped main so they
// stick across restart.
static unordered_set<unsigned> aBPs;
static unordered_set<unsigned> eBPs;


// Definitions for our command line options
DEFINE_string(load, "../images/klad/dfkaa.a10", ".A10 or .SAV file to load");
DEFINE_string(rel, "../images/klad/dfkaa.rel", ".REL file to load symbols from");
DEFINE_bool(debug, false, "run the built-in debugger instead of starting execution");

////////////////////////////////////////////////////////////////
// Constructor
KM10::KM10(KMState &aState)
  : state(aState),
    apr(aState),
    cca(aState, apr),
    mtr(aState),
    pi(aState),
    pag(aState),
    tim(aState),
    dte(040, aState),
    noDevice(0777777ul, "?NoDevice?", aState),
    debuggerP(nullptr),
    ops(512, nullptr)
{
  ops[0000] = &KM10::doMUUO;	// ILLEGAL
  for (unsigned op=1; op < 037; ++op) ops[op] = &KM10::doLUUO;
  for (unsigned op=040; op < 0102; ++op) ops[op] = &KM10::doMUUO;
  ops[0102] = &KM10::nyi;


  for (unsigned op=0700; op <= 0777; ++op) ops[op] = &KM10::doIO;
}


////////////////////////////////////////////////////////////////
KM10::InstructionResult KM10::nyi() {
  cerr << "Not yet implemented: " << oct << iw.op << logger.endl << flush;
  return normal;
}


// XXX do we need to implement the skipping IO instructions somehow
// here? I think so.
KM10::InstructionResult KM10::doIO() {
  if (logger.io) logger.s << "; ioDev=" << oct << iw.ioDev << " ioOp=" << oct << iw.ioOp;
  Device::handleIO(iw, ea, state, state.nextPC);
  return normal;
}


KM10::InstructionResult KM10::doLUUO() {

  if (state.pc.isSection0()) {
    W36 uuoState;
    uuoState.op = iw.op;
    uuoState.ac = iw.ac;
    uuoState.i = 0;
    uuoState.x = 0;
    uuoState.y = ea.rhu;

    // XXX this should select executive virtual space first.
    state.memPutN(040, uuoState);
    cerr << "LUUO at " << state.pc.fmtVMA() << " uuoState=" << uuoState.fmt36()
	 << logger.endl << flush;
    state.nextPC = 041;
    state.exceptionPC = state.pc + 1;
    state.inInterrupt = true;
    return InstructionResult::trap;
  } else {

    if (state.flags.usr) {
      W36 uuoA(state.uptP->luuoAddr);
      state.memPutN(W36(((uint64_t) state.flags.u << 23) |
			((uint64_t) iw.op << 15) |
			((uint64_t) iw.ac << 5)), uuoA.u++);
      state.memPutN(state.pc, uuoA.u++);
      state.memPutN(ea.u, uuoA.u++);
      state.nextPC = state.memGetN(uuoA);
      state.exceptionPC = state.pc + 1;
      state.inInterrupt = true;
      return InstructionResult::trap;
    } else {	       // Executive mode treats LUUOs as MUUOs
      return doMUUO();
    }
  }
}


KM10::InstructionResult KM10::doMUUO() {
  cerr << "MUUOs aren't implemented yet" << logger.endl << flush;
  state.exceptionPC = state.pc + 1;
  state.inInterrupt = true;
  logger.nyi(state);
  return muuo;
}


KM10::InstructionResult KM10::doDADD() {
  auto a1 = W72{state.memGetN(ea.u+0), state.memGetN(ea.u+1)};
  auto a2 = W72{state.acGetN(iw.ac+0), state.acGetN(iw.ac+1)};

  int128_t s1 = a1.toS70();
  int128_t s2 = a2.toS70();
  uint128_t u1 = a1.toU70();
  uint128_t u2 = a2.toU70();
  auto isNeg1 = s1 < 0;
  auto isNeg2 = s2 < 0;
  int128_t sum128 = s1 + s2;
  InstructionResult result = normal;

  if (sum128 >= W72::sBit1) {
    state.flags.cy1 = state.flags.tr1 = state.flags.ov = 1;
    result = trap;
  } else if (sum128 < -W72::sBit1) {
    state.flags.cy0 = state.flags.tr1 = state.flags.ov = 1;
    result = trap;
  } else if ((s1 < 0 && s2 < 0) ||
	     (isNeg1 != isNeg2 &&
	      (u1 == u2 || ((!isNeg1 && u1 > u2) || (!isNeg2 && u2 > u1)))))
    {
      state.flags.cy0 = state.flags.cy1 = state.flags.tr1 = state.flags.ov = 1;
      result = trap;
    }

  auto [hi36, lo36] = W72::toDW(sum128);
  state.acPutN(hi36, iw.ac+0);
  state.acPutN(lo36, iw.ac+1);
  return result;
}


KM10::InstructionResult KM10::doDSUB() {
  auto a1 = W72{state.memGetN(ea.u+0), state.memGetN(ea.u+1)};
  auto a2 = W72{state.acGetN(iw.ac+0), state.acGetN(iw.ac+1)};

  int128_t s1 = a1.toS70();
  int128_t s2 = a2.toS70();
  uint128_t u1 = a1.toU70();
  uint128_t u2 = a2.toU70();
  auto isNeg1 = s1 < 0;
  auto isNeg2 = s2 < 0;
  int128_t diff128 = s1 - s2;
  InstructionResult result = normal;

  if (diff128 >= W72::sBit1) {
    state.flags.cy1 = state.flags.tr1 = state.flags.ov = 1;
    result = trap;
  } else if (diff128 < -W72::sBit1) {
    state.flags.cy0 = state.flags.tr1 = state.flags.ov = 1;
    result = trap;
  } else if ((isNeg1 && isNeg2 && u2 >= u1) ||
	     (isNeg1 != isNeg2 && s2 < 0))
    {
      state.flags.cy0 = state.flags.cy1 = state.flags.tr1 = state.flags.ov = 1;
      result = trap;
    }

  auto [hi36, lo36] = W72::toDW(diff128);
  state.acPutN(hi36, iw.ac+0);
  state.acPutN(lo36, iw.ac+1);
  return result;
}


KM10::InstructionResult KM10::doDMUL() {
  auto a = W72{state.memGetN(ea.u+0), state.memGetN(ea.u+1)};
  auto b = W72{state.acGetN(iw.ac+0), state.acGetN(iw.ac+1)};
  const uint128_t a70 = a.toU70();
  const uint128_t b70 = b.toU70();

  if (a.isMaxNeg() && b.isMaxNeg()) {
    const W36 big1{0400000,0};
    state.flags.tr1 = state.flags.ov = 1;
    state.acPutN(big1, iw.ac+0);
    state.acPutN(big1, iw.ac+1);
    state.acPutN(big1, iw.ac+2);
    state.acPutN(big1, iw.ac+3);
    return trap;
  }

  W144 prod{W144::product(a70, b70, (a.s < 0) ^ (b.s < 0))};
  auto [r0, r1, r2, r3] = prod.toQuadWord();
  state.acPutN(r0, iw.ac+0);
  state.acPutN(r1, iw.ac+1);
  state.acPutN(r2, iw.ac+2);
  state.acPutN(r3, iw.ac+3);
  return normal;
}


KM10::InstructionResult KM10::doDDIV() {
  const W144 den{
    state.acGetN(iw.ac+0),
    state.acGetN(iw.ac+1),
    state.acGetN(iw.ac+2),
    state.acGetN(iw.ac+3)};
  const W72 div72{state.memGetN(ea.u+0), state.memGetN(ea.u+1)};
  auto const div = div72.toU70();

  if (den >= div) {
    state.flags.tr1 = state.flags.ov = state.flags.ndv = 1;
    return trap;
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

  state.acPutN(quo72.hi, iw.ac+0);
  state.acPutN(quo72.lo, iw.ac+1);
  state.acPutN(rem72.hi, iw.ac+2);
  state.acPutN(rem72.lo, iw.ac+3);
  return normal;
}


KM10::InstructionResult KM10::doIBP_ADJBP() {
  BytePointer *bp = BytePointer::makeFrom(ea, state);

  if (iw.ac == 0) {	// IBP
    bp->inc(state);
  } else {		// ADJBP
    bp->adjust(iw.ac, state);
  }

  return normal;
}


KM10::InstructionResult KM10::doILBP() {
  BytePointer *bp = BytePointer::makeFrom(ea, state);
  bp->inc(state);
  acPut(bp->getByte(state));
  return normal;
}


KM10::InstructionResult KM10::doLDB() {
  BytePointer *bp = BytePointer::makeFrom(ea, state);
  acPut(bp->getByte(state));
  return normal;
}


KM10::InstructionResult KM10::doIDPB() {
  BytePointer *bp = BytePointer::makeFrom(ea, state);
  bp->inc(state);
  bp->putByte(acGet(), state);
  return normal;
}


KM10::InstructionResult KM10::doDPB() {
  BytePointer *bp = BytePointer::makeFrom(ea, state);
  bp->putByte(acGet(), state);
  return normal;
}


KM10::InstructionResult KM10::doMOVE() {
  doMOVXX(memGet, noMod1, acPut);
  return normal;
}


KM10::InstructionResult KM10::doMOVEI() {
  doMOVXX(immediate, noMod1, acPut);
  return normal;
}

KM10::InstructionResult KM10::doMOVEM() {
  doMOVXX(acGet, noMod1, memPut);
  return normal;
}

KM10::InstructionResult KM10::doMOVES() {
  doMOVXX(memGet, noMod1, selfPut);
  return normal;
}

KM10::InstructionResult KM10::doMOVS() {
  doMOVXX(memGet, swap, acPut);
  return normal;
}

KM10::InstructionResult KM10::doMOVSI() {
  doMOVXX(immediate, swap, acPut);
  return normal;
}

KM10::InstructionResult KM10::doMOVSM() {
  doMOVXX(acGet, swap, memPut);
  return normal;
}

KM10::InstructionResult KM10::doMOVSS() {
  doMOVXX(memGet, swap, selfPut);
  return normal;
}

KM10::InstructionResult KM10::doMOVN() {
  doMOVXX(memGet, negate, acPut);
  return normal;
}

KM10::InstructionResult KM10::doMOVNI() {
  doMOVXX(immediate, negate, acPut);
  return normal;
}

KM10::InstructionResult KM10::doMOVNM() {
  doMOVXX(acGet, negate, memPut);
  return normal;
}

KM10::InstructionResult KM10::doMOVNS() {
  doMOVXX(memGet, negate, selfPut);
  return normal;
}

KM10::InstructionResult KM10::doMOVM() {
  doMOVXX(memGet, magnitude, acPut);
  return normal;
}

KM10::InstructionResult KM10::doMOVMI() {
  doMOVXX(immediate, magnitude, acPut);
  return normal;
}

KM10::InstructionResult KM10::doMOVMM() {
  doMOVXX(acGet, magnitude, memPut);
  return normal;
}

KM10::InstructionResult KM10::doMOVMS() {
  doMOVXX(memGet, magnitude, selfPut);
  return normal;
}

KM10::InstructionResult KM10::doIMUL() {
  doBinOp(acGet, memGet, imulWord, acPut);
  return normal;
}

KM10::InstructionResult KM10::doIMULI() {
  doBinOp(acGet, immediate, imulWord, acPut);
  return normal;
}

KM10::InstructionResult KM10::doIMULM() {
  doBinOp(acGet, memGet, imulWord, memPut);
  return normal;
}

KM10::InstructionResult KM10::doIMULB() {
  doBinOp(acGet, memGet, imulWord, bothPut);
  return normal;
}

KM10::InstructionResult KM10::doMUL() {
  doBinOp(acGet, memGet, mulWord, acPut2);
  return normal;
}

KM10::InstructionResult KM10::doMULI() {
  doBinOp(acGet, immediate, mulWord, acPut2);
  return normal;
}

KM10::InstructionResult KM10::doMULM() {
  doBinOp(acGet, memGet, mulWord, memPutHi);
  return normal;
}

KM10::InstructionResult KM10::doMULB() {
  doBinOp(acGet, memGet, mulWord, bothPut2);
  return normal;
}

KM10::InstructionResult KM10::doIDIV() {
  doBinOp(acGet, memGet, idivWord, acPut2);
  return normal;
}

KM10::InstructionResult KM10::doIDIVI() {
  doBinOp(acGet, immediate, idivWord, acPut2);
  return normal;
}

KM10::InstructionResult KM10::doIDIVM() {
  doBinOp(acGet, memGet, idivWord, memPutHi);
  return normal;
}

KM10::InstructionResult KM10::doIDIVB() {
  doBinOp(acGet, memGet, idivWord, bothPut2);
  return normal;
}

KM10::InstructionResult KM10::doDIV() {
  doBinOp(acGet2, memGet, divWord, acPut2);
  return normal;
}

KM10::InstructionResult KM10::doDIVI() {
  doBinOp(acGet2, immediate, divWord, acPut2);
  return normal;
}

KM10::InstructionResult KM10::doDIVM() {
  doBinOp(acGet2, memGet, divWord, memPutHi);
  return normal;
}

KM10::InstructionResult KM10::doDIVB() {
  doBinOp(acGet2, memGet, divWord, bothPut2);
  return normal;
}





KM10::InstructionResult KM10::doASH() {
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
    state.flags.tr1 = state.flags.ov = 1;

  // Restore sign bit from before shift.
  a.u = (aSigned & W36::bit0) | (a.u & ~W36::bit0);
  acPut(a);
  return normal;
}

KM10::InstructionResult KM10::doROT() {
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
  return normal;
}

KM10::InstructionResult KM10::doLSH() {
  int n = ea.rhs % 36;
  W36 a(acGet());

  if (n > 0)
    a.u <<= n;
  else if (n < 0)
    a.u >>= -n;

  acPut(a);
  return normal;
}

KM10::InstructionResult KM10::doJFFO() {
  W36 tmp = acGet();

  if (tmp.ext64() != 0) {
    unsigned count = 0;

    while (tmp.ext64() >= 0) {
      ++count;
      tmp.u <<= 1;
    }

    tmp.u = count;
  }

  state.acPutN(tmp, iw.ac+1);
  return normal;
}

KM10::InstructionResult KM10::doROTC() {
  int n = ea.rhs % 72;
  uint128_t a = ((uint128_t) state.acGetN(iw.ac+0) << 36) | state.acGetN(iw.ac+1);

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

  state.acPutN((a >> 36) & W36::all1s, iw.ac+0);
  state.acPutN(a & W36::all1s, iw.ac+1);
  return normal;
}

KM10::InstructionResult KM10::doLSHC() {
  W72 a(acGet(), state.acGetN(iw.ac+1));

  if (ea.rhs > 0)
    a.u <<= ea.rhs & 0377;
  else if (ea.rhs < 0)
    a.u >>= -(ea.rhs & 0377);

  state.acPutN(a.hi, iw.ac+0);
  state.acPutN(a.lo, iw.ac+1);
  return normal;
}

KM10::InstructionResult KM10::doEXCH() {
  W36 tmp = acGet();
  acPut(memGet());
  memPut(tmp);
  return normal;
}

KM10::InstructionResult KM10::doBLT() {
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
    state.memPutN(state.memGetN(srcA), dstA);
    ac = W36(ac.lhu + 1, ac.rhu + 1);

    // Put it back for traps or page faults.
    acPut(ac);
  } while (ac.rhu <= ea.rhu);

  if (logger.mem) logger.s << prefix << "BLT at end ac=" << ac.fmt36();
  logger.mem = mem;
  return normal;
}

KM10::InstructionResult KM10::doAOBJP() {
  W36 tmp = acGet();
  tmp = W36(tmp.lhu + 1, tmp.rhu + 1);
  acPut(tmp);

  if (tmp.ext64() >= 0) {
    logFlow("jump");
    state.nextPC = ea;
  }

  return normal;
}

KM10::InstructionResult KM10::doAOBJN() {
  W36 tmp = acGet();
  tmp = W36(tmp.lhu + 1, tmp.rhu + 1);
  acPut(tmp);

  if (tmp.ext64() < 0) {
    logFlow("jump");
    state.nextPC = ea;
  }

  return normal;
}


KM10::InstructionResult KM10::doJRST() {

  switch (iw.ac) {
  case 000:					// JRST
    state.nextPC.rhu = ea.rhu;
    break;

  case 001:					// PORTAL
    logger.nyi(state);
    break;

  case 002:					// JRSTF
    state.restoreFlags(ea);
    state.nextPC.rhu = ea.rhu;
    break;

  case 004:					// HALT
    cerr << "[HALT at " << state.pc.fmtVMA() << "]" << logger.endl;
    state.running = false;
    state.nextPC.rhu = ea.rhu;		// HALT actually does change PC
    return halt;

  case 005:					// XJRSTF
    logger.nyi(state);
    break;

  case 006:					// XJEN
    pi.dismissInterrupt();
    logger.nyi(state);
    break;

  case 007:					// XPCW
    logger.nyi(state);
    break;

  case 010:					// 25440 - no mnemonic
    state.restoreFlags(ea);
    break;

  case 012:					// JEN
    cerr << ">>>>>> JEN ea=" << ea.fmtVMA() << logger.endl << flush;
    pi.dismissInterrupt();
    state.restoreFlags(ea);
    state.nextPC.rhu = ea.rhu;
    break;

  case 014:					// SFM
    logger.nyi(state);
    break;

  default:
    logger.nyi(state);
    break;
  }

  return jump;
}



KM10::InstructionResult KM10::doJFCL() {
  unsigned wasFlags = state.flags.u;
  unsigned testFlags = (unsigned) iw.ac << 9; // Align with OV,CY0,CY1,FOV
  state.flags.u &= ~testFlags;
  if (wasFlags & testFlags) state.nextPC = ea;
  return jump;
}


KM10::InstructionResult KM10::doPXCT() {

  if (state.userMode() || iw.ac == 0) {
    state.pc = ea;
    state.inXCT = true;
    return xct;
  } else {					// PXCT
    logger.nyi(state);
    state.running = false;
    return halt;		// XXX for now
  }
}


KM10::InstructionResult KM10::doPUSHJ() {
  // Note this sets the flags that are cleared by PUSHJ before
  // doPush() since doPush() can set flags.tr2.
  state.flags.fpd = state.flags.afi = state.flags.tr1 = state.flags.tr2 = 0;
  doPush(state.pc.isSection0() ? state.flagsWord(state.nextPC.rhu) : W36(state.nextPC.vma), iw.ac);
  state.nextPC = ea;
  if (state.inInterrupt) state.flags.usr = state.flags.pub = 0;
  return jump;
}


KM10::InstructionResult KM10::doPUSH() {
  doPush(memGet(), iw.ac);
  return normal;
}

KM10::InstructionResult KM10::doPOP() {
  memPut(doPop(iw.ac));
  return normal;
}

KM10::InstructionResult KM10::doPOPJ() {
  state.nextPC.rhu = doPop(iw.ac).rhu;
  return jump;
}

KM10::InstructionResult KM10::doJSR() {
  W36 tmp = state.inInterrupt ? state.exceptionPC : state.nextPC;
  tmp = state.pc.isSection0() ? state.flagsWord(tmp.rhu) : W36(tmp.vma);
  cerr << ">>>>>> JSR saved PC=" << tmp.fmt36() << "  ea=" << ea.fmt36()
       << (state.inInterrupt ? "[inInterrupt]" : "[!inInterrupt]")
       << logger.endl << flush;
  memPut(tmp);
  state.nextPC.rhu = ea.rhu + 1;
  state.flags.fpd = state.flags.afi = state.flags.tr2 = state.flags.tr1 = 0;
  if (state.inInterrupt) state.flags.usr = state.flags.pub = 0;
  return jump;
}

KM10::InstructionResult KM10::doJSP() {
  W36 tmp = state.inInterrupt ? state.exceptionPC : state.nextPC;
  tmp = state.pc.isSection0() ? state.flagsWord(tmp.rhu) : W36(tmp.vma);
  cerr << ">>>>>> JSP set ac=" << tmp.fmt36() << "  ea=" << ea.fmt36()
       << (state.inInterrupt ? "[inInterrupt]" : "[!inInterrupt]")
       << logger.endl << flush;
  acPut(tmp);
  state.nextPC.rhu = ea.rhu;
  state.flags.fpd = state.flags.afi = state.flags.tr2 = state.flags.tr1 = 0;
  if (state.inInterrupt) state.flags.usr = state.flags.pub = 0;
  return jump;
}

KM10::InstructionResult KM10::doJSA() {
  memPut(acGet());
  state.nextPC.rhu = ea.rhu + 1;
  acPut(W36(ea.rhu, state.pc.rhu + 1));
  if (state.inInterrupt) state.flags.usr = 0;
  return jump;
}

KM10::InstructionResult KM10::doJRA() {
  acPut(state.memGetN(acGet().lhu));
  state.nextPC = ea;
  return jump;
}

KM10::InstructionResult KM10::doADD() {
  doBinOp(acGet, memGet, addWord, acPut);
  return normal;
}

KM10::InstructionResult KM10::doADDI() {
  doBinOp(acGet, immediate, addWord, acPut);
  return normal;
}

KM10::InstructionResult KM10::doADDM() {
  doBinOp(acGet, memGet, addWord, memPut);
  return normal;
}

KM10::InstructionResult KM10::doADDB() {
  doBinOp(acGet, memGet, addWord, bothPut);
  return normal;
}

KM10::InstructionResult KM10::doSUB() {
  doBinOp(acGet, memGet, subWord, acPut);
  return normal;
}

KM10::InstructionResult KM10::doSUBI() {
  doBinOp(acGet, immediate, subWord, acPut);
  return normal;
}

KM10::InstructionResult KM10::doSUBM() {
  doBinOp(acGet, memGet, subWord, memPut);
  return normal;
}

KM10::InstructionResult KM10::doSUBB() {
  doBinOp(acGet, memGet, subWord, bothPut);
  return normal;
}

KM10::InstructionResult KM10::doCAI() {
  return normal;
}

KM10::InstructionResult KM10::doCAIL() {
  doCAXXX(acGet, immediate, isLT);
  return normal;
}

KM10::InstructionResult KM10::doCAIE() {
  doCAXXX(acGet, immediate, isEQ);
  return normal;
}

KM10::InstructionResult KM10::doCAILE() {
  doCAXXX(acGet, immediate, isLE);
  return normal;
}

KM10::InstructionResult KM10::doCAIA() {
  doCAXXX(acGet, immediate, always2);
  return normal;
}

KM10::InstructionResult KM10::doCAIGE() {
  doCAXXX(acGet, immediate, isGE);
  return normal;
}

KM10::InstructionResult KM10::doCAIN() {
  doCAXXX(acGet, immediate, isNE);
  return normal;
}

KM10::InstructionResult KM10::doCAIG() {
  doCAXXX(acGet, immediate, isGT);
  return normal;
}

KM10::InstructionResult KM10::doCAM() {
  return normal;
}

KM10::InstructionResult KM10::doCAML() {
  doCAXXX(acGet, memGet, isLT);
  return normal;
}

KM10::InstructionResult KM10::doCAME() {
  doCAXXX(acGet, memGet, isEQ);
  return normal;
}

KM10::InstructionResult KM10::doCAMLE() {
  doCAXXX(acGet, memGet, isLE);
  return normal;
}

KM10::InstructionResult KM10::doCAMA() {
  doCAXXX(acGet, memGet, always2);
  return normal;
}

KM10::InstructionResult KM10::doCAMGE() {
  doCAXXX(acGet, memGet, isGE);
  return normal;
}

KM10::InstructionResult KM10::doCAMN() {
  doCAXXX(acGet, memGet, isNE);
  return normal;
}

KM10::InstructionResult KM10::doCAMG() {
  doCAXXX(acGet, memGet, isGT);
  return normal;
}

KM10::InstructionResult KM10::doJUMP() {
  doJUMP(never);
  return normal;
}

KM10::InstructionResult KM10::doJUMPL() {
  doJUMP(isLT0);
  return normal;
}

KM10::InstructionResult KM10::doJUMPE() {
  doJUMP(isEQ0);
  return normal;
}

KM10::InstructionResult KM10::doJUMPLE() {
  doJUMP(isLE0);
  return normal;
}

KM10::InstructionResult KM10::doJUMPA() {
  doJUMP(always);
  return normal;
}

KM10::InstructionResult KM10::doJUMPGE() {
  doJUMP(isGE0);
  return normal;
}

KM10::InstructionResult KM10::doJUMPN() {
  doJUMP(isNE0);
  return normal;
}

KM10::InstructionResult KM10::doJUMPG() {
  doJUMP(isGT0);
  return normal;
}

KM10::InstructionResult KM10::doSKIP() {
  doSKIP(never);
  return normal;
}

KM10::InstructionResult KM10::doSKIPL() {
  doSKIP(isLT0);
  return normal;
}

KM10::InstructionResult KM10::doSKIPE() {
  doSKIP(isEQ0);
  return normal;
}

KM10::InstructionResult KM10::doSKIPLE() {
  doSKIP(isLE0);
  return normal;
}

KM10::InstructionResult KM10::doSKIPA() {
  doSKIP(always);
  return normal;
}

KM10::InstructionResult KM10::doSKIPGE() {
  doSKIP(isGE0);
  return normal;
}

KM10::InstructionResult KM10::doSKIPN() {
  doSKIP(isNE0);
  return normal;
}

KM10::InstructionResult KM10::doSKIPGT() {
  doSKIP(isGT0);
  return normal;
}

KM10::InstructionResult KM10::doAOJ() {
  doAOSXX(acGet, 1, acPut, never, jumpAction);
  return normal;
}

KM10::InstructionResult KM10::doAOJL() {
  doAOSXX(acGet, 1, acPut, isLT0, jumpAction);
  return normal;
}

KM10::InstructionResult KM10::doAOJE() {
  doAOSXX(acGet, 1, acPut, isEQ0, jumpAction);
  return normal;
}

KM10::InstructionResult KM10::doAOJLE() {
  doAOSXX(acGet, 1, acPut, isLE0, jumpAction);
  return normal;
}

KM10::InstructionResult KM10::doAOJA() {
  doAOSXX(acGet, 1, acPut, always, jumpAction);
  return normal;
}

KM10::InstructionResult KM10::doAOJGE() {
  doAOSXX(acGet, 1, acPut, isGE0, jumpAction);
  return normal;
}

KM10::InstructionResult KM10::doAOJN() {
  doAOSXX(acGet, 1, acPut, isNE0, jumpAction);
  return normal;
}

KM10::InstructionResult KM10::doAOJG() {
  doAOSXX(acGet, 1, acPut, isGT0, jumpAction);
  return normal;
}

KM10::InstructionResult KM10::doAOS() {
  doAOSXX(memGet, 1, memPut, never, skipAction);
  return normal;
}

KM10::InstructionResult KM10::doAOSL() {
  doAOSXX(memGet, 1, memPut, isLT0, skipAction);
  return normal;
}

KM10::InstructionResult KM10::doAOSE() {
  doAOSXX(memGet, 1, memPut, isEQ0, skipAction);
  return normal;
}

KM10::InstructionResult KM10::doAOSLE() {
  doAOSXX(memGet, 1, memPut, isLE0, skipAction);
  return normal;
}

KM10::InstructionResult KM10::doAOSA() {
  doAOSXX(memGet, 1, memPut, always, skipAction);
  return normal;
}

KM10::InstructionResult KM10::doAOSGE() {
  doAOSXX(memGet, 1, memPut, isGE0, skipAction);
  return normal;
}

KM10::InstructionResult KM10::doAOSN() {
  doAOSXX(memGet, 1, memPut, isNE0, skipAction);
  return normal;
}

KM10::InstructionResult KM10::doAOSG() {
  doAOSXX(memGet, 1, memPut, isGT0, skipAction);
  return normal;
}

KM10::InstructionResult KM10::doSOJ() {
  doAOSXX(acGet, -1, acPut, never, jumpAction);
  return normal;
}

KM10::InstructionResult KM10::doSOJL() {
  doAOSXX(acGet, -1, acPut, isLT0, jumpAction);
  return normal;
}

KM10::InstructionResult KM10::doSOJE() {
  doAOSXX(acGet, -1, acPut, isEQ0, jumpAction);
  return normal;
}

KM10::InstructionResult KM10::doSOJLE() {
  doAOSXX(acGet, -1, acPut, isLE0, jumpAction);
  return normal;
}

KM10::InstructionResult KM10::doSOJA() {
  doAOSXX(acGet, -1, acPut, always, jumpAction);
  return normal;
}

KM10::InstructionResult KM10::doSOJGE() {
  doAOSXX(acGet, -1, acPut, isGE0, jumpAction);
  return normal;
}

KM10::InstructionResult KM10::doSOJN() {
  doAOSXX(acGet, -1, acPut, isNE0, jumpAction);
  return normal;
}

KM10::InstructionResult KM10::doSOJG() {
  doAOSXX(acGet, -1, acPut, isGT0, jumpAction);
  return normal;
}

KM10::InstructionResult KM10::doSOS() {
  doAOSXX(memGet, -1, memPut, never, skipAction);
  return normal;
}

KM10::InstructionResult KM10::doSOSL() {
  doAOSXX(memGet, -1, memPut, isLT0, skipAction);
  return normal;
}

KM10::InstructionResult KM10::doSOSE() {
  doAOSXX(memGet, -1, memPut, isEQ0, skipAction);
  return normal;
}

KM10::InstructionResult KM10::doSOSLE() {
  doAOSXX(memGet, -1, memPut, isLE0, skipAction);
  return normal;
}

KM10::InstructionResult KM10::doSOSA() {
  doAOSXX(memGet, -1, memPut, always, skipAction);
  return normal;
}

KM10::InstructionResult KM10::doSOSGE() {
  doAOSXX(memGet, -1, memPut, isGE0, skipAction);
  return normal;
}

KM10::InstructionResult KM10::doSOSN() {
  doAOSXX(memGet, -1, memPut, isNE0, skipAction);
  return normal;
}

KM10::InstructionResult KM10::doSOSG() {
  doAOSXX(memGet, -1, memPut, isGT0, skipAction);
  return normal;
}

KM10::InstructionResult KM10::doSETZ() {
  doSETXX(memGet, zeroWord, acPut);
  return normal;
}

KM10::InstructionResult KM10::doSETZI() {
  doSETXX(immediate, zeroWord, acPut);
  return normal;
}

KM10::InstructionResult KM10::doSETZM() {
  doSETXX(memGet, zeroWord, memPut);
  return normal;
}

KM10::InstructionResult KM10::doSETZB() {
  doSETXX(memGet, zeroWord, bothPut);
  return normal;
}

KM10::InstructionResult KM10::doAND() {
  doBinOp(memGet, acGet, andWord, acPut);
  return normal;
}

KM10::InstructionResult KM10::doANDI() {
  doBinOp(immediate, acGet, andWord, acPut);
  return normal;
}

KM10::InstructionResult KM10::doANDM() {
  doBinOp(memGet, acGet, andWord, memPut);
  return normal;
}

KM10::InstructionResult KM10::doANDB() {
  doBinOp(memGet, acGet, andWord, bothPut);
  return normal;
}

KM10::InstructionResult KM10::doANDCA() {
  doBinOp(memGet, acGet, andCWord, acPut);
  return normal;
}

KM10::InstructionResult KM10::doANDCAI() {
  doBinOp(immediate, acGet, andCWord, acPut);
  return normal;
}

KM10::InstructionResult KM10::doANDCAM() {
  doBinOp(memGet, acGet, andCWord, memPut);
  return normal;
}

KM10::InstructionResult KM10::doANDCAB() {
  doBinOp(memGet, acGet, andCWord, bothPut);
  return normal;
}

KM10::InstructionResult KM10::doSETM() {
  doSETXX(memGet, noMod1, acPut);
  return normal;
}

KM10::InstructionResult KM10::doSETMI() {
  doSETXX(immediate, noMod1, acPut);
  return normal;
}

KM10::InstructionResult KM10::doSETMM() {
  doSETXX(memGet, noMod1, memPut);
  return normal;
}

KM10::InstructionResult KM10::doSETMB() {
  doSETXX(memGet, noMod1, bothPut);
  return normal;
}

KM10::InstructionResult KM10::doANDCM() {
  doBinOp(acGet, memGet, andCWord, acPut);
  return normal;
}

KM10::InstructionResult KM10::doANDCMI() {
  doBinOp(acGet, immediate, andCWord, acPut);
  return normal;
}

KM10::InstructionResult KM10::doANDCMM() {
  doBinOp(acGet, memGet, andCWord, memPut);
  return normal;
}

KM10::InstructionResult KM10::doANDCMB() {
  doBinOp(acGet, memGet, andCWord, bothPut);
  return normal;
}

KM10::InstructionResult KM10::doSETA() {
  doSETXX(acGet, noMod1, acPut);
  return normal;
}

KM10::InstructionResult KM10::doSETAI() {
  doSETXX(acGet, noMod1, acPut);
  return normal;
}

KM10::InstructionResult KM10::doSETAM() {
  doSETXX(acGet, noMod1, memPut);
  return normal;
}

KM10::InstructionResult KM10::doSETAB() {
  doSETXX(acGet, noMod1, bothPut);
  return normal;
}

KM10::InstructionResult KM10::doXOR() {
  doBinOp(memGet, acGet, xorWord, acPut);
  return normal;
}

KM10::InstructionResult KM10::doXORI() {
  doBinOp(immediate, acGet, xorWord, acPut);
  return normal;
}

KM10::InstructionResult KM10::doXORM() {
  doBinOp(memGet, acGet, xorWord, memPut);
  return normal;
}

KM10::InstructionResult KM10::doXORB() {
  doBinOp(memGet, acGet, xorWord, bothPut);
  return normal;
}

KM10::InstructionResult KM10::doIOR() {
  doBinOp(memGet, acGet, iorWord, acPut);
  return normal;
}

KM10::InstructionResult KM10::doIORI() {
  doBinOp(immediate, acGet, iorWord, acPut);
  return normal;
}

KM10::InstructionResult KM10::doIORM() {
  doBinOp(memGet, acGet, iorWord, memPut);
  return normal;
}

KM10::InstructionResult KM10::doIORB() {
  doBinOp(memGet, acGet, iorWord, bothPut);
  return normal;
}

KM10::InstructionResult KM10::doANDCBM() {
  doBinOp(memGet, acGet, andCBWord, acPut);
  return normal;
}

KM10::InstructionResult KM10::doANDCBMI() {
  doBinOp(immediate, acGet, andCBWord, acPut);
  return normal;
}

KM10::InstructionResult KM10::doANDCBMM() {
  doBinOp(memGet, acGet, andCBWord, memPut);
  return normal;
}

KM10::InstructionResult KM10::doANDCBMB() {
  doBinOp(memGet, acGet, andCBWord, bothPut);
  return normal;
}

KM10::InstructionResult KM10::doEQV() {
  doBinOp(memGet, acGet, eqvWord, acPut);
  return normal;
}

KM10::InstructionResult KM10::doEQVI() {
  doBinOp(immediate, acGet, eqvWord, acPut);
  return normal;
}

KM10::InstructionResult KM10::doEQVM() {
  doBinOp(memGet, acGet, eqvWord, memPut);
  return normal;
}

KM10::InstructionResult KM10::doEQVB() {
  doBinOp(memGet, acGet, eqvWord, bothPut);
  return normal;
}

KM10::InstructionResult KM10::doSETCA() {
  doSETXX(acGet, compWord, acPut);
  return normal;
}

KM10::InstructionResult KM10::doSETCAI() {
  doSETXX(acGet, compWord, acPut);
  return normal;
}

KM10::InstructionResult KM10::doSETCAM() {
  doSETXX(acGet, compWord, memPut);
  return normal;
}

KM10::InstructionResult KM10::doSETCAB() {
  doSETXX(acGet, compWord, bothPut);
  return normal;
}

KM10::InstructionResult KM10::doORCA() {
  doBinOp(memGet, acGet, iorCWord, acPut);
  return normal;
}

KM10::InstructionResult KM10::doORCAI() {
  doBinOp(immediate, acGet, iorCWord, acPut);
  return normal;
}

KM10::InstructionResult KM10::doORCAM() {
  doBinOp(memGet, acGet, iorCWord, memPut);
  return normal;
}

KM10::InstructionResult KM10::doORCAB() {
  doBinOp(memGet, acGet, iorCWord, bothPut);
  return normal;
}

KM10::InstructionResult KM10::doSETCM() {
  doSETXX(memGet, compWord, acPut);
  return normal;
}

KM10::InstructionResult KM10::doSETCMI() {
  doSETXX(immediate, compWord, acPut);
  return normal;
}

KM10::InstructionResult KM10::doSETCMM() {
  doSETXX(memGet, compWord, memPut);
  return normal;
}

KM10::InstructionResult KM10::doSETCMB() {
  doSETXX(memGet, compWord, bothPut);
  return normal;
}

KM10::InstructionResult KM10::doORCM() {
  doBinOp(acGet, memGet, iorCWord, acPut);
  return normal;
}

KM10::InstructionResult KM10::doORCMI() {
  doBinOp(acGet, immediate, iorCWord, acPut);
  return normal;
}

KM10::InstructionResult KM10::doORCMM() {
  doBinOp(acGet, memGet, iorCWord, memPut);
  return normal;
}

KM10::InstructionResult KM10::doORCMB() {
  doBinOp(acGet, memGet, iorCWord, bothPut);
  return normal;
}

KM10::InstructionResult KM10::doORCB() {
  doBinOp(memGet, acGet, iorCBWord, acPut);
  return normal;
}

KM10::InstructionResult KM10::doORCBI() {
  doBinOp(immediate, acGet, iorCBWord, acPut);
  return normal;
}

KM10::InstructionResult KM10::doORCBM() {
  doBinOp(memGet, acGet, iorCBWord, memPut);
  return normal;
}

KM10::InstructionResult KM10::doORCBB() {
  doBinOp(memGet, acGet, iorCBWord, bothPut);
  return normal;
}

KM10::InstructionResult KM10::doSETO() {
  doSETXX(acGet, onesWord, acPut);
  return normal;
}

KM10::InstructionResult KM10::doSETOI() {
  doSETXX(acGet, onesWord, acPut);
  return normal;
}

KM10::InstructionResult KM10::doSETOM() {
  doSETXX(memGet, onesWord, memPut);
  return normal;
}

KM10::InstructionResult KM10::doSETOB() {
  doSETXX(memGet, onesWord, bothPut);
  return normal;
}

KM10::InstructionResult KM10::doHLL() {
  doHXXXX(memGet, acGet, copyHLL, noMod1, acPut);
  return normal;
}

KM10::InstructionResult KM10::doHLLI() {
  doHXXXX(immediate, acGet, copyHLL, noMod1, acPut);
  return normal;
}

KM10::InstructionResult KM10::doHLLM() {
  doHXXXX(acGet, memGet, copyHLL, noMod1, memPut);
  return normal;
}

KM10::InstructionResult KM10::doHLLS() {
  doHXXXX(memGet, memGet, copyHLL, noMod1, selfPut);
  return normal;
}

KM10::InstructionResult KM10::doHRL() {
  doHXXXX(memGet, acGet, copyHRL, noMod1, acPut);
  return normal;
}

KM10::InstructionResult KM10::doHRLI() {
  doHXXXX(immediate, acGet, copyHRL, noMod1, acPut);
  return normal;
}

KM10::InstructionResult KM10::doHRLM() {
  doHXXXX(acGet, memGet, copyHRL, noMod1, memPut);
  return normal;
}

KM10::InstructionResult KM10::doHRLS() {
  doHXXXX(memGet, memGet, copyHRL, noMod1, selfPut);
  return normal;
}

KM10::InstructionResult KM10::doHLLZ() {
  doHXXXX(memGet, acGet, copyHLL, zeroR, acPut);
  return normal;
}

KM10::InstructionResult KM10::doHLLZI() {
  doHXXXX(immediate, acGet, copyHLL, zeroR, acPut);
  return normal;
}

KM10::InstructionResult KM10::doHLLZM() {
  doHXXXX(acGet, memGet, copyHLL, zeroR, memPut);
  return normal;
}

KM10::InstructionResult KM10::doHLLZS() {
  doHXXXX(memGet, memGet, copyHLL, zeroR, selfPut);
  return normal;
}

KM10::InstructionResult KM10::doHRLZ() {
  doHXXXX(memGet, acGet, copyHRL, zeroR, acPut);
  return normal;
}

KM10::InstructionResult KM10::doHRLZI() {
  doHXXXX(immediate, acGet, copyHRL, zeroR, acPut);
  return normal;
}

KM10::InstructionResult KM10::doHRLZM() {
  doHXXXX(acGet, memGet, copyHRL, zeroR, memPut);
  return normal;
}

KM10::InstructionResult KM10::doHRLZS() {
  doHXXXX(memGet, memGet, copyHRL, zeroR, selfPut);
  return normal;
}

KM10::InstructionResult KM10::doHLLO() {
  doHXXXX(memGet, acGet, copyHLL, onesR, acPut);
  return normal;
}

KM10::InstructionResult KM10::doHLLOI() {
  doHXXXX(immediate, acGet, copyHLL, onesR, acPut);
  return normal;
}

KM10::InstructionResult KM10::doHLLOM() {
  doHXXXX(acGet, memGet, copyHLL, onesR, memPut);
  return normal;
}

KM10::InstructionResult KM10::doHLLOS() {
  doHXXXX(memGet, memGet, copyHLL, onesR, selfPut);
  return normal;
}

KM10::InstructionResult KM10::doHRLO() {
  doHXXXX(memGet, acGet, copyHRL, onesR, acPut);
  return normal;
}

KM10::InstructionResult KM10::doHRLOI() {
  doHXXXX(immediate, acGet, copyHRL, onesR, acPut);
  return normal;
}

KM10::InstructionResult KM10::doHRLOM() {
  doHXXXX(acGet, memGet, copyHRL, onesR, memPut);
  return normal;
}

KM10::InstructionResult KM10::doHRLOS() {
  doHXXXX(memGet, memGet, copyHRL, onesR, selfPut);
  return normal;
}

KM10::InstructionResult KM10::doHLLE() {
  doHXXXX(memGet, acGet, copyHLL, extnL, acPut);
  return normal;
}

KM10::InstructionResult KM10::doHLLEI() {
  doHXXXX(immediate, acGet, copyHLL, extnL, acPut);
  return normal;
}

KM10::InstructionResult KM10::doHLLEM() {
  doHXXXX(acGet, memGet, copyHLL, extnL, memPut);
  return normal;
}

KM10::InstructionResult KM10::doHLLES() {
  doHXXXX(memGet, memGet, copyHLL, extnL, selfPut);
  return normal;
}

KM10::InstructionResult KM10::doHRLE() {
  doHXXXX(memGet, acGet, copyHRL, extnL, acPut);
  return normal;
}

KM10::InstructionResult KM10::doHRLEI() {
  doHXXXX(immediate, acGet, copyHRL, extnL, acPut);
  return normal;
}

KM10::InstructionResult KM10::doHRLEM() {
  doHXXXX(acGet, memGet, copyHRL, extnL, memPut);
  return normal;
}

KM10::InstructionResult KM10::doHRLES() {
  doHXXXX(memGet, memGet, copyHRL, extnL, selfPut);
  return normal;
}

KM10::InstructionResult KM10::doHRR() {
  doHXXXX(memGet, acGet, copyHRR, noMod1, acPut);
  return normal;
}

KM10::InstructionResult KM10::doHRRI() {
  doHXXXX(immediate, acGet, copyHRR, noMod1, acPut);
  return normal;
}

KM10::InstructionResult KM10::doHRRM() {
  doHXXXX(acGet, memGet, copyHRR, noMod1, memPut);
  return normal;
}

KM10::InstructionResult KM10::doHRRS() {
  doHXXXX(memGet, memGet, copyHRR, noMod1, selfPut);
  return normal;
}

KM10::InstructionResult KM10::doHLR() {
  doHXXXX(memGet, acGet, copyHLR, noMod1, acPut);
  return normal;
}

KM10::InstructionResult KM10::doHLRI() {
  doHXXXX(immediate, acGet, copyHLR, noMod1, acPut);
  return normal;
}

KM10::InstructionResult KM10::doHLRM() {
  doHXXXX(acGet, memGet, copyHLR, noMod1, memPut);
  return normal;
}

KM10::InstructionResult KM10::doHLRS() {
  doHXXXX(memGet, memGet, copyHLR, noMod1, selfPut);
  return normal;
}

KM10::InstructionResult KM10::doHRRZ() {
  doHXXXX(memGet, acGet, copyHRR, zeroL, acPut);
  return normal;
}

KM10::InstructionResult KM10::doHRRZI() {
  doHXXXX(immediate, acGet, copyHRR, zeroL, acPut);
  return normal;
}

KM10::InstructionResult KM10::doHRRZM() {
  doHXXXX(acGet, memGet, copyHRR, zeroL, memPut);
  return normal;
}

KM10::InstructionResult KM10::doHRRZS() {
  doHXXXX(memGet, memGet, copyHRR, zeroL, selfPut);
  return normal;
}

KM10::InstructionResult KM10::doHLRZ() {
  doHXXXX(memGet, acGet, copyHLR, zeroL, acPut);
  return normal;
}

KM10::InstructionResult KM10::doHLRZI() {
  doHXXXX(immediate, acGet, copyHLR, zeroL, acPut);
  return normal;
}

KM10::InstructionResult KM10::doHLRZM() {
  doHXXXX(acGet, memGet, copyHLR, zeroL, memPut);
  return normal;
}

KM10::InstructionResult KM10::doHLRZS() {
  doHXXXX(memGet, memGet, copyHLR, zeroL, selfPut);
  return normal;
}

KM10::InstructionResult KM10::doHRRO() {
  doHXXXX(memGet, acGet, copyHRR, onesL, acPut);
  return normal;
}

KM10::InstructionResult KM10::doHRROI() {
  doHXXXX(immediate, acGet, copyHRR, onesL, acPut);
  return normal;
}

KM10::InstructionResult KM10::doHRROM() {
  doHXXXX(acGet, memGet, copyHRR, onesL, memPut);
  return normal;
}

KM10::InstructionResult KM10::doHRROS() {
  doHXXXX(memGet, memGet, copyHRR, onesL, selfPut);
  return normal;
}

KM10::InstructionResult KM10::doHLRO() {
  doHXXXX(memGet, acGet, copyHLR, onesL, acPut);
  return normal;
}

KM10::InstructionResult KM10::doHLROI() {
  doHXXXX(immediate, acGet, copyHLR, onesL, acPut);
  return normal;
}

KM10::InstructionResult KM10::doHLROM() {
  doHXXXX(acGet, memGet, copyHLR, onesL, memPut);
  return normal;
}

KM10::InstructionResult KM10::doHLROS() {
  doHXXXX(memGet, memGet, copyHLR, onesL, selfPut);
  return normal;
}

KM10::InstructionResult KM10::doHRRE() {
  doHXXXX(memGet, acGet, copyHRR, extnR, acPut);
  return normal;
}

KM10::InstructionResult KM10::doHRREI() {
  doHXXXX(immediate, acGet, copyHRR, extnR, acPut);
  return normal;
}

KM10::InstructionResult KM10::doHRREM() {
  doHXXXX(acGet, memGet, copyHRR, extnR, memPut);
  return normal;
}

KM10::InstructionResult KM10::doHRRES() {
  doHXXXX(memGet, memGet, copyHRR, extnR, selfPut);
  return normal;
}

KM10::InstructionResult KM10::doHLRE() {
  doHXXXX(memGet, acGet, copyHLR, extnR, acPut);
  return normal;
}

KM10::InstructionResult KM10::doHLREI() {
  doHXXXX(immediate, acGet, copyHLR, extnR, acPut);
  return normal;
}

KM10::InstructionResult KM10::doHLREM() {
  doHXXXX(acGet, memGet, copyHLR, extnR, memPut);
  return normal;
}

KM10::InstructionResult KM10::doHLRES() {
  doHXXXX(memGet, memGet, copyHLR, extnR, selfPut);
  return normal;
}

KM10::InstructionResult KM10::doTRN() {
  return normal;
}

KM10::InstructionResult KM10::doTLN() {
  return normal;
}

KM10::InstructionResult KM10::doTRNE() {
  doTXXXX(acGetRH, getE, noMod2, isEQ0T, noStore);
  return normal;
}

KM10::InstructionResult KM10::doTLNE() {
  doTXXXX(acGetLH, getE, noMod2, isEQ0T, noStore);
  return normal;
}

KM10::InstructionResult KM10::doTRNA() {
  ++state.nextPC.rhu;
  return normal;
}

KM10::InstructionResult KM10::doTLNA() {
  ++state.nextPC.rhu;
  return normal;
}

KM10::InstructionResult KM10::doTRNN() {
  doTXXXX(acGetRH, getE, noMod2, isNE0T, noStore);
  return normal;
}

KM10::InstructionResult KM10::doTLNN() {
  doTXXXX(acGetLH, getE, noMod2, isNE0T, noStore);
  return normal;
}

KM10::InstructionResult KM10::doTRZ() {
  doTXXXX(acGetRH, getE, zeroMaskR, neverT, acPutRH);
  return normal;
}

KM10::InstructionResult KM10::doTLZ() {
  doTXXXX(acGetLH, getE, zeroMaskR, neverT, acPutLH);
  return normal;
}

KM10::InstructionResult KM10::doTRZE() {
  doTXXXX(acGetRH, getE, zeroMaskR, isEQ0T, acPutRH);
  return normal;
}

KM10::InstructionResult KM10::doTLZE() {
  doTXXXX(acGetLH, getE, zeroMaskR, isEQ0T, acPutLH);
  return normal;
}

KM10::InstructionResult KM10::doTRZA() {
  doTXXXX(acGetRH, getE, zeroMaskR, alwaysT, acPutRH);
  return normal;
}

KM10::InstructionResult KM10::doTLZA() {
  doTXXXX(acGetLH, getE, zeroMaskR, alwaysT, acPutLH);
  return normal;
}

KM10::InstructionResult KM10::doTRZN() {
  doTXXXX(acGetRH, getE, zeroMaskR, isNE0T, acPutRH);
  return normal;
}

KM10::InstructionResult KM10::doTLZN() {
  doTXXXX(acGetLH, getE, zeroMaskR, isNE0T, acPutLH);
  return normal;
}

KM10::InstructionResult KM10::doTRC() {
  doTXXXX(acGetRH, getE, compMaskR, neverT, acPutRH);
  return normal;
}

KM10::InstructionResult KM10::doTLC() {
  doTXXXX(acGetLH, getE, compMaskR, neverT, acPutLH);
  return normal;
}

KM10::InstructionResult KM10::doTRCE() {
  doTXXXX(acGetRH, getE, compMaskR, isEQ0T, acPutRH);
  return normal;
}

KM10::InstructionResult KM10::doTLCE() {
  doTXXXX(acGetLH, getE, compMaskR, isEQ0T, acPutLH);
  return normal;
}

KM10::InstructionResult KM10::doTRCA() {
  doTXXXX(acGetRH, getE, compMaskR, alwaysT, acPutRH);
  return normal;
}

KM10::InstructionResult KM10::doTLCA() {
  doTXXXX(acGetLH, getE, compMaskR, alwaysT, acPutLH);
  return normal;
}

KM10::InstructionResult KM10::doTRCN() {
  doTXXXX(acGetRH, getE, compMaskR, isNE0T, acPutRH);
  return normal;
}

KM10::InstructionResult KM10::doTLCN() {
  doTXXXX(acGetLH, getE, compMaskR, isNE0T, acPutLH);
  return normal;
}

KM10::InstructionResult KM10::doTRO() {
  doTXXXX(acGetRH, getE, onesMaskR, neverT, acPutRH);
  return normal;
}

KM10::InstructionResult KM10::doTLO() {
  doTXXXX(acGetLH, getE, onesMaskR, neverT, acPutLH);
  return normal;
}

KM10::InstructionResult KM10::doTROE() {
  doTXXXX(acGetRH, getE, onesMaskR, isEQ0T, acPutRH);
  return normal;
}

KM10::InstructionResult KM10::doTLOE() {
  doTXXXX(acGetLH, getE, onesMaskR, isEQ0T, acPutLH);
  return normal;
}

KM10::InstructionResult KM10::doTROA() {
  doTXXXX(acGetRH, getE, onesMaskR, alwaysT, acPutRH);
  return normal;
}

KM10::InstructionResult KM10::doTLOA() {
  doTXXXX(acGetLH, getE, onesMaskR, alwaysT, acPutLH);
  return normal;
}

KM10::InstructionResult KM10::doTRON() {
  doTXXXX(acGetRH, getE, onesMaskR, isNE0T, acPutRH);
  return normal;
}

KM10::InstructionResult KM10::doTLON() {
  doTXXXX(acGetLH, getE, onesMaskR, isNE0T, acPutLH);
  return normal;
}

KM10::InstructionResult KM10::doTDN() {
  return normal;
}

KM10::InstructionResult KM10::doTSN() {
  return normal;
}

KM10::InstructionResult KM10::doTDNE() {
  doTXXXX(acGet, memGet, noMod2, isEQ0T, noStore);
  return normal;
}

KM10::InstructionResult KM10::doTSNE() {
  doTXXXX(acGet, memGetSwapped, noMod2, isEQ0T, noStore);
  return normal;
}

KM10::InstructionResult KM10::doTDNA() {
  ++state.nextPC.rhu;
  return normal;
}

KM10::InstructionResult KM10::doTSNA() {
  ++state.nextPC.rhu;
  return normal;
}

KM10::InstructionResult KM10::doTDNN() {
  doTXXXX(acGet, memGet, noMod2, isNE0T, noStore);
  return normal;
}

KM10::InstructionResult KM10::doTSNN() {
  doTXXXX(acGet, memGetSwapped, noMod2, isNE0T, noStore);
  return normal;
}

KM10::InstructionResult KM10::doTDZ() {
  doTXXXX(acGet, memGet, zeroMask, neverT, acPut);
  return normal;
}

KM10::InstructionResult KM10::doTSZ() {
  doTXXXX(acGet, memGetSwapped, zeroMask, neverT, acPut);
  return normal;
}

KM10::InstructionResult KM10::doTDZE() {
  doTXXXX(acGet, memGet, zeroMask, isEQ0T, acPut);
  return normal;
}

KM10::InstructionResult KM10::doTSZE() {
  doTXXXX(acGet, memGetSwapped, zeroMask, isEQ0T, acPut);
  return normal;
}

KM10::InstructionResult KM10::doTDZA() {
  doTXXXX(acGet, memGet, zeroMask, alwaysT, acPut);
  return normal;
}

KM10::InstructionResult KM10::doTSZA() {
  doTXXXX(acGet, memGetSwapped, zeroMask, alwaysT, acPut);
  return normal;
}

KM10::InstructionResult KM10::doTDZN() {
  doTXXXX(acGet, memGet, zeroMask, isNE0T, acPut);
  return normal;
}

KM10::InstructionResult KM10::doTSZN() {
  doTXXXX(acGet, memGetSwapped, zeroMask, isNE0T, acPut);
  return normal;
}

KM10::InstructionResult KM10::doTDC() {
  doTXXXX(acGet, memGet, compMask, neverT, acPut);
  return normal;
}

KM10::InstructionResult KM10::doTSC() {
  doTXXXX(acGet, memGetSwapped, compMask, neverT, acPut);
  return normal;
}

KM10::InstructionResult KM10::doTDCE() {
  doTXXXX(acGet, memGet, compMask, isEQ0T, acPut);
  return normal;
}

KM10::InstructionResult KM10::doTSCE() {
  doTXXXX(acGet, memGetSwapped, compMask, isEQ0T, acPut);
  return normal;
}

KM10::InstructionResult KM10::doTDCA() {
  doTXXXX(acGet, memGet, compMask, alwaysT, acPut);
  return normal;
}

KM10::InstructionResult KM10::doTSCA() {
  doTXXXX(acGet, memGetSwapped, compMask, alwaysT, acPut);
  return normal;
}

KM10::InstructionResult KM10::doTDCN() {
  doTXXXX(acGet, memGet, compMask, isNE0T, acPut);
  return normal;
}

KM10::InstructionResult KM10::doTSZCN() {
  doTXXXX(acGet, memGetSwapped, compMask, isNE0T, acPut);
  return normal;
}

KM10::InstructionResult KM10::doTDO() {
  doTXXXX(acGet, memGet, onesMask, neverT, acPut);
  return normal;
}

KM10::InstructionResult KM10::doTSO() {
  doTXXXX(acGet, memGetSwapped, onesMask, neverT, acPut);
  return normal;
}

KM10::InstructionResult KM10::doTDOE() {
  doTXXXX(acGet, memGet, onesMask, isEQ0T, acPut);
  return normal;
}

KM10::InstructionResult KM10::doTSOE() {
  doTXXXX(acGet, memGetSwapped, onesMask, isEQ0T, acPut);
  return normal;
}

KM10::InstructionResult KM10::doTDOA() {
  doTXXXX(acGet, memGet, onesMask, alwaysT, acPut);
  return normal;
}

KM10::InstructionResult KM10::doTSOA() {
  doTXXXX(acGet, memGetSwapped, onesMask, alwaysT, acPut);
  return normal;
}

KM10::InstructionResult KM10::doTDON() {
  doTXXXX(acGet, memGet, onesMask, isNE0T, acPut);
  return normal;
}

KM10::InstructionResult KM10::doTSON() {
  doTXXXX(acGet, memGetSwapped, onesMask, isNE0T, acPut);
  return normal;
}




////////////////////////////////////////////////////////////////
void KM10::emulate(Debugger *debuggerP) {
  W36 exceptionPC{0};

  ////////////////////////////////////////////////////////////////
  // Connect our DTE20 (put console into raw mode)
  dte.connect();

  // The instruction loop
  for (;;) {
    // Keep the cache sweep timer ticking until it goes DING.
    cca.handleSweep();

    // Handle execution (PC) breakpoints
    if (state.executeBPs.contains(state.pc.vma)) state.running = false;

    // Prepare to fetch next iw and remember if it's an interrupt or
    // trap.
    if ((state.flags.tr1 || state.flags.tr2) && pag.pagerEnabled()) {
      // We have a trap.
      state.exceptionPC = state.pc;
      state.pc = state.eptAddressFor(state.flags.tr1 ?
				     &state.eptP->trap1Insn :
				     &state.eptP->stackOverflowInsn);
      state.inInterrupt = true;
      cerr << ">>>>> trap cycle PC now=" << state.pc.fmtVMA()
	   << "  exceptionPC=" << state.exceptionPC.fmtVMA()
	   << logger.endl << flush;
    } else if (W36 vector = pi.setUpInterruptCycleIfPending(); vector != W36(0)) {
      // We have an active interrupt.
      state.exceptionPC = state.pc;
      state.pc = vector;
      state.inInterrupt = true;
      cerr << ">>>>> interrupt cycle PC now=" << state.pc.fmtVMA()
	   << "  exceptionPC=" << state.exceptionPC.fmtVMA()
	   << logger.endl << flush;
    }

    // Now fetch the instruction at our normal, exception, or interrupt PC.
    iw = state.memGetN(state.pc);

    // Capture next PC AFTER we possibly set up to handle an exception or interrupt.
    if (!state.inXCT) {
      state.nextPC.rhu = state.pc.rhu + 1;
      state.nextPC.lhu = state.pc.lhu;
    }

    // If we're debugging, this is where we pause to let the user
    // inspect and change things. The debugger tells us what our next
    // action should be based on its return value.
    if (!state.running) {

      switch (debuggerP->debug()) {
      case Debugger::step:	// Debugger has set step count in state.nSteps.
	break;

      case Debugger::run:	// Continue from current PC (state.nSteps set by debugger to 0).
	break;

      case Debugger::quit:	// Quit from emulator.
	return;

      case Debugger::restart:	// Restart emulator like a PDP10 reboot
	return;

      case Debugger::pcChanged:	// PC changed by debugger - go fetch again
	continue;

      default:			// This should never happen...
	assert("Debugger returned unknown action" == nullptr);
	return;
      }
    }

    // Handle nSteps so we don't keep running if we run out of step
    // count. THIS instruction is our single remaining step. If
    // state.nSteps is zero we just keep going "forever".
    if (state.nSteps > 0) {
      if (--state.nSteps <= 0) state.running = false;
    }

    if (logger.loggingToFile && logger.pc) {
      logger.s << state.pc.fmtVMA() << ": " << debuggerP->dump(iw, state.pc);
    }

    // Unless we encounter ANOTHER XCT we're not in one now.
    state.inXCT = false;

    // Compute effective address
    ea.u = state.getEA(iw.i, iw.x, iw.y);

    // Execute the instruction in `iw`.
    InstructionResult result = (this->*(ops[iw.op]))();

    // XXX update PC, etc.
    switch (result) {
    case normal:
    case skip:
    case jump:
    case muuo:
    case luuo:
    case trap:
    case halt:
    case xct:
      break;
    }

    // If we're in a xUUO trap, only the first instruction we execute
    // there is special, so clear state.inInterrupt.
    if (state.inInterrupt) {
      cerr << "[IN INTERRUPT and about to clear that state]" << logger.endl << flush;
      state.inInterrupt = false;
    }


    if (logger.pc || logger.mem || logger.ac || logger.io || logger.dte)
      logger.s << logger.endl << flush;
  }

  // Restore console to normal
  dte.disconnect();
}


//////////////////////////////////////////////////////////////
// This is invoked in a loop to allow the "restart" command to work
// properly. Therefore this needs to clean up the state of the machine
// before it returns. This is mostly done by auto destructors.
static int loopedMain(int argc, char *argv[]) {
  assert(sizeof(KMState::ExecutiveProcessTable) == 512 * 8);
  assert(sizeof(KMState::UserProcessTable) == 512 * 8);
  gflags::ParseCommandLineFlags(&argc, &argv, true);

  KMState state(4 * 1024 * 1024, aBPs, eBPs);

  if (FLAGS_load != "none") {

    if (FLAGS_load.ends_with(".a10")) {
      state.loadA10(FLAGS_load.c_str());
    } else if (FLAGS_load.ends_with(".sav")) {
      cerr << "ERROR: For now, '-load' option must name a .a10 file" << logger.endl;
      return -1;
      //      state.loadSAV(FLAGS_load.c_str());
    } else {
      cerr << "ERROR: '-load' option must name a .a10 or .sav file" << logger.endl;
      return -1;
    }

    cerr << "[Loaded " << FLAGS_load << "  start=" << state.pc.fmtVMA() << "]" << logger.endl;
  }

  KM10 km10(state);
  Debugger debugger(km10, state);

  if (FLAGS_rel != "none") {
    debugger.loadREL(FLAGS_rel.c_str());
  }

  state.running = !FLAGS_debug;
  km10.emulate(&debugger);

  return state.restart ? 1 : 0;
}


////////////////////////////////////////////////////////////////
int main(int argc, char *argv[]) {
  int st;
  
  while ((st = loopedMain(argc, argv)) > 0) {
    cerr << endl << "[restarting]" << endl;
  }

  return st;
}


////////////////////////////////////////////////////////////////
void Logger::nyi(KMState &state, const string &context) {
  s << " [not yet implemented: " << context << "]";
  cerr << "Not yet implemented at " << state.pc.fmtVMA() << endl;
}


void Logger::nsd(KMState &state, const string &context) {
  s << " [no such device: " << context << "]";
  cerr << "No such device at " << state.pc.fmtVMA() << endl;
}
