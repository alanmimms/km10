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
};



void InstallTestGroup(KM10 &km10) {
  km10.defOp(0600, "TRN", static_cast<KM10::OpcodeHandler>(&TestGroup::doTRN));
  km10.defOp(0601, "TLN", static_cast<KM10::OpcodeHandler>(&TestGroup::doTLN));
  km10.defOp(0602, "TRNE", static_cast<KM10::OpcodeHandler>(&TestGroup::doTRNE));
  km10.defOp(0603, "TLNE", static_cast<KM10::OpcodeHandler>(&TestGroup::doTLNE));
  km10.defOp(0604, "TRNA", static_cast<KM10::OpcodeHandler>(&TestGroup::doTRNA));
  km10.defOp(0605, "TLNA", static_cast<KM10::OpcodeHandler>(&TestGroup::doTLNA));
  km10.defOp(0606, "TRNN", static_cast<KM10::OpcodeHandler>(&TestGroup::doTRNN));
  km10.defOp(0607, "TLNN", static_cast<KM10::OpcodeHandler>(&TestGroup::doTLNN));
  km10.defOp(0610, "TRZ", static_cast<KM10::OpcodeHandler>(&TestGroup::doTRZ));
  km10.defOp(0611, "TLZ", static_cast<KM10::OpcodeHandler>(&TestGroup::doTLZ));
  km10.defOp(0612, "TRZE", static_cast<KM10::OpcodeHandler>(&TestGroup::doTRZE));
  km10.defOp(0613, "TLZE", static_cast<KM10::OpcodeHandler>(&TestGroup::doTLZE));
  km10.defOp(0614, "TRZA", static_cast<KM10::OpcodeHandler>(&TestGroup::doTRZA));
  km10.defOp(0615, "TLZA", static_cast<KM10::OpcodeHandler>(&TestGroup::doTLZA));
  km10.defOp(0616, "TRZN", static_cast<KM10::OpcodeHandler>(&TestGroup::doTRZN));
  km10.defOp(0617, "TLZN", static_cast<KM10::OpcodeHandler>(&TestGroup::doTLZN));
  km10.defOp(0620, "TRC", static_cast<KM10::OpcodeHandler>(&TestGroup::doTRC));
  km10.defOp(0621, "TLC", static_cast<KM10::OpcodeHandler>(&TestGroup::doTLC));
  km10.defOp(0622, "TRCE", static_cast<KM10::OpcodeHandler>(&TestGroup::doTRCE));
  km10.defOp(0623, "TLCE", static_cast<KM10::OpcodeHandler>(&TestGroup::doTLCE));
  km10.defOp(0624, "TRCA", static_cast<KM10::OpcodeHandler>(&TestGroup::doTRCA));
  km10.defOp(0625, "TLCA", static_cast<KM10::OpcodeHandler>(&TestGroup::doTLCA));
  km10.defOp(0626, "TRCN", static_cast<KM10::OpcodeHandler>(&TestGroup::doTRCN));
  km10.defOp(0627, "TLCN", static_cast<KM10::OpcodeHandler>(&TestGroup::doTLCN));
  km10.defOp(0630, "TRO", static_cast<KM10::OpcodeHandler>(&TestGroup::doTRO));
  km10.defOp(0631, "TLO", static_cast<KM10::OpcodeHandler>(&TestGroup::doTLO));
  km10.defOp(0632, "TROE", static_cast<KM10::OpcodeHandler>(&TestGroup::doTROE));
  km10.defOp(0633, "TLOE", static_cast<KM10::OpcodeHandler>(&TestGroup::doTLOE));
  km10.defOp(0634, "TROA", static_cast<KM10::OpcodeHandler>(&TestGroup::doTROA));
  km10.defOp(0635, "TLOA", static_cast<KM10::OpcodeHandler>(&TestGroup::doTLOA));
  km10.defOp(0636, "TRON", static_cast<KM10::OpcodeHandler>(&TestGroup::doTRON));
  km10.defOp(0637, "TLON", static_cast<KM10::OpcodeHandler>(&TestGroup::doTLON));
  km10.defOp(0640, "TDN", static_cast<KM10::OpcodeHandler>(&TestGroup::doTDN));
  km10.defOp(0641, "TSN", static_cast<KM10::OpcodeHandler>(&TestGroup::doTSN));
  km10.defOp(0642, "TDNE", static_cast<KM10::OpcodeHandler>(&TestGroup::doTDNE));
  km10.defOp(0643, "TSNE", static_cast<KM10::OpcodeHandler>(&TestGroup::doTSNE));
  km10.defOp(0644, "TDNA", static_cast<KM10::OpcodeHandler>(&TestGroup::doTDNA));
  km10.defOp(0645, "TSNA", static_cast<KM10::OpcodeHandler>(&TestGroup::doTSNA));
  km10.defOp(0646, "TDNN", static_cast<KM10::OpcodeHandler>(&TestGroup::doTDNN));
  km10.defOp(0647, "TSNN", static_cast<KM10::OpcodeHandler>(&TestGroup::doTSNN));
  km10.defOp(0650, "TDZ", static_cast<KM10::OpcodeHandler>(&TestGroup::doTDZ));
  km10.defOp(0651, "TSZ", static_cast<KM10::OpcodeHandler>(&TestGroup::doTSZ));
  km10.defOp(0652, "TDZE", static_cast<KM10::OpcodeHandler>(&TestGroup::doTDZE));
  km10.defOp(0653, "TSZE", static_cast<KM10::OpcodeHandler>(&TestGroup::doTSZE));
  km10.defOp(0654, "TDZA", static_cast<KM10::OpcodeHandler>(&TestGroup::doTDZA));
  km10.defOp(0655, "TSZA", static_cast<KM10::OpcodeHandler>(&TestGroup::doTSZA));
  km10.defOp(0656, "TDZN", static_cast<KM10::OpcodeHandler>(&TestGroup::doTDZN));
  km10.defOp(0657, "TSZN", static_cast<KM10::OpcodeHandler>(&TestGroup::doTSZN));
  km10.defOp(0660, "TDC", static_cast<KM10::OpcodeHandler>(&TestGroup::doTDC));
  km10.defOp(0661, "TSC", static_cast<KM10::OpcodeHandler>(&TestGroup::doTSC));
  km10.defOp(0662, "TDCE", static_cast<KM10::OpcodeHandler>(&TestGroup::doTDCE));
  km10.defOp(0663, "TSCE", static_cast<KM10::OpcodeHandler>(&TestGroup::doTSCE));
  km10.defOp(0664, "TDCA", static_cast<KM10::OpcodeHandler>(&TestGroup::doTDCA));
  km10.defOp(0665, "TSCA", static_cast<KM10::OpcodeHandler>(&TestGroup::doTSCA));
  km10.defOp(0666, "TDCN", static_cast<KM10::OpcodeHandler>(&TestGroup::doTDCN));
  km10.defOp(0667, "TSZCN", static_cast<KM10::OpcodeHandler>(&TestGroup::doTSZCN));
  km10.defOp(0670, "TDO", static_cast<KM10::OpcodeHandler>(&TestGroup::doTDO));
  km10.defOp(0671, "TSO", static_cast<KM10::OpcodeHandler>(&TestGroup::doTSO));
  km10.defOp(0672, "TDOE", static_cast<KM10::OpcodeHandler>(&TestGroup::doTDOE));
  km10.defOp(0673, "TSOE", static_cast<KM10::OpcodeHandler>(&TestGroup::doTSOE));
  km10.defOp(0674, "TDOA", static_cast<KM10::OpcodeHandler>(&TestGroup::doTDOA));
  km10.defOp(0675, "TSOA", static_cast<KM10::OpcodeHandler>(&TestGroup::doTSOA));
  km10.defOp(0676, "TDON", static_cast<KM10::OpcodeHandler>(&TestGroup::doTDON));
  km10.defOp(0677, "TSON", static_cast<KM10::OpcodeHandler>(&TestGroup::doTSON));
}
