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
    debuggerP(nullptr)
{
  ops[0000] = &KM10::doMUUO;	// ILLEGAL
  for (unsigned op=1; op < 037; ++op) ops[op] = &KM10::doLUUO;
  for (unsigned op=040; op < 0102; ++op) ops[op] = &KM10::doMUUO;

  ops[0201] = &KM10::doMOVEI;
  ops[0202] = &KM10::doMOVEM;
  ops[0203] = &KM10::doMOVES;
  ops[0204] = &KM10::doMOVS;
  ops[0205] = &KM10::doMOVSI;
  ops[0206] = &KM10::doMOVSM;
  ops[0207] = &KM10::doMOVSS;
  ops[0210] = &KM10::doMOVN;
  ops[0211] = &KM10::doMOVNI;
  ops[0212] = &KM10::doMOVNM;
  ops[0213] = &KM10::doMOVNS;
  ops[0214] = &KM10::doMOVM;
  ops[0215] = &KM10::doMOVMI;
  ops[0216] = &KM10::doMOVMM;
  ops[0217] = &KM10::doMOVMS;
  ops[0220] = &KM10::doIMUL;
  ops[0221] = &KM10::doIMULI;
  ops[0222] = &KM10::doIMULM;
  ops[0223] = &KM10::doIMULB;
  ops[0224] = &KM10::doMUL;
  ops[0225] = &KM10::doMULI;
  ops[0226] = &KM10::doMULM;
  ops[0227] = &KM10::doMULB;
  ops[0230] = &KM10::doIDIV;
  ops[0231] = &KM10::doIDIVI;
  ops[0232] = &KM10::doIDIVM;
  ops[0233] = &KM10::doIDIVB;
  ops[0234] = &KM10::doDIV;
  ops[0235] = &KM10::doDIVI;
  ops[0236] = &KM10::doDIVM;
  ops[0237] = &KM10::doDIVB;
  ops[0240] = &KM10::doASH;
  ops[0241] = &KM10::doROT;
  ops[0242] = &KM10::doLSH;
  ops[0243] = &KM10::doJFFO;
  ops[0245] = &KM10::doROTC;
  ops[0246] = &KM10::doLSHC;
  ops[0250] = &KM10::doEXCH;
  ops[0251] = &KM10::doBLT;
  ops[0252] = &KM10::doAOBJP;
  ops[0253] = &KM10::doAOBJN;
  ops[0254] = &KM10::doJRST;
  ops[0255] = &KM10::doJFCL;
  ops[0256] = &KM10::doPXCT;
  ops[0260] = &KM10::doPUSHJ;
  ops[0261] = &KM10::doPUSH;
  ops[0262] = &KM10::doPOP;
  ops[0263] = &KM10::doPOPJ;
  ops[0264] = &KM10::doJSR;
  ops[0265] = &KM10::doJSP;
  ops[0266] = &KM10::doJSA;
  ops[0267] = &KM10::doJRA;
  ops[0270] = &KM10::doADD;
  ops[0271] = &KM10::doADDI;
  ops[0272] = &KM10::doADDM;
  ops[0273] = &KM10::doADDB;
  ops[0274] = &KM10::doSUB;
  ops[0275] = &KM10::doSUBI;
  ops[0276] = &KM10::doSUBM;
  ops[0277] = &KM10::doSUBB;
  ops[0300] = &KM10::doCAI;
  ops[0301] = &KM10::doCAIL;
  ops[0302] = &KM10::doCAIE;
  ops[0303] = &KM10::doCAILE;
  ops[0304] = &KM10::doCAIA;
  ops[0305] = &KM10::doCAIGE;
  ops[0306] = &KM10::doCAIN;
  ops[0307] = &KM10::doCAIG;
  ops[0310] = &KM10::doCAM;
  ops[0311] = &KM10::doCAML;
  ops[0312] = &KM10::doCAME;
  ops[0313] = &KM10::doCAMLE;
  ops[0314] = &KM10::doCAMA;
  ops[0315] = &KM10::doCAMGE;
  ops[0316] = &KM10::doCAMN;
  ops[0317] = &KM10::doCAMG;
  ops[0320] = &KM10::doJUMP;
  ops[0321] = &KM10::doJUMPL;
  ops[0322] = &KM10::doJUMPE;
  ops[0323] = &KM10::doJUMPLE;
  ops[0324] = &KM10::doJUMPA;
  ops[0325] = &KM10::doJUMPGE;
  ops[0326] = &KM10::doJUMPN;
  ops[0327] = &KM10::doJUMPG;
  ops[0330] = &KM10::doSKIP;
  ops[0331] = &KM10::doSKIPL;
  ops[0332] = &KM10::doSKIPE;
  ops[0333] = &KM10::doSKIPLE;
  ops[0334] = &KM10::doSKIPA;
  ops[0335] = &KM10::doSKIPGE;
  ops[0336] = &KM10::doSKIPN;
  ops[0337] = &KM10::doSKIPGT;
  ops[0340] = &KM10::doAOJ;
  ops[0341] = &KM10::doAOJL;
  ops[0342] = &KM10::doAOJE;
  ops[0343] = &KM10::doAOJLE;
  ops[0344] = &KM10::doAOJA;
  ops[0345] = &KM10::doAOJGE;
  ops[0346] = &KM10::doAOJN;
  ops[0347] = &KM10::doAOJG;
  ops[0350] = &KM10::doAOS;
  ops[0351] = &KM10::doAOSL;
  ops[0352] = &KM10::doAOSE;
  ops[0353] = &KM10::doAOSLE;
  ops[0354] = &KM10::doAOSA;
  ops[0355] = &KM10::doAOSGE;
  ops[0356] = &KM10::doAOSN;
  ops[0357] = &KM10::doAOSG;
  ops[0360] = &KM10::doSOJ;
  ops[0361] = &KM10::doSOJL;
  ops[0362] = &KM10::doSOJE;
  ops[0363] = &KM10::doSOJLE;
  ops[0364] = &KM10::doSOJA;
  ops[0365] = &KM10::doSOJGE;
  ops[0366] = &KM10::doSOJN;
  ops[0367] = &KM10::doSOJG;
  ops[0370] = &KM10::doSOS;
  ops[0371] = &KM10::doSOSL;
  ops[0372] = &KM10::doSOSE;
  ops[0373] = &KM10::doSOSLE;
  ops[0374] = &KM10::doSOSA;
  ops[0375] = &KM10::doSOSGE;
  ops[0376] = &KM10::doSOSN;
  ops[0377] = &KM10::doSOSG;
  ops[0400] = &KM10::doSETZ;
  ops[0401] = &KM10::doSETZI;
  ops[0402] = &KM10::doSETZM;
  ops[0403] = &KM10::doSETZB;
  ops[0404] = &KM10::doAND;
  ops[0405] = &KM10::doANDI;
  ops[0406] = &KM10::doANDM;
  ops[0407] = &KM10::doANDB;
  ops[0410] = &KM10::doANDCA;
  ops[0411] = &KM10::doANDCAI;
  ops[0412] = &KM10::doANDCAM;
  ops[0413] = &KM10::doANDCAB;
  ops[0414] = &KM10::doSETM;
  ops[0415] = &KM10::doSETMI;
  ops[0416] = &KM10::doSETMM;
  ops[0417] = &KM10::doSETMB;
  ops[0420] = &KM10::doANDCM;
  ops[0421] = &KM10::doANDCMI;
  ops[0422] = &KM10::doANDCMM;
  ops[0423] = &KM10::doANDCMB;
  ops[0424] = &KM10::doSETA;
  ops[0425] = &KM10::doSETAI;
  ops[0426] = &KM10::doSETAM;
  ops[0427] = &KM10::doSETAB;
  ops[0430] = &KM10::doXOR;
  ops[0431] = &KM10::doXORI;
  ops[0432] = &KM10::doXORM;
  ops[0433] = &KM10::doXORB;
  ops[0434] = &KM10::doIOR;
  ops[0435] = &KM10::doIORI;
  ops[0436] = &KM10::doIORM;
  ops[0437] = &KM10::doIORB;
  ops[0440] = &KM10::doANDCBM;
  ops[0441] = &KM10::doANDCBMI;
  ops[0442] = &KM10::doANDCBMM;
  ops[0443] = &KM10::doANDCBMB;
  ops[0444] = &KM10::doEQV;
  ops[0445] = &KM10::doEQVI;
  ops[0446] = &KM10::doEQVM;
  ops[0447] = &KM10::doEQVB;
  ops[0450] = &KM10::doSETCA;
  ops[0451] = &KM10::doSETCAI;
  ops[0452] = &KM10::doSETCAM;
  ops[0453] = &KM10::doSETCAB;
  ops[0454] = &KM10::doORCA;
  ops[0455] = &KM10::doORCAI;
  ops[0456] = &KM10::doORCAM;
  ops[0457] = &KM10::doORCAB;
  ops[0460] = &KM10::doSETCM;
  ops[0461] = &KM10::doSETCMI;
  ops[0462] = &KM10::doSETCMM;
  ops[0463] = &KM10::doSETCMB;
  ops[0464] = &KM10::doORCM;
  ops[0465] = &KM10::doORCMI;
  ops[0466] = &KM10::doORCMM;
  ops[0467] = &KM10::doORCMB;
  ops[0470] = &KM10::doORCB;
  ops[0471] = &KM10::doORCBI;
  ops[0472] = &KM10::doORCBM;
  ops[0473] = &KM10::doORCBB;
  ops[0474] = &KM10::doSETO;
  ops[0475] = &KM10::doSETOI;
  ops[0476] = &KM10::doSETOM;
  ops[0477] = &KM10::doSETOB;
  ops[0500] = &KM10::doHLL;
  ops[0501] = &KM10::doHLLI;
  ops[0502] = &KM10::doHLLM;
  ops[0503] = &KM10::doHLLS;
  ops[0504] = &KM10::doHRL;
  ops[0505] = &KM10::doHRLI;
  ops[0506] = &KM10::doHRLM;
  ops[0507] = &KM10::doHRLS;
  ops[0510] = &KM10::doHLLZ;
  ops[0511] = &KM10::doHLLZI;
  ops[0512] = &KM10::doHLLZM;
  ops[0513] = &KM10::doHLLZS;
  ops[0514] = &KM10::doHRLZ;
  ops[0515] = &KM10::doHRLZI;
  ops[0516] = &KM10::doHRLZM;
  ops[0517] = &KM10::doHRLZS;
  ops[0520] = &KM10::doHLLO;
  ops[0521] = &KM10::doHLLOI;
  ops[0522] = &KM10::doHLLOM;
  ops[0523] = &KM10::doHLLOS;
  ops[0524] = &KM10::doHRLO;
  ops[0525] = &KM10::doHRLOI;
  ops[0526] = &KM10::doHRLOM;
  ops[0527] = &KM10::doHRLOS;
  ops[0530] = &KM10::doHLLE;
  ops[0531] = &KM10::doHLLEI;
  ops[0532] = &KM10::doHLLEM;
  ops[0533] = &KM10::doHLLES;
  ops[0534] = &KM10::doHRLE;
  ops[0535] = &KM10::doHRLEI;
  ops[0536] = &KM10::doHRLEM;
  ops[0537] = &KM10::doHRLES;
  ops[0540] = &KM10::doHRR;
  ops[0541] = &KM10::doHRRI;
  ops[0542] = &KM10::doHRRM;
  ops[0543] = &KM10::doHRRS;
  ops[0544] = &KM10::doHLR;
  ops[0545] = &KM10::doHLRI;
  ops[0546] = &KM10::doHLRM;
  ops[0547] = &KM10::doHLRS;
  ops[0550] = &KM10::doHRRZ;
  ops[0551] = &KM10::doHRRZI;
  ops[0552] = &KM10::doHRRZM;
  ops[0553] = &KM10::doHRRZS;
  ops[0554] = &KM10::doHLRZ;
  ops[0555] = &KM10::doHLRZI;
  ops[0556] = &KM10::doHLRZM;
  ops[0557] = &KM10::doHLRZS;
  ops[0560] = &KM10::doHRRO;
  ops[0561] = &KM10::doHRROI;
  ops[0562] = &KM10::doHRROM;
  ops[0563] = &KM10::doHRROS;
  ops[0564] = &KM10::doHLRO;
  ops[0565] = &KM10::doHLROI;
  ops[0566] = &KM10::doHLROM;
  ops[0567] = &KM10::doHLROS;
  ops[0570] = &KM10::doHRRE;
  ops[0571] = &KM10::doHRREI;
  ops[0572] = &KM10::doHRREM;
  ops[0573] = &KM10::doHRRES;
  ops[0574] = &KM10::doHLRE;
  ops[0575] = &KM10::doHLREI;
  ops[0576] = &KM10::doHLREM;
  ops[0577] = &KM10::doHLRES;
  ops[0600] = &KM10::doTRN;
  ops[0601] = &KM10::doTLN;
  ops[0602] = &KM10::doTRNE;
  ops[0603] = &KM10::doTLNE;
  ops[0604] = &KM10::doTRNA;
  ops[0605] = &KM10::doTLNA;
  ops[0606] = &KM10::doTRNN;
  ops[0607] = &KM10::doTLNN;
  ops[0620] = &KM10::doTRZ;
  ops[0621] = &KM10::doTLZ;
  ops[0622] = &KM10::doTRZE;
  ops[0623] = &KM10::doTLZE;
  ops[0624] = &KM10::doTRZA;
  ops[0625] = &KM10::doTLZA;
  ops[0626] = &KM10::doTRZN;
  ops[0627] = &KM10::doTLZN;
  ops[0640] = &KM10::doTRC;
  ops[0641] = &KM10::doTLC;
  ops[0642] = &KM10::doTRCE;
  ops[0643] = &KM10::doTLCE;
  ops[0644] = &KM10::doTRCA;
  ops[0645] = &KM10::doTLCA;
  ops[0646] = &KM10::doTRCN;
  ops[0647] = &KM10::doTLCN;
  ops[0660] = &KM10::doTRO;
  ops[0661] = &KM10::doTLO;
  ops[0662] = &KM10::doTROE;
  ops[0663] = &KM10::doTLOE;
  ops[0664] = &KM10::doTROA;
  ops[0665] = &KM10::doTLOA;
  ops[0666] = &KM10::doTRON;
  ops[0667] = &KM10::doTLON;
  ops[0610] = &KM10::doTDN;
  ops[0611] = &KM10::doTSN;
  ops[0612] = &KM10::doTDNE;
  ops[0613] = &KM10::doTSNE;
  ops[0614] = &KM10::doTDNA;
  ops[0615] = &KM10::doTSNA;
  ops[0616] = &KM10::doTDNN;
  ops[0617] = &KM10::doTSNN;
  ops[0630] = &KM10::doTDZ;
  ops[0631] = &KM10::doTSZ;
  ops[0632] = &KM10::doTDZE;
  ops[0633] = &KM10::doTSZE;
  ops[0634] = &KM10::doTDZA;
  ops[0635] = &KM10::doTSZA;
  ops[0636] = &KM10::doTDZN;
  ops[0637] = &KM10::doTSZN;
  ops[0650] = &KM10::doTDC;
  ops[0651] = &KM10::doTSC;
  ops[0652] = &KM10::doTDCE;
  ops[0653] = &KM10::doTSCE;
  ops[0654] = &KM10::doTDCA;
  ops[0655] = &KM10::doTSCA;
  ops[0656] = &KM10::doTDCN;
  ops[0657] = &KM10::doTSZCN;
  ops[0670] = &KM10::doTDO;
  ops[0671] = &KM10::doTSO;
  ops[0672] = &KM10::doTDOE;
  ops[0673] = &KM10::doTSOE;
  ops[0674] = &KM10::doTDOA;
  ops[0675] = &KM10::doTSOA;
  ops[0676] = &KM10::doTDON;
  ops[0677] = &KM10::doTSON;

  for (unsigned op=0700; op <= 0777; ++op) ops[op] = &KM10::doIO;

  // Fill in all other opcodes as Not Yet Implemented for now.
  for (unsigned op=0; op < 0777; ++op) {

    if (ops[op] == nullptr) {
      ops[op] = &KM10::nyi;
    }
  }
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
