'use strict';

const fs = require('fs');
const util = require('util');
const assert = require('node:assert');

// Names of the flags and their bit numbers in a program flags word:
const flags = {
  OV: 0,
  PCP: 0,
  CY1: 1,
  CY0: 2,
  FOV: 3,
  FPD: 4,
  USR: 5,
  USRIO: 6,
  PCU: 6,
  PUB: 7,
  AFI: 8,
  TR2: 9,
  TR1: 10,
  FUF: 11,
  NDV: 12,
};


const macros = `
${Object.keys(flags)
  .map(f => `#define flag${f} BIT(${flags[f]})`)
  .join('\n')}


static volatile int running;
static volatile int tracePC;
static volatile int traceAC;

static volatile W36 AC[16];
static volatile W36 pc;
static volatile W36 flags;
static volatile W36 *memP;


// XXX For now
#define CHECK_GET_ABREAK(A)	((A) < 020 ? AC[A] : memP[A])


// XXX For now
#define CHECK_PUT_ABREAK(A,V)			\\
    do {					\\
      if ((A) < 020)				\\
        AC[A] = (V);				\\
      else					\\
        memP[A] = (V);				\\
     } while(0)					\\


static inline W36 checkGetAC(int a) {
  char buf[64];
  if (traceAC) fprintf(stderr, "AC%o=%s\\n", a, oct36(buf, AC[a]));
  return AC[a];
}


static inline void checkPutAC(int a, W36 v) {
  char buf[64];
  if (traceAC) fprintf(stderr, "AC%o=%s\\n", a, oct36(buf, v));
  AC[a] = v;
}


// XXX This needs nonzero section code added.
static inline W36 pcAndFlags() {
  return flags | pc;
}


static inline int nonZeroSection(W36 a) {
  return !!(a & LHMASK);
}


static void NYI(const char *mneP) {
  char buf[64];
  fprintf(stderr, "Not yet implemented: %s at %s\\n", mneP, octVMA(buf, pc));
}

`;


function oct6(v) {
  return v.toString(8).padStart(2, '0');
}


// Optionally `ac` param can be left off and 'ac' will be used. This
// makes call sites simpler.
function getAC(ac) {
  if (ac == undefined) ac = 'ac';
  return `checkGetAC(${ac})`;
}


// Optionally `ac` param can be left off and 'ac' will be used. This
// makes call sites simpler.
function getACRH(ac) {
  if (ac == undefined) ac = 'ac';
  return `checkGetAC(${ac}) & HMASK`;
}


// Optionally `ac` param can be left off and 'ac' will be used. This
// makes call sites simpler.
function getACLH(ac) {
  if (ac == undefined) ac = 'ac';
  return `checkGetAC((${ac}) >> 18) & HMASK`;
}


// Optionally `ac` param can be left off and 'ac' will be used. This
// makes call sites simpler.
function putAC(ac, value) {

  if (value == undefined) {
    value = ac;
    ac = 'ac';
  }

  return `checkPutAC(${ac}, ${value})`;
}


// Optionally `ac` param can be left off and 'ac' will be used. This
// makes call sites simpler.
function putACRH(ac, value) {

  if (value == undefined) {
    value = ac;
    ac = 'ac';
  }

  return `checkPutAC(${ac}, CONS(LH(${ac}), ${value}))`;
}


// Optionally `ac` param can be left off and 'ac' will be used. This
// makes call sites simpler.
function putACLH(ac, value) {

  if (value == undefined) {
    value = ac;
    ac = 'ac';
  }

  return `checkPutAC(${ac}, CONS(${value}, ${ac}))`;
}


// Optionally `a` param can be left off and 'ea' will be used. This
// makes call sites simpler.
function getMem(a) {
  if (a == undefined) a = 'ea';
  return `CHECK_GET_ABREAK(${a})`;
}


// Optionally `a` param can be left off and 'ea' will be used. This
// makes call sites simpler.
function putMem(a, value) {

  if (value == undefined) {
    value = a;
    a = 'ea';
  }

  return `CHECK_PUT_ABREAK(${a}, ${value})`;
}


function setFlags(flagsList) {
  return `flags |= ${flagsList
.split(/\s+/)
.map(flag => `flag${flag}`)
.join('|')}`;
}


// Handle MOVN and MOVM flag setting.
function setSignedMoveFlags() {
  return `\
	if (tmp == BIT0) {
	  ${setFlags(`TR1 OV CY1`)};
	}
	if (tmp == 0) {
	  ${setFlags(`CY0 CY1`)};
	}`;
}


// If cond is undefined never skip. If cond is 1 always skip.
// Else generate if (cond) for skip.
function doSKIP(cond) {
  const prefix = `    if (ac != 0) ${putAC('ea')};`;

  if (cond == undefined) return prefix;
  if (cond === 1 && cond !== undefined)
    return `${prefix}
    ++pc;`;
  else
    return `${prefix}
    if (${getMem()} ${cond}) ++pc;`;
}


// Handle the Test, Modify, and Skip instructions by passing appropriate
// getMask, modifier, and skipper functions for each case.

// Always get the AC.
//
// Get mask LH, RH, @ea, or swapped(@ea)
//
// Do nothing, set masked bits in AC to zeros, complement masked bits in AC,
// or set masked bits in AC to ones.
//
// Don't skip, always skip, or skip if masked bits all zero or not all mask bits are zero.
function testInsn(getMask, modifier, skipper) {
  return `\
  W36 mask = ${getMask()};
  W36 testAC = ${getAC()};
  ${skipper()};
  ${modifier()};`;
}


function getMemSwapped() {
  return `SWAP(memP[ea])`;
}


function getERH() {
  return `RH(ea)`;
}


function getELH() {
  return `RH(ea)`;
}


function getMemRH() {
  return `RH(memP[ea])`;
}


function getMemLH() {
  return `LH(memP[ea])`;
}


function getEARH() {
  return `RH(ea)`;
}


function getEALH() {
  return `LH(ea)`;
}


function skipN() {
  return `if ((testAC & mask) != 0) ++pc`;
}


function skipE() {
  return `if ((testAC & mask) == 0) ++pc`;
}


function skipAlways() {
  return `++pc`;
}


function skipNever() {
  return `(void) 0`;
}


function modZeros() {
  return `${putAC('testAC & ~mask')}`
}


function modOnes(mask) {
  return `${putAC('testAC | mask')}`
}


function modComplement(mask) {
  return `${putAC('testAC ^ mask')}`
}


function modNone(mask) {
  return `(void) 0`;
}


// getSrc() always puts its source right justified in tmp.
// putDest() always puts tmp to one or the other half.
// modifier() does nothing, zero, set to all 1s or sign extend the right half of tmp.
// isSelf is true if result should also go to AC[ac] if ac !== 0.

// Take a half word data transmission mnemonic and return the code to do it.
//
// `mne` must be padded so modifier byte of mnemonic is blank if
// modifier should be Do Nothing and mode byte of mnemonic is blank
// if mode is Basic.
function halfInsn(mne) {
  assert(mne[0] === 'H');
  const [S,D,MOD,MODE] = mne.substr(1, 4);
  let srcCode, dstGet, getHalf, dstPut, modCode;

  switch (MODE) {
  case 'I':
    srcCode = `W36 src = CONS(0, ea)`;
    dstGet = `W36 dst = ${getAC()}`;
    dstPut = putAC('dst');
    break;

  case 'M':
    srcCode = `W36 src = ${getAC()}`;
    dstGet = `W36 dst = ${getMem()}`;
    dstPut = putMem('dst');
    break;

  case 'S':
    srcCode = `W36 src = ${getAC()}`;
    dstGet = `W36 dst = ${getMem()}`;
    dstPut = `${putMem('dst')}; if (ac != 0) ${putAC('dst')}`;
    break;

  default:
    srcCode = `W36 src = ${getAC()}`;
    dstGet = `W36 dst = ${getMem()}`;
    dstPut = putMem('dst');
    break;
  }

  // This seems like it works in zero and in nonzero section.
  if (D == 'R') {		// RH

    if (MOD == 'Z') {
      modCode = `dst = CONS(0, src)`;
    } else if (MOD == 'O') {
      modCode = `dst = CONS(ALL1s, src)`;
    } else if (MOD == 'E') {
      modCode = `dst = SEXTEND(src)`;
    } else {
      modCode = `dst = CONS(LH(dst), src)`;
    }
  } else {			// LH

    if (MOD == 'Z') {
      modCode = `dst = CONS(src, 0)`;
    } else if (MOD == 'O') {
      modCode = `dst = CONS(src, ALL1s)`;
    } else if (MOD == 'E') {
      modCode = `dst = SWAP(SEXTEND(LH(src)))`;
    } else {
      modCode = `dst = CONS(src), RH(dst))`;
    }
  }

  // Now fetch the proper half of the source.
  switch (mne[1]) {
  case 'R':
    getHalf = `src = RH(src);`;
    break;

  case 'L':
    getHalf = `src = LH(src);`;
    break;

  default:
    assert(false, `halfInsn with bad mnemonic '${mne}'.`);
    return `!!!! assertion failed !!!!`;
  }

  return `\
  ${srcCode};
  ${getHalf};
  ${dstGet};
  ${dstPut}`;
}


////////////////////////////////////////////////////////////////////////////////////////////////
const kl10Instructions = {
// Full Word Movement
'250': `// EXCH	(ac) <-> (e)
	W36 tmp = ${getAC()};
	${putAC(getMem())};
	${putMem('tmp')};
`,
'200': `// MOVE (e) -> (ac)
	${putAC(getMem())};
`,
'201': `// MOVEI 0,,e -> (ac)
	${putAC('RH(ea)')};
`,
'202': `// MOVEM (ac) -> (e)
	${putMem(getAC())};
`,
'203': `// MOVES if ac !== 0: (e) -> (ac)
	if (ac != 0) ${putAC(getMem())};
`,
'210': `// MOVN -(e) -> (ac)
	W36 tmp = -${getMem()};
	${putAC('tmp')};
	${setSignedMoveFlags()}
`,
'211': `// MOVNI -[0,,e] -> (ac)
	W36 tmp = -RH(${getMem()});
	${putAC('tmp')};	
`,
'212': `// MOVNM -(e) -> (e)
	W36 tmp = -LH(${getAC()});
	${putMem('tmp')};	
	${setSignedMoveFlags()}
`,
'213': `// MOVNS -(e) -> (e)
	W36 tmp = -${getMem()};
	${putMem('tmp')};
	if (ac != 0) ${putAC('tmp')};
	${setSignedMoveFlags()}
`,
'415': `// SETMI/XMOVEI
	if (nonZeroSection(pc)) {

	  if (eaIsLocal) {
	    W36 tmp = BIT17 | RH(ea);
	    ${putAC('tmp')};
	  } else {
	    W36 tmp = ea & 07777777777ull;
	    ${putAC('tmp')};
	  }
	} else {	// SETMI
	  ${putAC('RH(ea)')};
	}
`,
'254': `// JRST
	switch(ac) {		// AC is the JRST function F
	case 000: // JRST
	  pc = ea;
	  goto SKIP_PC_INCR;

	case 001: // PORTAL
	  NYI("PORTAL");
	  break;

	case 002: // JRSTF
	  NYI("JRSTF");
	  break;

	case 004: // HALT
	  running = 0;
	  break;

	case 005: // XJRSTF
	  NYI("XJRSTF");
	  break;

	case 006: // XJEN
	  NYI("XJEN");
	  break;

	case 007: // XPCW
	  NYI("XPCW");
	  break;

	case 010: // RSTOR
	  NYI("RSTOR");
	  break;

	case 012: // JEN
	  NYI("JEN");
	  break;

	case 014: // SFM
	  NYI("SFM");
	  break;
	}
`,
'264': `// JSR
    memP[ea] = nonZeroSection(pc) ? (pc & 0007777777777ull) : (pc | flags);
    pc = ea + 1;
    flags &= ~(flagFPD | flagAFI | flagTR2 | flagTR1);
    goto SKIP_PC_INCR;
`,
'265': `// JSP
    AC[ac] = nonZeroSection(pc) ? (pc & 0007777777777ull) : (pc | flags);
    pc = ea;
    flags &= ~(flagFPD | flagAFI | flagTR2 | flagTR1);
    goto SKIP_PC_INCR;
`,
'330': `// SKIP if ac !== 0: (e) -> (ac)
    ${doSKIP()}
`,
'331': `// SKIPL
    ${doSKIP('< 0')}
`,
'332': `// SKIPE
    ${doSKIP('== 0')}
`,
'333': `// SKIPLE
    ${doSKIP('<= 0')}
`,
'334': `// SKIPA
    ${doSKIP(1)}
`,
'335': `// SKIPGE
    ${doSKIP('>= 0')}
`,
'336': `// SKIPN
    ${doSKIP('!= 0')}
`,
'337': `// SKIPG
    ${doSKIP('> 0')}
`,
'120': `// DMOVE D(e,,e+1) -> D(ac,,ac+1)
    NYI("DMOVE");
`,
'121': `// DMOVEN -D(e,,e+1) -> D(ac,,ac+1)
    NYI("DMOVEN");
`,
'124': `// DMOVEM D(ac,,ac+1) -> D(e,,e+1)
    NYI("DMOVEM");
`,
'125': `// DMOVNM -D(ac,,ac+1) -> D(e,,e+1)
    NYI("DMOVNM");
`,
'251': `// BLT
    NYI("BLT");
`,
'204': `// MOVS (e).s -> (ac)
    NYI("MOVS");
`,
'205': `// MOVSI e,,0 -> (ac)
    NYI("MOVSI");
`,
'206': `// MOVSM (ac).s -> (e)
    NYI("MOVSM");
`,
'207': `// MOVSS (e).s -> (e); if ac !== 0: (e) -> (ac)
    NYI("MOVSS");
`,
'214': `// MOVM |(e)| -> (ac)
    NYI("MOVM");
`,
'215': `// MOVMI 0,,e -> (ac)
    NYI("MOVMI");
`,
'216': `// MOVMM
/*	|(e)| -> (tmp)
		(tmp) -> (e)
		if (tmp) == 400000_000000: 1 -> tr1 ov cy1
		if (tmp) == 0: 1 -> cy0 cy1 */
    NYI("MOVMM");
`,
'217': `// MOVMS
/*	|(e)| -> (tmp)
		(tmp) -> (e)
      		if ac !== 0: (tmp) -> (ac)
		if (tmp) == 400000_000000: 1 -> tr1 ov cy1
		if (tmp) == 0: 1 -> cy0 cy1 */
    NYI("MOVMS");
`,
'052': `// PMOVE
    NYI("PMOVE");
`,
'053': `// PMOVEM
    NYI("PMOVEM");
`,
'270': `// ADD (ac) + (e) -> (ac)
    NYI("ADD");
`,
'271': `// ADDI (ac) + 0,,e -> (ac)
    NYI("ADDI");
`,
'272': `// ADDM (ac) + (e) -> (e)
    NYI("ADDM");
`,
'273': `// ADDB (ac) + (e) -> (ac) (e)
    NYI("ADDB");
`,
'274': `// SUB (ac) - (e) -> (ac)
    NYI("SUB");
`,
'275': `// SUBI (ac) - 0,,e -> (ac)
    NYI("SUBI");
`,
'276': `// SUBM (ac) - (e) -> (e)
    NYI("SUBM");
`,
'277': `// SUBB (ac) - (e) -> (ac) (e)
    NYI("SUBB");
`,
'220': `// IMUL (ac) * (e) -> (ac).discardHi
    NYI("IMUL");
`,
'221': `// IMULI (ac) * 0,,e -> (ac).discardHi
    NYI("IMULI");
`,
'222': `// IMULM (ac) * (e) -> (e).discardHi
    NYI("IMULM");
`,
'223': `// IMULB (ac) * (e) -> (ac).discardHi (e).discardHi
    NYI("IMULB");
`,
'224': `// MUL (ac) * (e) -> D(ac,,ac+1)
    NYI("MUL");
`,
'225': `// MULI (ac) * 0,,e -> D(ac,,ac+1)
    NYI("MULI");
`,
'226': `// MULM (ac) * (e) -> (e).discardLo
    NYI("MULM");
`,
'227': `// MULB (ac) * (e) -> (ac,,ac+1) (e).discardLo
    NYI("MULB");
`,
'230': `// IDIV (ac) / (e) -> (ac).quo (ac+1).rem
    NYI("IDIV");
`,
'231': `// IDIVI (ac) / 0,,e -> (ac).quo (ac+1).rem
    NYI("IDIVI");
`,
'232': `// IDIVM (ac) / (e) -> (e).quo
    NYI("IDIVM");
`,
'233': `// IDIVB (ac) / (e) -> (ac).quo (ac+1).rem (e).quo
    NYI("IDIVB");
`,
'234': `// DIV D(ac,,ac+1) / (e) -> (ac).quo (ac+1).rem
    NYI("DIV");
`,
'235': `// DIVI D(ac,,ac+1) / 0,,e -> (ac).quo (ac+1).rem
    NYI("DIVI");
`,
'236': `// DIVM D(ac,,ac+1) / (e) -> (e).quo
    NYI("DIVM");
`,
'237': `// DIVB D(ac,,ac+1) / (e) -> (ac).quo (ac+1).rem (e).quo
    NYI("DIVB");
`,
'114': `// DADD D(ac,,ac+1) d+ D(e,,e+1) -> D(ac,,ac+1)
    NYI("DADD");
`,
'115': `// DSUB D(ac,,ac+1) d- D(e,,e+1) -> D(ac,,ac+1)
    NYI("DSUB");
`,
'116': `// DMUL D(ac,,ac+1) d* D(e,,e+1) -> Q(ac,,ac+3)
    NYI("DMUL");
`,
'117': `// DDIV D(ac,,ac+1) d/ D(e,,e+1) -> Q(ac,,ac+3)
    NYI("DDIV");
`,
'140': `// FAD (ac) f+ (e) -> (ac)
    NYI("FAD");
`,
'141': `// FADL (ac) fl+ (e) -> D(ac,,ac+1)
    NYI("FADL");
`,
'142': `// FADM (ac) f+ (e) -> (e)
    NYI("FADM");
`,
'143': `// FADB (ac) f+ (e) -> (ac) (e)
    NYI("FADB");
`,
'144': `// FADR (ac) fr+ (e) -> (ac)
    NYI("FADR");
`,
'145': `// FADRI (ac) fr+ e,,0 -> (ac)
    NYI("FADRI");
`,
'146': `// FADRM (ac) fr+ (e) -> (e)
    NYI("FADRM");
`,
'147': `// FADRB (ac) fr+ (e) -> (ac) (e)
    NYI("FADRB");
`,
'150': `// FSB (ac) f- (e) -> (ac)
    NYI("FSB");
`,
'151': `// FSBL (ac) fl- (e) -> D(ac,,ac+1)
    NYI("FSBL");
`,
'152': `// FSBM (ac) f- (e) -> (e)
    NYI("FSBM");
`,
'153': `// FSBB (ac) f- (e) -> (ac) (e)
    NYI("FSBB");
`,
'154': `// FSBR (ac) fr- (e) -> (ac)
    NYI("FSBR");
`,
'155': `// FSBRI (ac) fr- e,,0 -> (ac)
    NYI("FSBRI");
`,
'156': `// FSBRM (ac) fr- (e) -> (e)
    NYI("FSBRM");
`,
'157': `// FSBRB (ac) fr- (e) -> (ac) (e)
    NYI("FSBRB");
`,
'160': `// FMP (ac) f* (e) -> (ac)
    NYI("FMP");
`,
'161': `// FMPL (ac) fl* (e) -> D(ac,,ac+1)
    NYI("FMPL");
`,
'162': `// FMPM (ac) f* (e) -> (e)
    NYI("FMPM");
`,
'163': `// FMPB (ac) f* (e) -> (ac) (e)
    NYI("FMPB");
`,
'164': `// FMPR (ac) fr* (e) -> (ac)
    NYI("FMPR");
`,
'165': `// FMPRI (ac) fr* e,,0 -> (ac)
    NYI("FMPRI");
`,
'166': `// FMPRM (ac) fr* (e) -> (e)
    NYI("FMPRM");
`,
'167': `// FMPRB (ac) fr* (e) -> (ac) (e)
    NYI("FMPRB");
`,
'170': `// FDV (ac) f/ (e) -> (ac)
    NYI("FDV");
`,
'171': `// FDVL (ac) fl/ (e) -> D(ac,,ac+1)
    NYI("FDVL");
`,
'172': `// FDVM (ac) f/ (e) -> (e)
    NYI("FDVM");
`,
'173': `// FDVB (ac) f/ (e) -> (ac) (e)
    NYI("FDVB");
`,
'174': `// FDVR (ac) fr/ (e) -> (ac)
    NYI("FDVR");
`,
'175': `// FDVRI (ac) fr/ e,,0 -> (ac)
    NYI("FDVRI");
`,
'176': `// FDVRM (ac) fr/ (e) -> (e)
    NYI("FDVRM");
`,
'177': `// FDVRB (ac) fr/ (e) -> (ac) (e)
    NYI("FDVRB");
`,
'110': `// DFAD D(ac,,ac+1) d+ D(e,,e+1) -> D(ac,,ac+1)
    NYI("DFAD");
`,
'111': `// DFSB D(ac,,ac+1) d- D(e,,e+1) -> D(ac,,ac+1)
    NYI("DFSB");
`,
'112': `// DFMP D(ac,,ac+1) d* D(e,,e+1) -> D(ac,,ac+1)
    NYI("DFMP");
`,
'113': `// DFDV D(ac,,ac+1) d/ D(e,,e+1) -> D(ac,,ac+1)
    NYI("DFDV");
`,
'132': `// FSC (ac) f<< e -> (ac)
    NYI("FSC");
`,
'031': `// GFSC D(ac,,ac+1) f<< e -> D(ac,,ac+1)
    NYI("GFSC");
`,
'127': `// FLTR (e).ifr -> (ac)
    NYI("FLTR");
`,
'030': `// GFLTR (e).ifrD -> D(ac,,ac+1)
    NYI("GFLTR");
`,
'027': `// DGFLTR (e,,e+1).DifrD -> D(ac,,ac+1)
    NYI("DGFLTR");
`,
'122': `// FIX (e).fix -> (ac)
    NYI("FIX");
`,
'126': `// FIXR (e).fixR -> (ac)
    NYI("FIXR");
`,
'024': `// GFIX (e,,e+1).Dfix -> (ac)
    NYI("GFIX");
`,
'026': `// GFIXR (e,,e+1).DfixR -> (ac)
    NYI("GFIXR");
`,
'023': `// GDFIX (e,,e+1).DfixD -> D(ac,,ac+1)
    NYI("GDFIX");
`,
'025': `// GDFIXR (e,,e+1).DfixRD -> D(ac,,ac+1)
    NYI("GDFIXR");
`,
'021': `// GSNGL (e,,e+1).sngl -> (ac)
    NYI("GSNGL");
`,
'022': `// GDBLE (e).dble -> D(ac,,ac+1)
    NYI("GDBLE");
`,
'130': `// UFA (ac) fu+ (e) -> (ac+1)
    NYI("UFA");
`,
'131': `// DFN {cpu.doDFN(ac, e)}
    NYI("DFN");
`,
'400': `// SETZ 0 -> (ac)
    NYI("SETZ");
`,
'401': `// SETZI 0 -> (ac)
    NYI("SETZI");
`,
'402': `// SETZM 0 -> (e)
    NYI("SETZM");
`,
'403': `// SETZB 0 -> (ac) (e)
    NYI("SETZB");
`,
'474': `// SETO 777777777777 -> (ac)
    NYI("SETO");
`,
'475': `// SETOI 777777777777 -> (ac)
    NYI("SETOI");
`,
'476': `// SETOM 777777777777 -> (e)
    NYI("SETOM");
`,
'477': `// SETOB 777777777777 -> (ac) (e)
    NYI("SETOB");
`,
'424': `// SETA { }	// (ac) -> (ac)
    NYI("SETA");
`,
'425': `// SETAI { }	// (ac) -> (ac)
    NYI("SETAI");
`,
'426': `// SETAM (ac) -> (e)
    NYI("SETAM");
`,
'427': `// SETAB (ac) -> (e)
    NYI("SETAB");
`,
'450': `// SETCA ~(ac) -> (ac)
    NYI("SETCA");
`,
'451': `// SETCAI ~(ac) -> (ac)
    NYI("SETCAI");
`,
'452': `// SETCAM ~(ac) -> (e)
    NYI("SETCAM");
`,
'453': `// SETCAB ~(ac) -> (ac) (e)
    NYI("SETCAB");
`,
'414': `// SETM (e) -> (ac)
    NYI("SETM");
`,
'416': `// SETMM (e) -> (e)// No-op
    NYI("SETMM");
`,
'417': `// SETMB (e) -> (ac) (e)
    NYI("SETMB");
`,
'460': `// SETCM ~(e) -> (ac)
    NYI("SETCM");
`,
'461': `// SETCMI ~[0,,e] -> (ac)
    NYI("SETCMI");
`,
'462': `// SETCMM ~(e) -> (e)
    NYI("SETCMM");
`,
'463': `// SETCMB ~(e) -> (ac) (e)
    NYI("SETCMB");
`,
'404': `// AND (ac) & (e) -> (ac)
    NYI("AND");
`,
'405': `// ANDI (ac) & 0,,e -> (ac)
    NYI("ANDI");
`,
'406': `// ANDM (ac) & (e) -> (e)
    NYI("ANDM");
`,
'407': `// ANDB (ac) & (e) -> (ac) (e)
    NYI("ANDB");
`,
'410': `// ANDCA ~(ac) & (e) -> (ac)
    NYI("ANDCA");
`,
'411': `// ANDCAI ~(ac) & 0,,e -> (ac)
    NYI("ANDCAI");
`,
'412': `// ANDCAM ~(ac) & (e) -> (e)
    NYI("ANDCAM");
`,
'413': `// ANDCAB ~(ac) & (e) -> (ac) (e)
    NYI("ANDCAB");
`,
'420': `// ANDCM (ac) & ~(e) -> (ac)
    NYI("ANDCM");
`,
'421': `// ANDCMI (ac) & ~[0,,e] -> (ac)
    NYI("ANDCMI");
`,
'422': `// ANDCMM (ac) & ~(e) -> (e)
    NYI("ANDCMM");
`,
'423': `// ANDCMB (ac) & ~(e) -> (ac) (e)
    NYI("ANDCMB");
`,
'440': `// ANDCB ~(ac) & ~(e) -> (ac)
    NYI("ANDCB");
`,
'441': `// ANDCBI ~(ac) & ~[0,,e] -> (ac)
    NYI("ANDCBI");
`,
'442': `// ANDCBM ~(ac) & ~(e) -> (e)
    NYI("ANDCBM");
`,
'443': `// ANDCBB ~(ac) & ~(e) -> (ac) (e)
    NYI("ANDCBB");
`,
'434': `// IOR (ac) | (e) -> (ac)
    NYI("IOR");
`,
'435': `// IORI (ac) | 0,,e -> (ac)
    NYI("IORI");
`,
'436': `// IORM (ac) | (e) -> (e)
    NYI("IORM");
`,
'437': `// IORB (ac) | (e) -> (ac) (e)
    NYI("IORB");
`,
'454': `// ORCA ~(ac) | (e) -> (ac)
    NYI("ORCA");
`,
'455': `// ORCAI ~(ac) | 0,,e -> (ac)
    NYI("ORCAI");
`,
'456': `// ORCAM ~(ac) | (e) -> (e)
    NYI("ORCAM");
`,
'457': `// ORCAB ~(ac) | (e) -> (ac) (e)
    NYI("ORCAB");
`,
'464': `// ORCM (ac) | ~(e) -> (ac)
    NYI("ORCM");
`,
'465': `// ORCMI (ac) | ~[0,,e] -> (ac)
    NYI("ORCMI");
`,
'466': `// ORCMM (ac) | ~(e) -> (e)
    NYI("ORCMM");
`,
'467': `// ORCMB (ac) | ~(e) -> (ac) (e)
    NYI("ORCMB");
`,
'470': `// ORCB ~(ac) | ~(e) -> (ac)
    NYI("ORCB");
`,
'471': `// ORCBI ~(ac) | ~[0,,e] -> (ac)
    NYI("ORCBI");
`,
'472': `// ORCBM ~(ac) | ~(e) -> (e)
    NYI("ORCBM");
`,
'473': `// ORCBB ~(ac) | ~(e) -> (ac) (e)
    NYI("ORCBB");
`,
'430': `// XOR (ac) ^ (e) -> (ac)
    NYI("XOR");
`,
'431': `// XORI (ac) ^ 0,,e -> (ac)
    NYI("XORI");
`,
'432': `// XORM (ac) ^ (e) -> (e)
    NYI("XORM");
`,
'433': `// XORB (ac) ^ (e) -> (ac) (e)
    NYI("XORB");
`,
'444': `// EQV (ac) >< (e) -> (ac)
    NYI("EQV");
`,
'445': `// EQVI (ac) >< 0,,e -> (ac)
    NYI("EQVI");
`,
'446': `// EQVM (ac) >< (e) -> (e)
    NYI("EQVM");
`,
'447': `// EQVB (ac) >< (e) -> (ac) (e)
    NYI("EQVB");
`,
'240': `// ASH (ac) <<a e -> (ac)
    NYI("ASH");
`,
'241': `// ROT (ac) <<r e -> (ac)
    NYI("ROT");
`,
'242': `// LSH (ac) <<u e -> (ac)
    NYI("LSH");
`,
'244': `// ASHC D(ac,,ac+1) <<a e -> D(ac,,ac+1)
    NYI("ASHC");
`,
'245': `// ROTC D(ac,,ac+1) <<r e -> D(ac,,ac+1)
    NYI("ROTC");
`,
'246': `// LSHC D(ac,,ac+1) <<u e -> D(ac,,ac+1)
    NYI("LSHC");
`,
'252': `// AOBJP
/*	(ac).l + 1 -> (ac).l
		(ac).r + 1 -> (ac).r
      		if (ac) >= 0: e -> (pc) */
    NYI("AOBJP");
`,
'253': `// AOBJN
/*	(ac).l + 1 -> (ac).l
		(ac).r + 1 -> (ac).r
      		if (ac) < 0: e -> (pc) */
    NYI("AOBJN");
`,
'300': `// CAI { }
    NYI("CAI");
`,
'301': `// CAIL if (ac) < e: skip
    NYI("CAIL");
`,
'302': `// CAIE if (ac) === e: skip
    NYI("CAIE");
`,
'303': `// CAILE if (ac) <= e: skip
    NYI("CAILE");
`,
'304': `// CAIA skip
    NYI("CAIA");
`,
'305': `// CAIGE if (ac) >= e: skip
    NYI("CAIGE");
`,
'306': `// CAIN if (ac) !== e: skip
    NYI("CAIN");
`,
'307': `// CAIG if (ac) > e: skip
    NYI("CAIG");
`,
'310': `// CAM { }
    NYI("CAM");
`,
'311': `// CAML if (ac) < (e): skip
    NYI("CAML");
`,
'312': `// CAME if (ac) === (e): skip
    NYI("CAME");
`,
'313': `// CAMLE if (ac) <= (e): skip
    NYI("CAMLE");
`,
'314': `// CAMA skip
    NYI("CAMA");
`,
'315': `// CAMGE if (ac) >= (e): skip
    NYI("CAMGE");
`,
'316': `// CAMN if (ac) !== (e): skip
    NYI("CAMN");
`,
'317': `// CAMG if (ac) > (e): skip
    NYI("CAMG");
`,
'320': `// JUMP { }
    NYI("JUMP");
`,
'321': `// JUMPL if (ac) < 0: e -> (pc)
    NYI("JUMPL");
`,
'322': `// JUMPE if (ac) === 0: e -> (pc)
    NYI("JUMPE");
`,
'323': `// JUMPLE if (ac) <= 0: e -> (pc)
    NYI("JUMPLE");
`,
'324': `// JUMPA e -> (pc)
    NYI("JUMPA");
`,
'325': `// JUMPGE if (ac) >= 0: e -> (pc)
    NYI("JUMPGE");
`,
'326': `// JUMPN if (ac) !== 0: e -> (pc)
    NYI("JUMPN");
`,
'327': `// JUMPG if (ac) > 0: e -> (pc)
    NYI("JUMPG");
`,
'340': `// AOJ (ac).aox -> (ac)
    NYI("AOJ");
`,
'341': `// AOJL (ac).aox -> (ac); if (ac) < 0: e -> pc
    NYI("AOJL");
`,
'342': `// AOJE (ac).aox -> (ac); if (ac) === 0: e -> pc
    NYI("AOJE");
`,
'343': `// AOJLE (ac).aox -> (ac); if (ac) <= 0: e -> pc
    NYI("AOJLE");
`,
'344': `// AOJA (ac).aox -> (ac); e -> pc
    NYI("AOJA");
`,
'345': `// AOJGE (ac).aox -> (ac); if (ac) >= 0: e -> pc
    NYI("AOJGE");
`,
'346': `// AOJN (ac).aox -> (ac); if (ac) !== 0: e -> pc
    NYI("AOJN");
`,
'347': `// AOJG (ac).aox -> (ac); if (ac) > 0: e -> pc
    NYI("AOJC");
`,
'360': `// SOJ (ac).sox -> (ac)
    NYI("SOJ");
`,
'361': `// SOJL (ac).sox -> (ac); if (ac) < 0: e -> pc
    NYI("SOJL");
`,
'362': `// SOJE (ac).sox -> (ac); if (ac) === 0: e -> pc
    NYI("SOJE");
`,
'363': `// SOJLE (ac).sox -> (ac); if (ac) <= 0: e -> pc
    NYI("SOJLE");
`,
'364': `// SOJA (ac).sox -> (ac); e -> pc
    NYI("SOJA");
`,
'365': `// SOJGE (ac).sox -> (ac); if (ac) >= 0: e -> pc
    NYI("SOJGE");
`,
'366': `// SOJN (ac).sox -> (ac); if (ac) !== 0: e -> pc
    NYI("SOJN");
`,
'367': `// SOJG (ac).sox -> (ac); if (ac) > 0: e -> pc
    NYI("SOJG");
`,
'350': `// AOS (e).aox -> (e); if ac !== 0: (e) -> (ac)
    NYI("AOS");
`,
'351': `// AOSL (e).aox -> (e); if ac !== 0: (e) -> (ac); if (e) < 0: skip
    NYI("AOSL");
`,
'352': `// AOSE (e).aox -> (e); if ac !== 0: (e) -> (ac); if (e) === 0: skip
    NYI("AOSE");
`,
'353': `// AOSLE (e).aox -> (e); if ac !== 0: (e) -> (ac); if (e) <= 0: skip
    NYI("AOSLE");
`,
'354': `// AOSA (e).aox -> (e); if ac !== 0: (e) -> (ac); skip
    NYI("AOSA");
`,
'355': `// AOSGE (e).aox -> (e); if ac !== 0: (e) -> (ac); if (e) >= 0: skip
    NYI("AOSGE");
`,
'356': `// AOSN (e).aox -> (e); if ac !== 0: (e) -> (ac); if (e) !== 0: skip
    NYI("AOSN");
`,
'357': `// AOSG (e).aox -> (e); if ac !== 0: (e) -> (ac); if (e) > 0: skip
    NYI("AOSG");
`,
'370': `// SOS (e).sox -> (e); if ac !== 0: (e) -> (ac)
    NYI("SOS");
`,
'371': `// SOSL (e).sox -> (e); if ac !== 0: (e) -> (ac); if (e) < 0: skip
    NYI("SOSL");
`,
'372': `// SOSE (e).sox -> (e); if ac !== 0: (e) -> (ac); if (e) === 0: skip
    NYI("SOSE");
`,
'373': `// SOSLE (e).sox -> (e); if ac !== 0: (e) -> (ac); if (e) <= 0: skip
    NYI("SOSLE");
`,
'374': `// SOSA (e).sox -> (e); if ac !== 0: (e) -> (ac); skip
    NYI("SOSA");
`,
'375': `// SOSGE (e).sox -> (e); if ac !== 0: (e) -> (ac); if (e) >= 0: skip
    NYI("SOSGE");
`,
'376': `// SOSN (e).sox -> (e); if ac !== 0: (e) -> (ac); if (e) !== 0: skip
    NYI("SOSN");
`,
'377': `// SOSG (e).sox -> (e); if ac !== 0: (e) -> (ac); if (e) > 0: skip
    NYI("SOSG");
`,
'600': `// TRN
  (void) 0;
`,
'601': `// TLN
  (void) 0;
`,
'602': `// TRNE
  ${testInsn(getERH, modNone, skipE)}
`,
'603': `// TLNE
  ${testInsn(getELH, modNone, skipE)}
`,
'604': `// TRNA
  ++pc;
`,
'605': `// TLNA
  ++pc;
`,
'606': `// TRNN
  ${testInsn(getERH, modNone, skipN)}
`,
'607': `// TLNN
  ${testInsn(getELH, modNone, skipN)}
`,
'620': `// TRZ
  ${testInsn(getERH, modZeros, skipNever)}
`,
'621': `// TLZ
  ${testInsn(getELH, modZeros, skipNever)}
`,
'622': `// TRZE
  ${testInsn(getERH, modZeros, skipE)};
`,
'623': `// TLZE
  ${testInsn(getELH, modZeros, skipE)};
`,
'624': `// TRZA
  ${testInsn(getERH, modZeros, skipAlways)};
`,
'625': `// TLZA
  ${testInsn(getELH, modZeros, skipAlways)};
`,
'626': `// TRZN
  ${testInsn(getERH, modZeros, skipN)};
`,
'627': `// TLZN
  ${testInsn(getELH, modZeros, skipN)};
`,
'640': `// TRC (ac).r ^ e.r -> (ac).r
  ${testInsn(getERH, modComplement, skipN)};
`,
'641': `// TLC (ac).l ^ e.r -> (ac).l
  ${testInsn(getELH, modComplement, skipN)};
`,
'642': `// TRCE
  ${testInsn(getERH, modComplement, skipE)};
`,
'643': `// TLCE
  ${testInsn(getELH, modComplement, skipE)};
`,
'644': `// TRCA skip; (ac).r ^ e.r -> (ac).r
  ${testInsn(getERH, modComplement, skipAlways)};
`,
'645': `// TLCA skip; (ac).l ^ e.r -> (ac).l
  ${testInsn(getELH, modComplement, skipAlways)};
`,
'646': `// TRCN
  ${testInsn(getERH, modComplement, skipN)};
`,
'647': `// TLCN
  ${testInsn(getELH, modComplement, skipN)};
`,
'660': `// TRO (ac).r | e -> (ac).r
  ${testInsn(getERH, modOnes, skipN)};
`,
'661': `// TLO (ac).l | e -> (ac).l
  ${testInsn(getELH, modOnes, skipN)};
`,
'662': `// TROE
  ${testInsn(getERH, modOnes, skipE)};
`,
'663': `// TLOE
  ${testInsn(getELH, modOnes, skipE)};
`,
'664': `// TROA
  ${testInsn(getERH, modOnes, skipAlways)};
`,
'665': `// TLOA
  ${testInsn(getELH, modOnes, skipAlways)};
`,
'666': `// TRON
  ${testInsn(getERH, modOnes, skipN)};
`,
'667': `// TLON
  ${testInsn(getELH, modOnes, skipN)};
`,
'610': `// TDN
    (void) 0;
`,
'611': `// TSN
    (void) 0;
`,
'612': `// TDNE if (ac) & (e) === 0: skip
  ${testInsn(getMem, modNone, skipE)};
`,
'613': `// TSNE if (ac) & (e).s === 0: skip
  ${testInsn(getMemSwapped, modNone, skipE)};
`,
'614': `// TDNA
    ++pc;
`,
'615': `// TSNA
    ++pc;
`,
'616': `// TDNN if (ac) & (e) !== 0: skip
  ${testInsn(getMem, modNone, skipN)};
`,
'617': `// TSNN if (ac) & (e).s !== 0: skip
  ${testInsn(getMemSwapped, modNone, skipN)};
`,
'630': `// TDZ (ac) & ~(e) -> (ac)
  ${testInsn(getMem, modZeros, skipNever)};
`,
'631': `// TSZ
  ${testInsn(getMemSwapped, modZeros, skipNever)};
`,
'632': `// TDZE
  ${testInsn(getMem, modZeros, skipE)};
`,
'633': `// TSZE
  ${testInsn(getMemSwapped, modZeros, skipE)};
`,
'634': `// TDZA
  ${testInsn(getMem, modZeros, skipAlways)};
`,
'635': `// TSZA
  ${testInsn(getMemSwapped, modZeros, skipAlways)};
`,
'636': `// TDZN
  ${testInsn(getMem, modZeros, skipN)};
`,
'637': `// TSZN
  ${testInsn(getMemSwapped, modZeros, skipN)};
`,
'650': `// TDC
  ${testInsn(getMem, modComplement, skipNever)};
`,
'651': `// TSC
  ${testInsn(getMemSwapped, modComplement, skipNever)};
`,
'652': `// TDCE
  ${testInsn(getMem, modComplement, skipE)};
`,
'653': `// TSCE
  ${testInsn(getMemSwapped, modComplement, skipE)};
`,
'654': `// TDCA
  ${testInsn(getMem, modComplement, skipAlways)};
`,
'655': `// TSCA
  ${testInsn(getMemSwapped, modComplement, skipAlways)};
`,
'656': `// TDCN
  ${testInsn(getMem, modComplement, skipN)};
`,
'657': `// TSCN
  ${testInsn(getMemSwapped, modComplement, skipAlways)};
`,
'670': `// TDO
  ${testInsn(getMem, modOnes, skipNever)};
`,
'671': `// TSO
  ${testInsn(getMemSwapped, modOnes, skipNever)};
`,
'672': `// TDOE
  ${testInsn(getMem, modOnes, skipE)};
`,
'673': `// TSOE
  ${testInsn(getMemSwapped, modOnes, skipE)};
`,
'674': `// TDOA
  ${testInsn(getMem, modOnes, skipAlways)};
`,
'675': `// TSOA
  ${testInsn(getMemSwapped, modOnes, skipAlways)};
`,
'676': `// TDON
  ${testInsn(getMem, modOnes, skipN)};
`,
'677': `// TSON
  ${testInsn(getMemSwapped, modOnes, skipN)};
`,
'500': `// HLL
  ${halfInsn('HLL  ')};
`,
'501': `// HLLI/XHLLI
  if (LH(pc) == 0) {
    ${halfInsn('HLLI ')};
  } else {
    ${putAC('ea')};
  }
`,
'502': `// HLLM
  ${halfInsn('HLLM ')};
`,
'503': `// HLLS
  ${halfInsn('HLLS ')};
`,
'510': `// HLLZ
  ${halfInsn('HLLZ  ')};
`,
'511': `// HLLZI
  ${putAC(0)};
`,
'512': `// HLLZM
  ${halfInsn('HLLZM')};
`,
'513': `// HLLZS
  ${halfInsn('HLLZS')};
`,
'530': `// HLLE
  ${halfInsn('HLLE ')};
`,
'531': `// HLLEI
  ${halfInsn('HLLEI')};
`,
'532': `// HLLEM
  ${halfInsn('HLLEM')};
`,
'533': `// HLLES
  ${halfInsn('HLLES')};
`,
'520': `// HLLO
  ${halfInsn('HLLO ')};
`,
'521': `// HLLOI
  ${halfInsn('HLLOI')};
`,
'522': `// HLLOM
  ${halfInsn('HLLOM')};
`,
'523': `// HLLOS
  ${halfInsn('HLLOS')};
`,
'544': `// HLR
  ${halfInsn('HLR  ')};
`,
'545': `// HLRI
  ${halfInsn('HLRI ')};
`,
'546': `// HLRM
  ${halfInsn('HLRM ')};
`,
'547': `// HLRS
  ${halfInsn('HLRS ')};
`,
'554': `// HLRZ
  ${halfInsn('HLRZ ')};
`,
'555': `// HLRZI
  ${putAC(0)};
`,
'556': `// HLRZM
  ${halfInsn('HLRZM')};
`,
'557': `// HLRZS
  ${halfInsn('HLRZS')};
`,
'564': `// HLRO
  ${halfInsn('HLRO ')};
`,
'565': `// HLROI
  ${halfInsn('HLROI')};
`,
'566': `// HLROM
  ${halfInsn('HLROM')};
`,
'567': `// HLROS
  ${halfInsn('HLROS')};
`,
'574': `// HLRE
  ${halfInsn('HLRE ')};
`,
'575': `// HLREI
  ${halfInsn('HLREI')};
`,
'576': `// HLREM
  ${halfInsn('HLREM')};
`,
'577': `// HLRES
  ${halfInsn('HLRES')};
`,
'540': `// HRR
  ${halfInsn('HRR  ')};
`,
'541': `// HRRI
  ${halfInsn('HRRI ')};
`,
'542': `// HRRM
  ${halfInsn('HRRM ')};
`,
'543': `// HRRS
  ${halfInsn('HRRS ')};
`,
'550': `// HRRZ
  ${halfInsn('HRRZ ')};
`,
'551': `// HRRZI
  ${halfInsn('HRRZI')};
`,
'552': `// HRRZM
  ${halfInsn('HRRZM')};
`,
'553': `// HRRZS
  ${halfInsn('HRRZS')};
`,
'560': `// HRRO
  ${halfInsn('HRRZO')};
`,
'561': `// HRROI
  ${halfInsn('HRROI')};
`,
'562': `// HRROM
  ${halfInsn('HRROM')};
`,
'563': `// HRROS
  ${halfInsn('HRROS')};
`,
'570': `// HRRE
  ${halfInsn('HRRE ')};
`,
'571': `// HRREI
  ${halfInsn('HRREI')};
`,
'572': `// HRREM
  ${halfInsn('HRREM')};
`,
'573': `// HRRES
  ${halfInsn('HRRES')};
`,
'504': `// HRL
  ${halfInsn('HRL  ')};
`,
'505': `// HRLI
  ${halfInsn('HRLI ')};
`,
'506': `// HRLM
  ${halfInsn('HRLM ')};
`,
'507': `// HRLS
  ${halfInsn('HRLS ')};
`,
'514': `// HRLZ
  ${halfInsn('HRLZ ')};
`,
'515': `// HRLZI
  ${halfInsn('HRLZI')};
`,
'516': `// HRLZM
  ${halfInsn('HRLZM')};
`,
'517': `// HRLZS
  ${halfInsn('HRLZS')};
`,
'524': `// HRLO
  ${halfInsn('HRLO ')};
`,
'525': `// HRLOI
  ${halfInsn('HRLOI')};
`,
'526': `// HRLOM
  ${halfInsn('HRLOM')};
`,
'527': `// HRLOS
  ${halfInsn('HRLOS')};
`,
'534': `// HRLE
  ${halfInsn('HRLE ')};
`,
'535': `// HRLEI
  ${halfInsn('HRLEI')};
`,
'536': `// HRLEM
  ${halfInsn('HRLEM')};
`,
'537': `// HRLES
  ${halfInsn('HRLES')};
`,
'25600': `// XCT
    NYI("XCT");
`,
'243': `// JFFO
/*	if (ac) === 0: 0 -> (ac+1)
		if (ac) !== 0: (ac).jffo -> (ac+1)
		if (ac) !== 0: e -> (pc)*/
    NYI("JFFO");
`,
'255': `// JFCL
    NYI("JFCL");
`,
'266': `// JSA
/*	(ac) -> (e)
		e.r -> (ac).l
		(pc).r -> (ac).r
		e + 1 -> (pc) */
    NYI("JSA");
`,
'267': `// JRA
    NYI("JRA");
`,
'257': `// MAP
    NYI("MAP");
`,
/*
'256nn': `// PXCT
    NYI("PXCT");
`,
*/
  '261': `// PUSH
    NYI("PUSH");
`,
'262': `// POP
    NYI("POP");
`,
'260': `// PUSHJ
    NYI("PUSHJ");
`,
'263': `// POPJ
    NYI("POPJ");
`,
'105': `// ADJSP
    NYI("ADJSP");
`,
'13300': `// IBP
    NYI("IBP");
`,
/*
'133nn': `// ADJBP
    NYI("ADJBP");
`,
*/
'135': `// LDB
    NYI("LDB");
`,
'137': `// DPB
    NYI("DPB");
`,
'134': `// ILDB
    NYI("ILDB");
`,
'136': `// IDPB
    NYI("IDPB");
`,
'001': `// LUUO
    NYI("LUUO");
`,
'104': `// JSYS
    NYI("JSYS");
`,
'123': `// EXTEND
    NYI("EXTEND");
`,
'70144': `// SWPIA
    NYI("SWPIA");
`,
'70164': `// SWPIO
    NYI("SWPIO");
`,
'70150': `// SWPVA
    NYI("SWPVA");
`,
'70170': `// SWPVO
    NYI("SWPVO");
`,
'70154': `// SWPUA
    NYI("SWPUA");
`,
'70174': `// SWPUO
    NYI("SWPUO");
`,
'70010': `// WRFIL
    NYI("WRFIL");
`,
'70000': `// APRID
    NYI("APRID");
`,
'70110': `// CLRPT
    NYI("CLRPT");
`,
'70260': `// WRTIME
    NYI("WRTIME");
`,
'70204': `// RDTIME
    NYI("RDTIME");
`,
'70244': `// RDEACT
    NYI("RDEACT");
`,
'70240': `// RDMACT
    NYI("RDMACT");
`,
'70210': `// WRPAE
    NYI("WRPAE");
`,
'70200': `// RDPERF
    NYI("RDPERF");
`,
'70040': `// RDERA
    NYI("RDERA");
`,
'70050': `// SBDIAG
    NYI("SBDIAG");
`,
};


console.log(`\
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <stdint.h>

#include <stdio.h>
#include <stdint.h>
#include <string.h>

#include "kl10.h"
#include "disasm.h"
#include "loada10.h"


${macros}

static void emulate() {

  do {
    // XXX Put interrupt, trap, HALT, etc. handling here...
    W36 iw = memP[pc];

    W36 ea;
    int eaIsLocal = 1;		// XXX THIS IS TEMPORARY UNTIL WE HAVE EXTENDED STUFF
    unsigned op;
    unsigned ac;
    unsigned i;
    unsigned x;
    unsigned y;

    // XXX support extended version of this...
    W36 eaw = iw;
    do {
      op = Extract(eaw, 0, 8);
      ac = Extract(eaw, 9, 12);
      i = Extract(eaw, 13, 13);
      x = Extract(eaw, 14, 17);
      y = Extract(eaw, 18, 35);

      ea = x ? RH(AC[x] + y) : y;

      if (i) eaw = memP[ea];
    } while (i);

    if (tracePC) {
      char pcBuf[64];
      char eaBuf[64];
      char daBuf[256];

      DisassembleToString(iw, daBuf);
      fprintf(stderr, "%s: [ea=%s] %s\\n", oct36(pcBuf, pc), oct36(eaBuf, ea), daBuf);
    }

    switch (op) {
    ${Object.keys(kl10Instructions)
	    .map(k => `
	case 0${k}: {
${kl10Instructions[k]}\
	  break; }`).join('\n')}
    }

    ++pc;

  SKIP_PC_INCR: ;
  } while (running);
}


int main(int argc, char *argv[]) {
  static W36 memory[256*1024];
  W36 startAddr, lowestAddr, highestAddr;
  char *fileNameP;

  if (argc == 2) {
    fileNameP = argv[1];
  } else {
    fprintf(stderr, "Usage:\\n\\
    %s <filename to load>\\n", argv[0]);
    return -1;
  }

  int st = LoadA10(fileNameP, memory, &startAddr, &lowestAddr, &highestAddr);
  fprintf(stderr, "[Loaded %s  st=%d  start=" PRI06o64 "]\\n", fileNameP, st, startAddr);

  pc = startAddr;
  memP = memory;
  tracePC = 1;
  traceAC = 1;
  running = 1;
  emulate();
}`);
