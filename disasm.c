// Disassemble a KL10 instruction word into its component values,
// including mnemonic, I, X, and Y.
//
// TODO: Support I/O instructions properly. They aren't at all correct right now.
#include <stdint.h>
#include "kl10.h"

typedef struct SpecialMnemonic {
  uint32_t bits;
  char mnemonic[8];
} SpecialMnemonic;


static const char mnemonics[512][8] = {
  // Full Word Movement
  [0250] "EXCH",
  [0200] "MOVE",
  [0201] "MOVEI",
  [0202] "MOVEM",
  [0203] "MOVES",
  [0210] "MOVN",
  [0211] "MOVNI",
  [0212] "MOVNM",
  [0213] "MOVNS",
  [0415] "XMOVEI",
  [0120] "DMOVE",
  [0121] "DMOVEN",
  [0124] "DMOVEM",
  [0125] "DMOVNM",
  [0251] "BLT",
  [0204] "MOVS",
  [0205] "MOVSI",
  [0206] "MOVSM",
  [0207] "MOVSS",
  [0214] "MOVM",
  [0215] "MOVMI",
  [0216] "MOVMM",
  [0217] "MOVMS",
  [0052] "PMOVE",
  [0053] "PMOVEM",

  // Fixed Point Arithmetic
  [0270] "ADD",
  [0271] "ADDI",
  [0272] "ADDM",
  [0273] "ADDB",
  [0274] "SUB",
  [0275] "SUBI",
  [0276] "SUBM",
  [0277] "SUBB",
  [0220] "IMUL",
  [0221] "IMULI",
  [0222] "IMULM",
  [0223] "IMULB",
  [0224] "MUL",
  [0225] "MULI",
  [0226] "MULM",
  [0227] "MULB",
  [0230] "IDIV",
  [0231] "IDIVI",
  [0232] "IDIVM",
  [0233] "IDIVB",
  [0234] "DIV",
  [0235] "DIVI",
  [0236] "DIVM",
  [0237] "DIVB",
  [0114] "DADD",
  [0115] "DSUB",
  [0116] "DMUL",
  [0117] "DDIV",

  // Floating Point Arithmetic
  [0140] "FAD",
  [0141] "FADL",
  [0142] "FADM",
  [0143] "FADB",
  [0144] "FADR",
  [0145] "FADRI",
  [0146] "FADRM",
  [0147] "FADRB",
  [0150] "FSB",
  [0151] "FSBL",
  [0152] "FSBM",
  [0153] "FSBB",
  [0154] "FSBR",
  [0155] "FSBRI",
  [0156] "FSBRM",
  [0157] "FSBRB",
  [0160] "FMP",
  [0161] "FMPL",
  [0162] "FMPM",
  [0163] "FMPB",
  [0164] "FMPR",
  [0165] "FMPRI",
  [0166] "FMPRM",
  [0167] "FMPRB",
  [0170] "FDV",
  [0171] "FDVL",
  [0172] "FDVM",
  [0173] "FDVB",
  [0174] "FDVR",
  [0175] "FDVRI",
  [0176] "FDVRM",
  [0177] "FDVRB",
  [0110] "DFAD",
  [0111] "DFSB",
  [0112] "DFMP",
  [0113] "DFDV",
  [0132] "FSC",
  [0031] "GFSC",
  [0127] "FLTR",
  [0030] "GFLTR",
  [0027] "DGFLTR",
  [0122] "FIX",
  [0126] "FIXR",
  [0024] "GFIX",
  [0026] "GFIXR",
  [0023] "GDFIX",
  [0025] "GDFIXR",
  [0021] "GSNGL",
  [0022] "GDBLE",
  [0130] "UFA",
  [0131] "DFN",

  // Boolean
  [0400] "SETZ",
  [0401] "SETZI",
  [0402] "SETZM",
  [0403] "SETZB",
  [0474] "SETO",
  [0475] "SETOI",
  [0476] "SETOM",
  [0477] "SETOB",
  [0424] "SETA",
  [0425] "SETAI",
  [0426] "SETAM",
  [0427] "SETAB",
  [0450] "SETCA",
  [0451] "SETCAI",
  [0452] "SETCAM",
  [0453] "SETCAB",
  [0414] "SETM",
  [0416] "SETMM",
  [0417] "SETMB",
  [0460] "SETCM",
  [0461] "SETCMI",
  [0462] "SETCMM",
  [0463] "SETCMB",
  [0404] "AND",
  [0405] "ANDI",
  [0406] "ANDM",
  [0407] "ANDB",
  [0410] "ANDCA",
  [0411] "ANDCAI",
  [0412] "ANDCAM",
  [0413] "ANDCAB",
  [0420] "ANDCM",
  [0421] "ANDCMI",
  [0422] "ANDCMM",
  [0423] "ANDCMB",
  [0440] "ANDCB",
  [0441] "ANDCBI",
  [0442] "ANDCBM",
  [0443] "ANDCBB",
  [0434] "IOR",
  [0435] "IORI",
  [0436] "IORM",
  [0437] "IORB",
  [0454] "ORCA",
  [0455] "ORCAI",
  [0456] "ORCAM",
  [0457] "ORCAB",
  [0464] "ORCM",
  [0465] "ORCMI",
  [0466] "ORCMM",
  [0467] "ORCMB",
  [0470] "ORCB",
  [0471] "ORCBI",
  [0472] "ORCBM",
  [0473] "ORCBB",
  [0430] "XOR",
  [0431] "XORI",
  [0432] "XORM",
  [0433] "XORB",
  [0444] "EQV",
  [0445] "EQVI",
  [0446] "EQVM",
  [0447] "EQVB",

  // Shift and Rotate
  [0240] "ASH",
  [0241] "ROT",
  [0242] "LSH",
  [0244] "ASHC",
  [0245] "ROTC",
  [0246] "LSHC",

  // Arithmetic Testing
  [0252] "AOBJP",
  [0253] "AOBJN",
  [0300] "CAI",
  [0301] "CAIL",
  [0302] "CAIE",
  [0303] "CAILE",
  [0304] "CAIA",
  [0305] "CAIGE",
  [0306] "CAIN",
  [0307] "CAIG",
  [0310] "CAM",
  [0311] "CAML",
  [0312] "CAME",
  [0313] "CAMLE",
  [0314] "CAMA",
  [0315] "CAMGE",
  [0316] "CAMN",
  [0317] "CAMG",
  [0320] "JUMP",
  [0321] "JUMPL",
  [0322] "JUMPE",
  [0323] "JUMPLE",
  [0324] "JUMPA",
  [0325] "JUMPGE",
  [0326] "JUMPN",
  [0327] "JUMPG",
  [0330] "SKIP",
  [0331] "SKIPL",
  [0332] "SKIPE",
  [0333] "SKIPLE",
  [0334] "SKIPA",
  [0335] "SKIPGE",
  [0336] "SKIPN",
  [0337] "SKIPG",
  [0340] "AOJ",
  [0341] "AOJL",
  [0342] "AOJE",
  [0343] "AOJLE",
  [0344] "AOJA",
  [0345] "AOJGE",
  [0346] "AOJN",
  [0347] "AOJG",
  [0360] "SOJ",
  [0361] "SOJL",
  [0362] "SOJE",
  [0363] "SOJLE",
  [0364] "SOJA",
  [0365] "SOJGE",
  [0366] "SOJN",
  [0367] "SOJG",
  [0350] "AOS",
  [0351] "AOSL",
  [0352] "AOSE",
  [0353] "AOSLE",
  [0354] "AOSA",
  [0355] "AOSGE",
  [0356] "AOSN",
  [0357] "AOSG",
  [0370] "SOS",
  [0371] "SOSL",
  [0372] "SOSE",
  [0373] "SOSLE",
  [0374] "SOSA",
  [0375] "SOSGE",
  [0376] "SOSN",
  [0377] "SOSG",

  // Logical Testing and Modification
  [0600] "TRN",
  [0601] "TLN",
  [0602] "TRNE",
  [0603] "TLNE",
  [0604] "TRNA",
  [0605] "TLNA",
  [0606] "TRNN",
  [0607] "TLNN",
  [0620] "TRZ",
  [0621] "TLZ",
  [0622] "TRZE",
  [0623] "TLZE",
  [0624] "TRZA",
  [0625] "TLZA",
  [0626] "TRZN",
  [0627] "TLZN",
  [0640] "TRC",
  [0641] "TLC",
  [0642] "TRCE",
  [0643] "TLCE",
  [0644] "TRCA",
  [0645] "TLCA",
  [0646] "TRCN",
  [0647] "TLCN",
  [0660] "TRO",
  [0661] "TLO",
  [0662] "TROE",
  [0663] "TLOE",
  [0664] "TROA",
  [0665] "TLOA",
  [0666] "TRON",
  [0667] "TLON",
  [0610] "TDN",
  [0611] "TSN",
  [0612] "TDNE",
  [0613] "TSNE",
  [0614] "TDNA",
  [0615] "TSNA",
  [0616] "TDNN",
  [0617] "TSNN",
  [0630] "TDZ",
  [0631] "TSZ",
  [0632] "TDZE",
  [0633] "TSZE",
  [0634] "TDZA",
  [0635] "TSZA",
  [0636] "TDZN",
  [0637] "TSZN",
  [0650] "TDC",
  [0651] "TSC",
  [0652] "TDCE",
  [0653] "TSCE",
  [0654] "TDCA",
  [0655] "TSCA",
  [0656] "TDCN",
  [0657] "TSCN",
  [0670] "TDO",
  [0671] "TSO",
  [0672] "TDOE",
  [0673] "TSOE",
  [0674] "TDOA",
  [0675] "TSOA",
  [0676] "TDON",
  [0677] "TSON",

  // Half Word Data Transmission
  [0500] "HLL",
  [0501] "XHLLI",
  [0502] "HLLM",
  [0503] "HLLS",
  [0510] "HLLZ",
  [0511] "HLLZI",
  [0512] "HLLZM",
  [0513] "HLLZS",
  [0530] "HLLE",
  [0531] "HLLEI",
  [0532] "HLLEM",
  [0533] "HLLES",
  [0520] "HLLO",
  [0521] "HLLOI",
  [0522] "HLLOM",
  [0523] "HLLOS",
  [0544] "HLR",
  [0545] "HLRI",
  [0546] "HLRM",
  [0547] "HLRS",
  [0554] "HLRZ",
  [0555] "HLRZI",
  [0556] "HLRZM",
  [0557] "HLRZS",
  [0564] "HLRO",
  [0565] "HLROI",
  [0566] "HLROM",
  [0567] "HLROS",
  [0574] "HLRE",
  [0575] "HLREI",
  [0576] "HLREM",
  [0577] "HLRES",
  [0540] "HRR",
  [0541] "HRRI",
  [0542] "HRRM",
  [0543] "HRRS",
  [0550] "HRRZ",
  [0551] "HRRZI",
  [0552] "HRRZM",
  [0553] "HRRZS",
  [0560] "HRRO",
  [0561] "HRROI",
  [0562] "HRROM",
  [0563] "HRROS",
  [0570] "HRRE",
  [0571] "HRREI",
  [0572] "HRREM",
  [0573] "HRRES",
  [0504] "HRL",
  [0505] "HRLI",
  [0506] "HRLM",
  [0507] "HRLS",
  [0514] "HRLZ",
  [0515] "HRLZI",
  [0516] "HRLZM",
  [0517] "HRLZS",
  [0524] "HRLO",
  [0525] "HRLOI",
  [0526] "HRLOM",
  [0527] "HRLOS",
  [0534] "HRLE",
  [0535] "HRLEI",
  [0536] "HRLEM",
  [0537] "HRLES",

  // Program Control
  [0256] "XCT",
  [0243] "JFFO",
  [0255] "JFCL",
  [0254] "JRST",
  [0264] "JSR",
  [0265] "JSP",
  [0266] "JSA",
  [0267] "JRA",
  [0257] "MAP",

  // Stack
  [0261] "PUSH",
  [0262] "POP",
  [0260] "PUSHJ",
  [0263] "POPJ",
  [0105] "ADJSP",

  // Byte Manipulation
  [0133] "IBP",
  [0135] "LDB",
  [0137] "DPB",
  [0134] "ILDB",
  [0136] "IDPB",

  // UUO
  [0001] "LUUO",
  [0104] "JSYS",

  // EXTEND
  [0123] "EXTEND",
};



// Call this with an instruction to disassemble. It returns the
// mnemonic, the indirect bit, the AC number, the index register, and
// the EA/offset. All of these return values are optional. Pass a NULL
// pointer if any is not needed. For I/O instructions optionally
// return the I/O device name as well.
void Disassemble(const W36 iw,
		 const char **mnemonicPP,
		 const char **ioDevPP,
		 unsigned *acP,
		 unsigned *iP,
		 unsigned *xP,
		 unsigned *yP)
{
  unsigned op = Extract(iw, 0, 8);	/* Opcode */
  unsigned ac = Extract(iw, 9, 12);	/* AC */
  unsigned i = Extract(iw, 13, 13);	/* Indirect */
  unsigned x = Extract(iw, 14, 17);	/* X */
  unsigned y = Extract(iw, 18, 35);	/* Y */
  unsigned d = 0;			/* I/O device */
  const char *mneP = NULL;

  *ioDevPP = NULL;		/* Pessimistically assume not an I/O instruction */

  // Handle some special cases first.
  if (op == 0133 && ac != 0) {		/* IBP becomes ADJBP for nonzero AC */
    mneP = "ADJBP";
  } else if (op == 0254) {		/* JRST family */

    switch (ac) {
    case 000: mneP = "JRST"; break;
    case 001: mneP = "PORTAL"; break;
    case 002: mneP = "JRSTF"; break;
    case 004: mneP = "HALT"; break;
    case 005: mneP = "XJRSTF"; break;
    case 006: mneP = "XJEN"; break;
    case 007: mneP = "XPCW"; break;
    case 010: mneP = "RESTOR"; break;
    case 012: mneP = "JEN"; break;
    case 014: mneP = "SFM"; break;
    default: mneP = "JRST"; break;
    }
  } else if ((op & 0700) == 0700) {	/* I/O instructions */

    switch (Extract(iw, 0, 9)) {
    case 070000: mneP = "APRID"; break;
    case 070010: mneP = "WRFIL"; break;
    case 070040: mneP = "RDERA"; break;
    case 070050: mneP = "SBDIAG"; break;
    case 070110: mneP = "CLRPT"; break;
    case 070144: mneP = "SWPIA"; break;
    case 070150: mneP = "SWPVA"; break;
    case 070154: mneP = "SWPUA"; break;
    case 070164: mneP = "SWPIO"; break;
    case 070170: mneP = "SWPVO"; break;
    case 070174: mneP = "SWPUO"; break;
    case 070200: mneP = "RDPERF"; break;
    case 070204: mneP = "RDTIME"; break;
    case 070210: mneP = "WRPAE"; break;
    case 070240: mneP = "RDMACT"; break;
    case 070244: mneP = "RDEACT"; break;
    case 070260: mneP = "WRTIME"; break;

    default:
      d = Extract(iw, 3, 9);

      static const char ioDevName[][8] = {
	"APR",
	"PI",
	"PAG",
	"CCA",
	"TIM",
	"MTR",
      };

      if (ioDevPP) *ioDevPP = ioDevName[d];
      op = Extract(iw, 0, 3);

      switch (Extract(iw, 10, 12) << 2) {
      case 000: mneP = "BLKI"; break;
      case 004: mneP = "DATAI"; break;
      case 010: mneP = "BLKO"; break;
      case 014: mneP = "DATAO"; break;
      case 020: mneP = "CONO"; break;
      case 024: mneP = "CONI"; break;
      case 030: mneP = "CONSZ"; break;
      case 034: mneP = "CONSO"; break;

      default:
	break;
      }
      
      break;
    }
  } else {
    mneP = mnemonics[op];
  }


  if (mnemonicPP) *mnemonicPP = mneP;
  if (acP) *acP = ac;
  if (iP) *iP = i;
  if (xP) *xP = x;
  if (yP) *yP = y;
}


void DisassembleToString(W36 iw, char *bufferP) {
  const char *mnemonicP;
  const char *ioDevP = NULL;
  unsigned ac, i, x, y;
  int nChars;

  Disassemble(iw, &mnemonicP, &ioDevP, &ac, &i, &x, &y);

  if (!mnemonicP) {
    oct36(bufferP, iw);
  } else {

    // First lay down "mnemonic iodevname," or "mnemonic ac,"
    if (ioDevP)
      nChars = sprintf(bufferP, "%s %s,", mnemonicP, ioDevP);
    else
      nChars = sprintf(bufferP, "%s %2o,", mnemonicP, ac);

    // Then add the EA stuff
    if (x) {
      sprintf(bufferP + nChars, "%s%o(%o)", i ? "@" : "", y, x);
    } else {
      sprintf(bufferP + nChars, "%s%o", i ? "@" : "", y);
    }
  }
}


#if TEST_DISASM
#include "acutest.h"

static void testSimpleOps(void) {
  char disassembly[100];
  const char *mneP;
  const char *ioDevP;
  unsigned ac, i, x, y;

  // EXCH 12,654321(7)
  Disassemble(0250ull << ShiftForBit(8) | /* op */
	     012ull << ShiftForBit(12)  | /* ac */
	     0ull << ShiftForBit(13)    | /* i */
	     07ull << ShiftForBit(17)   | /* x */
	     0654321ull,		  /* y */
	     &mneP, &ioDevP, &ac, &i, &x, &y);
  TEST_CHECK(strcmp(mneP, "EXCH") == 0);
  TEST_CHECK(ac == 012);
  TEST_CHECK(i == 0);
  TEST_CHECK(x == 07);
  TEST_CHECK(y == 0654321ull);

  // EXCH 12,@654321(7)
  Disassemble(0250ull << ShiftForBit(8) | /* op */
	     012ull << ShiftForBit(12)  | /* ac */
	     1ull << ShiftForBit(13)    | /* i */
	     07ull << ShiftForBit(17)   | /* x */
	     0654321ull,		  /* y */
	     &mneP, &ioDevP, &ac, &i, &x, &y);
  TEST_CHECK(strcmp(mneP, "EXCH") == 0);
  TEST_CHECK(ac == 012);
  TEST_CHECK(i == 1);
  TEST_CHECK(x == 07);
  TEST_CHECK(y == 0654321ull);

  // EXCH 12,654321(7)
  DisassembleToString(0250ull << ShiftForBit(8) | /* op */
		      012ull << ShiftForBit(12)  | /* ac */
		      0ull << ShiftForBit(13)    | /* i */
		      07ull << ShiftForBit(17)   | /* x */
		      0654321ull,		  /* y */
		      disassembly);
  TEST_CHECK(strcmp(disassembly, "EXCH 12,654321(7)") == 0);

  // EXCH 12,@654321(7)
  DisassembleToString(0250ull << ShiftForBit(8) | /* op */
		      012ull << ShiftForBit(12)  | /* ac */
		      1ull << ShiftForBit(13)    | /* i */
		      07ull << ShiftForBit(17)   | /* x */
		      0654321ull,		  /* y */
		      disassembly);
  TEST_CHECK(strcmp(disassembly, "EXCH 12,@654321(7)") == 0);
}


TEST_LIST = {
  {"SimpleOps", testSimpleOps},
  {NULL, NULL},
};

#endif
