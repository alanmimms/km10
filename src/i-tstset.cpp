#include "km10.hpp"

struct TstSetGroup: KM10 {
  W36 zeroMaskR(W36 a, W36 b) const {return a.u & ~(uint64_t) b.rhu;};
  W36 zeroMask(W36 a, W36 b) const {return a.u & ~b.u;};

  W36 onesMaskR(W36 a, W36 b) const {return a.u | b.rhu;};
  W36 onesMask(W36 a, W36 b) const {return a.u | b.u;};

  W36 compMaskR(W36 a, W36 b) const {return a.u ^ b.rhu;};
  W36 compMask(W36 a, W36 b) const {return a.u ^ b.u;};

  W36 zeroWord(W36 a) const {return 0;};
  W36 onesWord(W36 a) const {return W36::all1s;};
  W36 compWord(W36 a) const {return ~a.u;};


  inline W36 swap(W36 src) {
    return W36{src.rhu, src.lhu};
  }


  inline W36 memGetSwapped() {
    return swap(memGet());
  }


  IResult doTRN() { return iNormal; }

  IResult doTLN() { return iNormal; }

  IResult doTRNE() {
    W36 a1 = acGetRH();
    W36 a2 = ea.rhu;
    const bool doSkip = (a1.u & a2.u) == 0;
    /* No store */;
    return doSkip ? iSkip : iNormal;
  }

  IResult doTLNE() {
    W36 a1 = acGetLH();
    W36 a2 = ea.rhu;
    W36 andResult = a2.u & a1.u;
    const bool doSkip = andResult.u == 0;
    /* No store */;
    return doSkip ? iSkip : iNormal;
  }

  IResult doTRNA() { return iSkip; }

  IResult doTLNA() { return iSkip; }

  IResult doTRNN() {
    W36 a1 = acGetRH();
    W36 a2 = ea.rhu;
    const bool doSkip = (a1.u & a2.u) != 0;
    /* No store */;
    return doSkip ? iSkip : iNormal;
  }

  IResult doTLNN() {
    W36 a1 = acGetLH();
    W36 a2 = ea.rhu;
    W36 andResult = a2.u & a1.u;
    const bool doSkip = andResult.u != 0;
    /* No store */;
    return doSkip ? iSkip : iNormal;
  }

  IResult doTRZ() {
    W36 a1 = acGetRH();
    W36 a2 = ea.rhu;
    const bool doSkip = false;
    acPutRH(zeroMaskR(a1, a2));
    return doSkip ? iSkip : iNormal;
  }

  IResult doTLZ() {
    W36 a1 = acGetLH();
    W36 a2 = ea.rhu;
    const bool doSkip = false;
    acPutLH(zeroMaskR(a1, a2));
    return doSkip ? iSkip : iNormal;
  }

  IResult doTRZE() {
    W36 a1 = acGetRH();
    W36 a2 = ea;
    const bool doSkip = (a1.u & a2.u) == 0;
    acPutRH(zeroMaskR(a1, a2));
    return doSkip ? iSkip : iNormal;
  }

  IResult doTLZE() {
    W36 a1 = acGetLH();
    W36 a2 = ea;
    const bool doSkip = (a1.u & a2.u) == 0;
    acPutLH(zeroMaskR(a1, a2));
    return doSkip ? iSkip : iNormal;
  }

  IResult doTRZA() {
    W36 a1 = acGetRH();
    W36 a2 = ea;
    const bool doSkip = true;
    acPutRH(zeroMaskR(a1, a2));
    return doSkip ? iSkip : iNormal;
  }

  IResult doTLZA() {
    W36 a1 = acGetLH();
    W36 a2 = ea;
    const bool doSkip = true;
    acPutLH(zeroMaskR(a1, a2));
    return doSkip ? iSkip : iNormal;
  }

  IResult doTRZN() {
    W36 a1 = acGetRH();
    W36 a2 = ea;
    const bool doSkip = (a1.u & a2.u) != 0;
    acPutRH(zeroMaskR(a1, a2));
    return doSkip ? iSkip : iNormal;
  }

  IResult doTLZN() {
    W36 a1 = acGetLH();
    W36 a2 = ea;
    const bool doSkip = (a1.u & a2.u) != 0;
    acPutLH(zeroMaskR(a1, a2));
    return doSkip ? iSkip : iNormal;
  }

  IResult doTRC() {
    W36 a1 = acGetRH();
    W36 a2 = ea;
    const bool doSkip = false;
    acPutRH(compMaskR(a1, a2));
    return doSkip ? iSkip : iNormal;
  }

  IResult doTLC() {
    W36 a1 = acGetLH();
    W36 a2 = ea;
    const bool doSkip = false;
    acPutLH(compMaskR(a1, a2));
    return doSkip ? iSkip : iNormal;
  }

  IResult doTRCE() {
    W36 a1 = acGetRH();
    W36 a2 = ea;
    const bool doSkip = (a1.u & a2.u) == 0;
    acPutRH(compMaskR(a1, a2));
    return doSkip ? iSkip : iNormal;
  }

  IResult doTLCE() {
    W36 a1 = acGetLH();
    W36 a2 = ea;
    const bool doSkip = (a1.u & a2.u) == 0;
    acPutLH(compMaskR(a1, a2));
    return doSkip ? iSkip : iNormal;
  }

  IResult doTRCA() {
    W36 a1 = acGetRH();
    W36 a2 = ea;
    const bool doSkip = true;
    acPutRH(compMaskR(a1, a2));
    return doSkip ? iSkip : iNormal;
  }

  IResult doTLCA() {
    W36 a1 = acGetLH();
    W36 a2 = ea;
    const bool doSkip = true;
    acPutLH(compMaskR(a1, a2));
    return doSkip ? iSkip : iNormal;
  }

  IResult doTRCN() {
    W36 a1 = acGetRH();
    W36 a2 = ea;
    const bool doSkip = (a1.u & a2.u) != 0;
    acPutRH(compMaskR(a1, a2));
    return doSkip ? iSkip : iNormal;
  }

  IResult doTLCN() {
    W36 a1 = acGetLH();
    W36 a2 = ea;
    const bool doSkip = (a1.u & a2.u) != 0;
    acPutLH(compMaskR(a1, a2));
    return doSkip ? iSkip : iNormal;
  }

  IResult doTRO() {
    W36 a1 = acGetRH();
    W36 a2 = ea;
    const bool doSkip = false;
    acPutRH(onesMaskR(a1, a2));
    return doSkip ? iSkip : iNormal;
  }

  IResult doTLO() {
    W36 a1 = acGetLH();
    W36 a2 = ea;
    const bool doSkip = false;
    acPutLH(onesMaskR(a1, a2));
    return doSkip ? iSkip : iNormal;
  }

  IResult doTROE() {
    W36 a1 = acGetRH();
    W36 a2 = ea;
    const bool doSkip = (a1.u & a2.u) == 0;
    acPutRH(onesMaskR(a1, a2));
    return doSkip ? iSkip : iNormal;
  }

  IResult doTLOE() {
    W36 a1 = acGetLH();
    W36 a2 = ea;
    const bool doSkip = (a1.u & a2.u) == 0;
    acPutLH(onesMaskR(a1, a2));
    return doSkip ? iSkip : iNormal;
  }

  IResult doTROA() {
    W36 a1 = acGetRH();
    W36 a2 = ea;
    const bool doSkip = true;
    acPutRH(onesMaskR(a1, a2));
    return doSkip ? iSkip : iNormal;
  }

  IResult doTLOA() {
    W36 a1 = acGetLH();
    W36 a2 = ea;
    const bool doSkip = true;
    acPutLH(onesMaskR(a1, a2));
    return doSkip ? iSkip : iNormal;
  }

  IResult doTRON() {
    W36 a1 = acGetRH();
    W36 a2 = ea;
    const bool doSkip = (a1.u & a2.u) != 0;
    acPutRH(onesMaskR(a1, a2));
    return doSkip ? iSkip : iNormal;
  }

  IResult doTLON() {
    W36 a1 = acGetLH();
    W36 a2 = ea;
    const bool doSkip = (a1.u & a2.u) != 0;
    acPutLH(onesMaskR(a1, a2));
    return doSkip ? iSkip : iNormal;
  }

  IResult doTDN() { (void) memGet(); return iNormal; }

  IResult doTSN() { (void) memGet(); return iNormal; }

  IResult doTDNE() {
    W36 a1 = acGet();
    W36 a2 = memGet();
    const bool doSkip = (a1.u & a2.u) == 0;
    /* No store */;
    return doSkip ? iSkip : iNormal;
  }

  IResult doTSNE() {
    W36 a1 = acGet();
    W36 a2 = memGetSwapped();
    const bool doSkip = (a1.u & a2.u) == 0;
    /* No store */;
    return doSkip ? iSkip : iNormal;
  }

  IResult doTDNA() { (void) memGet(); return iSkip; }

  IResult doTSNA() { (void) memGet(); return iSkip; }

  IResult doTDNN() {
    W36 a1 = acGet();
    W36 a2 = memGet();
    const bool doSkip = (a1.u & a2.u) != 0;
    /* No store */;
    return doSkip ? iSkip : iNormal;
  }

  IResult doTSNN() {
    W36 a1 = acGet();
    W36 a2 = memGetSwapped();
    const bool doSkip = (a1.u & a2.u) != 0;
    /* No store */;
    return doSkip ? iSkip : iNormal;
  }

  IResult doTDZ() {
    W36 a1 = acGet();
    W36 a2 = memGet();
    const bool doSkip = false;
    acPut(zeroMask(a1, a2));
    return doSkip ? iSkip : iNormal;
  }

  IResult doTSZ() {
    W36 a1 = acGet();
    W36 a2 = memGetSwapped();
    const bool doSkip = false;
    acPut(zeroMask(a1, a2));
    return doSkip ? iSkip : iNormal;
  }

  IResult doTDZE() {
    W36 a1 = acGet();
    W36 a2 = memGet();
    const bool doSkip = (a1.u & a2.u) == 0;
    acPut(zeroMask(a1, a2));
    return doSkip ? iSkip : iNormal;
  }

  IResult doTSZE() {
    W36 a1 = acGet();
    W36 a2 = memGetSwapped();
    const bool doSkip = (a1.u & a2.u) == 0;
    acPut(zeroMask(a1, a2));
    return doSkip ? iSkip : iNormal;
  }

  IResult doTDZA() {
    W36 a1 = acGet();
    W36 a2 = memGet();
    const bool doSkip = true;
    acPut(zeroMask(a1, a2));
    return doSkip ? iSkip : iNormal;
  }

  IResult doTSZA() {
    W36 a1 = acGet();
    W36 a2 = memGetSwapped();
    const bool doSkip = true;
    acPut(zeroMask(a1, a2));
    return doSkip ? iSkip : iNormal;
  }

  IResult doTDZN() {
    W36 a1 = acGet();
    W36 a2 = memGet();
    const bool doSkip = (a1.u & a2.u) != 0;
    acPut(zeroMask(a1, a2));
    return doSkip ? iSkip : iNormal;
  }

  IResult doTSZN() {
    W36 a1 = acGet();
    W36 a2 = memGetSwapped();
    const bool doSkip = (a1.u & a2.u) != 0;
    acPut(zeroMask(a1, a2));
    return doSkip ? iSkip : iNormal;
  }

  IResult doTDC() {
    W36 a1 = acGet();
    W36 a2 = memGet();
    const bool doSkip = false;
    acPut(compMask(a1, a2));
    return doSkip ? iSkip : iNormal;
  }

  IResult doTSC() {
    W36 a1 = acGet();
    W36 a2 = memGetSwapped();
    const bool doSkip = false;
    acPut(compMask(a1, a2));
    return doSkip ? iSkip : iNormal;
  }

  IResult doTDCE() {
    W36 a1 = acGet();
    W36 a2 = memGet();
    const bool doSkip = (a1.u & a2.u) == 0;
    acPut(compMask(a1, a2));
    return doSkip ? iSkip : iNormal;
  }

  IResult doTSCE() {
    W36 a1 = acGet();
    W36 a2 = memGetSwapped();
    const bool doSkip = (a1.u & a2.u) == 0;
    acPut(compMask(a1, a2));
    return doSkip ? iSkip : iNormal;
  }

  IResult doTDCA() {
    W36 a1 = acGet();
    W36 a2 = memGet();
    const bool doSkip = true;
    acPut(compMask(a1, a2));
    return doSkip ? iSkip : iNormal;
  }

  IResult doTSCA() {
    W36 a1 = acGet();
    W36 a2 = memGetSwapped();
    const bool doSkip = true;
    acPut(compMask(a1, a2));
    return doSkip ? iSkip : iNormal;
  }

  IResult doTDCN() {
    W36 a1 = acGet();
    W36 a2 = memGet();
    const bool doSkip = (a1.u & a2.u) != 0;
    acPut(compMask(a1, a2));
    return doSkip ? iSkip : iNormal;
  }

  IResult doTSCN() {
    W36 a1 = acGet();
    W36 a2 = memGetSwapped();
    const bool doSkip = (a1.u & a2.u) != 0;
    acPut(compMask(a1, a2));
    return doSkip ? iSkip : iNormal;
  }

  IResult doTDO() {
    W36 a1 = acGet();
    W36 a2 = memGet();
    const bool doSkip = false;
    acPut(onesMask(a1, a2));
    return doSkip ? iSkip : iNormal;
  }

  IResult doTSO() {
    W36 a1 = acGet();
    W36 a2 = memGetSwapped();
    const bool doSkip = false;
    acPut(onesMask(a1, a2));
    return doSkip ? iSkip : iNormal;
  }

  IResult doTDOE() {
    W36 a1 = acGet();
    W36 a2 = memGet();
    const bool doSkip = (a1.u & a2.u) == 0;
    acPut(onesMask(a1, a2));
    return doSkip ? iSkip : iNormal;
  }

  IResult doTSOE() {
    W36 a1 = acGet();
    W36 a2 = memGetSwapped();
    const bool doSkip = (a1.u & a2.u) == 0;
    acPut(onesMask(a1, a2));
    return doSkip ? iSkip : iNormal;
  }

  IResult doTDOA() {
    W36 a1 = acGet();
    W36 a2 = memGet();
    const bool doSkip = true;
    acPut(onesMask(a1, a2));
    return doSkip ? iSkip : iNormal;
  }

  IResult doTSOA() {
    W36 a1 = acGet();
    W36 a2 = memGetSwapped();
    const bool doSkip = true;
    acPut(onesMask(a1, a2));
    return doSkip ? iSkip : iNormal;
  }

  IResult doTDON() {
    W36 a1 = acGet();
    W36 a2 = memGet();
    const bool doSkip = (a1.u & a2.u) != 0;
    acPut(onesMask(a1, a2));
    return doSkip ? iSkip : iNormal;
  }

  IResult doTSON() {
    W36 a1 = acGet();
    W36 a2 = memGetSwapped();
    const bool doSkip = (a1.u & a2.u) != 0;
    acPut(onesMask(a1, a2));
    return doSkip ? iSkip : iNormal;
  }

  IResult doSETZ() { (void) memGet(); acPut(0); return iNormal; }
  IResult doSETZI() { acPut(0); return iNormal; }
  IResult doSETZM() { (void) memGet(); memPut(0); return iNormal; }
  IResult doSETZB() { (void) memGet(); acPut(0); memPut(0); return iNormal; }

  IResult doSETM() { acPut(memGet()); return iNormal; }
  IResult doSETMI() { acPut(immediate()); return iNormal; }
  IResult doSETMM() { memPut(memGet()); return iNormal; }
  IResult doSETMB() { W36 a = memGet(); acPut(a); memPut(a); return iNormal; }
  IResult doSETCM() { acPut(~memGet().u); return iNormal; }
  IResult doSETCMI() { acPut(~immediate().u); return iNormal; }
  IResult doSETCMM() { memPut(~memGet().u); return iNormal; }
  IResult doSETCMB() { W36 a = ~memGet().u; acPut(a); memPut(a); return iNormal; }
  IResult doSETO() { (void) acGet(); acPut(W36::all1s); return iNormal; }
  IResult doSETOI() { acPut(W36::all1s); return iNormal; }
  IResult doSETOM() { memPut(W36::all1s); return iNormal; }
  IResult doSETOB() { acPut(W36::all1s); memPut(W36::all1s); return iNormal; }
  IResult doSETA() { acPut(acGet()); return iNormal; }
  IResult doSETAI() { acPut(acGet()); return iNormal; }
  IResult doSETAM() { memPut(acGet()); return iNormal; }
  IResult doSETAB() { W36 a = acGet(); acPut(a); memPut(a); return iNormal; }
  IResult doSETCA() { acPut(~acGet().u); return iNormal; }
  IResult doSETCAI() { acPut(~acGet().u); return iNormal; }
  IResult doSETCAM() { memPut(~acGet().u); return iNormal; }
  IResult doSETCAB() { W36 a = ~acGet().u; acPut(a); memPut(a); return iNormal; }
};



void InstallTstSetGroup(KM10 &km10) {
  km10.defOp(0400, "SETZ",  static_cast<KM10::OpcodeHandler>(&TstSetGroup::doSETZ));
  km10.defOp(0401, "SETZI", static_cast<KM10::OpcodeHandler>(&TstSetGroup::doSETZI));
  km10.defOp(0402, "SETZM", static_cast<KM10::OpcodeHandler>(&TstSetGroup::doSETZM));
  km10.defOp(0403, "SETZB", static_cast<KM10::OpcodeHandler>(&TstSetGroup::doSETZB));

  km10.defOp(0414, "SETM",  static_cast<KM10::OpcodeHandler>(&TstSetGroup::doSETM));
  km10.defOp(0415, "SETMI", static_cast<KM10::OpcodeHandler>(&TstSetGroup::doSETMI));
  km10.defOp(0416, "SETMM", static_cast<KM10::OpcodeHandler>(&TstSetGroup::doSETMM));
  km10.defOp(0417, "SETMB", static_cast<KM10::OpcodeHandler>(&TstSetGroup::doSETMB));

  km10.defOp(0424, "SETA",  static_cast<KM10::OpcodeHandler>(&TstSetGroup::doSETA));
  km10.defOp(0425, "SETAI", static_cast<KM10::OpcodeHandler>(&TstSetGroup::doSETAI));
  km10.defOp(0426, "SETAM", static_cast<KM10::OpcodeHandler>(&TstSetGroup::doSETAM));
  km10.defOp(0427, "SETAB", static_cast<KM10::OpcodeHandler>(&TstSetGroup::doSETAB));

  km10.defOp(0450, "SETCA",  static_cast<KM10::OpcodeHandler>(&TstSetGroup::doSETCA));
  km10.defOp(0451, "SETCAI", static_cast<KM10::OpcodeHandler>(&TstSetGroup::doSETCAI));
  km10.defOp(0452, "SETCAM", static_cast<KM10::OpcodeHandler>(&TstSetGroup::doSETCAM));
  km10.defOp(0453, "SETCAB", static_cast<KM10::OpcodeHandler>(&TstSetGroup::doSETCAB));

  km10.defOp(0460, "SETCM",  static_cast<KM10::OpcodeHandler>(&TstSetGroup::doSETCM));
  km10.defOp(0461, "SETCMI", static_cast<KM10::OpcodeHandler>(&TstSetGroup::doSETCMI));
  km10.defOp(0462, "SETCMM", static_cast<KM10::OpcodeHandler>(&TstSetGroup::doSETCMM));
  km10.defOp(0463, "SETCMB", static_cast<KM10::OpcodeHandler>(&TstSetGroup::doSETCMB));

  km10.defOp(0474, "SETO",  static_cast<KM10::OpcodeHandler>(&TstSetGroup::doSETO));
  km10.defOp(0475, "SETOI", static_cast<KM10::OpcodeHandler>(&TstSetGroup::doSETOI));
  km10.defOp(0476, "SETOM", static_cast<KM10::OpcodeHandler>(&TstSetGroup::doSETOM));
  km10.defOp(0477, "SETOB", static_cast<KM10::OpcodeHandler>(&TstSetGroup::doSETOB));


  km10.defOp(0600, "TRN",  static_cast<KM10::OpcodeHandler>(&TstSetGroup::doTRN));
  km10.defOp(0601, "TLN",  static_cast<KM10::OpcodeHandler>(&TstSetGroup::doTLN));
  km10.defOp(0602, "TRNE", static_cast<KM10::OpcodeHandler>(&TstSetGroup::doTRNE));
  km10.defOp(0603, "TLNE", static_cast<KM10::OpcodeHandler>(&TstSetGroup::doTLNE));
  km10.defOp(0604, "TRNA", static_cast<KM10::OpcodeHandler>(&TstSetGroup::doTRNA));
  km10.defOp(0605, "TLNA", static_cast<KM10::OpcodeHandler>(&TstSetGroup::doTLNA));
  km10.defOp(0606, "TRNN", static_cast<KM10::OpcodeHandler>(&TstSetGroup::doTRNN));
  km10.defOp(0607, "TLNN", static_cast<KM10::OpcodeHandler>(&TstSetGroup::doTLNN));

  km10.defOp(0610, "TDN",  static_cast<KM10::OpcodeHandler>(&TstSetGroup::doTDN));
  km10.defOp(0611, "TSN",  static_cast<KM10::OpcodeHandler>(&TstSetGroup::doTSN));
  km10.defOp(0612, "TDNE", static_cast<KM10::OpcodeHandler>(&TstSetGroup::doTDNE));
  km10.defOp(0613, "TSNE", static_cast<KM10::OpcodeHandler>(&TstSetGroup::doTSNE));
  km10.defOp(0614, "TDNA", static_cast<KM10::OpcodeHandler>(&TstSetGroup::doTDNA));
  km10.defOp(0615, "TSNA", static_cast<KM10::OpcodeHandler>(&TstSetGroup::doTSNA));
  km10.defOp(0616, "TDNN", static_cast<KM10::OpcodeHandler>(&TstSetGroup::doTDNN));
  km10.defOp(0617, "TSNN", static_cast<KM10::OpcodeHandler>(&TstSetGroup::doTSNN));

  km10.defOp(0620, "TRZ",  static_cast<KM10::OpcodeHandler>(&TstSetGroup::doTRZ));
  km10.defOp(0621, "TLZ",  static_cast<KM10::OpcodeHandler>(&TstSetGroup::doTLZ));
  km10.defOp(0622, "TRZE", static_cast<KM10::OpcodeHandler>(&TstSetGroup::doTRZE));
  km10.defOp(0623, "TLZE", static_cast<KM10::OpcodeHandler>(&TstSetGroup::doTLZE));
  km10.defOp(0624, "TRZA", static_cast<KM10::OpcodeHandler>(&TstSetGroup::doTRZA));
  km10.defOp(0625, "TLZA", static_cast<KM10::OpcodeHandler>(&TstSetGroup::doTLZA));
  km10.defOp(0626, "TRZN", static_cast<KM10::OpcodeHandler>(&TstSetGroup::doTRZN));
  km10.defOp(0627, "TLZN", static_cast<KM10::OpcodeHandler>(&TstSetGroup::doTLZN));

  km10.defOp(0630, "TDZ",  static_cast<KM10::OpcodeHandler>(&TstSetGroup::doTDZ));
  km10.defOp(0631, "TSZ",  static_cast<KM10::OpcodeHandler>(&TstSetGroup::doTSZ));
  km10.defOp(0632, "TDZE", static_cast<KM10::OpcodeHandler>(&TstSetGroup::doTDZE));
  km10.defOp(0633, "TSZE", static_cast<KM10::OpcodeHandler>(&TstSetGroup::doTSZE));
  km10.defOp(0634, "TDZA", static_cast<KM10::OpcodeHandler>(&TstSetGroup::doTDZA));
  km10.defOp(0635, "TSZA", static_cast<KM10::OpcodeHandler>(&TstSetGroup::doTSZA));
  km10.defOp(0636, "TDZN", static_cast<KM10::OpcodeHandler>(&TstSetGroup::doTDZN));
  km10.defOp(0637, "TSZN", static_cast<KM10::OpcodeHandler>(&TstSetGroup::doTSZN));

  km10.defOp(0640, "TRC",  static_cast<KM10::OpcodeHandler>(&TstSetGroup::doTRC));
  km10.defOp(0641, "TLC",  static_cast<KM10::OpcodeHandler>(&TstSetGroup::doTLC));
  km10.defOp(0642, "TRCE", static_cast<KM10::OpcodeHandler>(&TstSetGroup::doTRCE));
  km10.defOp(0643, "TLCE", static_cast<KM10::OpcodeHandler>(&TstSetGroup::doTLCE));
  km10.defOp(0644, "TRCA", static_cast<KM10::OpcodeHandler>(&TstSetGroup::doTRCA));
  km10.defOp(0645, "TLCA", static_cast<KM10::OpcodeHandler>(&TstSetGroup::doTLCA));
  km10.defOp(0646, "TRCN", static_cast<KM10::OpcodeHandler>(&TstSetGroup::doTRCN));
  km10.defOp(0647, "TLCN", static_cast<KM10::OpcodeHandler>(&TstSetGroup::doTLCN));

  km10.defOp(0650, "TDC",  static_cast<KM10::OpcodeHandler>(&TstSetGroup::doTDC));
  km10.defOp(0651, "TSC",  static_cast<KM10::OpcodeHandler>(&TstSetGroup::doTSC));
  km10.defOp(0652, "TDCE", static_cast<KM10::OpcodeHandler>(&TstSetGroup::doTDCE));
  km10.defOp(0653, "TSCE", static_cast<KM10::OpcodeHandler>(&TstSetGroup::doTSCE));
  km10.defOp(0654, "TDCA", static_cast<KM10::OpcodeHandler>(&TstSetGroup::doTDCA));
  km10.defOp(0655, "TSCA", static_cast<KM10::OpcodeHandler>(&TstSetGroup::doTSCA));
  km10.defOp(0656, "TDCN", static_cast<KM10::OpcodeHandler>(&TstSetGroup::doTDCN));
  km10.defOp(0657, "TSCN", static_cast<KM10::OpcodeHandler>(&TstSetGroup::doTSCN));

  km10.defOp(0660, "TRO",  static_cast<KM10::OpcodeHandler>(&TstSetGroup::doTRO));
  km10.defOp(0661, "TLO",  static_cast<KM10::OpcodeHandler>(&TstSetGroup::doTLO));
  km10.defOp(0662, "TROE", static_cast<KM10::OpcodeHandler>(&TstSetGroup::doTROE));
  km10.defOp(0663, "TLOE", static_cast<KM10::OpcodeHandler>(&TstSetGroup::doTLOE));
  km10.defOp(0664, "TROA", static_cast<KM10::OpcodeHandler>(&TstSetGroup::doTROA));
  km10.defOp(0665, "TLOA", static_cast<KM10::OpcodeHandler>(&TstSetGroup::doTLOA));
  km10.defOp(0666, "TRON", static_cast<KM10::OpcodeHandler>(&TstSetGroup::doTRON));
  km10.defOp(0667, "TLON", static_cast<KM10::OpcodeHandler>(&TstSetGroup::doTLON));

  km10.defOp(0670, "TDO",  static_cast<KM10::OpcodeHandler>(&TstSetGroup::doTDO));
  km10.defOp(0671, "TSO",  static_cast<KM10::OpcodeHandler>(&TstSetGroup::doTSO));
  km10.defOp(0672, "TDOE", static_cast<KM10::OpcodeHandler>(&TstSetGroup::doTDOE));
  km10.defOp(0673, "TSOE", static_cast<KM10::OpcodeHandler>(&TstSetGroup::doTSOE));
  km10.defOp(0674, "TDOA", static_cast<KM10::OpcodeHandler>(&TstSetGroup::doTDOA));
  km10.defOp(0675, "TSOA", static_cast<KM10::OpcodeHandler>(&TstSetGroup::doTSOA));
  km10.defOp(0676, "TDON", static_cast<KM10::OpcodeHandler>(&TstSetGroup::doTDON));
  km10.defOp(0677, "TSON", static_cast<KM10::OpcodeHandler>(&TstSetGroup::doTSON));
}
