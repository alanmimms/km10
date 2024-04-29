'use strict';
const assert = require('assert');


// KM10 instruction phases:
//
// * (`ea` is always calculated by Emulate() ahead of instruction phases.)
// * (`eaw` is always fetched by Emulate() ahead of instruction phases.)
// * `getSrc`: Retrieve the instruction's source data into W36 `src`.
// * `getSrc2`: Retrieve the instruction's second source data into W36 `src2`.
// * `setFlags`: Set any flags.
// * `operate`: Perform the instructions' operation.
// * `putResult`: Put the (possibly normalized) result in its destination.
// * `setPC`: Set skip or PC.

// Emit code to get the current accumulator content into W36 src.
function getAC(toName = 'src') {
  return `\
  W36 ${toName} = loadAC(ac);
`;
}


function getACLH(toName = 'src') {
  return `\
  W36 ${toName} = LH(loadAC(ac));
`;
}


function getACRH(toName = 'src') {
  return `\
  W36 ${toName} = RH(loadAC(ac));
`;
}


function getE(toName = 'src') {
  return `\
  W36 ${toName} = eaw;
`;
}


function getELH(toName = 'src') {
  return `\
  W36 ${toName} = LH(eaw);
`;
}


function getERH(toName = 'src') {
  return `\
  W36 ${toName} = RH(eaw);
`;
}


function putAC(fromName = 'result') {
  return `\
  checkPutAC(ac, ${fromName});
`;
}


function putACLH(fromName = 'result') {
  return `\
  checkPutAC(ac, CONS(${fromName}), checkGetAC());
`;
}


function putACLHZ(fromName = 'result') {
  return `\
  checkPutAC(ac, CONS(${fromName}), 0);
`;
}


function putACLHE(fromName = 'result') {
  return `\
  checkPutAC(ac, CONS(${fromName}),
                              (${fromName} & BIT(0)) ? ALL1s : 0);
`;
}


function putACLHO(fromName = 'result') {
  return `\
  checkPutAC(ac, CONS(${fromName}), ALL1s);
`;
}


function putACRH(fromName = 'result') {
  return `\
  checkPutAC(ac, CONS(checkGetAC(), ${fromName}));
`;
}


function putERHandACnon0(fromName = 'result') {
  return `\
  storeMem(ea, CONS(eaw, ${fromName})));
  if (ac != 0) storeAC(ac, CONS(eaw, (${fromName})));
`;
}


function putERHZandACnon0(fromName = 'result') {
  return `\
  storeMem(ea, RH(${fromName}));
  if (ac != 0) storeAC(ac, RH(${fromName}));
`;
}


function putAC_AC1(fromName = 'result') {
  return `\
  checkPutAC(ac, DLH(${fromName}));
  checkPutAC(ac+1, DRH(${fromName}));
`;
}


function getI(toName = 'src') {
  return `\
  W36 ${toName} = RH(ea);
`;
}


function getXI(toName = 'src') {
  return `\
  W36 ${toName} = ea;
`;
}


function getILH(toName = 'src') {
  return `\
  W36 ${toName} = LH(ea);
`;
}


function getXILH(toName = 'src') {
  return `\
  W36 ${toName} = LH(ea);
`;
}


function putE(fromName = 'result') {
  return `\
  storeMem(ea, ${fromName});
`;
}


function putELHZ(fromName = 'result') {
  return `\
  storeMem(ea, CONS(${fromName}, 0));
`;
}


function putELHO(fromName = 'result') {
  return `\
  storeMem(ea, CONS(${fromName}, ALL1s));
`;
}


function putELHE(fromName = 'result') {
  return `\
  storeMem(ea, CONS(${fromName},
                            (${fromName} & BIT(0)) ? ALL1s : 0));
`;
}


function putELHZandACnon0(fromName = 'result') {
  return `\
  storeMem(ea, CONS(${fromName}, 0));
  if (ac != 0) storeAC(ac, CONS(${fromName}, 0));
`;
}


function putELHOandACnon0(fromName = 'result') {
  return `\
  storeMem(ea, CONS(${fromName}, ALL1s));
  if (ac != 0) storeAC(ac, CONS(${fromName}, ALL1s));
`;
}


function putELHEandACnon0(fromName = 'result') {
  return `\
  storeMem(ea, CONS(${fromName},
                            (${fromName} & BIT(0)) ? ALL1s : 0));
  if (ac != 0) storeAC(ac, CONS(${fromName},
                                (${fromName} & BIT(0)) ? ALL1s : 0));
`;
}


function putE_E1(fromName = 'result') {
  return `\
  storeMem(ea, DLH(${fromName}));
  storeMem(ea+1, DRH(${fromName}));
`;
}


function putELH(fromName = 'result') {
  return `\
  storeMem(ea, CONS(${fromName}, eaw));
`;
}


function putERH(fromName = 'result') {
  return `\
  storeMem(ea, CONS(eaw, ${fromName}));
`;
}


function putEandAC(fromName = 'result') {
  return `\
  storeMem(ea, ${fromName});
  storeAC(ac, ${fromName});
`;
}


function putEandACnon0(fromName = 'result') {
  return `\
  storeMem(ea, ${fromName});
  if (ac != 0) storeAC(ac, ${fromName});
`;
}


function putELHandACnon0(fromName = 'result') {
  return `\
  storeMem(ea, CONS(${fromName}, eaw);
  if (ac != 0) storeAC(ac, ${fromName});
`;
}


function negate() {
  return `\
  W36 result = -TOSIGNED(src);
`;
}


function negateFlags() {
  return `\
  if (src == BIT(0)) {
    flags |= flagTR1 | flagOV | flagCY1;
  } else if (src == 0) {
    flags |= flagCY0 | flagCY1;
  }
`;
}


function doAdd() {
  return `\
  W36 result = TOSIGNED(src) + TOSIGNED(src2);
`;
}


function addFlags() {
  return `\
  if (result >= BIT(0)) {
    flags |= flagTR1 | flagOV | flagCY1;
  } else if (result < -BIT(0)) {
    flags |= flagTR1 | flagOV | flagCY1;
  } else if (src < 0 && src2 << 0 ||
      (src & BIT(0)) != (src2 & BIT(0)) && (MAGNITUDE(src) == MAGNITUDE(dst)) ||
      src >= 0 && src2 < 0 && MAGNITUDE(src) > MAGNITUDE(src2) ||
      src2 >= 0 && src < 0 && MAGNITUDE(src2) > MAGNITUDE(src))

  {
    flags |= flagCY0 | flagCY1;
  }
`;
}


function getAC_AC1(toName = 'src') {
  return `\
  W72 ${toName} = DCONS(loadAC(ac), loadAC(ac+1));
`;
}


function getE_E1(toName = 'src') {
  return `\
  W72 ${toName} = DCONS(eaw, loadMem(ea+1));
`;
}


function negateD() {
  return `\
  W72 result = -DTOSIGNED(src);
`;
}


function negateDFlags() {
  return `\
  // Note this IGNORES low word sign bit.
  if ((src & ~(W72) BIT(0)) == DBIT(0)) {
    flags |= flagTR1 | flagOV | flagCY1;
  } else if (src == 0) {
    flags |= flagCY0 | flagCY1;
  }
`;
}


function doXMOVEI() {
  return `\
  // Do XMOVEI here
`;
}


function setupBLT() {
  return `\
  // Setup BLT here
`;
}


function doBLT() {
  return `\
  // Do BLT here
`;
}


function finishBLT() {
  return `\
  // Finish BLT here
`;
}


function setupXBLT() {
  return `\
  // Setup XBLT here
`;
}


function doXBLT() {
  return `\
  // Do XBLT here
`;
}


function finishXBLT() {
  return `\
  // Finish XBLT here
`;
}


// Return a that calls, e.g., f('src2'). This allows the getter
// functions to be used for src or src2.
function makeGetter(f, src = 'src2') {
  return () => f(src);
}


const km10Ops = {
  // Full Word Movement
  0o250: {
    mne: "EXCH",
  },
  0o200: {
    mne: "MOVE",
    getSrc: getE,
    putResult: putAC,
  },
  0o201: {
    mne: "MOVEI",
    getSrc: getI,
    putResult: putAC,
  },
  0o202: {
    mne: "MOVEM",
    getSrc: getAC,
    putResult: putEandAC,
  },
  0o203: {
    mne: "MOVES",
    getSrc: getE,
    putResult: putEandACnon0,
  },
  0o210: {
    mne: "MOVN",
    getSrc: getE,
    setFlags: negateFlags,
    operate: negate,
    putResult: putAC,
  },
  0o211: {
    mne: "MOVNI",
    getSrc: getI,
    setFlags: negateFlags,
    operate: negate,
    putResult: putAC,
  },
  0o212: {
    mne: "MOVNM",
    getSrc: getAC,
    setFlags: negateFlags,
    operate: negate,
    putResult: putEandAC,
  },
  0o213: {
    mne: "MOVNS",
    getSrc: getE,
    setFlags: negateFlags,
    operate: negate,
    putResult: putEandACnon0,
  },
  0o415: {
    mne: "XMOVEI",
    getSrc: getI,
    operate: doXMOVEI,
    putResult: putAC,
  },
  0o120: {
    mne: "DMOVE",
    getSrc: getE_E1,
    putResult: putAC_AC1,
  },
  0o121: {
    mne: "DMOVN",
    getSrc: getE_E1,
    setFlags: negateFlags,
    operate: negateD,
    putResult: putAC_AC1,
  },
  0o124: {
    mne: "DMOVEM",
    getSrc: getAC_AC1,
    putResult: putE_E1,
  },
  0o125: {
    mne: "DMOVNM",
    getSrc: getAC_AC1,
    setFlags: negateDFlags,
    operate: negateD,
    putResult: putE_E1,
  },
  0o251: {
    mne: "BLT",
    getSrc: setupBLT,
    operate: doBLT,
    putResult: finishBLT,
  },
  0o123020: {
    mne: "XBLT",
    getSrc: setupXBLT,
    operate: doXBLT,
    putResult: finishXBLT,
  },
  0o204: {
    mne: "MOVS",
  },
  0o205: {
    mne: "MOVSI",
  },
  0o206: {
    mne: "MOVSM",
  },
  0o207: {
    mne: "MOVSS",
  },
  0o214: {
    mne: "MOVM",
  },
  0o215: {
    mne: "MOVMI",
  },
  0o216: {
    mne: "MOVMM",
  },
  0o217: {
    mne: "MOVMS",
  },
  0o052: {
    mne: "PMOVE",
  },
  0o053: {
    mne: "PMOVEM",
  },

  // Fixed Point Arithmetic
  0o270: {
    mne: "ADD",
    getSrc: getAC,
    getSrc2: makeGetter(getE),
    setFlags: addFlags,
    operate: doAdd,
    putResult: putAC,
  },
  0o271: {
    mne: "ADDI",
  },
  0o272: {
    mne: "ADDM",
  },
  0o273: {
    mne: "ADDB",
  },
  0o274: {
    mne: "SUB",
  },
  0o275: {
    mne: "SUBI",
  },
  0o276: {
    mne: "SUBM",
  },
  0o277: {
    mne: "SUBB",
  },
  0o220: {
    mne: "IMUL",
  },
  0o221: {
    mne: "IMULI",
  },
  0o222: {
    mne: "IMULM",
  },
  0o223: {
    mne: "IMULB",
  },
  0o224: {
    mne: "MUL",
  },
  0o225: {
    mne: "MULI",
  },
  0o226: {
    mne: "MULM",
  },
  0o227: {
    mne: "MULB",
  },
  0o230: {
    mne: "IDIV",
  },
  0o231: {
    mne: "IDIVI",
  },
  0o232: {
    mne: "IDIVM",
  },
  0o233: {
    mne: "IDIVB",
  },
  0o234: {
    mne: "DIV",
  },
  0o235: {
    mne: "DIVI",
  },
  0o236: {
    mne: "DIVM",
  },
  0o237: {
    mne: "DIVB",
  },
  0o114: {
    mne: "DADD",
  },
  0o115: {
    mne: "DSUB",
  },
  0o116: {
    mne: "DMUL",
  },
  0o117: {
    mne: "DDIV",
  },

  // Floating Point Arithmetic
  0o140: {
    mne: "FAD",
  },
  0o141: {
    mne: "FADL",
  },
  0o142: {
    mne: "FADM",
  },
  0o143: {
    mne: "FADB",
  },
  0o144: {
    mne: "FADR",
  },
  0o145: {
    mne: "FADRI",
  },
  0o146: {
    mne: "FADRM",
  },
  0o147: {
    mne: "FADRB",
  },
  0o150: {
    mne: "FSB",
  },
  0o151: {
    mne: "FSBL",
  },
  0o152: {
    mne: "FSBM",
  },
  0o153: {
    mne: "FSBB",
  },
  0o154: {
    mne: "FSBR",
  },
  0o155: {
    mne: "FSBRI",
  },
  0o156: {
    mne: "FSBRM",
  },
  0o157: {
    mne: "FSBRB",
  },
  0o160: {
    mne: "FMP",
  },
  0o161: {
    mne: "FMPL",
  },
  0o162: {
    mne: "FMPM",
  },
  0o163: {
    mne: "FMPB",
  },
  0o164: {
    mne: "FMPR",
  },
  0o165: {
    mne: "FMPRI",
  },
  0o166: {
    mne: "FMPRM",
  },
  0o167: {
    mne: "FMPRB",
  },
  0o170: {
    mne: "FDV",
  },
  0o171: {
    mne: "FDVL",
  },
  0o172: {
    mne: "FDVM",
  },
  0o173: {
    mne: "FDVB",
  },
  0o174: {
    mne: "FDVR",
  },
  0o175: {
    mne: "FDVRI",
  },
  0o176: {
    mne: "FDVRM",
  },
  0o177: {
    mne: "FDVRB",
  },
  0o110: {
    mne: "DFAD",
  },
  0o111: {
    mne: "DFSB",
  },
  0o112: {
    mne: "DFMP",
  },
  0o113: {
    mne: "DFDV",
  },
  0o132: {
    mne: "FSC",
  },
  0o031: {
    mne: "GFSC",
  },
  0o127: {
    mne: "FLTR",
  },
  0o030: {
    mne: "GFLTR",
  },
  0o027: {
    mne: "DGFLTR",
  },
  0o122: {
    mne: "FIX",
  },
  0o126: {
    mne: "FIXR",
  },
  0o024: {
    mne: "GFIX",
  },
  0o026: {
    mne: "GFIXR",
  },
  0o023: {
    mne: "GDFIX",
  },
  0o025: {
    mne: "GDFIXR",
  },
  0o021: {
    mne: "GSNGL",
  },
  0o022: {
    mne: "GDBLE",
  },
  0o130: {
    mne: "UFA",
  },
  0o131: {
    mne: "DFN",
  },

  // Boolean
  0o400: {
    mne: "SETZ",
  },
  0o401: {
    mne: "SETZI",
  },
  0o402: {
    mne: "SETZM",
  },
  0o403: {
    mne: "SETZB",
  },
  0o474: {
    mne: "SETO",
  },
  0o475: {
    mne: "SETOI",
  },
  0o476: {
    mne: "SETOM",
  },
  0o477: {
    mne: "SETOB",
  },
  0o424: {
    mne: "SETA",
  },
  0o425: {
    mne: "SETAI",
  },
  0o426: {
    mne: "SETAM",
  },
  0o427: {
    mne: "SETAB",
  },
  0o450: {
    mne: "SETCA",
  },
  0o451: {
    mne: "SETCAI",
  },
  0o452: {
    mne: "SETCAM",
  },
  0o453: {
    mne: "SETCAB",
  },
  0o414: {
    mne: "SETM",
  },
  0o416: {
    mne: "SETMM",
  },
  0o417: {
    mne: "SETMB",
  },
  0o460: {
    mne: "SETCM",
  },
  0o461: {
    mne: "SETCMI",
  },
  0o462: {
    mne: "SETCMM",
  },
  0o463: {
    mne: "SETCMB",
  },
  0o404: {
    mne: "AND",
  },
  0o405: {
    mne: "ANDI",
  },
  0o406: {
    mne: "ANDM",
  },
  0o407: {
    mne: "ANDB",
  },
  0o410: {
    mne: "ANDCA",
  },
  0o411: {
    mne: "ANDCAI",
  },
  0o412: {
    mne: "ANDCAM",
  },
  0o413: {
    mne: "ANDCAB",
  },
  0o420: {
    mne: "ANDCM",
  },
  0o421: {
    mne: "ANDCMI",
  },
  0o422: {
    mne: "ANDCMM",
  },
  0o423: {
    mne: "ANDCMB",
  },
  0o440: {
    mne: "ANDCB",
  },
  0o441: {
    mne: "ANDCBI",
  },
  0o442: {
    mne: "ANDCBM",
  },
  0o443: {
    mne: "ANDCBB",
  },
  0o434: {
    mne: "IOR",
  },
  0o435: {
    mne: "IORI",
  },
  0o436: {
    mne: "IORM",
  },
  0o437: {
    mne: "IORB",
  },
  0o454: {
    mne: "ORCA",
  },
  0o455: {
    mne: "ORCAI",
  },
  0o456: {
    mne: "ORCAM",
  },
  0o457: {
    mne: "ORCAB",
  },
  0o464: {
    mne: "ORCM",
  },
  0o465: {
    mne: "ORCMI",
  },
  0o466: {
    mne: "ORCMM",
  },
  0o467: {
    mne: "ORCMB",
  },
  0o470: {
    mne: "ORCB",
  },
  0o471: {
    mne: "ORCBI",
  },
  0o472: {
    mne: "ORCBM",
  },
  0o473: {
    mne: "ORCBB",
  },
  0o430: {
    mne: "XOR",
  },
  0o431: {
    mne: "XORI",
  },
  0o432: {
    mne: "XORM",
  },
  0o433: {
    mne: "XORB",
  },
  0o444: {
    mne: "EQV",
  },
  0o445: {
    mne: "EQVI",
  },
  0o446: {
    mne: "EQVM",
  },
  0o447: {
    mne: "EQVB",
  },

  // Shift and Rotate
  0o240: {
    mne: "ASH",
  },
  0o241: {
    mne: "ROT",
  },
  0o242: {
    mne: "LSH",
  },
  0o244: {
    mne: "ASHC",
  },
  0o245: {
    mne: "ROTC",
  },
  0o246: {
    mne: "LSHC",
  },

  // Arithmetic Testing
  0o252: {
    mne: "AOBJP",
  },
  0o253: {
    mne: "AOBJN",
  },
  0o300: {
    mne: "CAI",
  },
  0o301: {
    mne: "CAIL",
  },
  0o302: {
    mne: "CAIE",
  },
  0o303: {
    mne: "CAILE",
  },
  0o304: {
    mne: "CAIA",
  },
  0o305: {
    mne: "CAIGE",
  },
  0o306: {
    mne: "CAIN",
  },
  0o307: {
    mne: "CAIG",
  },
  0o310: {
    mne: "CAM",
  },
  0o311: {
    mne: "CAML",
  },
  0o312: {
    mne: "CAME",
  },
  0o313: {
    mne: "CAMLE",
  },
  0o314: {
    mne: "CAMA",
  },
  0o315: {
    mne: "CAMGE",
  },
  0o316: {
    mne: "CAMN",
  },
  0o317: {
    mne: "CAMG",
  },
  0o320: {
    mne: "JUMP",
  },
  0o321: {
    mne: "JUMPL",
  },
  0o322: {
    mne: "JUMPE",
  },
  0o323: {
    mne: "JUMPLE",
  },
  0o324: {
    mne: "JUMPA",
  },
  0o325: {
    mne: "JUMPGE",
  },
  0o326: {
    mne: "JUMPN",
  },
  0o327: {
    mne: "JUMPG",
  },
  0o330: {
    mne: "SKIP",
  },
  0o331: {
    mne: "SKIPL",
  },
  0o332: {
    mne: "SKIPE",
  },
  0o333: {
    mne: "SKIPLE",
  },
  0o334: {
    mne: "SKIPA",
  },
  0o335: {
    mne: "SKIPGE",
  },
  0o336: {
    mne: "SKIPN",
  },
  0o337: {
    mne: "SKIPG",
  },
  0o340: {
    mne: "AOJ",
  },
  0o341: {
    mne: "AOJL",
  },
  0o342: {
    mne: "AOJE",
  },
  0o343: {
    mne: "AOJLE",
  },
  0o344: {
    mne: "AOJA",
  },
  0o345: {
    mne: "AOJGE",
  },
  0o346: {
    mne: "AOJN",
  },
  0o347: {
    mne: "AOJG",
  },
  0o360: {
    mne: "SOJ",
  },
  0o361: {
    mne: "SOJL",
  },
  0o362: {
    mne: "SOJE",
  },
  0o363: {
    mne: "SOJLE",
  },
  0o364: {
    mne: "SOJA",
  },
  0o365: {
    mne: "SOJGE",
  },
  0o366: {
    mne: "SOJN",
  },
  0o367: {
    mne: "SOJG",
  },
  0o350: {
    mne: "AOS",
  },
  0o351: {
    mne: "AOSL",
  },
  0o352: {
    mne: "AOSE",
  },
  0o353: {
    mne: "AOSLE",
  },
  0o354: {
    mne: "AOSA",
  },
  0o355: {
    mne: "AOSGE",
  },
  0o356: {
    mne: "AOSN",
  },
  0o357: {
    mne: "AOSG",
  },
  0o370: {
    mne: "SOS",
  },
  0o371: {
    mne: "SOSL",
  },
  0o372: {
    mne: "SOSE",
  },
  0o373: {
    mne: "SOSLE",
  },
  0o374: {
    mne: "SOSA",
  },
  0o375: {
    mne: "SOSGE",
  },
  0o376: {
    mne: "SOSN",
  },
  0o377: {
    mne: "SOSG",
  },

  // Logical Testing and Modification
  0o600: {
    mne: "TRN",
  },
  0o601: {
    mne: "TLN",
  },
  0o602: {
    mne: "TRNE",
  },
  0o603: {
    mne: "TLNE",
  },
  0o604: {
    mne: "TRNA",
  },
  0o605: {
    mne: "TLNA",
  },
  0o606: {
    mne: "TRNN",
  },
  0o607: {
    mne: "TLNN",
  },
  0o620: {
    mne: "TRZ",
  },
  0o621: {
    mne: "TLZ",
  },
  0o622: {
    mne: "TRZE",
  },
  0o623: {
    mne: "TLZE",
  },
  0o624: {
    mne: "TRZA",
  },
  0o625: {
    mne: "TLZA",
  },
  0o626: {
    mne: "TRZN",
  },
  0o627: {
    mne: "TLZN",
  },
  0o640: {
    mne: "TRC",
  },
  0o641: {
    mne: "TLC",
  },
  0o642: {
    mne: "TRCE",
  },
  0o643: {
    mne: "TLCE",
  },
  0o644: {
    mne: "TRCA",
  },
  0o645: {
    mne: "TLCA",
  },
  0o646: {
    mne: "TRCN",
  },
  0o647: {
    mne: "TLCN",
  },
  0o660: {
    mne: "TRO",
  },
  0o661: {
    mne: "TLO",
  },
  0o662: {
    mne: "TROE",
  },
  0o663: {
    mne: "TLOE",
  },
  0o664: {
    mne: "TROA",
  },
  0o665: {
    mne: "TLOA",
  },
  0o666: {
    mne: "TRON",
  },
  0o667: {
    mne: "TLON",
  },
  0o610: {
    mne: "TDN",
  },
  0o611: {
    mne: "TSN",
  },
  0o612: {
    mne: "TDNE",
  },
  0o613: {
    mne: "TSNE",
  },
  0o614: {
    mne: "TDNA",
  },
  0o615: {
    mne: "TSNA",
  },
  0o616: {
    mne: "TDNN",
  },
  0o617: {
    mne: "TSNN",
  },
  0o630: {
    mne: "TDZ",
  },
  0o631: {
    mne: "TSZ",
  },
  0o632: {
    mne: "TDZE",
  },
  0o633: {
    mne: "TSZE",
  },
  0o634: {
    mne: "TDZA",
  },
  0o635: {
    mne: "TSZA",
  },
  0o636: {
    mne: "TDZN",
  },
  0o637: {
    mne: "TSZN",
  },
  0o650: {
    mne: "TDC",
  },
  0o651: {
    mne: "TSC",
  },
  0o652: {
    mne: "TDCE",
  },
  0o653: {
    mne: "TSCE",
  },
  0o654: {
    mne: "TDCA",
  },
  0o655: {
    mne: "TSCA",
  },
  0o656: {
    mne: "TDCN",
  },
  0o657: {
    mne: "TSCN",
  },
  0o670: {
    mne: "TDO",
  },
  0o671: {
    mne: "TSO",
  },
  0o672: {
    mne: "TDOE",
  },
  0o673: {
    mne: "TSOE",
  },
  0o674: {
    mne: "TDOA",
  },
  0o675: {
    mne: "TSOA",
  },
  0o676: {
    mne: "TDON",
  },
  0o677: {
    mne: "TSON",
  },

  // Half Word Data Transmission
  0o501: {
    mne: "XHLLI",
    getSrc: getXILH,
    putResult: putACLH,
  },
  0o500: {
    mne: "HLL",
    getSrc: getELH,
    putResult: putACLH,
  },
  0o502: {
    mne: "HLLM",
    getSrc: getACLH,
    putResult: putELH,
  },
  0o503: {
    mne: "HLLS",
    getSrc: getELH,
    putResult: putELHandACnon0,
  },
  0o510: {
    mne: "HLLZ",
    getSrc: getELH,
    putResult: putACLHZ,
  },
  0o511: {
    mne: "HLLZI",
    getSrc: getILH,
    putResult: putELHZ,
  },
  0o512: {
    mne: "HLLZM",
    getSrc: getACLH,
    putResult: putELHZ,
  },
  0o513: {
    mne: "HLLZS",
    getSrc: getELH,
    putResult: putELHZandACnon0,
  },
  0o530: {
    mne: "HLLE",
    getSrc: getELH,
    putResult: putACLHE,
  },
  0o531: {
    mne: "HLLEI",
    getSrc: getILH,
    putResult: putACLHZ,
  },
  0o532: {
    mne: "HLLEM",
    getSrc: getACLH,
    putResult: putELHE,
  },
  0o533: {
    mne: "HLLES",
    getSrc: getELH,
    putResult: putELHEandACnon0,
  },
  0o520: {
    mne: "HLLO",
    getSrc: getELH,
    putResult: putACLHO,
  },
  0o521: {
    mne: "HLLOI",
    getSrc: getILH,
    putResult: putACLHO,
  },
  0o522: {
    mne: "HLLOM",
    getSrc: getACLH,
    putResult: putELHO,
  },
  0o523: {
    mne: "HLLOS",
    getSrc: getELH,
    putResult: putELHOandACnon0,
  },
  0o544: {
    mne: "HLR",
    getSrc: getELH,
    putResult: putACRH,
  },
  0o545: {
    mne: "HLRI",
    getSrc: getILH,
    putResult: putACRH,
  },
  0o546: {
    mne: "HLRM",
    getSrc: getACLH,
    putResult: putERH,
  },
  0o547: {
    mne: "HLRS",
    getSrc: getELH,
    putResult: putERHandACnon0,
  },
  0o554: {
    mne: "HLRZ",
  },
  0o555: {
    mne: "HLRZI",
  },
  0o556: {
    mne: "HLRZM",
  },
  0o557: {
    mne: "HLRZS",
  },
  0o564: {
    mne: "HLRO",
  },
  0o565: {
    mne: "HLROI",
  },
  0o566: {
    mne: "HLROM",
  },
  0o567: {
    mne: "HLROS",
  },
  0o574: {
    mne: "HLRE",
  },
  0o575: {
    mne: "HLREI",
  },
  0o576: {
    mne: "HLREM",
  },
  0o577: {
    mne: "HLRES",
  },
  0o540: {
    mne: "HRR",
  },
  0o541: {
    mne: "HRRI",
  },
  0o542: {
    mne: "HRRM",
  },
  0o543: {
    mne: "HRRS",
  },
  0o550: {
    mne: "HRRZ",
  },
  0o551: {
    mne: "HRRZI",
  },
  0o552: {
    mne: "HRRZM",
  },
  0o553: {
    mne: "HRRZS",
  },
  0o560: {
    mne: "HRRO",
  },
  0o561: {
    mne: "HRROI",
  },
  0o562: {
    mne: "HRROM",
  },
  0o563: {
    mne: "HRROS",
  },
  0o570: {
    mne: "HRRE",
  },
  0o571: {
    mne: "HRREI",
  },
  0o572: {
    mne: "HRREM",
  },
  0o573: {
    mne: "HRRES",
  },
  0o504: {
    mne: "HRL",
  },
  0o505: {
    mne: "HRLI",
  },
  0o506: {
    mne: "HRLM",
  },
  0o507: {
    mne: "HRLS",
  },
  0o514: {
    mne: "HRLZ",
  },
  0o515: {
    mne: "HRLZI",
  },
  0o516: {
    mne: "HRLZM",
  },
  0o517: {
    mne: "HRLZS",
  },
  0o524: {
    mne: "HRLO",
  },
  0o525: {
    mne: "HRLOI",
  },
  0o526: {
    mne: "HRLOM",
  },
  0o527: {
    mne: "HRLOS",
  },
  0o534: {
    mne: "HRLE",
  },
  0o535: {
    mne: "HRLEI",
  },
  0o536: {
    mne: "HRLEM",
  },
  0o537: {
    mne: "HRLES",
  },

  // Program Control
  0o256: {
    mne: "XCT",
  },
  0o243: {
    mne: "JFFO",
  },
  0o255: {
    mne: "JFCL",
  },
  0o254: {
    mne: "JRST",
  },
  0o264: {
    mne: "JSR",
  },
  0o265: {
    mne: "JSP",
  },
  0o266: {
    mne: "JSA",
  },
  0o267: {
    mne: "JRA",
  },
  0o257: {
    mne: "MAP",
  },

  // Stack
  0o261: {
    mne: "PUSH",
  },
  0o262: {
    mne: "POP",
  },
  0o260: {
    mne: "PUSHJ",
  },
  0o263: {
    mne: "POPJ",
  },
  0o105: {
    mne: "ADJSP",
  },

  // Byte Manipulation
  0o133: {
    mne: "IBP",
  },
  0o135: {
    mne: "LDB",
  },
  0o137: {
    mne: "DPB",
  },
  0o134: {
    mne: "ILDB",
  },
  0o136: {
    mne: "IDPB",
  },

  // UUO
  0o001: {
    mne: "LUUO",
  },
  0o104: {
    mne: "JSYS",
  },

  // EXTEND
  0o123: {
    mne: "EXTEND",
  },

  // I/O instructions
  0o70000: {
    mne: "APRID",
  },

  0o070004: {
    mne: "DATAI APR,",
  },

  0o070014: {
    mne: "DATAO APR,",
  },

  0o070044: {
    mne: "DATAI PI,",
  },

  0o070054: {
    mne: "DATAO PI,",
  },

  0o070060: {
    mne: "CONO PI,",
    code: `\
    if (ea & CLEAR_PI) {
      piEnabled = 0;
      intLevelsInProgress = 0;
      intLevelsPending = 0;
      intLevel = 0;
    } else if (ea & TURN_ON_PI) {
      piEnabled = 1;
    } else if (ea & TURN_OFF_PI) {
      piEnabled = 0;
    } else if (ea & DROP_PROGRAM_REQUESTS) {
      intLevelsRequested &= ~(ea & PI_LEVEL_MASK);
    } else if (ea & INITIATE_INTERRUPTS) {
      intLevelsRequested |= ea & PI_LEVEL_MASK;
    } else if (ea & TURN_OFF_SELECTED_LEVELS) {
      intLevelsEnabled &= ~(ea & PI_LEVEL_MASK);
    } else if (ea & TURN_ON_SELECTED_LEVELS) {
      intLevelsEnabled |= ea & PI_LEVEL_MASK;
    } 
`,
  },

  0o070064: {
    mne: "CONI PI,",
    code: `\
    tmp =
      intLevelsRequested << ShiftForBit(17) |
      intLevelsInProgress << ShiftForBit(27) |
      piEnabled << ShiftForBit(28) |
      intLevelsEnabled << ShiftForBit(35);
    ${putAC('tmp')};
`,
  },

  0o070104: {
    mne: "DATAI PAG,",
  },

  0o070114: {
    mne: "DATAO PAG,",
  },

  0o070120: {
    mne: "CONO PAG,",
  },

  0o070124: {
    mne: "CONI PAG,",
  },

  0o070020: {
    mne: "CONO APR,",
  },

  0o070024: {
    mne: "CONI APR,",
  },

  0o070220: {
    mne: "CONO TIM,",
  },

  0o070224: {
    mne: "CONI TIM,",
  },

  0o070264: {
    mne: "CONI MTR,",
  },

  0o070144: {
    mne: "SWPIA",
  },

  0o070164: {
    mne: "SWPIO",
  },

  0o070150: {
    mne: "SWPVA",
  },

  0o070170: {
    mne: "SWPVO",
  },

  0o070154: {
    mne: "SWPUA",
  },

  0o070174: {
    mne: "SWPUO",
  },

  0o070010: {
    mne: "WRFIL",
  },

  0o070110: {
    mne: "CLRPT",
  },

  0o070260: {
    mne: "WRTIME",
  },

  0o070204: {
    mne: "RDTIME",
  },

  0o070244: {
    mne: "RDEACT",
  },

  0o070240: {
    mne: "RDMACT",
  },

  0o070210: {
    mne: "WRPAE",
  },

  0o070200: {
    mne: "RDPERF",
  },

  0o070040: {
    mne: "RDERA",
  },

  0o070050: {
    mne: "SBDIAG",
  },
};


// This returns the C code for the cases in the switch statement in
// Emulate(). Each includes a `break` or `continue`, where `break`
// means the end-of-instruction `pc = nextPC` should occur, and
// `continue` means it should not.
function emitAllCases() {
  return Object.entries(km10Ops)
    .map(([opcode, insn]) => `\
case ${opcode.toString(8).padStart(7, '0')}: { // ${insn.mne}
${insn.getSrc && insn.getSrc() || ''}\
${insn.getSrc2 && insn.getSrc2() || ''}\
${insn.setFlags && insn.setFlags() || ''}\
${insn.operate && insn.operate() || ''}\
${insn.putResult && insn.putResult() || ''}\
${insn.code || ''}\
  break;
}
`)
    .join('\n');
}


module.exports = {
  km10Ops,
  emitAllCases,
};


if (process.argv[2] == 'TEST') {
  console.log(emitAllCases());
}
