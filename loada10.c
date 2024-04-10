// This reads the DEC A10 format from the specified file into memory.
#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include "kl10.h"
#include "loada10.h"

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


// Reverse the 16-bit value v.
static uint16_t bitReverse(uint16_t v) {
  uint16_t result = 0;

  for (uint16_t toMask = 0x8000; v; toMask >>= 1, v >>= 1) {
    if (v & 1) result |= toMask;
  }

  return result;
}


// This takes a "word" from the comma-delimited A10 format and
// converts it from its ASCIIized form into an 16-bit integer value.
// On entry, *pp must point to the first character of a token. On
// exit, *pp points to the start of the next token or else the NUL at
// the end of the string.
//
// Example:
//     |      <---- *pp points to the 'A' on entry
//     |   |  <---- and to the 'E' four chars later at exit.
// T ^,AEh,E,LF@,E,O?m,FC,E,Aru,Lj@,F,AEv,F@@,E,,AJB,L,AnT,F@@,E,Arz,Lk@,F,AEw,F@@,E,E,ND@,K,B,NJ@,E,B`K
static uint16_t unASCIIize(char **pp) {
  uint16_t v = 0;
  char *p = *pp;

  while (*p != ',' && *p != 0) {
    v = (v << 6) | (*p++ & 077);
  }

  *pp = *p == 0 ? p : p + 1;
  return v;
}


// Returns number of words loaded or -1 for error.
// Pass filename and a pointer to memory big enough to hold the program.
// Returns start address in startAddrP if non-NULL.
int LoadA10(const char *fileNameP, W36 *memP, W36 *startAddrP, W36 *lowestAddrP, W36 *highestAddrP) {
  W36 addr = 0;
  W36 highestAddr = 0;
  W36 lowestAddr = 0777777;
  W36 wordCount = 0;
  W36 startAddr;

  char line[1024];
  FILE *inF = fopen(fileNameP, "r");

  if (!inF) return -1;

  for (;;) {

    if (!fgets(line, sizeof(line) - 1, inF)) break;

    // Record type of this line.
    char recType = line[0];

    if (recType == ';') continue; /* Just ignore comment lines */

    // Kill newline at end of line if it's there.
    char *p = line + strlen(line) - 1;
    if (*p == '\n') *p = 0;

    // Initialize our token pointer for scanning.
    p = line + 2;

    // When we don't have any tokens we can skip processing this line.
    if (!p) break;

    // Count of words on this line.
    int wc = unASCIIize(&p);

    addr = unASCIIize(&p);
    addr |= wc & 0xC000;
    wc = wc & 0x3FFF;

    int zeroCount;
    W36 offset;

    switch (recType) {
    case 'Z':
      zeroCount = unASCIIize(&p);

      if (zeroCount == 0) zeroCount = 64*1024;

      for (W36 offset = 0; offset < zeroCount; ++offset) {
	W36 a = addr + offset;

	if (a > highestAddr) highestAddr = a;
	if (a < lowestAddr) lowestAddr = a;
	memP[a] = 0;
      }

      break;

    case 'T':
      if (wc == 0) startAddr = addr;

      for (offset = 0; offset < wc; ++offset) {
	W36 w0 = unASCIIize(&p);
	W36 w1 = unASCIIize(&p);
	W36 w2 = unASCIIize(&p);
	W36 w = ((w2 & 0x0F) << 32) | (w1 << 16) | w0;
	W36 a = addr + offset;

	if (a > highestAddr) highestAddr = a;
	if (a < lowestAddr) lowestAddr = a;
	memP[a] = w;
	++wordCount;
      }

      break;
      
    default:
      fprintf(stderr, "ERROR: Unknown record type '%c' in file \"%s\"\n", recType, fileNameP);
      break;      
    }
  }

  fclose(inF);

  if (startAddrP) *startAddrP = startAddr;
  if (lowestAddrP) *lowestAddrP = lowestAddr;
  if (highestAddrP) *highestAddrP = highestAddr;

  fprintf(stderr, "[loaded %s from " PRI06o64 " to " PRI06o64 " start=" PRI06o64 "]\n",
	  fileNameP, lowestAddr, highestAddr, startAddr);
  return wordCount;
}


#if TEST_LOADA10
#include "acutest.h"
#include "disasm.h"


static void testBitReverse(void) {
  uint16_t was;

  was = bitReverse( 0xFFFF);
  TEST_CHECK(was == 0xFFFF);
  was = bitReverse( 0x0000);
  TEST_CHECK(was == 0x0000);
  was = bitReverse( 0xAAAA);
  TEST_CHECK(was == 0x5555);
  was = bitReverse( 0x5555);
  TEST_CHECK(was == 0xAAAA);

  was = bitReverse( 0x8000);
  TEST_CHECK(was == 0x0001);
  was = bitReverse( 0x4000);
  TEST_CHECK(was == 0x0002);
  was = bitReverse( 0x2000);
  TEST_CHECK(was == 0x0004);
  was = bitReverse( 0x1000);
  TEST_CHECK(was == 0x0008);

  was = bitReverse( 0x0800);
  TEST_CHECK(was == 0x0010);
  was = bitReverse( 0x0400);
  TEST_CHECK(was == 0x0020);
  was = bitReverse( 0x0200);
  TEST_CHECK(was == 0x0040);
  was = bitReverse( 0x0100);
  TEST_CHECK(was == 0x0080);

  was = bitReverse( 0x0080);
  TEST_CHECK(was == 0x0100);
  was = bitReverse( 0x0040);
  TEST_CHECK(was == 0x0200);
  was = bitReverse( 0x0020);
  TEST_CHECK(was == 0x0400);
  was = bitReverse( 0x0010);
  TEST_CHECK(was == 0x0800);

  was = bitReverse( 0x0008);
  TEST_CHECK(was == 0x1000);
  was = bitReverse( 0x0004);
  TEST_CHECK(was == 0x2000);
  was = bitReverse( 0x0002);
  TEST_CHECK(was == 0x4000);
  was = bitReverse( 0x0001);
  TEST_CHECK(was == 0x8000);
}


static void testUnASCIIize(void) {
  char buf[64];
  char *p;
  W36 was;
  W36 w = 0;

  p = "A@@,A@B,F@@,E";
  was = unASCIIize(&p);
  TEST_CHECK(was == 010000);
  TEST_MSG("Was: " PRI06o64 "  SB: 010000", was);
  was = unASCIIize(&p);
  w = was;
  TEST_CHECK(was == 0010002);
  TEST_MSG("Was: " PRI06o64 "  SB: 0010002", was);
  was = unASCIIize(&p);
  w |= was << 16;
  TEST_CHECK(was == 0060000);
  TEST_MSG("Was: " PRI06o64 "  SB: 0060000", was);
  was = unASCIIize(&p);
  w |= was << 32;
  TEST_CHECK(was == 0000005);
  TEST_MSG("Was: " PRI06o64 "  SB: 0000005", was);
  TEST_MSG("W: %s", oct36(buf, w));

  p = ",IDj,H";
  was = unASCIIize(&p);
  w = was;
  TEST_CHECK(was == 0000000);
  TEST_MSG("Was: " PRI06o64 "  SB: 000000", was);
  was = unASCIIize(&p);
  w |= was << 16;
  TEST_CHECK(was == 0110452);
  TEST_MSG("Was: " PRI06o64 "  SB: 0110452", was);
  was = unASCIIize(&p);
  w |= was << 32;
  TEST_CHECK(was == 0000010);
  TEST_MSG("Was: " PRI06o64 "  SB: 000010", was);
  TEST_MSG("W: %s", oct36(buf, w));
}


static const char fileName[] = "../images/klddt/klddt.a10";


static void testLoadKLDDT(void) {
  static W36 memory[256*1024];
  W36 startAddr, lowestAddr, highestAddr;

  int st = LoadA10(fileName, memory, &startAddr, &lowestAddr, &highestAddr);
  fprintf(stderr, "[LoadA10 %s returned %d]\n", fileName, st);

  char disassembly[1024];

  for (W36 a = lowestAddr; a <= highestAddr; ++a) {
    char buf[64];
    DisassembleToString(memory[a], disassembly);
    fprintf(stderr, PRI06o64 ": %s    %s\n", a, oct36(buf, memory[a]), disassembly);
  }
}


TEST_LIST = {
  {"bitReverse", testBitReverse},
  {"unASCIIize", testUnASCIIize},
  {"loadKLDDT", testLoadKLDDT},
  {NULL, NULL},
};
#endif
