// This reads the DEC A10 format from the specified file into memory.
#include <stdio.h>
#include <stdint.h>
#include <string.h>


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
// converts it from its ASCIIized form into an integer value. On
// entry, *pp must point to the first character of a token. On exit,
// this is also the case or else **pp is 0.
//
// Example:
//   |      <---- *pp points to the '^' at start and to the next 'A' on exit.
// T ^,AEh,E,LF@,E,O?m,FC,E,Aru,Lj@,F,AEv,F@@,E,,AJB,L,AnT,F@@,E,Arz,Lk@,F,AEw,F@@,E,E,ND@,K,B,NJ@,E,B`K
static inline uint16_t unASCIIize(char **pp) {
  uint16_t v = 0;
  int shift = 0;
  char *tokenStartP = *pp;
  char *p = *pp;

  // Find the end of the token.
  while (*p != ',' && *p != 0) ++p;

  // Point to next token for next call, but leave pointer at NUL byte
  // if that's what is next.
  *pp = *p == 0 ? p : p + 1;

  // At this point `p` points to the delimiter after the token we need
  // to scan. Accumulate six-bit values from right to left.
  while (--p >= tokenStartP) {
    v |= (*p & 0077) << shift;
    shift += 6;
  }

  return v;
}


// Returns number of words loaded or -1 for error.
// Pass filename and a pointer to memory big enough to hold the program.
// Returns start address in startAddrP if non-NULL.
int LoadA10(const char *fileNameP, uint64_t *memP, uint32_t *startAddrP) {
  uint32_t addr = 0;
  uint32_t highestAddr = 0;
  uint32_t lowestAddr = 0777777;
  uint32_t wordCount = 0;
  uint32_t startAddr;

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

    switch (recType) {
    case 'Z': {
      int zeroCount = unASCIIize(&p);
      if (zeroCount == 0) zeroCount = 64*1024;

      for (uint32_t offset = 0; offset < zeroCount; ++offset) {
	uint32_t a = addr + offset;

	if (a > highestAddr) highestAddr = a;
	if (a < lowestAddr) lowestAddr = a;
	memP[a] = 0;
      }
      }
      break;

    case 'T': {

      if (wc == 0) {
        startAddr = addr;
	if (startAddrP) *startAddrP = addr;
      }

      uint32_t offset = 0;

      for (int token = 0; token < wc; token += 3) {
	uint16_t w0 = unASCIIize(&p);
	uint16_t w1 = unASCIIize(&p);
	uint16_t w2 = unASCIIize(&p);
	uint64_t w = ((w2 & 0x0F) << 16) | (w1 << 16) | w0;
	uint32_t a = addr + offset;

	if (a > highestAddr) highestAddr = a;
	if (a < lowestAddr) lowestAddr = a;
	memP[a] = w;
	++wordCount;
      }
      }
      break;
      
    default:
      fprintf(stderr, "ERROR: Unknown record type '%c' in file \"%s\"\n", recType, fileNameP);
      break;      
    }
  }

  fclose(inF);

  fprintf(stderr, "[loaded %s from %06o to %06o start=%06o]\n",
	  fileNameP, lowestAddr, highestAddr, startAddr);
  return wordCount;
}


#if TEST_LOADA10
int main(int argc, char *argv[]) {
  static uint64_t memory[256*1024];
  uint32_t startAddr = 0;
  char *fileNameP = NULL;

  if (argc == 2) fileNameP = argv[1];

  if (!fileNameP) {
    fprintf(stderr, "Usage:\n\
%s a10-file-path-name\n", argv[0]);
  }

  int st = LoadA10(fileNameP, memory, &startAddr);
  fprintf(stderr, "[LoadA10 returned %d]\n", st);
  return 0;
}
#endif
