#include <cstdint>
#include <sstream>
#include <iomanip>

#include "word.hpp"
#include "debugger.hpp"


// Accept an octal string of digits to create a word, optionally
// including ",," between halves.
W36::W36(string &s) {
  size_t commaPos = s.find(",,");

  if (commaPos != std::string::npos) {	// If ",," split the halves.
    lhu = stoul(s.substr(0, commaPos), nullptr, 8);
    rhu = stoul(s.substr(commaPos + 2), nullptr, 8);
  } else {			// Entire string is octal.
    u = stoull(s, nullptr, 8);
  }
}


string W36::fmt18() const {
  ostringstream s;
  s << right << setw(6) << setfill('0') << oct << rhu;
  return s.str();
}


string W36::fmtVMA() const {
  ostringstream s;
  s << (lhu & 07) << ",," << fmt18();
  return s.str();
}

string W36::fmt36() const {
  ostringstream s;
  s << right << setw(6) << setfill('0') << oct << lhu
    << ",,"
    << right << setw(6) << setfill('0') << oct << rhu;
  return s.str();
}


string W36::sixbit() const {
  string s;

  for (int k=30; k >= 0; k -= 6) {
    s.push_back(((u >> k) & 077) + 32);
  }

  return s;
}


string W36::ascii() const {
  string s;

  for (int k=29; k >= 0; k -= 7) {
    int ch = (u >> k) & 0177;
    if (ch < ' ' || ch == 0177) ch = '?';
    s.push_back(ch);
  }

  return s;
}


string W72::fmt72() const {
  ostringstream ss;
  ss << W36(hi).fmt36() << ",,," << W36(lo).fmt36();
  return ss.str();
}


// Format an unsigned 128-bit number as whatever base <= 10.
string W72::fmt128(uint128_t v128, int base) {
  if (v128 == 0) return "0";

  string s{};
  s.reserve(64);
  int digits = 0;

  do {
    s += '0' + (v128 % base);
    ++digits;
    if (base == 8 && digits % 6 == 0) s += " ";
    if (base == 8 && digits == 12) s += " ";
    v128 /= base;
  } while (v128 != 0);

  return string(s.rbegin(), s.rend());
}


// Format a signed 128-bit number as whatever base <= 10.
string W72::fmt128(int128_t v128, int base) {
  if (v128 == 0) return "0";
  bool isNeg = v128 < 0;
  if (isNeg) v128 = -v128;
  string s = fmt128((uint128_t) v128, base);
  if (isNeg) s += "-";
  return s;
}



W72 W72::negate() {
  W36 rLo{-sLo};		// Do this first so we can test for zero mag.
  W72 result{-sHi - (rLo.mag == 0), rLo};
  result.loSign = result.hiSign;
  return result;
}


W144 W144::negate() {
  W36 r3{-s3};
  W36 r2{-s2 - (r3.mag == 0)};
  W36 r1{-s1 - (r2.mag == 0)};
  W36 r0{-s0 - (r1.mag == 0)};
  W144 result{r0, r1, r2, r3};
  result.sign3 = result.sign2 = result.sign1 = result.sign0;
  return result;
}


W144::W144(W36 a0, W36 a1, W36 a2, W36 a3) {
  mag0 = a0.mag;
  sign0 = a0.sign;
  mag1 = a1.mag;
  sign1 = a0.sign;
  mag2 = a2.mag;
  sign2 = a0.sign;
  mag3 = a3.mag;
  sign3 = a0.sign;
}


W144 W144::fromMag(uint128_t aMag0, uint128_t aMag1, int aNeg) {
  aNeg = !!aNeg;

  cout << "fromMag sign=" << aNeg << logger.endl;

  return W144{
    W36::fromMag((uint64_t) ((aMag1 >> 105) | aMag0), aNeg),
    W36::fromMag((uint64_t) (aMag1 >> 70), aNeg),
    W36::fromMag((uint64_t) (aMag1 >> 35), aNeg),
    W36::fromMag((uint64_t) aMag1, aNeg)};
}


W144::W144(uint32_t a0, uint32_t a1, uint32_t a2, uint32_t a3, uint32_t a4) {
  w4 = a4;
  w3 = a3;
  w2 = a2;
  w1 = a1;
  w0 = a0;
}


// This is to simplify conversions from 128-bit to and from pieces we
// can manage.
struct W128 {

  union {

    struct ATTRPACKED {
      uint32_t w3;
      uint32_t w2;
      uint32_t w1;
      uint32_t w0;
    };

    uint128_t u;
  };
  
  W128(uint128_t value) : u(value) {}
};


// Same for 64-bit.
struct W64 {

  union {
    struct ATTRPACKED {
      uint32_t w1;
      uint32_t w0;
    };

    uint64_t u;
  };

  W64(uint64_t value) : u(value) {}
};


static ostream &oct11(ostream &os) {
  return os << oct << setfill('0') << setw(11);
}


// Do one "digit" of the 70bit*70b multiply (128-bit * 32-bit +
// carryIn), accumulating the partial product and returning carryOut.
static uint32_t doDigit(W128 &prod, W128 &a, uint32_t b, uint32_t carryIn) {
  W64 partial{0};

  partial.u = (uint64_t) a.w3 * (uint64_t) b + (uint64_t) prod.w3 + carryIn;
  prod.w3 = partial.w1;
  cout << oct11 << a.w3 << " * " << oct11 << b
       << " = " << oct11 << prod.w3 << " carry=" << partial.w0
       << logger.endl;

  partial.u = (uint64_t) a.w2 * (uint64_t) b + (uint64_t) prod.w2 + partial.w0;
  prod.w2 = partial.w1;
  cout << oct11 << a.w2 << " * " << oct11 << b << " + " << oct11 << partial.w0
       << " = " << oct11 << prod.w2 << " carry=" << partial.w0
       << logger.endl;
  
  partial.u = (uint64_t) a.w1 * (uint64_t) b + (uint64_t) prod.w1 + partial.w0;
  prod.w1 = partial.w1;
  cout << oct11 << a.w1 << " * " << oct11 << b << " + " << oct11 << partial.w0
       << " = " << oct11 << prod.w1 << " carry=" << partial.w0
       << logger.endl;
  
  partial.u = (uint64_t) a.w0 * (uint64_t) b + (uint64_t) prod.w0 + partial.w0;
  prod.w0 = partial.w1;
  cout << oct11 << a.w0 << " * " << oct11 << b << " + " << oct11 << partial.w0
       << " = " << oct11 << prod.w0 << " carry=" << partial.w0
       << logger.endl;
  return partial.w0;
}


// Make a 140-bit product from two 70-bit unsigned and a sign.
W144 W144::product(uint128_t a, uint128_t b) {
  W128 a128{a};
  W128 b128{b};
  W128 prod128{0};
  uint32_t carry = 0;

  carry = doDigit(prod128, a128, b128.w3, carry);
  carry = doDigit(prod128, a128, b128.w2, carry);
  carry = doDigit(prod128, a128, b128.w1, carry);
  carry = doDigit(prod128, a128, b128.w0, carry);
  return W144{carry, prod128.w0, prod128.w1, prod128.w2, prod128.w3};
}


uint128_t W144::lowerU70() const {
  return ((uint128_t) mag2 << 35) | (uint128_t) mag3; 
}

uint128_t W144::upperU70() const {
  return ((uint128_t) mag0 << 35) | (uint128_t) mag1;
}


// Compare this 140-bit magnitude against the specified 70-bit
// magnitude return true if this >= a70.
bool W144::operator >= (const uint128_t a70) const {return upperU70() != 0 || lowerU70() >= a70;}


// Accessors/converters
W144::tQuadWord W144::toQuadWord(const int sign) const {
  uint64_t const signBit = sign ? W36::bit0 : 0;

  cout << "toQuadWord sign=" << sign << logger.endl;

  return tQuadWord{
    mag0 | signBit,
    mag1 | signBit,
    mag2 | signBit,
    mag3 | signBit
  };
}


// Disassembly of instruction words
string W36::disasm(Debugger *debuggerP) {
  ostringstream s;
  bool isIO = false;

  // Handle some special cases first.
  if (op == 0133 && ac != 0) {		/* IBP becomes ADJBP for nonzero AC */
    s << "ADJBP";
  } else if (op == 0254) {		/* JRST family */

    switch (ac) {
    case 000: s << "JRST"; break;
    case 001: s << "PORTAL"; break;
    case 002: s << "JRSTF"; break;
    case 004: s << "HALT"; break;
    case 005: s << "XJRSTF"; break;
    case 006: s << "XJEN"; break;
    case 007: s << "XPCW"; break;
    case 010: s << "RESTOR"; break;
    case 012: s << "JEN"; break;
    case 014: s << "SFM"; break;
    default: s << "JRST"; break;
    }
  } else if (ioSeven == 7) {	/* I/O instructions */

    switch ((unsigned) ioAll << 2) {
    case 070000: s << "APRID"; break;
    case 070010: s << "WRFIL"; break;
    case 070040: s << "RDERA"; break;
    case 070050: s << "SBDIAG"; break;
    case 070110: s << "CLRPT"; break;
    case 070144: s << "SWPIA"; break;
    case 070150: s << "SWPVA"; break;
    case 070154: s << "SWPUA"; break;
    case 070164: s << "SWPIO"; break;
    case 070170: s << "SWPVO"; break;
    case 070174: s << "SWPUO"; break;
    case 070200: s << "RDPERF"; break;
    case 070204: s << "RDTIME"; break;
    case 070210: s << "WRPAE"; break;
    case 070240: s << "RDMACT"; break;
    case 070244: s << "RDEACT"; break;
    case 070260: s << "WRTIME"; break;

    default:
      isIO = true;

      switch ((unsigned) ioOp << 2) {
      case 000: s << "BLKI"; break;
      case 004: s << "DATAI"; break;
      case 010: s << "BLKO"; break;
      case 014: s << "DATAO"; break;
      case 020: s << "CONO"; break;
      case 024: s << "CONI"; break;
      case 030: s << "CONSZ"; break;
      case 034: s << "CONSO"; break;
      default: break;
      }

      break;
    }
  } else {

    switch (op) {
      // Full Word Movement
    case 0250: s << "EXCH"; break;
    case 0200: s << "MOVE"; break;
    case 0201: s << "MOVEI"; break;
    case 0202: s << "MOVEM"; break;
    case 0203: s << "MOVES"; break;
    case 0210: s << "MOVN"; break;
    case 0211: s << "MOVNI"; break;
    case 0212: s << "MOVNM"; break;
    case 0213: s << "MOVNS"; break;
    case 0415: s << "XMOVEI"; break;
    case 0120: s << "DMOVE"; break;
    case 0121: s << "DMOVEN"; break;
    case 0124: s << "DMOVEM"; break;
    case 0125: s << "DMOVNM"; break;
    case 0251: s << "BLT"; break;
    case 0204: s << "MOVS"; break;
    case 0205: s << "MOVSI"; break;
    case 0206: s << "MOVSM"; break;
    case 0207: s << "MOVSS"; break;
    case 0214: s << "MOVM"; break;
    case 0215: s << "MOVMI"; break;
    case 0216: s << "MOVMM"; break;
    case 0217: s << "MOVMS"; break;
    case 0052: s << "PMOVE"; break;
    case 0053: s << "PMOVEM"; break;

      // Fixed Point Arithmetic
    case 0270: s << "ADD"; break;
    case 0271: s << "ADDI"; break;
    case 0272: s << "ADDM"; break;
    case 0273: s << "ADDB"; break;
    case 0274: s << "SUB"; break;
    case 0275: s << "SUBI"; break;
    case 0276: s << "SUBM"; break;
    case 0277: s << "SUBB"; break;
    case 0220: s << "IMUL"; break;
    case 0221: s << "IMULI"; break;
    case 0222: s << "IMULM"; break;
    case 0223: s << "IMULB"; break;
    case 0224: s << "MUL"; break;
    case 0225: s << "MULI"; break;
    case 0226: s << "MULM"; break;
    case 0227: s << "MULB"; break;
    case 0230: s << "IDIV"; break;
    case 0231: s << "IDIVI"; break;
    case 0232: s << "IDIVM"; break;
    case 0233: s << "IDIVB"; break;
    case 0234: s << "DIV"; break;
    case 0235: s << "DIVI"; break;
    case 0236: s << "DIVM"; break;
    case 0237: s << "DIVB"; break;
    case 0114: s << "DADD"; break;
    case 0115: s << "DSUB"; break;
    case 0116: s << "DMUL"; break;
    case 0117: s << "DDIV"; break;

      // Floating Point Arithmetic
    case 0140: s << "FAD"; break;
    case 0141: s << "FADL"; break;
    case 0142: s << "FADM"; break;
    case 0143: s << "FADB"; break;
    case 0144: s << "FADR"; break;
    case 0145: s << "FADRI"; break;
    case 0146: s << "FADRM"; break;
    case 0147: s << "FADRB"; break;
    case 0150: s << "FSB"; break;
    case 0151: s << "FSBL"; break;
    case 0152: s << "FSBM"; break;
    case 0153: s << "FSBB"; break;
    case 0154: s << "FSBR"; break;
    case 0155: s << "FSBRI"; break;
    case 0156: s << "FSBRM"; break;
    case 0157: s << "FSBRB"; break;
    case 0160: s << "FMP"; break;
    case 0161: s << "FMPL"; break;
    case 0162: s << "FMPM"; break;
    case 0163: s << "FMPB"; break;
    case 0164: s << "FMPR"; break;
    case 0165: s << "FMPRI"; break;
    case 0166: s << "FMPRM"; break;
    case 0167: s << "FMPRB"; break;
    case 0170: s << "FDV"; break;
    case 0171: s << "FDVL"; break;
    case 0172: s << "FDVM"; break;
    case 0173: s << "FDVB"; break;
    case 0174: s << "FDVR"; break;
    case 0175: s << "FDVRI"; break;
    case 0176: s << "FDVRM"; break;
    case 0177: s << "FDVRB"; break;
    case 0110: s << "DFAD"; break;
    case 0111: s << "DFSB"; break;
    case 0112: s << "DFMP"; break;
    case 0113: s << "DFDV"; break;
    case 0132: s << "FSC"; break;
    case 0127: s << "FLTR"; break;
    case 0122: s << "FIX"; break;
    case 0126: s << "FIXR"; break;
    case 0130: s << "UFA"; break;
    case 0131: s << "DFN"; break;

      // Boolean
    case 0400: s << "SETZ"; break;
    case 0401: s << "SETZI"; break;
    case 0402: s << "SETZM"; break;
    case 0403: s << "SETZB"; break;
    case 0474: s << "SETO"; break;
    case 0475: s << "SETOI"; break;
    case 0476: s << "SETOM"; break;
    case 0477: s << "SETOB"; break;
    case 0424: s << "SETA"; break;
    case 0425: s << "SETAI"; break;
    case 0426: s << "SETAM"; break;
    case 0427: s << "SETAB"; break;
    case 0450: s << "SETCA"; break;
    case 0451: s << "SETCAI"; break;
    case 0452: s << "SETCAM"; break;
    case 0453: s << "SETCAB"; break;
    case 0414: s << "SETM"; break;
    case 0416: s << "SETMM"; break;
    case 0417: s << "SETMB"; break;
    case 0460: s << "SETCM"; break;
    case 0461: s << "SETCMI"; break;
    case 0462: s << "SETCMM"; break;
    case 0463: s << "SETCMB"; break;
    case 0404: s << "AND"; break;
    case 0405: s << "ANDI"; break;
    case 0406: s << "ANDM"; break;
    case 0407: s << "ANDB"; break;
    case 0410: s << "ANDCA"; break;
    case 0411: s << "ANDCAI"; break;
    case 0412: s << "ANDCAM"; break;
    case 0413: s << "ANDCAB"; break;
    case 0420: s << "ANDCM"; break;
    case 0421: s << "ANDCMI"; break;
    case 0422: s << "ANDCMM"; break;
    case 0423: s << "ANDCMB"; break;
    case 0440: s << "ANDCB"; break;
    case 0441: s << "ANDCBI"; break;
    case 0442: s << "ANDCBM"; break;
    case 0443: s << "ANDCBB"; break;
    case 0434: s << "IOR"; break;
    case 0435: s << "IORI"; break;
    case 0436: s << "IORM"; break;
    case 0437: s << "IORB"; break;
    case 0454: s << "ORCA"; break;
    case 0455: s << "ORCAI"; break;
    case 0456: s << "ORCAM"; break;
    case 0457: s << "ORCAB"; break;
    case 0464: s << "ORCM"; break;
    case 0465: s << "ORCMI"; break;
    case 0466: s << "ORCMM"; break;
    case 0467: s << "ORCMB"; break;
    case 0470: s << "ORCB"; break;
    case 0471: s << "ORCBI"; break;
    case 0472: s << "ORCBM"; break;
    case 0473: s << "ORCBB"; break;
    case 0430: s << "XOR"; break;
    case 0431: s << "XORI"; break;
    case 0432: s << "XORM"; break;
    case 0433: s << "XORB"; break;
    case 0444: s << "EQV"; break;
    case 0445: s << "EQVI"; break;
    case 0446: s << "EQVM"; break;
    case 0447: s << "EQVB"; break;

      // Shift and Rotate
    case 0240: s << "ASH"; break;
    case 0241: s << "ROT"; break;
    case 0242: s << "LSH"; break;
    case 0244: s << "ASHC"; break;
    case 0245: s << "ROTC"; break;
    case 0246: s << "LSHC"; break;

      // Arithmetic Testing
    case 0252: s << "AOBJP"; break;
    case 0253: s << "AOBJN"; break;
    case 0300: s << "CAI"; break;
    case 0301: s << "CAIL"; break;
    case 0302: s << "CAIE"; break;
    case 0303: s << "CAILE"; break;
    case 0304: s << "CAIA"; break;
    case 0305: s << "CAIGE"; break;
    case 0306: s << "CAIN"; break;
    case 0307: s << "CAIG"; break;
    case 0310: s << "CAM"; break;
    case 0311: s << "CAML"; break;
    case 0312: s << "CAME"; break;
    case 0313: s << "CAMLE"; break;
    case 0314: s << "CAMA"; break;
    case 0315: s << "CAMGE"; break;
    case 0316: s << "CAMN"; break;
    case 0317: s << "CAMG"; break;
    case 0320: s << "JUMP"; break;
    case 0321: s << "JUMPL"; break;
    case 0322: s << "JUMPE"; break;
    case 0323: s << "JUMPLE"; break;
    case 0324: s << "JUMPA"; break;
    case 0325: s << "JUMPGE"; break;
    case 0326: s << "JUMPN"; break;
    case 0327: s << "JUMPG"; break;
    case 0330: s << "SKIP"; break;
    case 0331: s << "SKIPL"; break;
    case 0332: s << "SKIPE"; break;
    case 0333: s << "SKIPLE"; break;
    case 0334: s << "SKIPA"; break;
    case 0335: s << "SKIPGE"; break;
    case 0336: s << "SKIPN"; break;
    case 0337: s << "SKIPG"; break;
    case 0340: s << "AOJ"; break;
    case 0341: s << "AOJL"; break;
    case 0342: s << "AOJE"; break;
    case 0343: s << "AOJLE"; break;
    case 0344: s << "AOJA"; break;
    case 0345: s << "AOJGE"; break;
    case 0346: s << "AOJN"; break;
    case 0347: s << "AOJG"; break;
    case 0360: s << "SOJ"; break;
    case 0361: s << "SOJL"; break;
    case 0362: s << "SOJE"; break;
    case 0363: s << "SOJLE"; break;
    case 0364: s << "SOJA"; break;
    case 0365: s << "SOJGE"; break;
    case 0366: s << "SOJN"; break;
    case 0367: s << "SOJG"; break;
    case 0350: s << "AOS"; break;
    case 0351: s << "AOSL"; break;
    case 0352: s << "AOSE"; break;
    case 0353: s << "AOSLE"; break;
    case 0354: s << "AOSA"; break;
    case 0355: s << "AOSGE"; break;
    case 0356: s << "AOSN"; break;
    case 0357: s << "AOSG"; break;
    case 0370: s << "SOS"; break;
    case 0371: s << "SOSL"; break;
    case 0372: s << "SOSE"; break;
    case 0373: s << "SOSLE"; break;
    case 0374: s << "SOSA"; break;
    case 0375: s << "SOSGE"; break;
    case 0376: s << "SOSN"; break;
    case 0377: s << "SOSG"; break;

      // Logical Testing and Modification
    case 0600: s << "TRN"; break;
    case 0601: s << "TLN"; break;
    case 0602: s << "TRNE"; break;
    case 0603: s << "TLNE"; break;
    case 0604: s << "TRNA"; break;
    case 0605: s << "TLNA"; break;
    case 0606: s << "TRNN"; break;
    case 0607: s << "TLNN"; break;
    case 0620: s << "TRZ"; break;
    case 0621: s << "TLZ"; break;
    case 0622: s << "TRZE"; break;
    case 0623: s << "TLZE"; break;
    case 0624: s << "TRZA"; break;
    case 0625: s << "TLZA"; break;
    case 0626: s << "TRZN"; break;
    case 0627: s << "TLZN"; break;
    case 0640: s << "TRC"; break;
    case 0641: s << "TLC"; break;
    case 0642: s << "TRCE"; break;
    case 0643: s << "TLCE"; break;
    case 0644: s << "TRCA"; break;
    case 0645: s << "TLCA"; break;
    case 0646: s << "TRCN"; break;
    case 0647: s << "TLCN"; break;
    case 0660: s << "TRO"; break;
    case 0661: s << "TLO"; break;
    case 0662: s << "TROE"; break;
    case 0663: s << "TLOE"; break;
    case 0664: s << "TROA"; break;
    case 0665: s << "TLOA"; break;
    case 0666: s << "TRON"; break;
    case 0667: s << "TLON"; break;
    case 0610: s << "TDN"; break;
    case 0611: s << "TSN"; break;
    case 0612: s << "TDNE"; break;
    case 0613: s << "TSNE"; break;
    case 0614: s << "TDNA"; break;
    case 0615: s << "TSNA"; break;
    case 0616: s << "TDNN"; break;
    case 0617: s << "TSNN"; break;
    case 0630: s << "TDZ"; break;
    case 0631: s << "TSZ"; break;
    case 0632: s << "TDZE"; break;
    case 0633: s << "TSZE"; break;
    case 0634: s << "TDZA"; break;
    case 0635: s << "TSZA"; break;
    case 0636: s << "TDZN"; break;
    case 0637: s << "TSZN"; break;
    case 0650: s << "TDC"; break;
    case 0651: s << "TSC"; break;
    case 0652: s << "TDCE"; break;
    case 0653: s << "TSCE"; break;
    case 0654: s << "TDCA"; break;
    case 0655: s << "TSCA"; break;
    case 0656: s << "TDCN"; break;
    case 0657: s << "TSCN"; break;
    case 0670: s << "TDO"; break;
    case 0671: s << "TSO"; break;
    case 0672: s << "TDOE"; break;
    case 0673: s << "TSOE"; break;
    case 0674: s << "TDOA"; break;
    case 0675: s << "TSOA"; break;
    case 0676: s << "TDON"; break;
    case 0677: s << "TSON"; break;

      // Half Word Data Transmission
    case 0500: s << "HLL"; break;
    case 0501: s << "XHLLI"; break;
    case 0502: s << "HLLM"; break;
    case 0503: s << "HLLS"; break;
    case 0510: s << "HLLZ"; break;
    case 0511: s << "HLLZI"; break;
    case 0512: s << "HLLZM"; break;
    case 0513: s << "HLLZS"; break;
    case 0530: s << "HLLE"; break;
    case 0531: s << "HLLEI"; break;
    case 0532: s << "HLLEM"; break;
    case 0533: s << "HLLES"; break;
    case 0520: s << "HLLO"; break;
    case 0521: s << "HLLOI"; break;
    case 0522: s << "HLLOM"; break;
    case 0523: s << "HLLOS"; break;
    case 0544: s << "HLR"; break;
    case 0545: s << "HLRI"; break;
    case 0546: s << "HLRM"; break;
    case 0547: s << "HLRS"; break;
    case 0554: s << "HLRZ"; break;
    case 0555: s << "HLRZI"; break;
    case 0556: s << "HLRZM"; break;
    case 0557: s << "HLRZS"; break;
    case 0564: s << "HLRO"; break;
    case 0565: s << "HLROI"; break;
    case 0566: s << "HLROM"; break;
    case 0567: s << "HLROS"; break;
    case 0574: s << "HLRE"; break;
    case 0575: s << "HLREI"; break;
    case 0576: s << "HLREM"; break;
    case 0577: s << "HLRES"; break;
    case 0540: s << "HRR"; break;
    case 0541: s << "HRRI"; break;
    case 0542: s << "HRRM"; break;
    case 0543: s << "HRRS"; break;
    case 0550: s << "HRRZ"; break;
    case 0551: s << "HRRZI"; break;
    case 0552: s << "HRRZM"; break;
    case 0553: s << "HRRZS"; break;
    case 0560: s << "HRRO"; break;
    case 0561: s << "HRROI"; break;
    case 0562: s << "HRROM"; break;
    case 0563: s << "HRROS"; break;
    case 0570: s << "HRRE"; break;
    case 0571: s << "HRREI"; break;
    case 0572: s << "HRREM"; break;
    case 0573: s << "HRRES"; break;
    case 0504: s << "HRL"; break;
    case 0505: s << "HRLI"; break;
    case 0506: s << "HRLM"; break;
    case 0507: s << "HRLS"; break;
    case 0514: s << "HRLZ"; break;
    case 0515: s << "HRLZI"; break;
    case 0516: s << "HRLZM"; break;
    case 0517: s << "HRLZS"; break;
    case 0524: s << "HRLO"; break;
    case 0525: s << "HRLOI"; break;
    case 0526: s << "HRLOM"; break;
    case 0527: s << "HRLOS"; break;
    case 0534: s << "HRLE"; break;
    case 0535: s << "HRLEI"; break;
    case 0536: s << "HRLEM"; break;
    case 0537: s << "HRLES"; break;

      // Program Control
    case 0256: s << "XCT"; break;
    case 0243: s << "JFFO"; break;
    case 0255: s << "JFCL"; break;
    case 0254: s << "JRST"; break;
    case 0264: s << "JSR"; break;
    case 0265: s << "JSP"; break;
    case 0266: s << "JSA"; break;
    case 0267: s << "JRA"; break;
    case 0257: s << "MAP"; break;

      // Stack
    case 0261: s << "PUSH"; break;
    case 0262: s << "POP"; break;
    case 0260: s << "PUSHJ"; break;
    case 0263: s << "POPJ"; break;
    case 0105: s << "ADJSP"; break;

      // Byte Manipulation
    case 0133: s << "IBP"; break;
    case 0135: s << "LDB"; break;
    case 0137: s << "DPB"; break;
    case 0134: s << "ILDB"; break;
    case 0136: s << "IDPB"; break;

      // UUO
    case 0001: s << "LUUO"; break;
    case 0002: s << "LUUO"; break;
    case 0003: s << "LUUO"; break;
    case 0004: s << "LUUO"; break;
    case 0005: s << "LUUO"; break;
    case 0006: s << "LUUO"; break;
    case 0007: s << "LUUO"; break;
    case 0010: s << "LUUO"; break;
    case 0011: s << "LUUO"; break;
    case 0012: s << "LUUO"; break;
    case 0013: s << "LUUO"; break;
    case 0014: s << "LUUO"; break;
    case 0015: s << "LUUO"; break;
    case 0016: s << "LUUO"; break;
    case 0017: s << "LUUO"; break;
    case 0020: s << "LUUO"; break;
    case 0021: s << "LUUO"; break;
    case 0022: s << "LUUO"; break;
    case 0023: s << "LUUO"; break;
    case 0024: s << "LUUO"; break;
    case 0025: s << "LUUO"; break;
    case 0026: s << "LUUO"; break;
    case 0027: s << "LUUO"; break;
    case 0030: s << "LUUO"; break;
    case 0031: s << "LUUO"; break;
    case 0032: s << "LUUO"; break;
    case 0033: s << "LUUO"; break;
    case 0034: s << "LUUO"; break;
    case 0035: s << "LUUO"; break;
    case 0036: s << "LUUO"; break;
    case 0037: s << "LUUO"; break;

    case 0104: s << "JSYS"; break;

      // EXTEND
    case 0123: s << "EXTEND"; break;

    default:
      s << "MUUO";
      break;
    }
  }

  s << "\t";

  if (isIO) {

    switch (ioDev) {
    case 00: s << "APR"; break;
    case 01: s << "PI"; break;
    case 02: s << "PAG"; break;
    case 03: s << "CCA"; break;
    case 04: s << "TIM"; break;
    case 05: s << "MTR"; break;
    default: s << setfill('0') << oct << ioDev; break;
    }
  } else {
    s << oct << setw(2) << right << ac;
  }

  s << ",";

  ostringstream eas;

  if (i != 0) eas << "@";

  if (debuggerP != nullptr) {

    if (x == 0) {
      eas << oct << left << setw(6) << debuggerP->symbolicForm(y);
    } else {
      eas << oct << debuggerP->symbolicForm(y) << "(" << oct << x << ")";
    }
  } else {

    if (x == 0) {
      eas << oct << left << setw(6) << y;
    } else {
      eas << oct << y << "(" << oct << x << ")";
    }
  }

  s << setfill(' ') << setw(13) << left << eas.str();

  return s.str();
}
