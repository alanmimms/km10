#include <stdio.h>
#include <stdint.h>
#include <string.h>

#include "kl10.h"
#include "disasm.h"
#include "loada10.h"


typedef struct KMContext {
  W36 ac[16];
  W36 pc;
  W36 flags;
  W36 *memP;
  unsigned running: 1;
  unsigned tracePC: 1;
} KMContext;


static const W36 flagOV = MaskForBit(0);
static const W36 flagPCP = MaskForBit(0);
static const W36 flagCY0 = MaskForBit(1);
static const W36 flagCY1 = MaskForBit(2);
static const W36 flagFOV = MaskForBit(3);
static const W36 flagFPD = MaskForBit(4);
static const W36 flagUSER = MaskForBit(5);
static const W36 flagUIO = MaskForBit(6);
static const W36 flagPCU = MaskForBit(6);
static const W36 flagPUB = MaskForBit(7);
static const W36 flagAFI = MaskForBit(8);
static const W36 flagT2 = MaskForBit(9);
static const W36 flagT1 = MaskForBit(10);
static const W36 flagFUF = MaskForBit(11);
static const W36 flagNDV = MaskForBit(12);


// XXX This needs nonzero section code added.
static inline W36 pcAndFlags(KMContext *cp) {
  return cp->flags | cp->pc;
}


static void emulate(KMContext *cp) {
  char octbuf[64];
  char octbuf2[64];

  do {
    // XXX Put interrupt, trap, HALT, etc. handling here...
    W36 iw = cp->memP[cp->pc];

    W36 ea;
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
      fprintf(stderr, "%s: [ea=%s] %s\n", oct36(pcBuf, cp->pc), oct36(eaBuf, ea), daBuf);
    }

    switch (op) {
    // Full Word Movement
    case 0250:		// EXCH
      tmp = cp->ac[ac];
      cp->ac[ac] = cp->memP[ea];
      cp->memP[ea] = tmp;
      break;

    case 0200:		// MOVE
      cp->memP[ea] = cp->ac[ac];
      break;

    case 0201:		// MOVEI
      cp->ac[ac] = RH(ea);
      break;

    case 0202:		// MOVEM
      cp->memP[ea] = cp->ac[ac];
      break;

    case 0203:		// MOVES
      if (ac) cp->ac[ac] = cp->memP[ea];

    case 0210:		// MOVN
      tmp = cp->memP[ea];

      if (tmp == MaskForBit(0)) {
      }

      break;

    case 0211:		// MOVNI
    case 0212:		// MOVNM
    case 0213:		// MOVNS
    case 0415:		// XMOVEI
    case 0120:		// DMOVE
    case 0121:		// DMOVEN
    case 0124:		// DMOVEM
    case 0125:		// DMOVNM
    case 0251:		// BLT
    case 0204:		// MOVS
    case 0205:		// MOVSI
    case 0206:		// MOVSM
    case 0207:		// MOVSS
    case 0214:		// MOVM
    case 0215:		// MOVMI
    case 0216:		// MOVMM
    case 0217:		// MOVMS
    case 0052:		// PMOVE
    case 0053:		// PMOVEM

      // Fixed Point Arithmetic
    case 0270:		// ADD
    case 0271:		// ADDI
    case 0272:		// ADDM
    case 0273:		// ADDB
    case 0274:		// SUB
    case 0275:		// SUBI
    case 0276:		// SUBM
    case 0277:		// SUBB
    case 0220:		// IMUL
    case 0221:		// IMULI
    case 0222:		// IMULM
    case 0223:		// IMULB
    case 0224:		// MUL
    case 0225:		// MULI
    case 0226:		// MULM
    case 0227:		// MULB
    case 0230:		// IDIV
    case 0231:		// IDIVI
    case 0232:		// IDIVM
    case 0233:		// IDIVB
    case 0234:		// DIV
    case 0235:		// DIVI
    case 0236:		// DIVM
    case 0237:		// DIVB
    case 0114:		// DADD
    case 0115:		// DSUB
    case 0116:		// DMUL
    case 0117:		// DDIV

      // Floating Point Arithmetic
    case 0140:		// FAD
    case 0141:		// FADL
    case 0142:		// FADM
    case 0143:		// FADB
    case 0144:		// FADR
    case 0145:		// FADRI
    case 0146:		// FADRM
    case 0147:		// FADRB
    case 0150:		// FSB
    case 0151:		// FSBL
    case 0152:		// FSBM
    case 0153:		// FSBB
    case 0154:		// FSBR
    case 0155:		// FSBRI
    case 0156:		// FSBRM
    case 0157:		// FSBRB
    case 0160:		// FMP
    case 0161:		// FMPL
    case 0162:		// FMPM
    case 0163:		// FMPB
    case 0164:		// FMPR
    case 0165:		// FMPRI
    case 0166:		// FMPRM
    case 0167:		// FMPRB
    case 0170:		// FDV
    case 0171:		// FDVL
    case 0172:		// FDVM
    case 0173:		// FDVB
    case 0174:		// FDVR
    case 0175:		// FDVRI
    case 0176:		// FDVRM
    case 0177:		// FDVRB
    case 0110:		// DFAD
    case 0111:		// DFSB
    case 0112:		// DFMP
    case 0113:		// DFDV
    case 0132:		// FSC
    case 0031:		// GFSC
    case 0127:		// FLTR
    case 0030:		// GFLTR
    case 0027:		// DGFLTR
    case 0122:		// FIX
    case 0126:		// FIXR
    case 0024:		// GFIX
    case 0026:		// GFIXR
    case 0023:		// GDFIX
    case 0025:		// GDFIXR
    case 0021:		// GSNGL
    case 0022:		// GDBLE
    case 0130:		// UFA
    case 0131:		// DFN

      // Boolean
    case 0400:		// SETZ
    case 0401:		// SETZI
    case 0402:		// SETZM
    case 0403:		// SETZB
    case 0474:		// SETO
    case 0475:		// SETOI
    case 0476:		// SETOM
    case 0477:		// SETOB
    case 0424:		// SETA
    case 0425:		// SETAI
    case 0426:		// SETAM
    case 0427:		// SETAB
    case 0450:		// SETCA
    case 0451:		// SETCAI
    case 0452:		// SETCAM
    case 0453:		// SETCAB
    case 0414:		// SETM
    case 0416:		// SETMM
    case 0417:		// SETMB
    case 0460:		// SETCM
    case 0461:		// SETCMI
    case 0462:		// SETCMM
    case 0463:		// SETCMB
    case 0404:		// AND
    case 0405:		// ANDI
    case 0406:		// ANDM
    case 0407:		// ANDB
    case 0410:		// ANDCA
    case 0411:		// ANDCAI
    case 0412:		// ANDCAM
    case 0413:		// ANDCAB
    case 0420:		// ANDCM
    case 0421:		// ANDCMI
    case 0422:		// ANDCMM
    case 0423:		// ANDCMB
    case 0440:		// ANDCB
    case 0441:		// ANDCBI
    case 0442:		// ANDCBM
    case 0443:		// ANDCBB
    case 0434:		// IOR
    case 0435:		// IORI
    case 0436:		// IORM
    case 0437:		// IORB
    case 0454:		// ORCA
    case 0455:		// ORCAI
    case 0456:		// ORCAM
    case 0457:		// ORCAB
    case 0464:		// ORCM
    case 0465:		// ORCMI
    case 0466:		// ORCMM
    case 0467:		// ORCMB
    case 0470:		// ORCB
    case 0471:		// ORCBI
    case 0472:		// ORCBM
    case 0473:		// ORCBB
    case 0430:		// XOR
    case 0431:		// XORI
    case 0432:		// XORM
    case 0433:		// XORB
    case 0444:		// EQV
    case 0445:		// EQVI
    case 0446:		// EQVM
    case 0447:		// EQVB

      // Shift and Rotate
    case 0240:		// ASH
    case 0241:		// ROT
    case 0242:		// LSH
    case 0244:		// ASHC
    case 0245:		// ROTC
    case 0246:		// LSHC

      // Arithmetic Testing
    case 0252:		// AOBJP
    case 0253:		// AOBJN
    case 0300:		// CAI
    case 0301:		// CAIL
    case 0302:		// CAIE
    case 0303:		// CAILE
    case 0304:		// CAIA
    case 0305:		// CAIGE
    case 0306:		// CAIN
    case 0307:		// CAIG
    case 0310:		// CAM
    case 0311:		// CAML
    case 0312:		// CAME
    case 0313:		// CAMLE
    case 0314:		// CAMA
    case 0315:		// CAMGE
    case 0316:		// CAMN
    case 0317:		// CAMG
    case 0320:		// JUMP
    case 0321:		// JUMPL
    case 0322:		// JUMPE
    case 0323:		// JUMPLE
    case 0324:		// JUMPA
    case 0325:		// JUMPGE
    case 0326:		// JUMPN
    case 0327:		// JUMPG

    case 0330:		// SKIP
      if (ac != 0) cp->ac[ac] = ea;
      break;

#define DO_SKIP(OP)				\
      if (cp->memP[ea] OP 0) ++cp->pc;		\
      if (ac != 0) cp->ac[ac] = ea;		\
      break

    case 0331:		// SKIPL
      DO_SKIP(<);
      
    case 0332:		// SKIPE
      DO_SKIP(==);

    case 0333:		// SKIPLE
      DO_SKIP(<=);

    case 0334:		// SKIPA
      if (ac != 0) cp->ac[ac] = ea;
      break;

    case 0335:		// SKIPGE
      DO_SKIP(>=);

    case 0336:		// SKIPN
      DO_SKIP(!=);

    case 0337:		// SKIPG
      DO_SKIP(>);

    case 0340:		// AOJ
    case 0341:		// AOJL
    case 0342:		// AOJE
    case 0343:		// AOJLE
    case 0344:		// AOJA
    case 0345:		// AOJGE
    case 0346:		// AOJN
    case 0347:		// AOJG
    case 0360:		// SOJ
    case 0361:		// SOJL
    case 0362:		// SOJE
    case 0363:		// SOJLE
    case 0364:		// SOJA
    case 0365:		// SOJGE
    case 0366:		// SOJN
    case 0367:		// SOJG
    case 0350:		// AOS
    case 0351:		// AOSL
    case 0352:		// AOSE
    case 0353:		// AOSLE
    case 0354:		// AOSA
    case 0355:		// AOSGE
    case 0356:		// AOSN
    case 0357:		// AOSG
    case 0370:		// SOS
    case 0371:		// SOSL
    case 0372:		// SOSE
    case 0373:		// SOSLE
    case 0374:		// SOSA
    case 0375:		// SOSGE
    case 0376:		// SOSN
    case 0377:		// SOSG

      // Logical Testing and Modification
#define DO_TEST(V,OP)		\
      if (((V) & ea) OP 0) ++cp->pc

#define DO_TEST_ZEROS(V,OP)	\
      if (((V) & ea) OP 0) ++cp->pc; \
      V &= ~ea

    case 0600:		// TRN
    case 0601:		// TLN
      break;

    case 0602:		// TRNE
      DO_TEST(RH(cp->ac[ac]), ==);
      break;

    case 0603:		// TLNE
      DO_TEST(LH(cp->ac[ac]), ==);
      break;

    case 0604:		// TRNA
    case 0605:		// TLNA
      ++cp->pc;
      break;

    case 0606:		// TRNN
      DO_TEST(RH(cp->ac[ac]), !=);
      break;

    case 0607:		// TLNN
      DO_TEST(LH(cp->ac[ac]), !=);
      break;

    case 0620:		// TRZ
      DO_TEST_ZEROS(RH(cp->ac[ac]), !=);
      break;

    case 0621:		// TLZ
      DO_TEST_ZEROS(LH(cp->ac[ac]), !=);
      break;

    case 0622:		// TRZE
    case 0623:		// TLZE
    case 0624:		// TRZA
    case 0625:		// TLZA
    case 0626:		// TRZN
    case 0627:		// TLZN
    case 0640:		// TRC
    case 0641:		// TLC
    case 0642:		// TRCE
    case 0643:		// TLCE
    case 0644:		// TRCA
    case 0645:		// TLCA
    case 0646:		// TRCN
    case 0647:		// TLCN
    case 0660:		// TRO
    case 0661:		// TLO
    case 0662:		// TROE
    case 0663:		// TLOE
    case 0664:		// TROA
    case 0665:		// TLOA
    case 0666:		// TRON
    case 0667:		// TLON
    case 0610:		// TDN
    case 0611:		// TSN
    case 0612:		// TDNE
    case 0613:		// TSNE
    case 0614:		// TDNA
    case 0615:		// TSNA
    case 0616:		// TDNN
    case 0617:		// TSNN
    case 0630:		// TDZ
    case 0631:		// TSZ
    case 0632:		// TDZE
    case 0633:		// TSZE
    case 0634:		// TDZA
    case 0635:		// TSZA
    case 0636:		// TDZN
    case 0637:		// TSZN
    case 0650:		// TDC
    case 0651:		// TSC
    case 0652:		// TDCE
    case 0653:		// TSCE
    case 0654:		// TDCA
    case 0655:		// TSCA
    case 0656:		// TDCN
    case 0657:		// TSCN
    case 0670:		// TDO
    case 0671:		// TSO
    case 0672:		// TDOE
    case 0673:		// TSOE
    case 0674:		// TDOA
    case 0675:		// TSOA
    case 0676:		// TDON
    case 0677:		// TSON

      // Half Word Data Transmission
    case 0500:		// HLL
    case 0501:		// XHLLI
    case 0502:		// HLLM
    case 0503:		// HLLS
    case 0510:		// HLLZ
    case 0511:		// HLLZI
    case 0512:		// HLLZM
    case 0513:		// HLLZS
    case 0530:		// HLLE
    case 0531:		// HLLEI
    case 0532:		// HLLEM
    case 0533:		// HLLES
    case 0520:		// HLLO
    case 0521:		// HLLOI
    case 0522:		// HLLOM
    case 0523:		// HLLOS
    case 0544:		// HLR
    case 0545:		// HLRI
    case 0546:		// HLRM
    case 0547:		// HLRS
    case 0554:		// HLRZ
    case 0555:		// HLRZI
    case 0556:		// HLRZM
    case 0557:		// HLRZS
    case 0564:		// HLRO
    case 0565:		// HLROI
    case 0566:		// HLROM
    case 0567:		// HLROS
    case 0574:		// HLRE
    case 0575:		// HLREI
    case 0576:		// HLREM
    case 0577:		// HLRES
    case 0540:		// HRR
    case 0541:		// HRRI
    case 0542:		// HRRM
    case 0543:		// HRRS
    case 0550:		// HRRZ
    case 0551:		// HRRZI
    case 0552:		// HRRZM
    case 0553:		// HRRZS
    case 0560:		// HRRO
    case 0561:		// HRROI
    case 0562:		// HRROM
    case 0563:		// HRROS
    case 0570:		// HRRE
    case 0571:		// HRREI
    case 0572:		// HRREM
    case 0573:		// HRRES
    case 0504:		// HRL
    case 0505:		// HRLI
    case 0506:		// HRLM
    case 0507:		// HRLS
    case 0514:		// HRLZ
    case 0515:		// HRLZI
    case 0516:		// HRLZM
    case 0517:		// HRLZS
    case 0524:		// HRLO
    case 0525:		// HRLOI
    case 0526:		// HRLOM
    case 0527:		// HRLOS
    case 0534:		// HRLE
    case 0535:		// HRLEI
    case 0536:		// HRLEM
    case 0537:		// HRLES

      // Program Control
    case 0256:		// XCT
    case 0243:		// JFFO
    case 0255:		// JFCL

    case 0254:		// JRST

      switch (ac) {
      case 0000:		/* JRST */
	cp->pc = ea;
	goto SKIP_PC_INCR;

      case 0005:		/* XJRSTF */
	tmp = cp->memP[ea];
	// XXX need to support processor-dependent information in [ea]<18:35>
	cp->flags = Extract(tmp, 0, 12) << ShiftForBit(12);
	cp->pc = Extract(cp->memP[ea+1], 6, 35);
	goto SKIP_PC_INCR;

      default:
	fprintf(stderr, "Unhandled JRST function code %03o at %s: %s\n",
		ac, oct36(octbuf, cp->pc), oct36(octbuf2, iw));
	cp->running = 0;
	break;
      }
      
      goto SKIP_PC_INCR;

    case 0264:		// JSR
      cp->memP[ea] = pcAndFlags(cp);
      cp->flags &= ~(flagAFI | flagFPD | flagT1 | flagT2);
      cp->pc = ea + 1;
      goto SKIP_PC_INCR;

    case 0265:		// JSP
      cp->ac[ac] = pcAndFlags(cp);
      cp->flags &= ~(flagAFI | flagFPD | flagT1 | flagT2);
      cp->pc = ea;
      goto SKIP_PC_INCR;

    case 0266:		// JSA
    case 0267:		// JRA
    case 0257:		// MAP

      // Stack
    case 0261:		// PUSH
    case 0262:		// POP
    case 0260:		// PUSHJ
    case 0263:		// POPJ
    case 0105:		// ADJSP

      // Byte Manipulation
    case 0133:		// IBP
    case 0135:		// LDB
    case 0137:		// DPB
    case 0134:		// ILDB
    case 0136:		// IDPB

      // UUO
    case 0001:		// LUUO
    case 0104:		// JSYS

      // EXTEND
    case 0123:		// EXTEND
      break;

    case 0700:		// I/O instructions
      unsigned ioOp = Extract(iw, 0, 12) << 2;

      switch (ioOp) {
      case 070000:	// APRID
	cp->memP[ea] =
	  MaskForBit(0) |	/* TOPS-20 paging */
	  MaskForBit(1) |	/* Extended Addressing */
	  0442 |		/* Microcode version number ??? */
//	  MaskForBit(2) |	/* Exotic microcode */
//	  MaskForBit(18) |	/* 50Hz */
//	  MaskForBit(19) |	/* Cache */
	  MaskForBit(20) |	/* Channels are implemented */
//	  MaskForBit(21) |	/* Extended KL10 XXX for now*/
//	  MaskForBit(22) |	/* Master Oscillator present */
	  04321;		/* Serial number */
	break;

      case 070040:	// RDERA
	cp->memP[ea] = 0;
	break;

      case 070044:	// DATAI PI, (???)
	break;

      case 070054:	// DATAO PI, (console on KA10/KI10)
	break;

      case 070004:	// DATAI APR,
      case 070014:	// DATAO APR,
      case 070020:	// CONO APR,
      case 070024:	// CONI APR,
      case 070050:	// SBDIAG
      case 070104:	// DATAI PAG,
      case 070110:	// CLRPT
      case 070114:	// DATAO PAG,
      case 070120:	// CONO PAG,
      case 070124:	// CONI PAG,
      case 070200:	// RDPERF
      case 070204:	// RDTIME
      case 070210:	// WRPAE
      case 070220:	// CONO TIM,
      case 070224:	// CONI TIM,
      case 070240:	// RDMACT
      case 070244:	// RDEACT
      case 070260:	// WRTIME
      case 070264:	// CONI MTR,
      default:
	fprintf(stderr, "Unhandled I/O opcode %05o at %s: %s\n",
		ioOp, oct36(octbuf, cp->pc), oct36(octbuf2, iw));
	cp->running = 0;
	break;
      }

      break;

    default:
      fprintf(stderr, "Unhandled opcode %03o at %s: %s\n",
	      op, oct36(octbuf, cp->pc), oct36(octbuf2, iw));
      cp->running = 0;
      break;
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
    fprintf(stderr, "Usage:\n\
    %s <filename to load>\n", argv[0]);
    return -1;
  }

  int st = LoadA10(fileNameP, memory, &startAddr, &lowestAddr, &highestAddr);
  fprintf(stderr, "[Loaded %s  st=%d  start=" PRI06o64 "]\n", fileNameP, st, startAddr);

  static KMContext context;
  memset(&context, 0, sizeof(context));
  context.pc = startAddr;
  context.memP = memory;
  context.tracePC = 1;
  context.running = 1;
  emulate(&context);
  return 0;
}
