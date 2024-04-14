'use strict';

const fs = require('fs');
const util = require('util');


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
#define RH(X)	((X) & 07000000ul)
#define LH(X)	RH((W36) (X) >> 18)

#define BIT(N)	(1ull << (35 - (N)))
#define BIT0	BIT(0)
#define BIT17	BIT(17)
#define BIT18	BIT(18)
#define BIT35	BIT(35)

#define LHMASK	(0777777ull << 18)
#define RHMASK	0777777ull


${Object.keys(flags)
  .map(f => `#define flag${f} BIT(${flags[f]})`)
  .join('\n')}


// XXX For now
#define CHECK_GET_ABREAK(A)	((A) < 020 ? cp->ac[A] : cp->memP[A])
#define CHECK_PUT_ABREAK(A,V)	do {if ((A) < 020) cp->ac[A] = (V); else cp->memP[A] = (V); } while(0)


typedef struct KMContext {
  W36 ac[16];
  W36 pc;
  W36 flags;
  W36 *memP;
  unsigned running: 1;
  unsigned tracePC: 1;
} KMContext;


// XXX This needs nonzero section code added.
static inline W36 pcAndFlags(KMContext *cp) {
  return cp->flags | cp->pc;
}


static inline int nonZeroSection(W36 a) {
  return !!(a & LHMASK);
}
`;


// Optionally `ac` param can be left off and 'ac' will be used. This
// makes call sites simpler.
function getAC(ac) {
  if (ac == undefined) ac = 'ac';
  return `CHECK_GET_ABREAK(${ac})`;
}


// Optionally `ac` param can be left off and 'ac' will be used. This
// makes call sites simpler.
function putAC(ac, value) {

  if (value == undefined) {
    value = ac;
    ac = 'ac';
  }

  return `CHECK_PUT_ABREAK(${ac}, ${value})`;
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
  return `cp->flags |= ${flagsList
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
	if (nonZeroSection(cp->pc)) {

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
/*
'120': `// DMOVE
	D(e,,e+1) -> D(ac,,ac+1)
`,
'121': `// DMOVEN
	-D(e,,e+1) -> D(ac,,ac+1)
`,
'124': `// DMOVEM
	D(ac,,ac+1) -> D(e,,e+1)
`,
'125': `// DMOVNM
	-D(ac,,ac+1) -> D(e,,e+1)
`,
'251': `// BLT
	{cpu.doBLT(ac, e)}
`,
'204': `// MOVS
	(e).s -> (ac)
`,
'205': `// MOVSI
	e,,0 -> (ac)
`,
'206': `// MOVSM
	(ac).s -> (e)
`,
'207': `// MOVSS
	(e).s -> (e)
      		if ac !== 0: (e) -> (ac)
`,
'214': `// MOVM
	|(e)| -> (ac)
`,
'215': `// MOVMI
	0,,e -> (ac)
`,
'216': `// MOVMM
	|(e)| -> (tmp)
		(tmp) -> (e)
		if (tmp) == 400000_000000: 1 -> tr1 ov cy1
		if (tmp) == 0: 1 -> cy0 cy1
`,
'217': `// MOVMS
	|(e)| -> (tmp)
		(tmp) -> (e)
      		if ac !== 0: (tmp) -> (ac)
		if (tmp) == 400000_000000: 1 -> tr1 ov cy1
		if (tmp) == 0: 1 -> cy0 cy1
`,
'052': `// PMOVE
	{cpu.doPMOVE(ac, e)}
`,
'053': `// PMOVEM
	{cpu.doPMOVEM(ac, e)}
`,
'270': `// ADD
	(ac) + (e) -> (ac)
`,
'271': `// ADDI
	(ac) + 0,,e -> (ac)
`,
'272': `// ADDM
	(ac) + (e) -> (e)
`,
'273': `// ADDB
	(ac) + (e) -> (ac) (e)
`,
'274': `// SUB
	(ac) - (e) -> (ac)
`,
'275': `// SUBI
	(ac) - 0,,e -> (ac)
`,
'276': `// SUBM
	(ac) - (e) -> (e)
`,
'277': `// SUBB
	(ac) - (e) -> (ac) (e)
`,
'220': `// IMUL
	(ac) * (e) -> (ac).discardHi
`,
'221': `// IMULI
	(ac) * 0,,e -> (ac).discardHi
`,
'222': `// IMULM
	(ac) * (e) -> (e).discardHi
`,
'223': `// IMULB
	(ac) * (e) -> (ac).discardHi (e).discardHi
`,
'224': `// MUL
	(ac) * (e) -> D(ac,,ac+1)
`,
'225': `// MULI
	(ac) * 0,,e -> D(ac,,ac+1)
`,
'226': `// MULM
	(ac) * (e) -> (e).discardLo
`,
'227': `// MULB
	(ac) * (e) -> (ac,,ac+1) (e).discardLo
`,
'230': `// IDIV
	(ac) / (e) -> (ac).quo (ac+1).rem
`,
'231': `// IDIVI
	(ac) / 0,,e -> (ac).quo (ac+1).rem
`,
'232': `// IDIVM
	(ac) / (e) -> (e).quo
`,
'233': `// IDIVB
	(ac) / (e) -> (ac).quo (ac+1).rem (e).quo
`,
'234': `// DIV
	D(ac,,ac+1) / (e) -> (ac).quo (ac+1).rem
`,
'235': `// DIVI
	D(ac,,ac+1) / 0,,e -> (ac).quo (ac+1).rem
`,
'236': `// DIVM
	D(ac,,ac+1) / (e) -> (e).quo
`,
'237': `// DIVB
	D(ac,,ac+1) / (e) -> (ac).quo (ac+1).rem (e).quo
`,
'114': `// DADD
	D(ac,,ac+1) d+ D(e,,e+1) -> D(ac,,ac+1)
`,
'115': `// DSUB
	D(ac,,ac+1) d- D(e,,e+1) -> D(ac,,ac+1)
`,
'116': `// DMUL
	D(ac,,ac+1) d* D(e,,e+1) -> Q(ac,,ac+3)
`,
'117': `// DDIV
	D(ac,,ac+1) d/ D(e,,e+1) -> Q(ac,,ac+3)
`,
'140': `// FAD
    	(ac) f+ (e) -> (ac)
`,
'141': `// FADL
    	(ac) fl+ (e) -> D(ac,,ac+1)
`,
'142': `// FADM
    	(ac) f+ (e) -> (e)
`,
'143': `// FADB
    	(ac) f+ (e) -> (ac) (e)
`,
'144': `// FADR
    	(ac) fr+ (e) -> (ac)
`,
'145': `// FADRI
    	(ac) fr+ e,,0 -> (ac)
`,
'146': `// FADRM
    	(ac) fr+ (e) -> (e)
`,
'147': `// FADRB
    	(ac) fr+ (e) -> (ac) (e)
`,
'150': `// FSB
	(ac) f- (e) -> (ac)
`,
'151': `// FSBL
	(ac) fl- (e) -> D(ac,,ac+1)
`,
'152': `// FSBM
	(ac) f- (e) -> (e)
`,
'153': `// FSBB
	(ac) f- (e) -> (ac) (e)
`,
'154': `// FSBR
    	(ac) fr- (e) -> (ac)
`,
'155': `// FSBRI
    	(ac) fr- e,,0 -> (ac)
`,
'156': `// FSBRM
    	(ac) fr- (e) -> (e)
`,
'157': `// FSBRB
    	(ac) fr- (e) -> (ac) (e)
`,
'160': `// FMP
	(ac) f* (e) -> (ac)
`,
'161': `// FMPL
	(ac) fl* (e) -> D(ac,,ac+1)
`,
'162': `// FMPM
	(ac) f* (e) -> (e)
`,
'163': `// FMPB
	(ac) f* (e) -> (ac) (e)
`,
'164': `// FMPR
    	(ac) fr* (e) -> (ac)
`,
'165': `// FMPRI
    	(ac) fr* e,,0 -> (ac)
`,
'166': `// FMPRM
    	(ac) fr* (e) -> (e)
`,
'167': `// FMPRB
    	(ac) fr* (e) -> (ac) (e)
`,
'170': `// FDV
	(ac) f/ (e) -> (ac)
`,
'171': `// FDVL
	(ac) fl/ (e) -> D(ac,,ac+1)
`,
'172': `// FDVM
	(ac) f/ (e) -> (e)
`,
'173': `// FDVB
	(ac) f/ (e) -> (ac) (e)
`,
'174': `// FDVR
    	(ac) fr/ (e) -> (ac)
`,
'175': `// FDVRI
    	(ac) fr/ e,,0 -> (ac)
`,
'176': `// FDVRM
    	(ac) fr/ (e) -> (e)
`,
'177': `// FDVRB
    	(ac) fr/ (e) -> (ac) (e)
`,
'110': `// DFAD
	D(ac,,ac+1) d+ D(e,,e+1) -> D(ac,,ac+1)
`,
'111': `// DFSB
	D(ac,,ac+1) d- D(e,,e+1) -> D(ac,,ac+1)
`,
'112': `// DFMP
	D(ac,,ac+1) d* D(e,,e+1) -> D(ac,,ac+1)
`,
'113': `// DFDV
	D(ac,,ac+1) d/ D(e,,e+1) -> D(ac,,ac+1)
`,
'132': `// FSC
	(ac) f<< e -> (ac)
`,
'031': `// GFSC
	D(ac,,ac+1) f<< e -> D(ac,,ac+1)
`,
'127': `// FLTR
	(e).ifr -> (ac)
`,
'030': `// GFLTR
	(e).ifrD -> D(ac,,ac+1)
`,
'027': `// DGFLTR
	(e,,e+1).DifrD -> D(ac,,ac+1)
`,
'122': `// FIX
	(e).fix -> (ac)
`,
'126': `// FIXR
	(e).fixR -> (ac)
`,
'024': `// GFIX
	(e,,e+1).Dfix -> (ac)
`,
'026': `// GFIXR
	(e,,e+1).DfixR -> (ac)
`,
'023': `// GDFIX
	(e,,e+1).DfixD -> D(ac,,ac+1)
`,
'025': `// GDFIXR
	(e,,e+1).DfixRD -> D(ac,,ac+1)
`,
'021': `// GSNGL
	(e,,e+1).sngl -> (ac)
`,
'022': `// GDBLE
	(e).dble -> D(ac,,ac+1)
`,
'130': `// UFA
	(ac) fu+ (e) -> (ac+1)
`,
'131': `// DFN
	{cpu.doDFN(ac, e)}
`,
'400': `// SETZ
	0 -> (ac)
`,
'401': `// SETZI
	0 -> (ac)
`,
'402': `// SETZM
	0 -> (e)
`,
'403': `// SETZB
	0 -> (ac) (e)
`,
'474': `// SETO
	777777777777 -> (ac)
`,
'475': `// SETOI
	777777777777 -> (ac)
`,
'476': `// SETOM
	777777777777 -> (e)
`,
'477': `// SETOB
	777777777777 -> (ac) (e)
`,
'424': `// SETA
	{ }	// (ac) -> (ac)
`,
'425': `// SETAI
	{ }	// (ac) -> (ac)
`,
'426': `// SETAM
	(ac) -> (e)
`,
'427': `// SETAB
	(ac) -> (e)
`,
'450': `// SETCA
	~(ac) -> (ac)
`,
'451': `// SETCAI
	~(ac) -> (ac)
`,
'452': `// SETCAM
	~(ac) -> (e)
`,
'453': `// SETCAB
	~(ac) -> (ac) (e)
`,
'414': `// SETM
	(e) -> (ac)
`,
'416': `// SETMM
	(e) -> (e)// No-op
`,
'417': `// SETMB
	(e) -> (ac) (e)
`,
'460': `// SETCM
	~(e) -> (ac)
`,
'461': `// SETCMI
	~[0,,e] -> (ac)
`,
'462': `// SETCMM
	~(e) -> (e)
`,
'463': `// SETCMB
	~(e) -> (ac) (e)
`,
'404': `// AND
	(ac) & (e) -> (ac)
`,
'405': `// ANDI
	(ac) & 0,,e -> (ac)
`,
'406': `// ANDM
	(ac) & (e) -> (e)
`,
'407': `// ANDB
	(ac) & (e) -> (ac) (e)
`,
'410': `// ANDCA
	~(ac) & (e) -> (ac)
`,
'411': `// ANDCAI
	~(ac) & 0,,e -> (ac)
`,
'412': `// ANDCAM
	~(ac) & (e) -> (e)
`,
'413': `// ANDCAB
	~(ac) & (e) -> (ac) (e)
`,
'420': `// ANDCM
	(ac) & ~(e) -> (ac)
`,
'421': `// ANDCMI
	(ac) & ~[0,,e] -> (ac)
`,
'422': `// ANDCMM
	(ac) & ~(e) -> (e)
`,
'423': `// ANDCMB
	(ac) & ~(e) -> (ac) (e)
`,
'440': `// ANDCB
	~(ac) & ~(e) -> (ac)
`,
'441': `// ANDCBI
	~(ac) & ~[0,,e] -> (ac)
`,
'442': `// ANDCBM
	~(ac) & ~(e) -> (e)
`,
'443': `// ANDCBB
	~(ac) & ~(e) -> (ac) (e)
`,
'434': `// IOR
	(ac) | (e) -> (ac)
`,
'435': `// IORI
	(ac) | 0,,e -> (ac)
`,
'436': `// IORM
	(ac) | (e) -> (e)
`,
'437': `// IORB
	(ac) | (e) -> (ac) (e)
`,
'454': `// ORCA
	~(ac) | (e) -> (ac)
`,
'455': `// ORCAI
	~(ac) | 0,,e -> (ac)
`,
'456': `// ORCAM
	~(ac) | (e) -> (e)
`,
'457': `// ORCAB
	~(ac) | (e) -> (ac) (e)
`,
'464': `// ORCM
	(ac) | ~(e) -> (ac)
`,
'465': `// ORCMI
	(ac) | ~[0,,e] -> (ac)
`,
'466': `// ORCMM
	(ac) | ~(e) -> (e)
`,
'467': `// ORCMB
	(ac) | ~(e) -> (ac) (e)
`,
'470': `// ORCB
	~(ac) | ~(e) -> (ac)
`,
'471': `// ORCBI
	~(ac) | ~[0,,e] -> (ac)
`,
'472': `// ORCBM
	~(ac) | ~(e) -> (e)
`,
'473': `// ORCBB
	~(ac) | ~(e) -> (ac) (e)
`,
'430': `// XOR
	(ac) ^ (e) -> (ac)
`,
'431': `// XORI
	(ac) ^ 0,,e -> (ac)
`,
'432': `// XORM
	(ac) ^ (e) -> (e)
`,
'433': `// XORB
	(ac) ^ (e) -> (ac) (e)
`,
'444': `// EQV
	(ac) >< (e) -> (ac)
`,
'445': `// EQVI
	(ac) >< 0,,e -> (ac)
`,
'446': `// EQVM
	(ac) >< (e) -> (e)
`,
'447': `// EQVB
	(ac) >< (e) -> (ac) (e)
`,
'240': `// ASH
	(ac) <<a e -> (ac)
`,
'241': `// ROT
	(ac) <<r e -> (ac)
`,
'242': `// LSH
	(ac) <<u e -> (ac)
`,
'244': `// ASHC
	D(ac,,ac+1) <<a e -> D(ac,,ac+1)
`,
'245': `// ROTC
	D(ac,,ac+1) <<r e -> D(ac,,ac+1)
`,
'246': `// LSHC
	D(ac,,ac+1) <<u e -> D(ac,,ac+1)
`,
'252': `// AOBJP
	(ac).l + 1 -> (ac).l
		(ac).r + 1 -> (ac).r
      		if (ac) >= 0: e -> (pc)
`,
'253': `// AOBJN
	(ac).l + 1 -> (ac).l
		(ac).r + 1 -> (ac).r
      		if (ac) < 0: e -> (pc)
`,
'300': `// CAI
	{ }
`,
'301': `// CAIL
	if (ac) < e: skip
`,
'302': `// CAIE
	if (ac) === e: skip
`,
'303': `// CAILE
	if (ac) <= e: skip
`,
'304': `// CAIA
	skip
`,
'305': `// CAIGE
	if (ac) >= e: skip
`,
'306': `// CAIN
	if (ac) !== e: skip
`,
'307': `// CAIG
	if (ac) > e: skip
`,
'310': `// CAM
	{ }
`,
'311': `// CAML
	if (ac) < (e): skip
`,
'312': `// CAME
	if (ac) === (e): skip
`,
'313': `// CAMLE
	if (ac) <= (e): skip
`,
'314': `// CAMA
	skip
`,
'315': `// CAMGE
	if (ac) >= (e): skip
`,
'316': `// CAMN
	if (ac) !== (e): skip
`,
'317': `// CAMG
	if (ac) > (e): skip
`,
'320': `// JUMP
	{ }
`,
'321': `// JUMPL
	if (ac) < 0: e -> (pc)
`,
'322': `// JUMPE
	if (ac) === 0: e -> (pc)
`,
'323': `// JUMPLE
	if (ac) <= 0: e -> (pc)
`,
'324': `// JUMPA
	e -> (pc)
`,
'325': `// JUMPGE
	if (ac) >= 0: e -> (pc)
`,
'326': `// JUMPN
	if (ac) !== 0: e -> (pc)
`,
'327': `// JUMPG
	if (ac) > 0: e -> (pc)
`,
'330': `// SKIP
	if ac !== 0: (e) -> (ac)
`,
'331': `// SKIPL
	if ac !== 0: (e) -> (ac)
      		if (e) < 0: skip
`,
'332': `// SKIPE
	if ac !== 0: (e) -> (ac)
      		if (e) === 0: skip
`,
'333': `// SKIPLE
	if ac !== 0: (e) -> (ac)
      		if (e) <= 0: skip
`,
'334': `// SKIPA
	if ac !== 0: (e) -> (ac)
      		skip
`,
'335': `// SKIPGE
	if ac !== 0: (e) -> (ac)
      		if (e) >= 0: skip
`,
'336': `// SKIPN
	if ac !== 0: (e) -> (ac)
      		if (e) !== 0: skip
`,
'337': `// SKIPG
	if ac !== 0: (e) -> (ac)
      		if (e) > 0: skip
`,
'340': `// AOJ
	(ac).aox -> (ac)
`,
'341': `// AOJL
	(ac).aox -> (ac)
     		if (ac) < 0: e -> pc
`,
'342': `// AOJE
	(ac).aox -> (ac)
     		if (ac) === 0: e -> pc
`,
'343': `// AOJLE
	(ac).aox -> (ac)
     		if (ac) <= 0: e -> pc
`,
'344': `// AOJA
	(ac).aox -> (ac)
     		e -> pc
`,
'345': `// AOJGE
	(ac).aox -> (ac)
     		if (ac) >= 0: e -> pc
`,
'346': `// AOJN
	(ac).aox -> (ac)
		if (ac) !== 0: e -> pc
`,
'347': `// AOJG
	(ac).aox -> (ac)
     		if (ac) > 0: e -> pc
`,
'360': `// SOJ
	(ac).sox -> (ac)
`,
'361': `// SOJL
	(ac).sox -> (ac)
     		if (ac) < 0: e -> pc
`,
'362': `// SOJE
	(ac).sox -> (ac)
     		if (ac) === 0: e -> pc
`,
'363': `// SOJLE
	(ac).sox -> (ac)
     		if (ac) <= 0: e -> pc
`,
'364': `// SOJA
	(ac).sox -> (ac)
     		e -> pc
`,
'365': `// SOJGE
	(ac).sox -> (ac)
     		if (ac) >= 0: e -> pc
`,
'366': `// SOJN
	(ac).sox -> (ac)
     		if (ac) !== 0: e -> pc
`,
'367': `// SOJG
	(ac).sox -> (ac)
     		if (ac) > 0: e -> pc
`,
'350': `// AOS
	(e).aox -> (e)
    		if ac !== 0: (e) -> (ac)
`,
'351': `// AOSL
	(e).aox -> (e)
     		if ac !== 0: (e) -> (ac)
		if (e) < 0: skip
`,
'352': `// AOSE
	(e).aox -> (e)
     		if ac !== 0: (e) -> (ac)
		if (e) === 0: skip
`,
'353': `// AOSLE
	(e).aox -> (e)
     		if ac !== 0: (e) -> (ac)
		if (e) <= 0: skip
`,
'354': `// AOSA
	(e).aox -> (e)
     		if ac !== 0: (e) -> (ac)
		skip
`,
'355': `// AOSGE
	(e).aox -> (e)
     		if ac !== 0: (e) -> (ac)
		if (e) >= 0: skip
`,
'356': `// AOSN
	(e).aox -> (e)
     		if ac !== 0: (e) -> (ac)
		if (e) !== 0: skip
`,
'357': `// AOSG
	(e).aox -> (e)
     		if ac !== 0: (e) -> (ac)
		if (e) > 0: skip
`,
'370': `// SOS
	(e).sox -> (e)
    		if ac !== 0: (e) -> (ac)
`,
'371': `// SOSL
	(e).sox -> (e)
     		if ac !== 0: (e) -> (ac)
		if (e) < 0: skip
`,
'372': `// SOSE
	(e).sox -> (e)
     		if ac !== 0: (e) -> (ac)
		if (e) === 0: skip
`,
'373': `// SOSLE
	(e).sox -> (e)
     		if ac !== 0: (e) -> (ac)
		if (e) <= 0: skip
`,
'374': `// SOSA
	(e).sox -> (e)
     		if ac !== 0: (e) -> (ac)
		skip
`,
'375': `// SOSGE
	(e).sox -> (e)
     		if ac !== 0: (e) -> (ac)
		if (e) >= 0: skip
`,
'376': `// SOSN
	(e).sox -> (e)
     		if ac !== 0: (e) -> (ac)
		if (e) !== 0: skip
`,
'377': `// SOSG
	(e).sox -> (e)
     		if ac !== 0: (e) -> (ac)
		if (e) > 0: skip
`,
'600': `// TRN
	{ }
`,
'601': `// TLN
	{ }
`,
'602': `// TRNE
	if (ac).r & e === 0: skip
`,
'603': `// TLNE
	if (ac).l & e === 0: skip
`,
'604': `// TRNA
	skip
`,
'605': `// TLNA
	skip
`,
'606': `// TRNN
	if (ac).r & e !== 0: skip
`,
'607': `// TLNN
	if (ac).l & e !== 0: skip
`,
'620': `// TRZ
	(ac).r & ~e -> (ac).r
`,
'621': `// TLZ
	(ac).l & ~e -> (ac).l
`,
'622': `// TRZE
	if (ac).r & e === 0: skip
		(ac).r & ~e -> (ac).r
`,
'623': `// TLZE
	if (ac).l & e === 0: skip
		(ac).l & ~e -> (ac).l
`,
'624': `// TRZA
	skip
		(ac).r & ~e -> (ac).r
`,
'625': `// TLZA
	skip
		(ac).l & ~e -> (ac).l
`,
'626': `// TRZN
	if (ac).r & e !== 0: skip
		(ac).r & ~e -> (ac).r
`,
'627': `// TLZN
	if (ac).l & e !== 0: skip
		(ac).l & ~e -> (ac).l
`,
'640': `// TRC
	(ac).r ^ e.r -> (ac).r
`,
'641': `// TLC
	(ac).l ^ e.r -> (ac).l
`,
'642': `// TRCE
	if (ac).r & e === 0: skip
		(ac).r ^ e.r -> (ac).r
`,
'643': `// TLCE
	if (ac).l & e === 0: skip
		(ac).l ^ e.r -> (ac).l
`,
'644': `// TRCA
	skip
		(ac).r ^ e.r -> (ac).r
`,
'645': `// TLCA
	skip
		(ac).l ^ e.r -> (ac).l
`,
'646': `// TRCN
	if (ac).r & e !== 0: skip
		(ac).r ^ e.r -> (ac).r
`,
'647': `// TLCN
	if (ac).l & e !== 0: skip
		(ac).l ^ e.r -> (ac).l
`,
'660': `// TRO
	(ac).r | e -> (ac).r
`,
'661': `// TLO
	(ac).l | e -> (ac).l
`,
'662': `// TROE
	if (ac).r & e === 0: skip
		(ac).r | e -> (ac).r
`,
'663': `// TLOE
	if (ac).l & e === 0: skip
		(ac).l | e -> (ac).l
`,
'664': `// TROA
	skip
		(ac).r | e -> (ac).r
`,
'665': `// TLOA
	skip
		(ac).l | e -> (ac).l
`,
'666': `// TRON
	if (ac).r & e !== 0: skip
		(ac).r | e -> (ac).r
`,
'667': `// TLON
	if (ac).l & e !== 0: skip
		(ac).l | e -> (ac).l
`,
'610': `// TDN
	{ }
`,
'611': `// TSN
	{ }
`,
'612': `// TDNE
	if (ac) & (e) === 0: skip
`,
'613': `// TSNE
	if (ac) & (e).s === 0: skip
`,
'614': `// TDNA
	skip
`,
'615': `// TSNA
	skip
`,
'616': `// TDNN
	if (ac) & (e) !== 0: skip
`,
'617': `// TSNN
	if (ac) & (e).s !== 0: skip
`,
'630': `// TDZ
	(ac) & ~(e) -> (ac)
`,
'631': `// TSZ
	(ac) & ~(e).s -> (ac)
`,
'632': `// TDZE
	if (ac) & (e) === 0: skip
		(ac) & ~(e) -> (ac)
`,
'633': `// TSZE
	if (ac) & (e).s === 0: skip
		(ac) & ~(e).s -> (ac)
`,
'634': `// TDZA
	skip
		(ac) & ~(e) -> (ac)
`,
'635': `// TSZA
	skip
		(ac) & ~(e).s -> (ac)
`,
'636': `// TDZN
	if (ac) & (e) !== 0: skip
		(ac) & ~(e) -> (ac)
`,
'637': `// TSZN
	if (ac) & (e).s !== 0: skip
		(ac) & ~(e).s -> (ac)
`,
'650': `// TDC
	(ac) ^ (e) -> (ac)
`,
'651': `// TSC
	(ac) ^ (e).s -> (ac)
`,
'652': `// TDCE
	if (ac) & (e) === 0: skip
		(ac) ^ (e) -> (ac)
`,
'653': `// TSCE
	if (ac) & (e).s === 0: skip
		(ac) ^ (e).s -> (ac)
`,
'654': `// TDCA
	skip
		(ac) ^ (e) -> (ac)
`,
'655': `// TSCA
	skip
		(ac) ^ (e).s -> (ac)
`,
'656': `// TDCN
	if (ac) & (e) !== 0: skip
		(ac) ^ (e) -> (ac)
`,
'657': `// TSCN
	if (ac) & (e).s !== 0: skip
		(ac) ^ (e).s -> (ac)
`,
'670': `// TDO
	(ac) | (e) -> (ac)
`,
'671': `// TSO
	(ac) | (e).s -> (ac)
`,
'672': `// TDOE
	if (ac) & (e) === 0: skip
		(ac) | (e) -> (ac)
`,
'673': `// TSOE
	if (ac) & (e).s === 0: skip
		(ac) | (e).s -> (ac)
`,
'674': `// TDOA
	skip
		(ac) | (e) -> (ac)
`,
'675': `// TSOA
	skip
		(ac) | (e).s -> (ac)
`,
'676': `// TDON
	if (ac) & (e) !== 0: skip
		(ac) | (e) -> (ac)
`,
'677': `// TSON
	if (ac) & (e).s !== 0: skip
		(ac) | (e).s -> (ac)
`,
'500': `// HLL
	(e).l -> (ac).l
`,
'501': `// XHLLI
	if (pc).l === 0: 0 -> (ac).l
		if (pc).l !== 0: e.l -> (ac).l
`,
'502': `// HLLM
	(ac).l -> (e).l
`,
'503': `// HLLS
	if ac !== 0: (e) -> (ac)
`,
'510': `// HLLZ
	(e).l,,0 -> (ac)
`,
'511': `// HLLZI
	0 -> (ac)
`,
'512': `// HLLZM
	(ac).l,,0 -> (e)
`,
'513': `// HLLZS
	0 -> (e).r
		if ac !== 0: (e) -> (ac)
`,
'530': `// HLLE
	(e).l,,(e).lextend -> (ac)
`,
'531': `// HLLEI
	0 -> (ac)
`,
'532': `// HLLEM
	(ac).l,,(ac).lextend -> (e)
`,
'533': `// HLLES
	(e).lextend -> (e).r
		if ac !== 0: (e) -> (ac)
`,
'520': `// HLLO
	(e).l,,777777 -> (ac)
`,
'521': `// HLLOI
	0,,777777 -> (ac)
`,
'522': `// HLLOM
	(ac).l,,777777 -> (e)
`,
'523': `// HLLOS
	777777 -> (e).r
		if ac !== 0: (e) -> (ac)
`,
'544': `// HLR
	(e).l -> (ac).r
`,
'545': `// HLRI
	0 -> (ac).r
`,
'546': `// HLRM
	(ac).l -> (e).r
`,
'547': `// HLRS
	(e).l -> (e).r
		if ac !== 0: (e) -> (ac)
`,
'554': `// HLRZ
	0,,(e).l -> (ac)
`,
'555': `// HLRZI
	0 -> (ac)
`,
'556': `// HLRZM
	0,,(ac).l -> (e)
`,
'557': `// HLRZS
	0,,(e).l -> (e)
		if ac !== 0: (e) -> (ac)
`,
'564': `// HLRO
	777777,,(e).l -> (ac)
`,
'565': `// HLROI
	777777,,0 -> (ac)
`,
'566': `// HLROM
	777777,,(ac).l -> (e)
`,
'567': `// HLROS
	777777,,(e).l -> (e)
		if ac !== 0: (e) -> (ac)
`,
'574': `// HLRE
	(e).lextend,,(e).l -> (ac)
`,
'575': `// HLREI
	0 -> (ac)
`,
'576': `// HLREM
	(ac).lextend,,(ac).l -> (e)
`,
'577': `// HLRES
	(e).lextend,,(e).l -> (e)
		if ac !== 0: (e) -> (ac)
`,
'540': `// HRR
	(e).r -> (ac).r
`,
'541': `// HRRI
	e -> (ac).r
`,
'542': `// HRRM
	(ac).r -> (e).r
`,
'543': `// HRRS
	if ac !== 0: (e) -> (ac)
`,
'550': `// HRRZ
	0,,(e).r -> (ac)
`,
'551': `// HRRZI
	0,,e -> (ac)
`,
'552': `// HRRZM
	0,,(ac).r -> (e)
`,
'553': `// HRRZS
	0 -> (e).l
		if ac !== 0: (e) -> (ac)
`,
'560': `// HRRO
	777777,,(e).r -> (ac)
`,
'561': `// HRROI
	777777,,e -> (ac)
`,
'562': `// HRROM
	777777,,(ac).r -> (e)
`,
'563': `// HRROS
	777777 -> (e).l
		if ac !== 0: (e) -> (ac)
`,
'570': `// HRRE
	(e).rextend,,(e).r -> (ac)
`,
'571': `// HRREI
	e.rextend,,e -> (ac)
`,
'572': `// HRREM
	(ac).rextend,,(ac).r -> (e)
`,
'573': `// HRRES
	(e).rextend -> (e).l
		if ac !== 0: (e) -> (ac)
`,
'504': `// HRL
	(e).r -> (ac).l
`,
'505': `// HRLI
	e -> (ac).l
`,
'506': `// HRLM
	(ac).r -> (e).l
`,
'507': `// HRLS
	(e).r -> (e).l
		if ac !== 0: (e) -> (ac)
`,
'514': `// HRLZ
	(e).r,,0 -> (ac)
`,
'515': `// HRLZI
	e,,0 -> (ac)
`,
'516': `// HRLZM
	(ac).r,,0 -> (e)
`,
'517': `// HRLZS
	(e).r,,0 -> (e)
		if ac !== 0: (e) -> (ac)
`,
'524': `// HRLO
	(e).r,,777777 -> (ac)
`,
'525': `// HRLOI
	e,,777777 -> (ac)
`,
'526': `// HRLOM
	(ac).r,,777777 -> (e)
`,
'527': `// HRLOS
	(e).r,,777777 -> (e)
		if ac !== 0: (e) -> (ac)
`,
'534': `// HRLE
	(e).r,,(e).rextend -> (ac)
`,
'535': `// HRLEI
	e.r,,e.rextend -> (ac)
`,
'536': `// HRLEM
	(ac).r,,(ac).rextend -> (e)
`,
'537': `// HRLES
	(e).r,,(e).rextend -> (e)
		if ac !== 0: (e) -> (ac)
`,
'25600': `// XCT
	{cpu.executeWord(cpu.fetchWord(e))}
`,
'243': `// JFFO
	if (ac) === 0: 0 -> (ac+1)
		if (ac) !== 0: (ac).jffo -> (ac+1)
		if (ac) !== 0: e -> (pc)
`,
'255': `// JFCL
	{cpu.doJFCL(ac, e)}
`,
'254': `// JRST
	{cpu.doJRST(e)}
`,
'264': `// JSR
	{cpu.doJSx(ac, e, true)}
`,
'265': `// JSP
	{cpu.doJSx(ac, e, false)}
`,
'266': `// JSA
	(ac) -> (e)
		e.r -> (ac).l
		(pc).r -> (ac).r
		e + 1 -> (pc)
`,
'267': `// JRA
	{cpu.doJRA(ac, e)}
`,
'257': `// MAP
	{cpu.doMAP(ac, e)}
`,
'256nn': `// PXCT
	{cpu.doPXCT(ac, e)}
`,
'261': `// PUSH
	{cpu.doPUSH(ac, e)}
`,
'262': `// POP
	{cpu.doPOP(ac, e)}
`,
'260': `// PUSHJ
	{cpu.doPUSHJ(ac, e)}
`,
'263': `// POPJ
	{cpu.doPOPJ(ac, e)}
`,
'105': `// ADJSP
	{cpu.doADJSP(ac, e)}
`,
'13300': `// IBP
	{cpu.doIBP(ac, e)}
`,
'133nn': `// ADJBP
	{cpu.doADJBP(ac, e)}
`,
'135': `// LDB
	{cpu.doLDB(ac, e)}
`,
'137': `// DPB
	{cpu.doDPB(ac, e)}
`,
'134': `// ILDB
	{cpu.doIBP(ac, e); cpu.doLDB(ac, e)}
`,
'136': `// IDPB
	{cpu.doIBP(ac, e); cpu.doDPB(ac, e)}

`,
'001': `// LUUO
	{cpu.doLUUO(ac, e)}
`,
'104': `// JSYS
	{cpu.doMUUO(ac, e)}
`,
'123': `// EXTEND
	{cpu.doEXTEND(ac, e)}
`,
'70144': `// SWPIA
	{cpu.doSWPIA(e)}
`,
'70164': `// SWPIO
	{cpu.doSWPIO(e)}
`,
'70150': `// SWPVA
	{cpu.doSWPVA(e)}
`,
'70170': `// SWPVO
	{cpu.doSWPVO(e)}
`,
'70154': `// SWPUA
	{cpu.doSWPUA(e)}
`,
'70174': `// SWPUO
	{cpu.doSWPUO(e)}
`,
'70010': `// WRFIL
	{cpu.doWRFIL(e)}
`,
'70000': `// APRID
	{cpu.doAPRID(e)}
`,
'70110': `// CLRPT
	{cpu.doCLRPT(e)}
`,
'70260': `// WRTIME
	{cpu.doWRTIME(e)}
`,
'70204': `// RDTIME
	{cpu.doRDTIME(e)}
`,
'70244': `// RDEACT
	{cpu.doRDEACT(e)}
`,
'70240': `// RDMACT
	{cpu.doRDMACT(e)}
`,
'70210': `// WRPAE
	{cpu.doWRPAE(e)}
`,
'70200': `// RDPERF
	{cpu.doRDPERF(e)}
`,
'70040': `// RDERA
	{cpu.doRDERA(e)}
`,
'70050': `// SBDIAG
	{cpu.doSBDIAG(e)}

`,
'7dd00': `// BLKI
	{cpu.dispatchBLKI(e)}
`,
'7dd04': `// DATAI
	{cpu.dispatchDATAI(e)}
`,
'7dd10': `// BLKO
	{cpu.dispatchBLKO(e)}
`,
'7dd14': `// DATAO
	{cpu.dispatchDATAO(e)}
`,
'7dd20': `// CONO
	{cpu.dispatchCONO(e)}
`,
'7dd24': `// CONI
	{cpu.dispatchCONI(e)}
`,
'7dd30': `// CONSZ
	{cpu.dispatchCONSZ(e)}
`,
'7dd34': `// CONSO
	{cpu.dispatchCONSO(e)}
`,
*/
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

static void emulate(KMContext *cp) {
  char octbuf[64];
  char octbuf2[64];

  do {
    // XXX Put interrupt, trap, HALT, etc. handling here...
    W36 iw = cp->memP[cp->pc];

    W36 ea;
    int eaIsLocal = 1;		// XXX THIS IS TEMPORARY UNTIL WE HAVE EXTENDED STUFF
    W36 tmp;
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

      ea = x ? RH(cp->ac[x] + y) : y;

      if (i) eaw = cp->memP[ea];
    } while (i);

    if (cp->tracePC) {
      char pcBuf[64];
      char eaBuf[64];
      char daBuf[256];

      DisassembleToString(iw, daBuf);
      fprintf(stderr, "%s: [ea=%s] %s\\n", oct36(pcBuf, cp->pc), oct36(eaBuf, ea), daBuf);
    }

    switch (op) {
    ${Object.keys(kl10Instructions)
	    .map(k => `\
	case ${k}: {
${kl10Instructions[k]}
	  break; };`).join('\n')}
    }

    ++cp->pc;

  SKIP_PC_INCR: ;
  } while (cp->running);
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

  static KMContext context;
  memset(&context, 0, sizeof(context));
  context.pc = startAddr;
  context.memP = memory;
  context.tracePC = 1;
  context.running = 1;
  emulate(&context);
}`);
