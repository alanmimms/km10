#include "km10.hpp"

struct IntBinGroup: KM10 {

  inline void bothPut(W36 v) {
    acPut(v);
    memPut(v);
  }

  // XXX this needs to be refactored so it can return iSkip vs iNormal.
  W36 addWord(W36 s1, W36 s2) {
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
  }
    

  W36 subWord(W36 s1, W36 s2) {
    int64_t diff = s1.ext64() - s2.ext64();

    if (diff < -(int64_t) W36::bit0) {
      flags.tr1 = flags.ov = flags.cy0 = 1;
    } else if ((uint64_t) diff >= W36::bit0) {
      flags.tr1 = flags.ov = flags.cy1 = 1;
    }

    return diff;
  }


  W72 mulWord(W36 s1, W36 s2) {
    int128_t prod128 = (int128_t) s1.ext64() * s2.ext64();
    W72 prod = W72::fromMag((uint128_t) (prod128 < 0 ? -prod128 : prod128), prod128 < 0);

    if (s1.u == W36::bit0 && s2.u == W36::bit0) {
      flags.tr1 = flags.ov = 1;
      return W72{W36{1ull << 34}, W36{0}};
    }

    return prod;
  }
    

  W36 imulWord(W36 s1, W36 s2) {
    int128_t prod128 = (int128_t) s1.ext64() * s2.ext64();
    W72 prod = W72::fromMag((uint128_t) (prod128 < 0 ? -prod128 : prod128), prod128 < 0);

    if (s1.u == W36::bit0 && s2.u == W36::bit0) {
      flags.tr1 = flags.ov = 1;
    }

    return W36((prod.s < 0 ? W36::bit0 : 0) | ((W36::all1s >> 1) & prod.u));
  }

    
  W72 idivWord(W36 s1, W36 s2) {

    if ((s1.u == W36::bit0 && s2.s == -1ll) || s2.u == 0ull) {
      flags.ndv = flags.tr1 = flags.ov = 1;
      return W72{s1, s2};
    } else {
      int64_t quo = s1.s / s2.s;
      int64_t rem = abs(s1.s % s2.s);
      if (quo < 0) rem = -rem;
      return W72{W36{quo}, W36{abs(rem)}};
    }
  }

    
  W72 divWord(W72 s1, W36 s2) {
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
  }

  InstructionResult doIMUL() {
    W36 a1 = acGet();
    W36 a2 = memGet();
    acPut(imulWord(a1, a2));
    return iNormal;
  }

  InstructionResult doIMULI() {
    W36 a1 = acGet();
    W36 a2 = immediate();
    acPut(imulWord(a1, a2));
    return iNormal;
  }

  InstructionResult doIMULM() {
    W36 a1 = acGet();
    W36 a2 = memGet();
    memPut(imulWord(a1, a2));
    return iNormal;
  }

  InstructionResult doIMULB() {
    W36 a1 = acGet();
    W36 a2 = memGet();
    bothPut(imulWord(a1, a2));
    return iNormal;
  }

  InstructionResult doMUL() {
    W36 a1 = acGet();
    W36 a2 = memGet();
    acPut2(mulWord(a1, a2));
    return iNormal;
  }

  InstructionResult doMULI() {
    W36 a1 = acGet();
    W36 a2 = immediate();
    acPut2(mulWord(a1, a2));
    return iNormal;
  }

  InstructionResult doMULM() {
    W36 a1 = acGet();
    W36 a2 = memGet();
    memPutHi(mulWord(a1, a2));
    return iNormal;
  }

  InstructionResult doMULB() {
    W36 a1 = acGet();
    W36 a2 = memGet();
    bothPut2(mulWord(a1, a2));
    return iNormal;
  }

  InstructionResult doIDIV() {
    W36 a1 = acGet();
    W36 a2 = memGet();
    acPut2(idivWord(a1, a2));
    return iNormal;
  }

  InstructionResult doIDIVI() {
    W36 a1 = acGet();
    W36 a2 = immediate();
    acPut2(idivWord(a1, a2));
    return iNormal;
  }

  InstructionResult doIDIVM() {
    W36 a1 = acGet();
    W36 a2 = memGet();
    memPutHi(idivWord(a1, a2));
    return iNormal;
  }

  InstructionResult doIDIVB() {
    W36 a1 = acGet();
    W36 a2 = memGet();
    bothPut2(idivWord(a1, a2));
    return iNormal;
  }

  InstructionResult doDIV() {
    W72 a1 = acGet2();
    W36 a2 = memGet();
    acPut2(divWord(a1, a2));
    return iNormal;
  }

  InstructionResult doDIVI() {
    W72 a1 = acGet2();
    W36 a2 = immediate();
    acPut2(divWord(a1, a2));
    return iNormal;
  }

  InstructionResult doDIVM() {
    W72 a1 = acGet2();
    W36 a2 = memGet();
    memPutHi(divWord(a1, a2));
    return iNormal;
  }

  InstructionResult doDIVB() {
    W72 a1 = acGet2();
    W36 a2 = memGet();
    bothPut2(divWord(a1, a2));
    return iNormal;
  }


  InstructionResult doADD() {
    W36 a1 = acGet();
    W36 a2 = memGet();
    acPut(addWord(a1, a2));
    return iNormal;
  }

  InstructionResult doADDI() {
    W36 a1 = acGet();
    W36 a2 = immediate();
    acPut(addWord(a1, a2));
    return iNormal;
  }

  InstructionResult doADDM() {
    W36 a1 = acGet();
    W36 a2 = memGet();
    memPut(addWord(a1, a2));
    return iNormal;
  }

  InstructionResult doADDB() {
    W36 a1 = acGet();
    W36 a2 = memGet();
    bothPut(addWord(a1, a2));
    return iNormal;
  }

  InstructionResult doSUB() {
    W36 a1 = acGet();
    W36 a2 = memGet();
    acPut(subWord(a1, a2));
    return iNormal;
  }

  InstructionResult doSUBI() {
    W36 a1 = acGet();
    W36 a2 = immediate();
    acPut(subWord(a1, a2));
    return iNormal;
  }

  InstructionResult doSUBM() {
    W36 a1 = acGet();
    W36 a2 = memGet();
    memPut(subWord(a1, a2));
    return iNormal;
  }

  InstructionResult doSUBB() {
    W36 a1 = acGet();
    W36 a2 = memGet();
    bothPut(subWord(a1, a2));
    return iNormal;
  }


  InstructionResult doXOR() {
    W36 a1 = memGet();
    W36 a2 = acGet();
    acPut(a1.u ^ a2.u);
    return iNormal;
  }

  InstructionResult doXORI() {
    W36 a1 = immediate();
    W36 a2 = acGet();
    acPut(a1.u ^ a2.u);
    return iNormal;
  }

  InstructionResult doXORM() {
    W36 a1 = memGet();
    W36 a2 = acGet();
    memPut(a1.u ^ a2.u);
    return iNormal;
  }

  InstructionResult doXORB() {
    W36 a1 = memGet();
    W36 a2 = acGet();
    bothPut(a1.u ^ a2.u);
    return iNormal;
  }

  InstructionResult doIOR() {
    W36 a1 = memGet();
    W36 a2 = acGet();
    acPut(a1.u | a2.u);
    return iNormal;
  }

  InstructionResult doIORI() {
    W36 a1 = immediate();
    W36 a2 = acGet();
    acPut(a1.u | a2.u);
    return iNormal;
  }

  InstructionResult doIORM() {
    W36 a1 = memGet();
    W36 a2 = acGet();
    memPut(a1.u | a2.u);
    return iNormal;
  }

  InstructionResult doIORB() {
    W36 a1 = memGet();
    W36 a2 = acGet();
    bothPut(a1.u | a2.u);
    return iNormal;
  }


  InstructionResult doAND() {
    W36 a1 = memGet();
    W36 a2 = acGet();
    acPut(a1 & a2);
    return iNormal;
  }

  InstructionResult doANDI() {
    W36 a1 = immediate();
    W36 a2 = acGet();
    acPut(a1 & a2);
    return iNormal;
  }

  InstructionResult doANDM() {
    W36 a1 = memGet();
    W36 a2 = acGet();
    memPut(a1 & a2);
    return iNormal;
  }

  InstructionResult doANDB() {
    W36 a1 = memGet();
    W36 a2 = acGet();
    bothPut(a1 & a2);
    return iNormal;
  }


  InstructionResult doANDCA() {
    W36 a1 = memGet();
    W36 a2 = acGet();
    acPut(a1 & ~a2);
    return iNormal;
  }

  InstructionResult doANDCAI() {
    W36 a1 = immediate();
    W36 a2 = acGet();
    acPut(a1 & ~a2);
    return iNormal;
  }

  InstructionResult doANDCAM() {
    W36 a1 = memGet();
    W36 a2 = acGet();
    memPut(a1 & ~a2);
    return iNormal;
  }

  InstructionResult doANDCAB() {
    W36 a1 = memGet();
    W36 a2 = acGet();
    bothPut(a1 & ~a2);
    return iNormal;
  }


  InstructionResult doANDCM() {
    W36 a1 = memGet();
    W36 a2 = acGet();
    acPut(~a1 & a2);
    return iNormal;
  }

  InstructionResult doANDCMI() {
    W36 a1 = immediate();
    W36 a2 = acGet();
    acPut(~a1 & a2);
    return iNormal;
  }

  InstructionResult doANDCMM() {
    W36 a1 = memGet();
    W36 a2 = acGet();
    memPut(~a1 & a2);
    return iNormal;
  }

  InstructionResult doANDCMB() {
    W36 a1 = memGet();
    W36 a2 = acGet();
    bothPut(~a1 & a2);
    return iNormal;
  }


  InstructionResult doANDCB() {
    W36 a1 = memGet();
    W36 a2 = acGet();
    acPut(~a1 & ~a2);
    return iNormal;
  }

  InstructionResult doANDCBI() {
    W36 a1 = immediate();
    W36 a2 = acGet();
    acPut(~a1 & ~a2);
    return iNormal;
  }

  InstructionResult doANDCBM() {
    W36 a1 = memGet();
    W36 a2 = acGet();
    memPut(~a1 & ~a2);
    return iNormal;
  }

  InstructionResult doANDCBB() {
    W36 a1 = memGet();
    W36 a2 = acGet();
    bothPut(~a1 & ~a2);
    return iNormal;
  }


  InstructionResult doORCA() {
    W36 a1 = memGet();
    W36 a2 = acGet();
    acPut(a1 | ~a2);
    return iNormal;
  }

  InstructionResult doORCAI() {
    W36 a1 = immediate();
    W36 a2 = acGet();
    acPut(a1 | ~a2);
    return iNormal;
  }

  InstructionResult doORCAM() {
    W36 a1 = memGet();
    W36 a2 = acGet();
    memPut(a1 | ~a2);
    return iNormal;
  }

  InstructionResult doORCAB() {
    W36 a1 = memGet();
    W36 a2 = acGet();
    bothPut(a1 | ~a2);
    return iNormal;
  }


  InstructionResult doORCM() {
    W36 a1 = memGet();
    W36 a2 = acGet();
    acPut(~a1 | a2);
    return iNormal;
  }

  InstructionResult doORCMI() {
    W36 a1 = immediate();
    W36 a2 = acGet();
    acPut(~a1 | a2);
    return iNormal;
  }

  InstructionResult doORCMM() {
    W36 a1 = memGet();
    W36 a2 = acGet();
    memPut(~a1 | a2);
    return iNormal;
  }

  InstructionResult doORCMB() {
    W36 a1 = memGet();
    W36 a2 = acGet();
    bothPut(~a1 | a2);
    return iNormal;
  }


  InstructionResult doORCB() {
    W36 a1 = memGet();
    W36 a2 = acGet();
    acPut(~a1 | ~a2);
    return iNormal;
  }

  InstructionResult doORCBI() {
    W36 a1 = immediate();
    W36 a2 = acGet();
    acPut(~a1 | ~a2);
    return iNormal;
  }

  InstructionResult doORCBM() {
    W36 a1 = memGet();
    W36 a2 = acGet();
    memPut(~a1 | ~a2);
    return iNormal;
  }

  InstructionResult doORCBB() {
    W36 a1 = memGet();
    W36 a2 = acGet();
    bothPut(~a1 | ~a2);
    return iNormal;
  }

  InstructionResult doEQV() {
    W36 a1 = memGet();
    W36 a2 = acGet();
    acPut(~(a1.u ^ a2.u));
    return iNormal;
  }

  InstructionResult doEQVI() {
    W36 a1 = immediate();
    W36 a2 = acGet();
    acPut(~(a1.u ^ a2.u));
    return iNormal;
  }

  InstructionResult doEQVM() {
    W36 a1 = memGet();
    W36 a2 = acGet();
    memPut(~(a1.u ^ a2.u));
    return iNormal;
  }

  InstructionResult doEQVB() {
    W36 a1 = memGet();
    W36 a2 = acGet();
    bothPut(~(a1.u ^ a2.u));
    return iNormal;
  }

  InstructionResult doASH() {
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


  InstructionResult doASHC() {
    W36 a0 = acGetN(iw.ac+0);
    W36 a1 = acGetN(iw.ac+1);
    int n = ea.rhs % 72;
    const bool isNeg = a0.s < 0;
    const uint64_t low35 = (1ull << 35) - 1;
    int128_t w70 = (int128_t) a0.s << 35 | (a1.s & low35);

    if (n > 0) {
      w70 <<= n;
    } else if (n < 0) {
      n = -n;

      // Mask of bits we're shifting away into oblivion.
      uint128_t lostMask = (((uint128_t) 1) << n) - 1;

      if (isNeg) {
	// If negative and we're losing some 0 bits set TR1 and OV.
	if ((w70 & lostMask) != lostMask) flags.tr1 = flags.ov = 1;
      } else {
	// If positive and we're losing some 1 bits set TR1 and OV.
	if ((w70 & lostMask) != 0) flags.tr1 = flags.ov = 1;
      }

      w70 >>= n;
    }

    W72 a{(uint64_t) (w70 >> 35), (uint64_t) (w70 & low35), isNeg};

    if (n != 0) {
      acPutN(a.hi, iw.ac+0);
      acPutN(a.lo, iw.ac+1);
    }

    return iNormal;
  }


  InstructionResult doROT() {
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


  InstructionResult doLSH() {
    int n = ea.rhs % 36;
    W36 a(acGet());

    if (n > 0)
      a.u <<= n;
    else if (n < 0)
      a.u >>= -n;

    acPut(a);
    return iNormal;
  }

  InstructionResult doJFFO() {
    W36 tmp = acGet();

    if (tmp.s != 0) {
      unsigned count = 0;

      while (tmp.s > 0) {
	++count;
	tmp.u <<= 1;
      }

      tmp.u = count;
    }

    acPutN(tmp, iw.ac+1);
    return tmp.u != 0 ? iJump : iNormal;
  }

  InstructionResult doROTC() {
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

  InstructionResult doLSHC() {
    W72 a(acGet(), acGetN(iw.ac+1));

    if (ea.rhs > 0)
      a.u <<= ea.rhs & 0377;
    else if (ea.rhs < 0)
      a.u >>= -(ea.rhs & 0377);

    acPutN(a.hi, iw.ac+0);
    acPutN(a.lo, iw.ac+1);
    return iNormal;
  }

  InstructionResult doEXCH() {
    W36 a = acGet();
    acPut(memGet());
    memPut(a);
    return iNormal;
  }

  InstructionResult doBLT() {
    W36 a(acGet());
    bool mem = logger.mem;

    logger.mem = false;
    const string prefix{logger.endl + "                                                 ; "};

    do {
      W36 srcA(ea.lhu, a.lhu);
      W36 dstA(ea.lhu, a.rhu);

      if (logger.mem) logger.s << prefix << "BLT src=" << srcA.vma << "  dst=" << dstA.vma;

      // Note this isn't bug-for-bug compatible with KL10. See
      // footnote [2] in 1982_ProcRefMan.pdf p.58. We do
      // wraparound.
      memPutN(memGetN(srcA), dstA);
      a = W36(a.lhu + 1, a.rhu + 1);

      // Put it back for traps or page faults.
      acPut(a);
    } while (a.rhu <= ea.rhu);

    if (logger.mem) logger.s << prefix << "BLT at end ac=" << a.fmt36();
    logger.mem = mem;
    return iNormal;
  }
};


void InstallIntBinGroup(KM10 &km10) {
  km10.defOp(0220, "IMUL", static_cast<KM10::OpcodeHandler>(&IntBinGroup::doIMUL));
  km10.defOp(0221, "IMULI", static_cast<KM10::OpcodeHandler>(&IntBinGroup::doIMULI));
  km10.defOp(0222, "IMULM", static_cast<KM10::OpcodeHandler>(&IntBinGroup::doIMULM));
  km10.defOp(0223, "IMULB", static_cast<KM10::OpcodeHandler>(&IntBinGroup::doIMULB));
  km10.defOp(0224, "MUL", static_cast<KM10::OpcodeHandler>(&IntBinGroup::doMUL));
  km10.defOp(0225, "MULI", static_cast<KM10::OpcodeHandler>(&IntBinGroup::doMULI));
  km10.defOp(0226, "MULM", static_cast<KM10::OpcodeHandler>(&IntBinGroup::doMULM));
  km10.defOp(0227, "MULB", static_cast<KM10::OpcodeHandler>(&IntBinGroup::doMULB));
  km10.defOp(0230, "IDIV", static_cast<KM10::OpcodeHandler>(&IntBinGroup::doIDIV));
  km10.defOp(0231, "IDIVI", static_cast<KM10::OpcodeHandler>(&IntBinGroup::doIDIVI));
  km10.defOp(0232, "IDIVM", static_cast<KM10::OpcodeHandler>(&IntBinGroup::doIDIVM));
  km10.defOp(0233, "IDIVB", static_cast<KM10::OpcodeHandler>(&IntBinGroup::doIDIVB));
  km10.defOp(0234, "DIV", static_cast<KM10::OpcodeHandler>(&IntBinGroup::doDIV));
  km10.defOp(0235, "DIVI", static_cast<KM10::OpcodeHandler>(&IntBinGroup::doDIVI));
  km10.defOp(0236, "DIVM", static_cast<KM10::OpcodeHandler>(&IntBinGroup::doDIVM));
  km10.defOp(0237, "DIVB", static_cast<KM10::OpcodeHandler>(&IntBinGroup::doDIVB));

  km10.defOp(0240, "ASH", static_cast<KM10::OpcodeHandler>(&IntBinGroup::doASH));
  km10.defOp(0241, "ROT", static_cast<KM10::OpcodeHandler>(&IntBinGroup::doROT));
  km10.defOp(0242, "LSH", static_cast<KM10::OpcodeHandler>(&IntBinGroup::doLSH));
  km10.defOp(0243, "JFFO", static_cast<KM10::OpcodeHandler>(&IntBinGroup::doJFFO));
  km10.defOp(0244, "ASHC", static_cast<KM10::OpcodeHandler>(&IntBinGroup::doASHC));
  km10.defOp(0245, "ROTC", static_cast<KM10::OpcodeHandler>(&IntBinGroup::doROTC));
  km10.defOp(0246, "LSHC", static_cast<KM10::OpcodeHandler>(&IntBinGroup::doLSHC));

  km10.defOp(0250, "EXCH", static_cast<KM10::OpcodeHandler>(&IntBinGroup::doEXCH));
  km10.defOp(0251, "LSHC", static_cast<KM10::OpcodeHandler>(&IntBinGroup::doBLT));

  km10.defOp(0270, "ADD", static_cast<KM10::OpcodeHandler>(&IntBinGroup::doADD));
  km10.defOp(0271, "ADDI", static_cast<KM10::OpcodeHandler>(&IntBinGroup::doADDI));
  km10.defOp(0272, "ADDM", static_cast<KM10::OpcodeHandler>(&IntBinGroup::doADDM));
  km10.defOp(0273, "ADDB", static_cast<KM10::OpcodeHandler>(&IntBinGroup::doADDB));
  km10.defOp(0274, "SUB", static_cast<KM10::OpcodeHandler>(&IntBinGroup::doSUB));
  km10.defOp(0275, "SUBI", static_cast<KM10::OpcodeHandler>(&IntBinGroup::doSUBI));
  km10.defOp(0276, "SUBM", static_cast<KM10::OpcodeHandler>(&IntBinGroup::doSUBM));
  km10.defOp(0277, "SUBB", static_cast<KM10::OpcodeHandler>(&IntBinGroup::doSUBB));

  km10.defOp(0404, "AND",    static_cast<KM10::OpcodeHandler>(&IntBinGroup::doAND));
  km10.defOp(0405, "ANDI",   static_cast<KM10::OpcodeHandler>(&IntBinGroup::doANDI));
  km10.defOp(0406, "ANDM",   static_cast<KM10::OpcodeHandler>(&IntBinGroup::doANDM));
  km10.defOp(0407, "ANDB",   static_cast<KM10::OpcodeHandler>(&IntBinGroup::doANDB));
  km10.defOp(0410, "ANDCA",  static_cast<KM10::OpcodeHandler>(&IntBinGroup::doANDCA));
  km10.defOp(0411, "ANDCAI", static_cast<KM10::OpcodeHandler>(&IntBinGroup::doANDCAI));
  km10.defOp(0412, "ANDCAM", static_cast<KM10::OpcodeHandler>(&IntBinGroup::doANDCAM));
  km10.defOp(0413, "ANDCAB", static_cast<KM10::OpcodeHandler>(&IntBinGroup::doANDCAB));

  km10.defOp(0420, "ANDCM",  static_cast<KM10::OpcodeHandler>(&IntBinGroup::doANDCM));
  km10.defOp(0421, "ANDCMI", static_cast<KM10::OpcodeHandler>(&IntBinGroup::doANDCMI));
  km10.defOp(0422, "ANDCMM", static_cast<KM10::OpcodeHandler>(&IntBinGroup::doANDCMM));
  km10.defOp(0423, "ANDCMB", static_cast<KM10::OpcodeHandler>(&IntBinGroup::doANDCMB));

  km10.defOp(0430, "XOR", static_cast<KM10::OpcodeHandler>(&IntBinGroup::doXOR));
  km10.defOp(0431, "XORI", static_cast<KM10::OpcodeHandler>(&IntBinGroup::doXORI));
  km10.defOp(0432, "XORM", static_cast<KM10::OpcodeHandler>(&IntBinGroup::doXORM));
  km10.defOp(0433, "XORB", static_cast<KM10::OpcodeHandler>(&IntBinGroup::doXORB));
  km10.defOp(0434, "IOR", static_cast<KM10::OpcodeHandler>(&IntBinGroup::doIOR));
  km10.defOp(0435, "IORI", static_cast<KM10::OpcodeHandler>(&IntBinGroup::doIORI));
  km10.defOp(0436, "IORM", static_cast<KM10::OpcodeHandler>(&IntBinGroup::doIORM));
  km10.defOp(0437, "IORB", static_cast<KM10::OpcodeHandler>(&IntBinGroup::doIORB));

  km10.defOp(0440, "ANDCB", static_cast<KM10::OpcodeHandler>(&IntBinGroup::doANDCB));
  km10.defOp(0441, "ANDCBI", static_cast<KM10::OpcodeHandler>(&IntBinGroup::doANDCBI));
  km10.defOp(0442, "ANDCBM", static_cast<KM10::OpcodeHandler>(&IntBinGroup::doANDCBM));
  km10.defOp(0443, "ANDCBB", static_cast<KM10::OpcodeHandler>(&IntBinGroup::doANDCBB));
  km10.defOp(0444, "EQV", static_cast<KM10::OpcodeHandler>(&IntBinGroup::doEQV));
  km10.defOp(0445, "EQVI", static_cast<KM10::OpcodeHandler>(&IntBinGroup::doEQVI));
  km10.defOp(0446, "EQVM", static_cast<KM10::OpcodeHandler>(&IntBinGroup::doEQVM));
  km10.defOp(0447, "EQVB", static_cast<KM10::OpcodeHandler>(&IntBinGroup::doEQVB));

  km10.defOp(0454, "ORCA", static_cast<KM10::OpcodeHandler>(&IntBinGroup::doORCA));
  km10.defOp(0455, "ORCAI", static_cast<KM10::OpcodeHandler>(&IntBinGroup::doORCAI));
  km10.defOp(0456, "ORCAM", static_cast<KM10::OpcodeHandler>(&IntBinGroup::doORCAM));
  km10.defOp(0457, "ORCAB", static_cast<KM10::OpcodeHandler>(&IntBinGroup::doORCAB));

  km10.defOp(0464, "ORCM",  static_cast<KM10::OpcodeHandler>(&IntBinGroup::doORCM));
  km10.defOp(0465, "ORCMI", static_cast<KM10::OpcodeHandler>(&IntBinGroup::doORCMI));
  km10.defOp(0466, "ORCMM", static_cast<KM10::OpcodeHandler>(&IntBinGroup::doORCMM));
  km10.defOp(0467, "ORCMB", static_cast<KM10::OpcodeHandler>(&IntBinGroup::doORCMB));

  km10.defOp(0470, "ORCB",  static_cast<KM10::OpcodeHandler>(&IntBinGroup::doORCB));
  km10.defOp(0471, "ORCBI", static_cast<KM10::OpcodeHandler>(&IntBinGroup::doORCBI));
  km10.defOp(0472, "ORCBM", static_cast<KM10::OpcodeHandler>(&IntBinGroup::doORCBM));
  km10.defOp(0473, "ORCBB", static_cast<KM10::OpcodeHandler>(&IntBinGroup::doORCBB));
}
