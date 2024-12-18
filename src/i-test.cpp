#include "km10.hpp"

struct TestGroup: KM10 {
  W36 zeroMaskR(W36 a, W36 b) const {return a.u & ~(uint64_t) b.rhu;};
  W36 zeroMask(W36 a, W36 b) const {return a.u & ~b.u;};

  W36 onesMaskR(W36 a, W36 b) const {return a.u | b.rhu;};
  W36 onesMask(W36 a, W36 b) const {return a.u | b.u;};

  W36 compMaskR(W36 a, W36 b) const {return a.u ^ b.rhu;};
  W36 compMask(W36 a, W36 b) const {return a.u ^ b.u;};

  W36 zeroWord(W36 a) const {return 0;};
  W36 onesWord(W36 a) const {return W36::all1s;};
  W36 compWord(W36 a) const {return ~a.u;};


  InstructionResult doTRN() { return iNormal; }

  InstructionResult doTLN() { return iNormal; }

  InstructionResult doTRNE() {
    W36 a1 = acGetRH();
    W36 a2 = getE();
    const bool doSkip = (a1.u & a2.u) == 0;
    /* No store */;
    return doSkip ? iSkip : iNormal;
  }

  InstructionResult doTLNE() {
    W36 a1 = acGetLH();
    W36 a2 = getE();
    const bool doSkip = (a1.u & a2.u) == 0;
    /* No store */;
    return doSkip ? iSkip : iNormal;
  }

  InstructionResult doTRNA() { return iSkip; }

  InstructionResult doTLNA() { return iSkip; }

  InstructionResult doTRNN() {
    W36 a1 = acGetRH();
    W36 a2 = getE();
    const bool doSkip = (a1.u & a2.u) != 0;
    /* No store */;
    return doSkip ? iSkip : iNormal;
  }

  InstructionResult doTLNN() {
    W36 a1 = acGetLH();
    W36 a2 = getE();
    const bool doSkip = (a1.u & a2.u) != 0;
    /* No store */;
    return doSkip ? iSkip : iNormal;
  }

  InstructionResult doTRZ() {
    W36 a1 = acGetRH();
    W36 a2 = getE();
    const bool doSkip = false;
    acPutRH(zeroMaskR(a1, a2));
    return doSkip ? iSkip : iNormal;
  }

  InstructionResult doTLZ() {
    W36 a1 = acGetLH();
    W36 a2 = getE();
    const bool doSkip = false;
    acPutLH(zeroMaskR(a1, a2));
    return doSkip ? iSkip : iNormal;
  }

  InstructionResult doTRZE() {
    W36 a1 = acGetRH();
    W36 a2 = getE();
    const bool doSkip = (a1.u & a2.u) == 0;
    acPutRH(zeroMaskR(a1, a2));
    return doSkip ? iSkip : iNormal;
  }

  InstructionResult doTLZE() {
    W36 a1 = acGetLH();
    W36 a2 = getE();
    const bool doSkip = (a1.u & a2.u) == 0;
    acPutLH(zeroMaskR(a1, a2));
    return doSkip ? iSkip : iNormal;
  }

  InstructionResult doTRZA() {
    W36 a1 = acGetRH();
    W36 a2 = getE();
    const bool doSkip = true;
    acPutRH(zeroMaskR(a1, a2));
    return doSkip ? iSkip : iNormal;
  }

  InstructionResult doTLZA() {
    W36 a1 = acGetLH();
    W36 a2 = getE();
    const bool doSkip = true;
    acPutLH(zeroMaskR(a1, a2));
    return doSkip ? iSkip : iNormal;
  }

  InstructionResult doTRZN() {
    W36 a1 = acGetRH();
    W36 a2 = getE();
    const bool doSkip = (a1.u & a2.u) != 0;
    acPutRH(zeroMaskR(a1, a2));
    return doSkip ? iSkip : iNormal;
  }

  InstructionResult doTLZN() {
    W36 a1 = acGetLH();
    W36 a2 = getE();
    const bool doSkip = (a1.u & a2.u) != 0;
    acPutLH(zeroMaskR(a1, a2));
    return doSkip ? iSkip : iNormal;
  }

  InstructionResult doTRC() {
    W36 a1 = acGetRH();
    W36 a2 = getE();
    const bool doSkip = false;
    acPutRH(compMaskR(a1, a2));
    return doSkip ? iSkip : iNormal;
  }

  InstructionResult doTLC() {
    W36 a1 = acGetLH();
    W36 a2 = getE();
    const bool doSkip = false;
    acPutLH(compMaskR(a1, a2));
    return doSkip ? iSkip : iNormal;
  }

  InstructionResult doTRCE() {
    W36 a1 = acGetRH();
    W36 a2 = getE();
    const bool doSkip = (a1.u & a2.u) == 0;
    acPutRH(compMaskR(a1, a2));
    return doSkip ? iSkip : iNormal;
  }

  InstructionResult doTLCE() {
    W36 a1 = acGetLH();
    W36 a2 = getE();
    const bool doSkip = (a1.u & a2.u) == 0;
    acPutLH(compMaskR(a1, a2));
    return doSkip ? iSkip : iNormal;
  }

  InstructionResult doTRCA() {
    W36 a1 = acGetRH();
    W36 a2 = getE();
    const bool doSkip = true;
    acPutRH(compMaskR(a1, a2));
    return doSkip ? iSkip : iNormal;
  }

  InstructionResult doTLCA() {
    W36 a1 = acGetLH();
    W36 a2 = getE();
    const bool doSkip = true;
    acPutLH(compMaskR(a1, a2));
    return doSkip ? iSkip : iNormal;
  }

  InstructionResult doTRCN() {
    W36 a1 = acGetRH();
    W36 a2 = getE();
    const bool doSkip = (a1.u & a2.u) != 0;
    acPutRH(compMaskR(a1, a2));
    return doSkip ? iSkip : iNormal;
  }

  InstructionResult doTLCN() {
    W36 a1 = acGetLH();
    W36 a2 = getE();
    const bool doSkip = (a1.u & a2.u) != 0;
    acPutLH(compMaskR(a1, a2));
    return doSkip ? iSkip : iNormal;
  }

  InstructionResult doTRO() {
    W36 a1 = acGetRH();
    W36 a2 = getE();
    const bool doSkip = false;
    acPutRH(onesMaskR(a1, a2));
    return doSkip ? iSkip : iNormal;
  }

  InstructionResult doTLO() {
    W36 a1 = acGetLH();
    W36 a2 = getE();
    const bool doSkip = false;
    acPutLH(onesMaskR(a1, a2));
    return doSkip ? iSkip : iNormal;
  }

  InstructionResult doTROE() {
    W36 a1 = acGetRH();
    W36 a2 = getE();
    const bool doSkip = (a1.u & a2.u) == 0;
    acPutRH(onesMaskR(a1, a2));
    return doSkip ? iSkip : iNormal;
  }

  InstructionResult doTLOE() {
    W36 a1 = acGetLH();
    W36 a2 = getE();
    const bool doSkip = (a1.u & a2.u) == 0;
    acPutLH(onesMaskR(a1, a2));
    return doSkip ? iSkip : iNormal;
  }

  InstructionResult doTROA() {
    W36 a1 = acGetRH();
    W36 a2 = getE();
    const bool doSkip = true;
    acPutRH(onesMaskR(a1, a2));
    return doSkip ? iSkip : iNormal;
  }

  InstructionResult doTLOA() {
    W36 a1 = acGetLH();
    W36 a2 = getE();
    const bool doSkip = true;
    acPutLH(onesMaskR(a1, a2));
    return doSkip ? iSkip : iNormal;
  }

  InstructionResult doTRON() {
    W36 a1 = acGetRH();
    W36 a2 = getE();
    const bool doSkip = (a1.u & a2.u) != 0;
    acPutRH(onesMaskR(a1, a2));
    return doSkip ? iSkip : iNormal;
  }

  InstructionResult doTLON() {
    W36 a1 = acGetLH();
    W36 a2 = getE();
    const bool doSkip = (a1.u & a2.u) != 0;
    acPutLH(onesMaskR(a1, a2));
    return doSkip ? iSkip : iNormal;
  }

  InstructionResult doTDN() { return iNormal; }

  InstructionResult doTSN() { return iNormal; }

  InstructionResult doTDNE() {
    W36 a1 = acGet();
    W36 a2 = memGet();
    const bool doSkip = (a1.u & a2.u) == 0;
    /* No store */;
    return doSkip ? iSkip : iNormal;
  }

  InstructionResult doTSNE() {
    W36 a1 = acGet();
    W36 a2 = memGetSwapped();
    const bool doSkip = (a1.u & a2.u) == 0;
    /* No store */;
    return doSkip ? iSkip : iNormal;
  }

  InstructionResult doTDNA() { return iSkip; }

  InstructionResult doTSNA() { return iSkip; }

  InstructionResult doTDNN() {
    W36 a1 = acGet();
    W36 a2 = memGet();
    const bool doSkip = (a1.u & a2.u) != 0;
    /* No store */;
    return doSkip ? iSkip : iNormal;
  }

  InstructionResult doTSNN() {
    W36 a1 = acGet();
    W36 a2 = memGetSwapped();
    const bool doSkip = (a1.u & a2.u) != 0;
    /* No store */;
    return doSkip ? iSkip : iNormal;
  }

  InstructionResult doTDZ() {
    W36 a1 = acGet();
    W36 a2 = memGet();
    const bool doSkip = false;
    acPut(zeroMask(a1, a2));
    return doSkip ? iSkip : iNormal;
  }

  InstructionResult doTSZ() {
    W36 a1 = acGet();
    W36 a2 = memGetSwapped();
    const bool doSkip = false;
    acPut(zeroMask(a1, a2));
    return doSkip ? iSkip : iNormal;
  }

  InstructionResult doTDZE() {
    W36 a1 = acGet();
    W36 a2 = memGet();
    const bool doSkip = (a1.u & a2.u) == 0;
    acPut(zeroMask(a1, a2));
    return doSkip ? iSkip : iNormal;
  }

  InstructionResult doTSZE() {
    W36 a1 = acGet();
    W36 a2 = memGetSwapped();
    const bool doSkip = (a1.u & a2.u) == 0;
    acPut(zeroMask(a1, a2));
    return doSkip ? iSkip : iNormal;
  }

  InstructionResult doTDZA() {
    W36 a1 = acGet();
    W36 a2 = memGet();
    const bool doSkip = true;
    acPut(zeroMask(a1, a2));
    return doSkip ? iSkip : iNormal;
  }

  InstructionResult doTSZA() {
    W36 a1 = acGet();
    W36 a2 = memGetSwapped();
    const bool doSkip = true;
    acPut(zeroMask(a1, a2));
    return doSkip ? iSkip : iNormal;
  }

  InstructionResult doTDZN() {
    W36 a1 = acGet();
    W36 a2 = memGet();
    const bool doSkip = (a1.u & a2.u) != 0;
    acPut(zeroMask(a1, a2));
    return doSkip ? iSkip : iNormal;
  }

  InstructionResult doTSZN() {
    W36 a1 = acGet();
    W36 a2 = memGetSwapped();
    const bool doSkip = (a1.u & a2.u) != 0;
    acPut(zeroMask(a1, a2));
    return doSkip ? iSkip : iNormal;
  }

  InstructionResult doTDC() {
    W36 a1 = acGet();
    W36 a2 = memGet();
    const bool doSkip = false;
    acPut(compMask(a1, a2));
    return doSkip ? iSkip : iNormal;
  }

  InstructionResult doTSC() {
    W36 a1 = acGet();
    W36 a2 = memGetSwapped();
    const bool doSkip = false;
    acPut(compMask(a1, a2));
    return doSkip ? iSkip : iNormal;
  }

  InstructionResult doTDCE() {
    W36 a1 = acGet();
    W36 a2 = memGet();
    const bool doSkip = (a1.u & a2.u) == 0;
    acPut(compMask(a1, a2));
    return doSkip ? iSkip : iNormal;
  }

  InstructionResult doTSCE() {
    W36 a1 = acGet();
    W36 a2 = memGetSwapped();
    const bool doSkip = (a1.u & a2.u) == 0;
    acPut(compMask(a1, a2));
    return doSkip ? iSkip : iNormal;
  }

  InstructionResult doTDCA() {
    W36 a1 = acGet();
    W36 a2 = memGet();
    const bool doSkip = true;
    acPut(compMask(a1, a2));
    return doSkip ? iSkip : iNormal;
  }

  InstructionResult doTSCA() {
    W36 a1 = acGet();
    W36 a2 = memGetSwapped();
    const bool doSkip = true;
    acPut(compMask(a1, a2));
    return doSkip ? iSkip : iNormal;
  }

  InstructionResult doTDCN() {
    W36 a1 = acGet();
    W36 a2 = memGet();
    const bool doSkip = (a1.u & a2.u) != 0;
    acPut(compMask(a1, a2));
    return doSkip ? iSkip : iNormal;
  }

  InstructionResult doTSZCN() {
    W36 a1 = acGet();
    W36 a2 = memGetSwapped();
    const bool doSkip = (a1.u & a2.u) != 0;
    acPut(compMask(a1, a2));
    return doSkip ? iSkip : iNormal;
  }

  InstructionResult doTDO() {
    W36 a1 = acGet();
    W36 a2 = memGet();
    const bool doSkip = false;
    acPut(onesMask(a1, a2));
    return doSkip ? iSkip : iNormal;
  }

  InstructionResult doTSO() {
    W36 a1 = acGet();
    W36 a2 = memGetSwapped();
    const bool doSkip = false;
    acPut(onesMask(a1, a2));
    return doSkip ? iSkip : iNormal;
  }

  InstructionResult doTDOE() {
    W36 a1 = acGet();
    W36 a2 = memGet();
    const bool doSkip = (a1.u & a2.u) == 0;
    acPut(onesMask(a1, a2));
    return doSkip ? iSkip : iNormal;
  }

  InstructionResult doTSOE() {
    W36 a1 = acGet();
    W36 a2 = memGetSwapped();
    const bool doSkip = (a1.u & a2.u) == 0;
    acPut(onesMask(a1, a2));
    return doSkip ? iSkip : iNormal;
  }

  InstructionResult doTDOA() {
    W36 a1 = acGet();
    W36 a2 = memGet();
    const bool doSkip = true;
    acPut(onesMask(a1, a2));
    return doSkip ? iSkip : iNormal;
  }

  InstructionResult doTSOA() {
    W36 a1 = acGet();
    W36 a2 = memGetSwapped();
    const bool doSkip = true;
    acPut(onesMask(a1, a2));
    return doSkip ? iSkip : iNormal;
  }

  InstructionResult doTDON() {
    W36 a1 = acGet();
    W36 a2 = memGet();
    const bool doSkip = (a1.u & a2.u) != 0;
    acPut(onesMask(a1, a2));
    return doSkip ? iSkip : iNormal;
  }

  InstructionResult doTSON() {
    W36 a1 = acGet();
    W36 a2 = memGetSwapped();
    const bool doSkip = (a1.u & a2.u) != 0;
    acPut(onesMask(a1, a2));
    return doSkip ? iSkip : iNormal;
  }

  void install() {
    defOp(0600, "TRN", static_cast<OpcodeHandler>(&TestGroup::doTRN));
    defOp(0601, "TLN", static_cast<OpcodeHandler>(&TestGroup::doTLN));
    defOp(0602, "TRNE", static_cast<OpcodeHandler>(&TestGroup::doTRNE));
    defOp(0603, "TLNE", static_cast<OpcodeHandler>(&TestGroup::doTLNE));
    defOp(0604, "TRNA", static_cast<OpcodeHandler>(&TestGroup::doTRNA));
    defOp(0605, "TLNA", static_cast<OpcodeHandler>(&TestGroup::doTLNA));
    defOp(0606, "TRNN", static_cast<OpcodeHandler>(&TestGroup::doTRNN));
    defOp(0607, "TLNN", static_cast<OpcodeHandler>(&TestGroup::doTLNN));
    defOp(0610, "TRZ", static_cast<OpcodeHandler>(&TestGroup::doTRZ));
    defOp(0611, "TLZ", static_cast<OpcodeHandler>(&TestGroup::doTLZ));
    defOp(0612, "TRZE", static_cast<OpcodeHandler>(&TestGroup::doTRZE));
    defOp(0613, "TLZE", static_cast<OpcodeHandler>(&TestGroup::doTLZE));
    defOp(0614, "TRZA", static_cast<OpcodeHandler>(&TestGroup::doTRZA));
    defOp(0615, "TLZA", static_cast<OpcodeHandler>(&TestGroup::doTLZA));
    defOp(0616, "TRZN", static_cast<OpcodeHandler>(&TestGroup::doTRZN));
    defOp(0617, "TLZN", static_cast<OpcodeHandler>(&TestGroup::doTLZN));
    defOp(0620, "TRC", static_cast<OpcodeHandler>(&TestGroup::doTRC));
    defOp(0621, "TLC", static_cast<OpcodeHandler>(&TestGroup::doTLC));
    defOp(0622, "TRCE", static_cast<OpcodeHandler>(&TestGroup::doTRCE));
    defOp(0623, "TLCE", static_cast<OpcodeHandler>(&TestGroup::doTLCE));
    defOp(0624, "TRCA", static_cast<OpcodeHandler>(&TestGroup::doTRCA));
    defOp(0625, "TLCA", static_cast<OpcodeHandler>(&TestGroup::doTLCA));
    defOp(0626, "TRCN", static_cast<OpcodeHandler>(&TestGroup::doTRCN));
    defOp(0627, "TLCN", static_cast<OpcodeHandler>(&TestGroup::doTLCN));
    defOp(0630, "TRO", static_cast<OpcodeHandler>(&TestGroup::doTRO));
    defOp(0631, "TLO", static_cast<OpcodeHandler>(&TestGroup::doTLO));
    defOp(0632, "TROE", static_cast<OpcodeHandler>(&TestGroup::doTROE));
    defOp(0633, "TLOE", static_cast<OpcodeHandler>(&TestGroup::doTLOE));
    defOp(0634, "TROA", static_cast<OpcodeHandler>(&TestGroup::doTROA));
    defOp(0635, "TLOA", static_cast<OpcodeHandler>(&TestGroup::doTLOA));
    defOp(0636, "TRON", static_cast<OpcodeHandler>(&TestGroup::doTRON));
    defOp(0637, "TLON", static_cast<OpcodeHandler>(&TestGroup::doTLON));
    defOp(0640, "TDN", static_cast<OpcodeHandler>(&TestGroup::doTDN));
    defOp(0641, "TSN", static_cast<OpcodeHandler>(&TestGroup::doTSN));
    defOp(0642, "TDNE", static_cast<OpcodeHandler>(&TestGroup::doTDNE));
    defOp(0643, "TSNE", static_cast<OpcodeHandler>(&TestGroup::doTSNE));
    defOp(0644, "TDNA", static_cast<OpcodeHandler>(&TestGroup::doTDNA));
    defOp(0645, "TSNA", static_cast<OpcodeHandler>(&TestGroup::doTSNA));
    defOp(0646, "TDNN", static_cast<OpcodeHandler>(&TestGroup::doTDNN));
    defOp(0647, "TSNN", static_cast<OpcodeHandler>(&TestGroup::doTSNN));
    defOp(0650, "TDZ", static_cast<OpcodeHandler>(&TestGroup::doTDZ));
    defOp(0651, "TSZ", static_cast<OpcodeHandler>(&TestGroup::doTSZ));
    defOp(0652, "TDZE", static_cast<OpcodeHandler>(&TestGroup::doTDZE));
    defOp(0653, "TSZE", static_cast<OpcodeHandler>(&TestGroup::doTSZE));
    defOp(0654, "TDZA", static_cast<OpcodeHandler>(&TestGroup::doTDZA));
    defOp(0655, "TSZA", static_cast<OpcodeHandler>(&TestGroup::doTSZA));
    defOp(0656, "TDZN", static_cast<OpcodeHandler>(&TestGroup::doTDZN));
    defOp(0657, "TSZN", static_cast<OpcodeHandler>(&TestGroup::doTSZN));
    defOp(0660, "TDC", static_cast<OpcodeHandler>(&TestGroup::doTDC));
    defOp(0661, "TSC", static_cast<OpcodeHandler>(&TestGroup::doTSC));
    defOp(0662, "TDCE", static_cast<OpcodeHandler>(&TestGroup::doTDCE));
    defOp(0663, "TSCE", static_cast<OpcodeHandler>(&TestGroup::doTSCE));
    defOp(0664, "TDCA", static_cast<OpcodeHandler>(&TestGroup::doTDCA));
    defOp(0665, "TSCA", static_cast<OpcodeHandler>(&TestGroup::doTSCA));
    defOp(0666, "TDCN", static_cast<OpcodeHandler>(&TestGroup::doTDCN));
    defOp(0667, "TSZCN", static_cast<OpcodeHandler>(&TestGroup::doTSZCN));
    defOp(0670, "TDO", static_cast<OpcodeHandler>(&TestGroup::doTDO));
    defOp(0671, "TSO", static_cast<OpcodeHandler>(&TestGroup::doTSO));
    defOp(0672, "TDOE", static_cast<OpcodeHandler>(&TestGroup::doTDOE));
    defOp(0673, "TSOE", static_cast<OpcodeHandler>(&TestGroup::doTSOE));
    defOp(0674, "TDOA", static_cast<OpcodeHandler>(&TestGroup::doTDOA));
    defOp(0675, "TSOA", static_cast<OpcodeHandler>(&TestGroup::doTSOA));
    defOp(0676, "TDON", static_cast<OpcodeHandler>(&TestGroup::doTDON));
    defOp(0677, "TSON", static_cast<OpcodeHandler>(&TestGroup::doTSON));
  }
};
