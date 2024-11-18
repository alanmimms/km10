#include <iostream>
#include <fstream>
#include <limits>

#include "kmstate.hpp"

using namespace std;


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

auto KMState::getWord(ifstream &inS, [[maybe_unused]] const char *whyP) -> uint16_t {
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
void KMState::loadA10(const char *fileNameP) {
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
