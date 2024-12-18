#include "km10.hpp"

struct IntBinGroup: KM10 {

  W36 andWord(W36 s1, W36 s2) {return s1.u & s2.u;}
  W36 andCWord(W36 s1, W36 s2) {return s1.u & ~s2.u;}
  W36 andCBWord(W36 s1, W36 s2) {return ~s1.u & ~s2.u;}
  W36 iorWord(W36 s1, W36 s2) {return s1.u | s2.u;}
  W36 iorCWord(W36 s1, W36 s2) {return s1.u | ~s2.u;}
  W36 iorCBWord(W36 s1, W36 s2) {return ~s1.u | ~s2.u;}
  W36 xorWord(W36 s1, W36 s2) {return s1.u ^ s2.u;}
  W36 xorCWord(W36 s1, W36 s2) {return s1.u ^ ~s2.u;}
  W36 xorCBWord(W36 s1, W36 s2) {return ~s1.u ^ ~s2.u;}
  W36 eqvWord(W36 s1, W36 s2) {return ~(s1.u ^ s2.u);}
  W36 eqvCWord(W36 s1, W36 s2) {return ~(s1.u ^ ~s2.u);}
  W36 eqvCBWord(W36 s1, W36 s2) {return ~(~s1.u ^ ~s2.u);}

  
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
    acPut(xorWord(a1, a2));
    return iNormal;
  }

  InstructionResult doXORI() {
    W36 a1 = immediate();
    W36 a2 = acGet();
    acPut(xorWord(a1, a2));
    return iNormal;
  }

  InstructionResult doXORM() {
    W36 a1 = memGet();
    W36 a2 = acGet();
    memPut(xorWord(a1, a2));
    return iNormal;
  }

  InstructionResult doXORB() {
    W36 a1 = memGet();
    W36 a2 = acGet();
    bothPut(xorWord(a1, a2));
    return iNormal;
  }

  InstructionResult doIOR() {
    W36 a1 = memGet();
    W36 a2 = acGet();
    acPut(iorWord(a1, a2));
    return iNormal;
  }

  InstructionResult doIORI() {
    W36 a1 = immediate();
    W36 a2 = acGet();
    acPut(iorWord(a1, a2));
    return iNormal;
  }

  InstructionResult doIORM() {
    W36 a1 = memGet();
    W36 a2 = acGet();
    memPut(iorWord(a1, a2));
    return iNormal;
  }

  InstructionResult doIORB() {
    W36 a1 = memGet();
    W36 a2 = acGet();
    bothPut(iorWord(a1, a2));
    return iNormal;
  }

  InstructionResult doANDCBM() {
    W36 a1 = memGet();
    W36 a2 = acGet();
    acPut(andCBWord(a1, a2));
    return iNormal;
  }

  InstructionResult doANDCBMI() {
    W36 a1 = immediate();
    W36 a2 = acGet();
    acPut(andCBWord(a1, a2));
    return iNormal;
  }

  InstructionResult doANDCBMM() {
    W36 a1 = memGet();
    W36 a2 = acGet();
    memPut(andCBWord(a1, a2));
    return iNormal;
  }

  InstructionResult doANDCBMB() {
    W36 a1 = memGet();
    W36 a2 = acGet();
    bothPut(andCBWord(a1, a2));
    return iNormal;
  }

  InstructionResult doEQV() {
    W36 a1 = memGet();
    W36 a2 = acGet();
    acPut(eqvWord(a1, a2));
    return iNormal;
  }

  InstructionResult doEQVI() {
    W36 a1 = immediate();
    W36 a2 = acGet();
    acPut(eqvWord(a1, a2));
    return iNormal;
  }

  InstructionResult doEQVM() {
    W36 a1 = memGet();
    W36 a2 = acGet();
    memPut(eqvWord(a1, a2));
    return iNormal;
  }

  InstructionResult doEQVB() {
    W36 a1 = memGet();
    W36 a2 = acGet();
    bothPut(eqvWord(a1, a2));
    return iNormal;
  }


  void install() {

    defOp(220, "IMUL", static_cast<OpcodeHandler>(&IntBinGroup::doIMUL));
    defOp(221, "IMULI", static_cast<OpcodeHandler>(&IntBinGroup::doIMULI));
    defOp(222, "IMULM", static_cast<OpcodeHandler>(&IntBinGroup::doIMULM));
    defOp(223, "IMULB", static_cast<OpcodeHandler>(&IntBinGroup::doIMULB));
    defOp(224, "MUL", static_cast<OpcodeHandler>(&IntBinGroup::doMUL));
    defOp(225, "MULI", static_cast<OpcodeHandler>(&IntBinGroup::doMULI));
    defOp(226, "MULM", static_cast<OpcodeHandler>(&IntBinGroup::doMULM));
    defOp(227, "MULB", static_cast<OpcodeHandler>(&IntBinGroup::doMULB));
    defOp(230, "IDIV", static_cast<OpcodeHandler>(&IntBinGroup::doIDIV));
    defOp(231, "IDIVI", static_cast<OpcodeHandler>(&IntBinGroup::doIDIVI));
    defOp(232, "IDIVM", static_cast<OpcodeHandler>(&IntBinGroup::doIDIVM));
    defOp(233, "IDIVB", static_cast<OpcodeHandler>(&IntBinGroup::doIDIVB));
    defOp(234, "DIV", static_cast<OpcodeHandler>(&IntBinGroup::doDIV));
    defOp(235, "DIVI", static_cast<OpcodeHandler>(&IntBinGroup::doDIVI));
    defOp(236, "DIVM", static_cast<OpcodeHandler>(&IntBinGroup::doDIVM));
    defOp(237, "DIVB", static_cast<OpcodeHandler>(&IntBinGroup::doDIVB));

    defOp(270, "ADD", static_cast<OpcodeHandler>(&IntBinGroup::doADD));
    defOp(271, "ADDI", static_cast<OpcodeHandler>(&IntBinGroup::doADDI));
    defOp(272, "ADDM", static_cast<OpcodeHandler>(&IntBinGroup::doADDM));
    defOp(273, "ADDB", static_cast<OpcodeHandler>(&IntBinGroup::doADDB));
    defOp(274, "SUB", static_cast<OpcodeHandler>(&IntBinGroup::doSUB));
    defOp(275, "SUBI", static_cast<OpcodeHandler>(&IntBinGroup::doSUBI));
    defOp(276, "SUBM", static_cast<OpcodeHandler>(&IntBinGroup::doSUBM));
    defOp(277, "SUBB", static_cast<OpcodeHandler>(&IntBinGroup::doSUBB));

    defOp(430, "XOR", static_cast<OpcodeHandler>(&IntBinGroup::doXOR));
    defOp(431, "XORI", static_cast<OpcodeHandler>(&IntBinGroup::doXORI));
    defOp(432, "XORM", static_cast<OpcodeHandler>(&IntBinGroup::doXORM));
    defOp(433, "XORB", static_cast<OpcodeHandler>(&IntBinGroup::doXORB));
    defOp(434, "IOR", static_cast<OpcodeHandler>(&IntBinGroup::doIOR));
    defOp(435, "IORI", static_cast<OpcodeHandler>(&IntBinGroup::doIORI));
    defOp(436, "IORM", static_cast<OpcodeHandler>(&IntBinGroup::doIORM));
    defOp(437, "IORB", static_cast<OpcodeHandler>(&IntBinGroup::doIORB));
    defOp(440, "ANDCBM", static_cast<OpcodeHandler>(&IntBinGroup::doANDCBM));
    defOp(441, "ANDCBMI", static_cast<OpcodeHandler>(&IntBinGroup::doANDCBMI));
    defOp(442, "ANDCBMM", static_cast<OpcodeHandler>(&IntBinGroup::doANDCBMM));
    defOp(443, "ANDCBMB", static_cast<OpcodeHandler>(&IntBinGroup::doANDCBMB));
    defOp(444, "EQV", static_cast<OpcodeHandler>(&IntBinGroup::doEQV));
    defOp(445, "EQVI", static_cast<OpcodeHandler>(&IntBinGroup::doEQVI));
    defOp(446, "EQVM", static_cast<OpcodeHandler>(&IntBinGroup::doEQVM));
    defOp(447, "EQVB", static_cast<OpcodeHandler>(&IntBinGroup::doEQVB));

  }
};
