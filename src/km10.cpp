#include <iostream>
#include <assert.h>
#include <atomic>
#include <unordered_set>
#include <sys/mman.h>
#include <assert.h>

using namespace std;

#include <gflags/gflags.h>

#include "word.hpp"
#include "km10.hpp"
#include "debugger.hpp"

#include "logger.hpp"

using InstructionResult = KM10::InstructionResult;


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
KM10::KM10()
  : apr(this),
    cca(this, apr),
    mtr(this),
    pi(this),
    pag(this),
    tim(this),
    dte(040, this),
    noDevice(0777777ul, "?NoDevice?", this),
    debuggerP(nullptr),
    running(false),
    restart(false),
    nextPC(0),
    exceptionPC(0),
    pc(0),
    ACbanks{},
    flags(0u),
    inInterrupt(false),
    era(0u),
    AC(ACbanks[0]),
    memorySize(nWords),
    nSteps(0),
    inXCT(false),
    addressBPs(aBPs),
    executeBPs(eBPs),    
    ops{
      [0000 .... 0777] = &doNYI, // All unspecified instruction entries are doNYI
      [0000] = &doMUUO,
      [0001] = &doLUUO,
      [0002] = &doLUUO,
      [0003] = &doLUUO,
      [0004] = &doLUUO,
      [0005] = &doLUUO,
      [0006] = &doLUUO,
      [0007] = &doLUUO,
      [0010] = &doLUUO,
      [0011] = &doLUUO,
      [0012] = &doLUUO,
      [0013] = &doLUUO,
      [0014] = &doLUUO,
      [0015] = &doLUUO,
      [0016] = &doLUUO,
      [0017] = &doLUUO,
      [0020] = &doLUUO,
      [0021] = &doLUUO,
      [0022] = &doLUUO,
      [0023] = &doLUUO,
      [0024] = &doLUUO,
      [0025] = &doLUUO,
      [0026] = &doLUUO,
      [0027] = &doLUUO,
      [0030] = &doLUUO,
      [0031] = &doLUUO,
      [0032] = &doLUUO,
      [0033] = &doLUUO,
      [0034] = &doLUUO,
      [0035] = &doLUUO,
      [0036] = &doLUUO,
      [0037] = &doLUUO,
      [0040] = &doMUUO,
      [0041] = &doMUUO,
      [0042] = &doMUUO,
      [0043] = &doMUUO,
      [0044] = &doMUUO,
      [0045] = &doMUUO,
      [0046] = &doMUUO,
      [0047] = &doMUUO,
      [0050] = &doMUUO,
      [0051] = &doMUUO,
      [0052] = &doMUUO,
      [0053] = &doMUUO,
      [0054] = &doMUUO,
      [0055] = &doMUUO,
      [0056] = &doMUUO,
      [0057] = &doMUUO,
      [0060] = &doMUUO,
      [0061] = &doMUUO,
      [0062] = &doMUUO,
      [0063] = &doMUUO,
      [0064] = &doMUUO,
      [0065] = &doMUUO,
      [0066] = &doMUUO,
      [0067] = &doMUUO,
      [0070] = &doMUUO,
      [0071] = &doMUUO,
      [0072] = &doMUUO,
      [0073] = &doMUUO,
      [0074] = &doMUUO,
      [0075] = &doMUUO,
      [0076] = &doMUUO,
      [0077] = &doMUUO,

      [0100] = &doMUUO,
      [0101] = &doMUUO,

      [0200] = &doMOVE,
      [0201] = &doMOVEI,
      [0202] = &doMOVEM,
      [0203] = &doMOVES,
      [0204] = &doMOVS,
      [0205] = &doMOVSI,
      [0206] = &doMOVSM,
      [0207] = &doMOVSS,
      [0210] = &doMOVN,
      [0211] = &doMOVNI,
      [0212] = &doMOVNM,
      [0213] = &doMOVNS,
      [0214] = &doMOVM,
      [0215] = &doMOVMI,
      [0216] = &doMOVMM,
      [0217] = &doMOVMS,
      [0220] = &doIMUL,
      [0221] = &doIMULI,
      [0222] = &doIMULM,
      [0223] = &doIMULB,
      [0224] = &doMUL,
      [0225] = &doMULI,
      [0226] = &doMULM,
      [0227] = &doMULB,
      [0230] = &doIDIV,
      [0231] = &doIDIVI,
      [0232] = &doIDIVM,
      [0233] = &doIDIVB,
      [0234] = &doDIV,
      [0235] = &doDIVI,
      [0236] = &doDIVM,
      [0237] = &doDIVB,
      [0240] = &doASH,
      [0241] = &doROT,
      [0242] = &doLSH,
      [0243] = &doJFFO,
      [0245] = &doROTC,
      [0246] = &doLSHC,
      [0250] = &doEXCH,
      [0251] = &doBLT,
      [0252] = &doAOBJP,
      [0253] = &doAOBJN,
      [0254] = &doJRST,
      [0255] = &doJFCL,
      [0256] = &doPXCT,
      [0260] = &doPUSHJ,
      [0261] = &doPUSH,
      [0262] = &doPOP,
      [0263] = &doPOPJ,
      [0264] = &doJSR,
      [0265] = &doJSP,
      [0266] = &doJSA,
      [0267] = &doJRA,
      [0270] = &doADD,
      [0271] = &doADDI,
      [0272] = &doADDM,
      [0273] = &doADDB,
      [0274] = &doSUB,
      [0275] = &doSUBI,
      [0276] = &doSUBM,
      [0277] = &doSUBB,

      [0300] = &doCAI,
      [0301] = &doCAIL,
      [0302] = &doCAIE,
      [0303] = &doCAILE,
      [0304] = &doCAIA,
      [0305] = &doCAIGE,
      [0306] = &doCAIN,
      [0307] = &doCAIG,
      [0310] = &doCAM,
      [0311] = &doCAML,
      [0312] = &doCAME,
      [0313] = &doCAMLE,
      [0314] = &doCAMA,
      [0315] = &doCAMGE,
      [0316] = &doCAMN,
      [0317] = &doCAMG,
      [0320] = &doJUMP,
      [0321] = &doJUMPL,
      [0322] = &doJUMPE,
      [0323] = &doJUMPLE,
      [0324] = &doJUMPA,
      [0325] = &doJUMPGE,
      [0326] = &doJUMPN,
      [0327] = &doJUMPG,
      [0330] = &doSKIP,
      [0331] = &doSKIPL,
      [0332] = &doSKIPE,
      [0333] = &doSKIPLE,
      [0334] = &doSKIPA,
      [0335] = &doSKIPGE,
      [0336] = &doSKIPN,
      [0337] = &doSKIPGT,
      [0340] = &doAOJ,
      [0341] = &doAOJL,
      [0342] = &doAOJE,
      [0343] = &doAOJLE,
      [0344] = &doAOJA,
      [0345] = &doAOJGE,
      [0346] = &doAOJN,
      [0347] = &doAOJG,
      [0350] = &doAOS,
      [0351] = &doAOSL,
      [0352] = &doAOSE,
      [0353] = &doAOSLE,
      [0354] = &doAOSA,
      [0355] = &doAOSGE,
      [0356] = &doAOSN,
      [0357] = &doAOSG,
      [0360] = &doSOJ,
      [0361] = &doSOJL,
      [0362] = &doSOJE,
      [0363] = &doSOJLE,
      [0364] = &doSOJA,
      [0365] = &doSOJGE,
      [0366] = &doSOJN,
      [0367] = &doSOJG,
      [0370] = &doSOS,
      [0371] = &doSOSL,
      [0372] = &doSOSE,
      [0373] = &doSOSLE,
      [0374] = &doSOSA,
      [0375] = &doSOSGE,
      [0376] = &doSOSN,
      [0377] = &doSOSG,

      [0400] = &doSETZ,
      [0401] = &doSETZI,
      [0402] = &doSETZM,
      [0403] = &doSETZB,
      [0404] = &doAND,
      [0405] = &doANDI,
      [0406] = &doANDM,
      [0407] = &doANDB,
      [0410] = &doANDCA,
      [0411] = &doANDCAI,
      [0412] = &doANDCAM,
      [0413] = &doANDCAB,
      [0414] = &doSETM,
      [0415] = &doSETMI,
      [0416] = &doSETMM,
      [0417] = &doSETMB,
      [0420] = &doANDCM,
      [0421] = &doANDCMI,
      [0422] = &doANDCMM,
      [0423] = &doANDCMB,
      [0424] = &doSETA,
      [0425] = &doSETAI,
      [0426] = &doSETAM,
      [0427] = &doSETAB,
      [0430] = &doXOR,
      [0431] = &doXORI,
      [0432] = &doXORM,
      [0433] = &doXORB,
      [0434] = &doIOR,
      [0435] = &doIORI,
      [0436] = &doIORM,
      [0437] = &doIORB,
      [0440] = &doANDCBM,
      [0441] = &doANDCBMI,
      [0442] = &doANDCBMM,
      [0443] = &doANDCBMB,
      [0444] = &doEQV,
      [0445] = &doEQVI,
      [0446] = &doEQVM,
      [0447] = &doEQVB,
      [0450] = &doSETCA,
      [0451] = &doSETCAI,
      [0452] = &doSETCAM,
      [0453] = &doSETCAB,
      [0454] = &doORCA,
      [0455] = &doORCAI,
      [0456] = &doORCAM,
      [0457] = &doORCAB,
      [0460] = &doSETCM,
      [0461] = &doSETCMI,
      [0462] = &doSETCMM,
      [0463] = &doSETCMB,
      [0464] = &doORCM,
      [0465] = &doORCMI,
      [0466] = &doORCMM,
      [0467] = &doORCMB,
      [0470] = &doORCB,
      [0471] = &doORCBI,
      [0472] = &doORCBM,
      [0473] = &doORCBB,
      [0474] = &doSETO,
      [0475] = &doSETOI,
      [0476] = &doSETOM,
      [0477] = &doSETOB,
      [0500] = &doHLL,
      [0501] = &doHLLI,
      [0502] = &doHLLM,
      [0503] = &doHLLS,
      [0504] = &doHRL,
      [0505] = &doHRLI,
      [0506] = &doHRLM,
      [0507] = &doHRLS,
      [0510] = &doHLLZ,
      [0511] = &doHLLZI,
      [0512] = &doHLLZM,
      [0513] = &doHLLZS,
      [0514] = &doHRLZ,
      [0515] = &doHRLZI,
      [0516] = &doHRLZM,
      [0517] = &doHRLZS,
      [0520] = &doHLLO,
      [0521] = &doHLLOI,
      [0522] = &doHLLOM,
      [0523] = &doHLLOS,
      [0524] = &doHRLO,
      [0525] = &doHRLOI,
      [0526] = &doHRLOM,
      [0527] = &doHRLOS,
      [0530] = &doHLLE,
      [0531] = &doHLLEI,
      [0532] = &doHLLEM,
      [0533] = &doHLLES,
      [0534] = &doHRLE,
      [0535] = &doHRLEI,
      [0536] = &doHRLEM,
      [0537] = &doHRLES,
      [0540] = &doHRR,
      [0541] = &doHRRI,
      [0542] = &doHRRM,
      [0543] = &doHRRS,
      [0544] = &doHLR,
      [0545] = &doHLRI,
      [0546] = &doHLRM,
      [0547] = &doHLRS,
      [0550] = &doHRRZ,
      [0551] = &doHRRZI,
      [0552] = &doHRRZM,
      [0553] = &doHRRZS,
      [0554] = &doHLRZ,
      [0555] = &doHLRZI,
      [0556] = &doHLRZM,
      [0557] = &doHLRZS,
      [0560] = &doHRRO,
      [0561] = &doHRROI,
      [0562] = &doHRROM,
      [0563] = &doHRROS,
      [0564] = &doHLRO,
      [0565] = &doHLROI,
      [0566] = &doHLROM,
      [0567] = &doHLROS,
      [0570] = &doHRRE,
      [0571] = &doHRREI,
      [0572] = &doHRREM,
      [0573] = &doHRRES,
      [0574] = &doHLRE,
      [0575] = &doHLREI,
      [0576] = &doHLREM,
      [0577] = &doHLRES,

      [0600] = &doTRN,
      [0601] = &doTLN,
      [0602] = &doTRNE,
      [0603] = &doTLNE,
      [0604] = &doTRNA,
      [0605] = &doTLNA,
      [0606] = &doTRNN,
      [0607] = &doTLNN,
      [0620] = &doTRZ,
      [0621] = &doTLZ,
      [0622] = &doTRZE,
      [0623] = &doTLZE,
      [0624] = &doTRZA,
      [0625] = &doTLZA,
      [0626] = &doTRZN,
      [0627] = &doTLZN,
      [0640] = &doTRC,
      [0641] = &doTLC,
      [0642] = &doTRCE,
      [0643] = &doTLCE,
      [0644] = &doTRCA,
      [0645] = &doTLCA,
      [0646] = &doTRCN,
      [0647] = &doTLCN,
      [0660] = &doTRO,
      [0661] = &doTLO,
      [0662] = &doTROE,
      [0663] = &doTLOE,
      [0664] = &doTROA,
      [0665] = &doTLOA,
      [0666] = &doTRON,
      [0667] = &doTLON,
      [0610] = &doTDN,
      [0611] = &doTSN,
      [0612] = &doTDNE,
      [0613] = &doTSNE,
      [0614] = &doTDNA,
      [0615] = &doTSNA,
      [0616] = &doTDNN,
      [0617] = &doTSNN,
      [0630] = &doTDZ,
      [0631] = &doTSZ,
      [0632] = &doTDZE,
      [0633] = &doTSZE,
      [0634] = &doTDZA,
      [0635] = &doTSZA,
      [0636] = &doTDZN,
      [0637] = &doTSZN,
      [0650] = &doTDC,
      [0651] = &doTSC,
      [0652] = &doTDCE,
      [0653] = &doTSCE,
      [0654] = &doTDCA,
      [0655] = &doTSCA,
      [0656] = &doTDCN,
      [0657] = &doTSZCN,
      [0670] = &doTDO,
      [0671] = &doTSO,
      [0672] = &doTDOE,
      [0673] = &doTSOE,
      [0674] = &doTDOA,
      [0675] = &doTSOA,
      [0676] = &doTDON,
      [0677] = &doTSON,

      [0700] = &doIO,
      [0701] = &doIO,
      [0702] = &doIO,
      [0703] = &doIO,
      [0704] = &doIO,
      [0705] = &doIO,
      [0706] = &doIO,
      [0707] = &doIO,
      [0710] = &doIO,
      [0711] = &doIO,
      [0712] = &doIO,
      [0713] = &doIO,
      [0714] = &doIO,
      [0715] = &doIO,
      [0716] = &doIO,
      [0717] = &doIO,
      [0720] = &doIO,
      [0721] = &doIO,
      [0722] = &doIO,
      [0723] = &doIO,
      [0724] = &doIO,
      [0725] = &doIO,
      [0726] = &doIO,
      [0727] = &doIO,
      [0730] = &doIO,
      [0731] = &doIO,
      [0732] = &doIO,
      [0733] = &doIO,
      [0734] = &doIO,
      [0735] = &doIO,
      [0736] = &doIO,
      [0737] = &doIO,
      [0740] = &doIO,
      [0741] = &doIO,
      [0742] = &doIO,
      [0743] = &doIO,
      [0744] = &doIO,
      [0745] = &doIO,
      [0746] = &doIO,
      [0747] = &doIO,
      [0750] = &doIO,
      [0751] = &doIO,
      [0752] = &doIO,
      [0753] = &doIO,
      [0754] = &doIO,
      [0755] = &doIO,
      [0756] = &doIO,
      [0757] = &doIO,
      [0760] = &doIO,
      [0761] = &doIO,
      [0762] = &doIO,
      [0763] = &doIO,
      [0764] = &doIO,
      [0765] = &doIO,
      [0766] = &doIO,
      [0767] = &doIO,
      [0770] = &doIO,
      [0771] = &doIO,
      [0772] = &doIO,
      [0773] = &doIO,
      [0774] = &doIO,
      [0775] = &doIO,
      [0776] = &doIO,
      [0777] = &doIO,
    }
{
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



////////////////////////////////////////////////////////////////
KM10::~KM10() {
  if (physicalP) munmap(physicalP, memorySize * sizeof(uint64_t));
}
  

////////////////////////////////////////////////////////////////
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


InstructionResult KM10::doLUUO() {

  if (pc.isSection0()) {
    W36 uuoState;
    uuoState.op = iw.op;
    uuoState.ac = iw.ac;
    uuoState.i = 0;
    uuoState.x = 0;
    uuoState.y = ea.rhu;

    // XXX this should select executive virtual space first.
    memPutN(040, uuoState);
    cerr << "LUUO at " << pc.fmtVMA() << " uuoState=" << uuoState.fmt36()
	 << logger.endl << flush;
    nextPC = 041;
    exceptionPC = pc + 1;
    inInterrupt = true;
    return iTrap;
  } else {

    if (flags.usr) {
      W36 uuoA(uptP->luuoAddr);
      memPutN(W36(((uint64_t) flags.u << 23) |
			((uint64_t) iw.op << 15) |
			((uint64_t) iw.ac << 5)), uuoA.u++);
      memPutN(pc, uuoA.u++);
      memPutN(ea.u, uuoA.u++);
      nextPC = memGetN(uuoA);
      exceptionPC = pc + 1;
      inInterrupt = true;
      return iTrap;
    } else {	       // Executive mode treats LUUOs as MUUOs
      return doMUUO();
    }
  }
}


InstructionResult KM10::doMUUO() {
  cerr << "MUUOs aren't implemented yet" << logger.endl << flush;
  exceptionPC = pc + 1;
  inInterrupt = true;
  logger.nyi(this);
  return iMUUO;
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


InstructionResult KM10::doIBP_ADJBP() {
  BytePointer *bp = BytePointer::makeFrom(ea, this);

  if (iw.ac == 0) {	// IBP
    bp->inc(this);
  } else {		// ADJBP
    bp->adjust(iw.ac, this);
  }

  return iNormal;
}


InstructionResult KM10::doILBP() {
  BytePointer *bp = BytePointer::makeFrom(ea, this);
  bp->inc(this);
  acPut(bp->getByte(this));
  return iNormal;
}


InstructionResult KM10::doLDB() {
  BytePointer *bp = BytePointer::makeFrom(ea, this);
  acPut(bp->getByte(this));
  return iNormal;
}


InstructionResult KM10::doIDPB() {
  BytePointer *bp = BytePointer::makeFrom(ea, this);
  bp->inc(this);
  bp->putByte(acGet(), this);
  return iNormal;
}


InstructionResult KM10::doDPB() {
  BytePointer *bp = BytePointer::makeFrom(ea, this);
  bp->putByte(acGet(), this);
  return iNormal;
}


InstructionResult KM10::doMOVE() {
  doMOVXX(memGet, noMod1, acPut);
  return iNormal;
}


InstructionResult KM10::doMOVEI() {
  doMOVXX(immediate, noMod1, acPut);
  return iNormal;
}

InstructionResult KM10::doMOVEM() {
  doMOVXX(acGet, noMod1, memPut);
  return iNormal;
}

InstructionResult KM10::doMOVES() {
  doMOVXX(memGet, noMod1, selfPut);
  return iNormal;
}

InstructionResult KM10::doMOVS() {
  doMOVXX(memGet, swap, acPut);
  return iNormal;
}

InstructionResult KM10::doMOVSI() {
  doMOVXX(immediate, swap, acPut);
  return iNormal;
}

InstructionResult KM10::doMOVSM() {
  doMOVXX(acGet, swap, memPut);
  return iNormal;
}

InstructionResult KM10::doMOVSS() {
  doMOVXX(memGet, swap, selfPut);
  return iNormal;
}

InstructionResult KM10::doMOVN() {
  doMOVXX(memGet, negate, acPut);
  return iNormal;
}

InstructionResult KM10::doMOVNI() {
  doMOVXX(immediate, negate, acPut);
  return iNormal;
}

InstructionResult KM10::doMOVNM() {
  doMOVXX(acGet, negate, memPut);
  return iNormal;
}

InstructionResult KM10::doMOVNS() {
  doMOVXX(memGet, negate, selfPut);
  return iNormal;
}

InstructionResult KM10::doMOVM() {
  doMOVXX(memGet, magnitude, acPut);
  return iNormal;
}

InstructionResult KM10::doMOVMI() {
  doMOVXX(immediate, magnitude, acPut);
  return iNormal;
}

InstructionResult KM10::doMOVMM() {
  doMOVXX(acGet, magnitude, memPut);
  return iNormal;
}

InstructionResult KM10::doMOVMS() {
  doMOVXX(memGet, magnitude, selfPut);
  return iNormal;
}

InstructionResult KM10::doIMUL() {
  doBinOp(acGet, memGet, imulWord, acPut);
  return iNormal;
}

InstructionResult KM10::doIMULI() {
  doBinOp(acGet, immediate, imulWord, acPut);
  return iNormal;
}

InstructionResult KM10::doIMULM() {
  doBinOp(acGet, memGet, imulWord, memPut);
  return iNormal;
}

InstructionResult KM10::doIMULB() {
  doBinOp(acGet, memGet, imulWord, bothPut);
  return iNormal;
}

InstructionResult KM10::doMUL() {
  doBinOp(acGet, memGet, mulWord, acPut2);
  return iNormal;
}

InstructionResult KM10::doMULI() {
  doBinOp(acGet, immediate, mulWord, acPut2);
  return iNormal;
}

InstructionResult KM10::doMULM() {
  doBinOp(acGet, memGet, mulWord, memPutHi);
  return iNormal;
}

InstructionResult KM10::doMULB() {
  doBinOp(acGet, memGet, mulWord, bothPut2);
  return iNormal;
}

InstructionResult KM10::doIDIV() {
  doBinOp(acGet, memGet, idivWord, acPut2);
  return iNormal;
}

InstructionResult KM10::doIDIVI() {
  doBinOp(acGet, immediate, idivWord, acPut2);
  return iNormal;
}

InstructionResult KM10::doIDIVM() {
  doBinOp(acGet, memGet, idivWord, memPutHi);
  return iNormal;
}

InstructionResult KM10::doIDIVB() {
  doBinOp(acGet, memGet, idivWord, bothPut2);
  return iNormal;
}

InstructionResult KM10::doDIV() {
  doBinOp(acGet2, memGet, divWord, acPut2);
  return iNormal;
}

InstructionResult KM10::doDIVI() {
  doBinOp(acGet2, immediate, divWord, acPut2);
  return iNormal;
}

InstructionResult KM10::doDIVM() {
  doBinOp(acGet2, memGet, divWord, memPutHi);
  return iNormal;
}

InstructionResult KM10::doDIVB() {
  doBinOp(acGet2, memGet, divWord, bothPut2);
  return iNormal;
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

InstructionResult KM10::doAOBJP() {
  W36 tmp = acGet();
  tmp = W36(tmp.lhu + 1, tmp.rhu + 1);
  acPut(tmp);

  if (tmp.ext64() >= 0) {
    logFlow("jump");
    nextPC = ea;
  }

  return iNormal;
}

InstructionResult KM10::doAOBJN() {
  W36 tmp = acGet();
  tmp = W36(tmp.lhu + 1, tmp.rhu + 1);
  acPut(tmp);

  if (tmp.ext64() < 0) {
    logFlow("jump");
    nextPC = ea;
  }

  return iNormal;
}


InstructionResult KM10::doJRST() {

  switch (iw.ac) {
  case 000:					// JRST
    nextPC.rhu = ea.rhu;
    break;

  case 001:					// PORTAL
    logger.nyi(this);
    break;

  case 002:					// JRSTF
    restoreFlags(ea);
    nextPC.rhu = ea.rhu;
    break;

  case 004:					// HALT
    cerr << "[HALT at " << pc.fmtVMA() << "]" << logger.endl;
    running = false;
    nextPC.rhu = ea.rhu;		// HALT actually does change PC
    return iHALT;

  case 005:					// XJRSTF
    logger.nyi(this);
    break;

  case 006:					// XJEN
    pi.dismissInterrupt();
    logger.nyi(this);
    break;

  case 007:					// XPCW
    logger.nyi(this);
    break;

  case 010:					// 25440 - no mnemonic
    restoreFlags(ea);
    break;

  case 012:					// JEN
    cerr << ">>>>>> JEN ea=" << ea.fmtVMA() << logger.endl << flush;
    pi.dismissInterrupt();
    restoreFlags(ea);
    nextPC.rhu = ea.rhu;
    break;

  case 014:					// SFM
    logger.nyi(this);
    break;

  default:
    logger.nyi(this);
    break;
  }

  return iJump;
}



InstructionResult KM10::doJFCL() {
  unsigned wasFlags = flags.u;
  unsigned testFlags = (unsigned) iw.ac << 9; // Align with OV,CY0,CY1,FOV
  flags.u &= ~testFlags;
  if (wasFlags & testFlags) nextPC = ea;
  return iJump;
}


InstructionResult KM10::doPXCT() {

  if (userMode() || iw.ac == 0) {
    pc = ea;
    inXCT = true;
    return iXCT;
  } else {					// PXCT
    logger.nyi(this);
    running = false;
    return iHALT;		// XXX for now
  }
}


InstructionResult KM10::doPUSHJ() {
  // Note this sets the flags that are cleared by PUSHJ before
  // doPush() since doPush() can set flags.tr2.
  flags.fpd = flags.afi = flags.tr1 = flags.tr2 = 0;
  doPush(pc.isSection0() ? flagsWord(nextPC.rhu) : W36(nextPC.vma), iw.ac);
  nextPC = ea;
  if (inInterrupt) flags.usr = flags.pub = 0;
  return iJump;
}


InstructionResult KM10::doPUSH() {
  doPush(memGet(), iw.ac);
  return iNormal;
}

InstructionResult KM10::doPOP() {
  memPut(doPop(iw.ac));
  return iNormal;
}

InstructionResult KM10::doPOPJ() {
  nextPC.rhu = doPop(iw.ac).rhu;
  return iJump;
}

InstructionResult KM10::doJSR() {
  W36 tmp = inInterrupt ? exceptionPC : nextPC;
  tmp = pc.isSection0() ? flagsWord(tmp.rhu) : W36(tmp.vma);
  cerr << ">>>>>> JSR saved PC=" << tmp.fmt36() << "  ea=" << ea.fmt36()
       << (inInterrupt ? "[inInterrupt]" : "[!inInterrupt]")
       << logger.endl << flush;
  memPut(tmp);
  nextPC.rhu = ea.rhu + 1;
  flags.fpd = flags.afi = flags.tr2 = flags.tr1 = 0;
  if (inInterrupt) flags.usr = flags.pub = 0;
  return iJump;
}

InstructionResult KM10::doJSP() {
  W36 tmp = inInterrupt ? exceptionPC : nextPC;
  tmp = pc.isSection0() ? flagsWord(tmp.rhu) : W36(tmp.vma);
  cerr << ">>>>>> JSP set ac=" << tmp.fmt36() << "  ea=" << ea.fmt36()
       << (inInterrupt ? "[inInterrupt]" : "[!inInterrupt]")
       << logger.endl << flush;
  acPut(tmp);
  nextPC.rhu = ea.rhu;
  flags.fpd = flags.afi = flags.tr2 = flags.tr1 = 0;
  if (inInterrupt) flags.usr = flags.pub = 0;
  return iJump;
}

InstructionResult KM10::doJSA() {
  memPut(acGet());
  nextPC.rhu = ea.rhu + 1;
  acPut(W36(ea.rhu, pc.rhu + 1));
  if (inInterrupt) flags.usr = 0;
  return iJump;
}

InstructionResult KM10::doJRA() {
  acPut(memGetN(acGet().lhu));
  nextPC = ea;
  return iJump;
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

InstructionResult KM10::doCAI() {
  return iNormal;
}

InstructionResult KM10::doCAIL() {
  doCAXXX(acGet, immediate, isLT);
  return iNormal;
}

InstructionResult KM10::doCAIE() {
  doCAXXX(acGet, immediate, isEQ);
  return iNormal;
}

InstructionResult KM10::doCAILE() {
  doCAXXX(acGet, immediate, isLE);
  return iNormal;
}

InstructionResult KM10::doCAIA() {
  doCAXXX(acGet, immediate, always2);
  return iNormal;
}

InstructionResult KM10::doCAIGE() {
  doCAXXX(acGet, immediate, isGE);
  return iNormal;
}

InstructionResult KM10::doCAIN() {
  doCAXXX(acGet, immediate, isNE);
  return iNormal;
}

InstructionResult KM10::doCAIG() {
  doCAXXX(acGet, immediate, isGT);
  return iNormal;
}

InstructionResult KM10::doCAM() {
  return iNormal;
}

InstructionResult KM10::doCAML() {
  doCAXXX(acGet, memGet, isLT);
  return iNormal;
}

InstructionResult KM10::doCAME() {
  doCAXXX(acGet, memGet, isEQ);
  return iNormal;
}

InstructionResult KM10::doCAMLE() {
  doCAXXX(acGet, memGet, isLE);
  return iNormal;
}

InstructionResult KM10::doCAMA() {
  doCAXXX(acGet, memGet, always2);
  return iNormal;
}

InstructionResult KM10::doCAMGE() {
  doCAXXX(acGet, memGet, isGE);
  return iNormal;
}

InstructionResult KM10::doCAMN() {
  doCAXXX(acGet, memGet, isNE);
  return iNormal;
}

InstructionResult KM10::doCAMG() {
  doCAXXX(acGet, memGet, isGT);
  return iNormal;
}

InstructionResult KM10::doJUMP() {
  doJUMP(never);
  return iNormal;
}

InstructionResult KM10::doJUMPL() {
  doJUMP(isLT0);
  return iNormal;
}

InstructionResult KM10::doJUMPE() {
  doJUMP(isEQ0);
  return iNormal;
}

InstructionResult KM10::doJUMPLE() {
  doJUMP(isLE0);
  return iNormal;
}

InstructionResult KM10::doJUMPA() {
  doJUMP(always);
  return iNormal;
}

InstructionResult KM10::doJUMPGE() {
  doJUMP(isGE0);
  return iNormal;
}

InstructionResult KM10::doJUMPN() {
  doJUMP(isNE0);
  return iNormal;
}

InstructionResult KM10::doJUMPG() {
  doJUMP(isGT0);
  return iNormal;
}

InstructionResult KM10::doSKIP() {
  doSKIP(never);
  return iNormal;
}

InstructionResult KM10::doSKIPL() {
  doSKIP(isLT0);
  return iNormal;
}

InstructionResult KM10::doSKIPE() {
  doSKIP(isEQ0);
  return iNormal;
}

InstructionResult KM10::doSKIPLE() {
  doSKIP(isLE0);
  return iNormal;
}

InstructionResult KM10::doSKIPA() {
  doSKIP(always);
  return iNormal;
}

InstructionResult KM10::doSKIPGE() {
  doSKIP(isGE0);
  return iNormal;
}

InstructionResult KM10::doSKIPN() {
  doSKIP(isNE0);
  return iNormal;
}

InstructionResult KM10::doSKIPGT() {
  doSKIP(isGT0);
  return iNormal;
}

InstructionResult KM10::doAOJ() {
  doAOSXX(acGet, 1, acPut, never, jumpAction);
  return iNormal;
}

InstructionResult KM10::doAOJL() {
  doAOSXX(acGet, 1, acPut, isLT0, jumpAction);
  return iNormal;
}

InstructionResult KM10::doAOJE() {
  doAOSXX(acGet, 1, acPut, isEQ0, jumpAction);
  return iNormal;
}

InstructionResult KM10::doAOJLE() {
  doAOSXX(acGet, 1, acPut, isLE0, jumpAction);
  return iNormal;
}

InstructionResult KM10::doAOJA() {
  doAOSXX(acGet, 1, acPut, always, jumpAction);
  return iNormal;
}

InstructionResult KM10::doAOJGE() {
  doAOSXX(acGet, 1, acPut, isGE0, jumpAction);
  return iNormal;
}

InstructionResult KM10::doAOJN() {
  doAOSXX(acGet, 1, acPut, isNE0, jumpAction);
  return iNormal;
}

InstructionResult KM10::doAOJG() {
  doAOSXX(acGet, 1, acPut, isGT0, jumpAction);
  return iNormal;
}

InstructionResult KM10::doAOS() {
  doAOSXX(memGet, 1, memPut, never, skipAction);
  return iNormal;
}

InstructionResult KM10::doAOSL() {
  doAOSXX(memGet, 1, memPut, isLT0, skipAction);
  return iNormal;
}

InstructionResult KM10::doAOSE() {
  doAOSXX(memGet, 1, memPut, isEQ0, skipAction);
  return iNormal;
}

InstructionResult KM10::doAOSLE() {
  doAOSXX(memGet, 1, memPut, isLE0, skipAction);
  return iNormal;
}

InstructionResult KM10::doAOSA() {
  doAOSXX(memGet, 1, memPut, always, skipAction);
  return iNormal;
}

InstructionResult KM10::doAOSGE() {
  doAOSXX(memGet, 1, memPut, isGE0, skipAction);
  return iNormal;
}

InstructionResult KM10::doAOSN() {
  doAOSXX(memGet, 1, memPut, isNE0, skipAction);
  return iNormal;
}

InstructionResult KM10::doAOSG() {
  doAOSXX(memGet, 1, memPut, isGT0, skipAction);
  return iNormal;
}

InstructionResult KM10::doSOJ() {
  doAOSXX(acGet, -1, acPut, never, jumpAction);
  return iNormal;
}

InstructionResult KM10::doSOJL() {
  doAOSXX(acGet, -1, acPut, isLT0, jumpAction);
  return iNormal;
}

InstructionResult KM10::doSOJE() {
  doAOSXX(acGet, -1, acPut, isEQ0, jumpAction);
  return iNormal;
}

InstructionResult KM10::doSOJLE() {
  doAOSXX(acGet, -1, acPut, isLE0, jumpAction);
  return iNormal;
}

InstructionResult KM10::doSOJA() {
  doAOSXX(acGet, -1, acPut, always, jumpAction);
  return iNormal;
}

InstructionResult KM10::doSOJGE() {
  doAOSXX(acGet, -1, acPut, isGE0, jumpAction);
  return iNormal;
}

InstructionResult KM10::doSOJN() {
  doAOSXX(acGet, -1, acPut, isNE0, jumpAction);
  return iNormal;
}

InstructionResult KM10::doSOJG() {
  doAOSXX(acGet, -1, acPut, isGT0, jumpAction);
  return iNormal;
}

InstructionResult KM10::doSOS() {
  doAOSXX(memGet, -1, memPut, never, skipAction);
  return iNormal;
}

InstructionResult KM10::doSOSL() {
  doAOSXX(memGet, -1, memPut, isLT0, skipAction);
  return iNormal;
}

InstructionResult KM10::doSOSE() {
  doAOSXX(memGet, -1, memPut, isEQ0, skipAction);
  return iNormal;
}

InstructionResult KM10::doSOSLE() {
  doAOSXX(memGet, -1, memPut, isLE0, skipAction);
  return iNormal;
}

InstructionResult KM10::doSOSA() {
  doAOSXX(memGet, -1, memPut, always, skipAction);
  return iNormal;
}

InstructionResult KM10::doSOSGE() {
  doAOSXX(memGet, -1, memPut, isGE0, skipAction);
  return iNormal;
}

InstructionResult KM10::doSOSN() {
  doAOSXX(memGet, -1, memPut, isNE0, skipAction);
  return iNormal;
}

InstructionResult KM10::doSOSG() {
  doAOSXX(memGet, -1, memPut, isGT0, skipAction);
  return iNormal;
}

InstructionResult KM10::doSETZ() {
  doSETXX(memGet, zeroWord, acPut);
  return iNormal;
}

InstructionResult KM10::doSETZI() {
  doSETXX(immediate, zeroWord, acPut);
  return iNormal;
}

InstructionResult KM10::doSETZM() {
  doSETXX(memGet, zeroWord, memPut);
  return iNormal;
}

InstructionResult KM10::doSETZB() {
  doSETXX(memGet, zeroWord, bothPut);
  return iNormal;
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

InstructionResult KM10::doSETM() {
  doSETXX(memGet, noMod1, acPut);
  return iNormal;
}

InstructionResult KM10::doSETMI() {
  doSETXX(immediate, noMod1, acPut);
  return iNormal;
}

InstructionResult KM10::doSETMM() {
  doSETXX(memGet, noMod1, memPut);
  return iNormal;
}

InstructionResult KM10::doSETMB() {
  doSETXX(memGet, noMod1, bothPut);
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

InstructionResult KM10::doSETA() {
  doSETXX(acGet, noMod1, acPut);
  return iNormal;
}

InstructionResult KM10::doSETAI() {
  doSETXX(acGet, noMod1, acPut);
  return iNormal;
}

InstructionResult KM10::doSETAM() {
  doSETXX(acGet, noMod1, memPut);
  return iNormal;
}

InstructionResult KM10::doSETAB() {
  doSETXX(acGet, noMod1, bothPut);
  return iNormal;
}

InstructionResult KM10::doXOR() {
  doBinOp(memGet, acGet, xorWord, acPut);
  return iNormal;
}

InstructionResult KM10::doXORI() {
  doBinOp(immediate, acGet, xorWord, acPut);
  return iNormal;
}

InstructionResult KM10::doXORM() {
  doBinOp(memGet, acGet, xorWord, memPut);
  return iNormal;
}

InstructionResult KM10::doXORB() {
  doBinOp(memGet, acGet, xorWord, bothPut);
  return iNormal;
}

InstructionResult KM10::doIOR() {
  doBinOp(memGet, acGet, iorWord, acPut);
  return iNormal;
}

InstructionResult KM10::doIORI() {
  doBinOp(immediate, acGet, iorWord, acPut);
  return iNormal;
}

InstructionResult KM10::doIORM() {
  doBinOp(memGet, acGet, iorWord, memPut);
  return iNormal;
}

InstructionResult KM10::doIORB() {
  doBinOp(memGet, acGet, iorWord, bothPut);
  return iNormal;
}

InstructionResult KM10::doANDCBM() {
  doBinOp(memGet, acGet, andCBWord, acPut);
  return iNormal;
}

InstructionResult KM10::doANDCBMI() {
  doBinOp(immediate, acGet, andCBWord, acPut);
  return iNormal;
}

InstructionResult KM10::doANDCBMM() {
  doBinOp(memGet, acGet, andCBWord, memPut);
  return iNormal;
}

InstructionResult KM10::doANDCBMB() {
  doBinOp(memGet, acGet, andCBWord, bothPut);
  return iNormal;
}

InstructionResult KM10::doEQV() {
  doBinOp(memGet, acGet, eqvWord, acPut);
  return iNormal;
}

InstructionResult KM10::doEQVI() {
  doBinOp(immediate, acGet, eqvWord, acPut);
  return iNormal;
}

InstructionResult KM10::doEQVM() {
  doBinOp(memGet, acGet, eqvWord, memPut);
  return iNormal;
}

InstructionResult KM10::doEQVB() {
  doBinOp(memGet, acGet, eqvWord, bothPut);
  return iNormal;
}

InstructionResult KM10::doSETCA() {
  doSETXX(acGet, compWord, acPut);
  return iNormal;
}

InstructionResult KM10::doSETCAI() {
  doSETXX(acGet, compWord, acPut);
  return iNormal;
}

InstructionResult KM10::doSETCAM() {
  doSETXX(acGet, compWord, memPut);
  return iNormal;
}

InstructionResult KM10::doSETCAB() {
  doSETXX(acGet, compWord, bothPut);
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

InstructionResult KM10::doSETCM() {
  doSETXX(memGet, compWord, acPut);
  return iNormal;
}

InstructionResult KM10::doSETCMI() {
  doSETXX(immediate, compWord, acPut);
  return iNormal;
}

InstructionResult KM10::doSETCMM() {
  doSETXX(memGet, compWord, memPut);
  return iNormal;
}

InstructionResult KM10::doSETCMB() {
  doSETXX(memGet, compWord, bothPut);
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

InstructionResult KM10::doSETO() {
  doSETXX(acGet, onesWord, acPut);
  return iNormal;
}

InstructionResult KM10::doSETOI() {
  doSETXX(acGet, onesWord, acPut);
  return iNormal;
}

InstructionResult KM10::doSETOM() {
  doSETXX(memGet, onesWord, memPut);
  return iNormal;
}

InstructionResult KM10::doSETOB() {
  doSETXX(memGet, onesWord, bothPut);
  return iNormal;
}

InstructionResult KM10::doHLL() {
  doHXXXX(memGet, acGet, copyHLL, noMod1, acPut);
  return iNormal;
}

InstructionResult KM10::doHLLI() {
  doHXXXX(immediate, acGet, copyHLL, noMod1, acPut);
  return iNormal;
}

InstructionResult KM10::doHLLM() {
  doHXXXX(acGet, memGet, copyHLL, noMod1, memPut);
  return iNormal;
}

InstructionResult KM10::doHLLS() {
  doHXXXX(memGet, memGet, copyHLL, noMod1, selfPut);
  return iNormal;
}

InstructionResult KM10::doHRL() {
  doHXXXX(memGet, acGet, copyHRL, noMod1, acPut);
  return iNormal;
}

InstructionResult KM10::doHRLI() {
  doHXXXX(immediate, acGet, copyHRL, noMod1, acPut);
  return iNormal;
}

InstructionResult KM10::doHRLM() {
  doHXXXX(acGet, memGet, copyHRL, noMod1, memPut);
  return iNormal;
}

InstructionResult KM10::doHRLS() {
  doHXXXX(memGet, memGet, copyHRL, noMod1, selfPut);
  return iNormal;
}

InstructionResult KM10::doHLLZ() {
  doHXXXX(memGet, acGet, copyHLL, zeroR, acPut);
  return iNormal;
}

InstructionResult KM10::doHLLZI() {
  doHXXXX(immediate, acGet, copyHLL, zeroR, acPut);
  return iNormal;
}

InstructionResult KM10::doHLLZM() {
  doHXXXX(acGet, memGet, copyHLL, zeroR, memPut);
  return iNormal;
}

InstructionResult KM10::doHLLZS() {
  doHXXXX(memGet, memGet, copyHLL, zeroR, selfPut);
  return iNormal;
}

InstructionResult KM10::doHRLZ() {
  doHXXXX(memGet, acGet, copyHRL, zeroR, acPut);
  return iNormal;
}

InstructionResult KM10::doHRLZI() {
  doHXXXX(immediate, acGet, copyHRL, zeroR, acPut);
  return iNormal;
}

InstructionResult KM10::doHRLZM() {
  doHXXXX(acGet, memGet, copyHRL, zeroR, memPut);
  return iNormal;
}

InstructionResult KM10::doHRLZS() {
  doHXXXX(memGet, memGet, copyHRL, zeroR, selfPut);
  return iNormal;
}

InstructionResult KM10::doHLLO() {
  doHXXXX(memGet, acGet, copyHLL, onesR, acPut);
  return iNormal;
}

InstructionResult KM10::doHLLOI() {
  doHXXXX(immediate, acGet, copyHLL, onesR, acPut);
  return iNormal;
}

InstructionResult KM10::doHLLOM() {
  doHXXXX(acGet, memGet, copyHLL, onesR, memPut);
  return iNormal;
}

InstructionResult KM10::doHLLOS() {
  doHXXXX(memGet, memGet, copyHLL, onesR, selfPut);
  return iNormal;
}

InstructionResult KM10::doHRLO() {
  doHXXXX(memGet, acGet, copyHRL, onesR, acPut);
  return iNormal;
}

InstructionResult KM10::doHRLOI() {
  doHXXXX(immediate, acGet, copyHRL, onesR, acPut);
  return iNormal;
}

InstructionResult KM10::doHRLOM() {
  doHXXXX(acGet, memGet, copyHRL, onesR, memPut);
  return iNormal;
}

InstructionResult KM10::doHRLOS() {
  doHXXXX(memGet, memGet, copyHRL, onesR, selfPut);
  return iNormal;
}

InstructionResult KM10::doHLLE() {
  doHXXXX(memGet, acGet, copyHLL, extnL, acPut);
  return iNormal;
}

InstructionResult KM10::doHLLEI() {
  doHXXXX(immediate, acGet, copyHLL, extnL, acPut);
  return iNormal;
}

InstructionResult KM10::doHLLEM() {
  doHXXXX(acGet, memGet, copyHLL, extnL, memPut);
  return iNormal;
}

InstructionResult KM10::doHLLES() {
  doHXXXX(memGet, memGet, copyHLL, extnL, selfPut);
  return iNormal;
}

InstructionResult KM10::doHRLE() {
  doHXXXX(memGet, acGet, copyHRL, extnL, acPut);
  return iNormal;
}

InstructionResult KM10::doHRLEI() {
  doHXXXX(immediate, acGet, copyHRL, extnL, acPut);
  return iNormal;
}

InstructionResult KM10::doHRLEM() {
  doHXXXX(acGet, memGet, copyHRL, extnL, memPut);
  return iNormal;
}

InstructionResult KM10::doHRLES() {
  doHXXXX(memGet, memGet, copyHRL, extnL, selfPut);
  return iNormal;
}

InstructionResult KM10::doHRR() {
  doHXXXX(memGet, acGet, copyHRR, noMod1, acPut);
  return iNormal;
}

InstructionResult KM10::doHRRI() {
  doHXXXX(immediate, acGet, copyHRR, noMod1, acPut);
  return iNormal;
}

InstructionResult KM10::doHRRM() {
  doHXXXX(acGet, memGet, copyHRR, noMod1, memPut);
  return iNormal;
}

InstructionResult KM10::doHRRS() {
  doHXXXX(memGet, memGet, copyHRR, noMod1, selfPut);
  return iNormal;
}

InstructionResult KM10::doHLR() {
  doHXXXX(memGet, acGet, copyHLR, noMod1, acPut);
  return iNormal;
}

InstructionResult KM10::doHLRI() {
  doHXXXX(immediate, acGet, copyHLR, noMod1, acPut);
  return iNormal;
}

InstructionResult KM10::doHLRM() {
  doHXXXX(acGet, memGet, copyHLR, noMod1, memPut);
  return iNormal;
}

InstructionResult KM10::doHLRS() {
  doHXXXX(memGet, memGet, copyHLR, noMod1, selfPut);
  return iNormal;
}

InstructionResult KM10::doHRRZ() {
  doHXXXX(memGet, acGet, copyHRR, zeroL, acPut);
  return iNormal;
}

InstructionResult KM10::doHRRZI() {
  doHXXXX(immediate, acGet, copyHRR, zeroL, acPut);
  return iNormal;
}

InstructionResult KM10::doHRRZM() {
  doHXXXX(acGet, memGet, copyHRR, zeroL, memPut);
  return iNormal;
}

InstructionResult KM10::doHRRZS() {
  doHXXXX(memGet, memGet, copyHRR, zeroL, selfPut);
  return iNormal;
}

InstructionResult KM10::doHLRZ() {
  doHXXXX(memGet, acGet, copyHLR, zeroL, acPut);
  return iNormal;
}

InstructionResult KM10::doHLRZI() {
  doHXXXX(immediate, acGet, copyHLR, zeroL, acPut);
  return iNormal;
}

InstructionResult KM10::doHLRZM() {
  doHXXXX(acGet, memGet, copyHLR, zeroL, memPut);
  return iNormal;
}

InstructionResult KM10::doHLRZS() {
  doHXXXX(memGet, memGet, copyHLR, zeroL, selfPut);
  return iNormal;
}

InstructionResult KM10::doHRRO() {
  doHXXXX(memGet, acGet, copyHRR, onesL, acPut);
  return iNormal;
}

InstructionResult KM10::doHRROI() {
  doHXXXX(immediate, acGet, copyHRR, onesL, acPut);
  return iNormal;
}

InstructionResult KM10::doHRROM() {
  doHXXXX(acGet, memGet, copyHRR, onesL, memPut);
  return iNormal;
}

InstructionResult KM10::doHRROS() {
  doHXXXX(memGet, memGet, copyHRR, onesL, selfPut);
  return iNormal;
}

InstructionResult KM10::doHLRO() {
  doHXXXX(memGet, acGet, copyHLR, onesL, acPut);
  return iNormal;
}

InstructionResult KM10::doHLROI() {
  doHXXXX(immediate, acGet, copyHLR, onesL, acPut);
  return iNormal;
}

InstructionResult KM10::doHLROM() {
  doHXXXX(acGet, memGet, copyHLR, onesL, memPut);
  return iNormal;
}

InstructionResult KM10::doHLROS() {
  doHXXXX(memGet, memGet, copyHLR, onesL, selfPut);
  return iNormal;
}

InstructionResult KM10::doHRRE() {
  doHXXXX(memGet, acGet, copyHRR, extnR, acPut);
  return iNormal;
}

InstructionResult KM10::doHRREI() {
  doHXXXX(immediate, acGet, copyHRR, extnR, acPut);
  return iNormal;
}

InstructionResult KM10::doHRREM() {
  doHXXXX(acGet, memGet, copyHRR, extnR, memPut);
  return iNormal;
}

InstructionResult KM10::doHRRES() {
  doHXXXX(memGet, memGet, copyHRR, extnR, selfPut);
  return iNormal;
}

InstructionResult KM10::doHLRE() {
  doHXXXX(memGet, acGet, copyHLR, extnR, acPut);
  return iNormal;
}

InstructionResult KM10::doHLREI() {
  doHXXXX(immediate, acGet, copyHLR, extnR, acPut);
  return iNormal;
}

InstructionResult KM10::doHLREM() {
  doHXXXX(acGet, memGet, copyHLR, extnR, memPut);
  return iNormal;
}

InstructionResult KM10::doHLRES() {
  doHXXXX(memGet, memGet, copyHLR, extnR, selfPut);
  return iNormal;
}

InstructionResult KM10::doTRN() {
  return iNormal;
}

InstructionResult KM10::doTLN() {
  return iNormal;
}

InstructionResult KM10::doTRNE() {
  doTXXXX(acGetRH, getE, noMod2, isEQ0T, noStore);
  return iNormal;
}

InstructionResult KM10::doTLNE() {
  doTXXXX(acGetLH, getE, noMod2, isEQ0T, noStore);
  return iNormal;
}

InstructionResult KM10::doTRNA() {
  ++nextPC.rhu;
  return iNormal;
}

InstructionResult KM10::doTLNA() {
  ++nextPC.rhu;
  return iNormal;
}

InstructionResult KM10::doTRNN() {
  doTXXXX(acGetRH, getE, noMod2, isNE0T, noStore);
  return iNormal;
}

InstructionResult KM10::doTLNN() {
  doTXXXX(acGetLH, getE, noMod2, isNE0T, noStore);
  return iNormal;
}

InstructionResult KM10::doTRZ() {
  doTXXXX(acGetRH, getE, zeroMaskR, neverT, acPutRH);
  return iNormal;
}

InstructionResult KM10::doTLZ() {
  doTXXXX(acGetLH, getE, zeroMaskR, neverT, acPutLH);
  return iNormal;
}

InstructionResult KM10::doTRZE() {
  doTXXXX(acGetRH, getE, zeroMaskR, isEQ0T, acPutRH);
  return iNormal;
}

InstructionResult KM10::doTLZE() {
  doTXXXX(acGetLH, getE, zeroMaskR, isEQ0T, acPutLH);
  return iNormal;
}

InstructionResult KM10::doTRZA() {
  doTXXXX(acGetRH, getE, zeroMaskR, alwaysT, acPutRH);
  return iNormal;
}

InstructionResult KM10::doTLZA() {
  doTXXXX(acGetLH, getE, zeroMaskR, alwaysT, acPutLH);
  return iNormal;
}

InstructionResult KM10::doTRZN() {
  doTXXXX(acGetRH, getE, zeroMaskR, isNE0T, acPutRH);
  return iNormal;
}

InstructionResult KM10::doTLZN() {
  doTXXXX(acGetLH, getE, zeroMaskR, isNE0T, acPutLH);
  return iNormal;
}

InstructionResult KM10::doTRC() {
  doTXXXX(acGetRH, getE, compMaskR, neverT, acPutRH);
  return iNormal;
}

InstructionResult KM10::doTLC() {
  doTXXXX(acGetLH, getE, compMaskR, neverT, acPutLH);
  return iNormal;
}

InstructionResult KM10::doTRCE() {
  doTXXXX(acGetRH, getE, compMaskR, isEQ0T, acPutRH);
  return iNormal;
}

InstructionResult KM10::doTLCE() {
  doTXXXX(acGetLH, getE, compMaskR, isEQ0T, acPutLH);
  return iNormal;
}

InstructionResult KM10::doTRCA() {
  doTXXXX(acGetRH, getE, compMaskR, alwaysT, acPutRH);
  return iNormal;
}

InstructionResult KM10::doTLCA() {
  doTXXXX(acGetLH, getE, compMaskR, alwaysT, acPutLH);
  return iNormal;
}

InstructionResult KM10::doTRCN() {
  doTXXXX(acGetRH, getE, compMaskR, isNE0T, acPutRH);
  return iNormal;
}

InstructionResult KM10::doTLCN() {
  doTXXXX(acGetLH, getE, compMaskR, isNE0T, acPutLH);
  return iNormal;
}

InstructionResult KM10::doTRO() {
  doTXXXX(acGetRH, getE, onesMaskR, neverT, acPutRH);
  return iNormal;
}

InstructionResult KM10::doTLO() {
  doTXXXX(acGetLH, getE, onesMaskR, neverT, acPutLH);
  return iNormal;
}

InstructionResult KM10::doTROE() {
  doTXXXX(acGetRH, getE, onesMaskR, isEQ0T, acPutRH);
  return iNormal;
}

InstructionResult KM10::doTLOE() {
  doTXXXX(acGetLH, getE, onesMaskR, isEQ0T, acPutLH);
  return iNormal;
}

InstructionResult KM10::doTROA() {
  doTXXXX(acGetRH, getE, onesMaskR, alwaysT, acPutRH);
  return iNormal;
}

InstructionResult KM10::doTLOA() {
  doTXXXX(acGetLH, getE, onesMaskR, alwaysT, acPutLH);
  return iNormal;
}

InstructionResult KM10::doTRON() {
  doTXXXX(acGetRH, getE, onesMaskR, isNE0T, acPutRH);
  return iNormal;
}

InstructionResult KM10::doTLON() {
  doTXXXX(acGetLH, getE, onesMaskR, isNE0T, acPutLH);
  return iNormal;
}

InstructionResult KM10::doTDN() {
  return iNormal;
}

InstructionResult KM10::doTSN() {
  return iNormal;
}

InstructionResult KM10::doTDNE() {
  doTXXXX(acGet, memGet, noMod2, isEQ0T, noStore);
  return iNormal;
}

InstructionResult KM10::doTSNE() {
  doTXXXX(acGet, memGetSwapped, noMod2, isEQ0T, noStore);
  return iNormal;
}

InstructionResult KM10::doTDNA() {
  ++nextPC.rhu;
  return iNormal;
}

InstructionResult KM10::doTSNA() {
  ++nextPC.rhu;
  return iNormal;
}

InstructionResult KM10::doTDNN() {
  doTXXXX(acGet, memGet, noMod2, isNE0T, noStore);
  return iNormal;
}

InstructionResult KM10::doTSNN() {
  doTXXXX(acGet, memGetSwapped, noMod2, isNE0T, noStore);
  return iNormal;
}

InstructionResult KM10::doTDZ() {
  doTXXXX(acGet, memGet, zeroMask, neverT, acPut);
  return iNormal;
}

InstructionResult KM10::doTSZ() {
  doTXXXX(acGet, memGetSwapped, zeroMask, neverT, acPut);
  return iNormal;
}

InstructionResult KM10::doTDZE() {
  doTXXXX(acGet, memGet, zeroMask, isEQ0T, acPut);
  return iNormal;
}

InstructionResult KM10::doTSZE() {
  doTXXXX(acGet, memGetSwapped, zeroMask, isEQ0T, acPut);
  return iNormal;
}

InstructionResult KM10::doTDZA() {
  doTXXXX(acGet, memGet, zeroMask, alwaysT, acPut);
  return iNormal;
}

InstructionResult KM10::doTSZA() {
  doTXXXX(acGet, memGetSwapped, zeroMask, alwaysT, acPut);
  return iNormal;
}

InstructionResult KM10::doTDZN() {
  doTXXXX(acGet, memGet, zeroMask, isNE0T, acPut);
  return iNormal;
}

InstructionResult KM10::doTSZN() {
  doTXXXX(acGet, memGetSwapped, zeroMask, isNE0T, acPut);
  return iNormal;
}

InstructionResult KM10::doTDC() {
  doTXXXX(acGet, memGet, compMask, neverT, acPut);
  return iNormal;
}

InstructionResult KM10::doTSC() {
  doTXXXX(acGet, memGetSwapped, compMask, neverT, acPut);
  return iNormal;
}

InstructionResult KM10::doTDCE() {
  doTXXXX(acGet, memGet, compMask, isEQ0T, acPut);
  return iNormal;
}

InstructionResult KM10::doTSCE() {
  doTXXXX(acGet, memGetSwapped, compMask, isEQ0T, acPut);
  return iNormal;
}

InstructionResult KM10::doTDCA() {
  doTXXXX(acGet, memGet, compMask, alwaysT, acPut);
  return iNormal;
}

InstructionResult KM10::doTSCA() {
  doTXXXX(acGet, memGetSwapped, compMask, alwaysT, acPut);
  return iNormal;
}

InstructionResult KM10::doTDCN() {
  doTXXXX(acGet, memGet, compMask, isNE0T, acPut);
  return iNormal;
}

InstructionResult KM10::doTSZCN() {
  doTXXXX(acGet, memGetSwapped, compMask, isNE0T, acPut);
  return iNormal;
}

InstructionResult KM10::doTDO() {
  doTXXXX(acGet, memGet, onesMask, neverT, acPut);
  return iNormal;
}

InstructionResult KM10::doTSO() {
  doTXXXX(acGet, memGetSwapped, onesMask, neverT, acPut);
  return iNormal;
}

InstructionResult KM10::doTDOE() {
  doTXXXX(acGet, memGet, onesMask, isEQ0T, acPut);
  return iNormal;
}

InstructionResult KM10::doTSOE() {
  doTXXXX(acGet, memGetSwapped, onesMask, isEQ0T, acPut);
  return iNormal;
}

InstructionResult KM10::doTDOA() {
  doTXXXX(acGet, memGet, onesMask, alwaysT, acPut);
  return iNormal;
}

InstructionResult KM10::doTSOA() {
  doTXXXX(acGet, memGetSwapped, onesMask, alwaysT, acPut);
  return iNormal;
}

InstructionResult KM10::doTDON() {
  doTXXXX(acGet, memGet, onesMask, isNE0T, acPut);
  return iNormal;
}

InstructionResult KM10::doTSON() {
  doTXXXX(acGet, memGetSwapped, onesMask, isNE0T, acPut);
  return iNormal;
}


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

auto KM10::getWord(ifstream &inS, [[maybe_unused]] const char *whyP) -> uint16_t {
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
    if (executeBPs.contains(pc.vma)) running = false;

    // Prepare to fetch next iw and remember if it's an interrupt or
    // trap.
    if ((flags.tr1 || flags.tr2) && pag.pagerEnabled()) {
      // We have a trap.
      exceptionPC = pc;
      pc = eptAddressFor(flags.tr1 ?
				     &eptP->trap1Insn :
				     &eptP->stackOverflowInsn);
      inInterrupt = true;
      cerr << ">>>>> trap cycle PC now=" << pc.fmtVMA()
	   << "  exceptionPC=" << exceptionPC.fmtVMA()
	   << logger.endl << flush;
    } else if (W36 vector = pi.setUpInterruptCycleIfPending(); vector != W36(0)) {
      // We have an active interrupt.
      exceptionPC = pc;
      pc = vector;
      inInterrupt = true;
      cerr << ">>>>> interrupt cycle PC now=" << pc.fmtVMA()
	   << "  exceptionPC=" << exceptionPC.fmtVMA()
	   << logger.endl << flush;
    }

    // Now fetch the instruction at our normal, exception, or interrupt PC.
    iw = memGetN(pc);

    // Capture next PC AFTER we possibly set up to handle an exception or interrupt.
    if (!inXCT) {
      nextPC.rhu = pc.rhu + 1;
      nextPC.lhu = pc.lhu;
    }

    // If we're debugging, this is where we pause to let the user
    // inspect and change things. The debugger tells us what our next
    // action should be based on its return value.
    if (!running) {

      switch (debuggerP->debug()) {
      case Debugger::step:	// Debugger has set step count in nSteps.
	break;

      case Debugger::run:	// Continue from current PC (nSteps set by debugger to 0).
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
    // nSteps is zero we just keep going "forever".
    if (nSteps > 0) {
      if (--nSteps <= 0) running = false;
    }

    if (logger.loggingToFile && logger.pc) {
      logger.s << pc.fmtVMA() << ": " << debuggerP->dump(iw, pc);
    }

    // Unless we encounter ANOTHER XCT we're not in one now.
    inXCT = false;

    // Compute effective address
    ea.u = getEA(iw.i, iw.x, iw.y);

    // Execute the instruction in `iw`.
    InstructionResult result = (this->*(ops[iw.op]))();

    // XXX update PC, etc.
    switch (result) {
    case iNormal:
    case iSkip:
    case iJump:
    case iMUUO:
    case iLUUO:
    case iTrap:
    case iHALT:
    case iXCT:
    case iNoSuchDevice:
    case iNYI:
      break;
    }

    // If we're in a xUUO trap, only the first instruction we execute
    // there is special, so clear inInterrupt.
    if (inInterrupt) {
      cerr << "[IN INTERRUPT and about to clear that state]" << logger.endl << flush;
      inInterrupt = false;
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
  assert(sizeof(ExecutiveProcessTable) == 512 * 8);
  assert(sizeof(UserProcessTable) == 512 * 8);
  gflags::ParseCommandLineFlags(&argc, &argv, true);

  if (FLAGS_load != "none") {

    if (FLAGS_load.ends_with(".a10")) {
      loadA10(FLAGS_load.c_str());
    } else if (FLAGS_load.ends_with(".sav")) {
      cerr << "ERROR: For now, '-load' option must name a .a10 file" << logger.endl;
      return -1;
      //      loadSAV(FLAGS_load.c_str());
    } else {
      cerr << "ERROR: '-load' option must name a .a10 or .sav file" << logger.endl;
      return -1;
    }

    cerr << "[Loaded " << FLAGS_load << "  start=" << pc.fmtVMA() << "]" << logger.endl;
  }

  KM10 km10(4 * 1024 * 1024, aBPs, eBPs);
  Debugger debugger(km10);

  if (FLAGS_rel != "none") {
    debugger.loadREL(FLAGS_rel.c_str());
  }

  running = !FLAGS_debug;
  km10.emulate(&debugger);

  return restart ? 1 : 0;
}


////////////////////////////////////////////////////////////////
int main(int argc, char *argv[]) {
  int st;
  
  while ((st = loopedMain(argc, argv)) > 0) {
    cerr << endl << "[restarting]" << endl;
  }

  return st;
}
