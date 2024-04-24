#ifndef __KL10_H__
#define __KL10_H__ 1

#include <stdint.h>
#include <stdio.h>


// A 36-bit PDP10 word.
typedef uint64_t W36;

#define PRI06o32	"%06o"
#define PRI06o64	"%06lo"
#define PRI6o32		"%6o"
#define PRI6o64		"%6lo"


#define BIT(N)	(1ull << (35 - (N)))

static const W36 BIT0 = BIT(0);
static const W36 BIT17 = BIT(17);
static const W36 BIT18 = BIT(18);
static const W36 BIT35 = BIT(35);


static const W36 RHMASK = 0777777ull;
static const W36 LHMASK = 0777777000000ull;

// Bitmask for 36 bits.
static const W36 ALL1s = 0777777777777ull;


// Mask off extra bits above 36-bit word
#define TO36(V)		((V) & ALL1s)

// Determine if V is a 36-bit negative number.
#define ISNEG(V)	(!!((V) & BIT(0)))

// Convert 36-bit V into a signed long long for math ops.
#define TOSIGNED(V)	(ISNEG(V) ? (V) | ~ALL1s : TO36(V))


// See 1982_ProcRefMan.pdf p.262
typedef struct DTE20ControlBlock {
  W36 to11BP;
  W36 to10BP;
  W36 vectorInsn;
  W36 reserved;
  W36 examineAreaSize;
  W36 examineAreaReloc;
  W36 depositAreaSize;
  W36 depositAreaReloc;
} DTE20ControlBlock;


// See 1982_ProcRefMan.pdf p.230
typedef struct ExecutiveProcessTable {

  struct {
    W36 initialCommand;
    W36 statusWord;
    W36 lastUpdatedCommand;
    W36 reserved;
  } channelLogout[8];

  W36 reserved40_41[2];		// 040

  W36 pioInstructions[14];

  W36 channelBlockFill[4];	// 060

  W36 reserved64_137[44];
  
  DTE20ControlBlock dte20[4];	// 140

  W36 reserved200_420[145];

  W36 trap1Insn;		// 421
  W36 stackOverflowInsn;	// 422
  W36 trap3Insn;		// 423 (not used in KL10?)

  W36 reserved424_507[52];

  W36 timeBase[2];		// 510
  W36 performanceCount[2];	// 512
  W36 intervalCounterIntInsn;	// 514

  W36 reserved515_537[19];

  W36 execSection[32];		// 540

  W36 reserved600_777[128];	// 600
} ExecutiveProcessTable;


typedef struct UserProcessTable {
  W36 reserved000_417[0420];	// 000
  W36 luuoAddr;			// 420
  W36 trap1Insn;		// 421
  W36 stackOverflowInsn;	// 422
  W36 trap3Insn;		// 423 (not used in KL10?)
  W36 muuoFlagsOpAC;		// 424
  W36 muuoOldPC;		// 425
  W36 muuoE;			// 426
  W36 muuoContext;		// 427
  W36 kernelNoTrapMUUOPC;	// 430
  W36 kernelTrapMUUOPC;		// 431
  W36 supvNoTrapMUUOPC;		// 432
  W36 supvTrapMUUOPC;		// 433
  W36 concNoTrapMUUOPC;		// 434
  W36 concTrapMUUOPC;		// 435
  W36 publNoTrapMUUOPC;		// 436
  W36 publTrapMUUOPC;		// 437

  W36 reserved440_477[32];

  W36 pfWord;			// 500
  W36 pfFlags;			// 501
  W36 pfOldPC;			// 502
  W36 pfNewPC;			// 503

  W36 userExecTime[2];		// 504
  W36 userMemRefCount[2];	// 506

  W36 reserved510_537[24];

  W36 userSection[32];		// 540

  W36 reserved600_777[128];	// 600
} UserProcessTable;


static inline W36 CONS(W36 lh, W36 rh) {
  return ((lh & RHMASK) << 18) | (rh & RHMASK);
}


static inline W36 RH(W36 w) {
  return w & RHMASK;
};


static inline W36 LH(W36 w) {
  return (w & LHMASK) >> 18;
}


static inline W36 SWAP(W36 w) {
  return CONS(RH(w), LH(w));
}


static inline W36 SEXTEND(W36 rh) {
  return (rh & 0400000) ? (rh | LHMASK) : (rh & RHMASK);
}


// Return the bit shift for PDP-10 numbered bit n.
#define ShiftForBit(B)	(35 - (B))

// Return the bit mask for PDP-10 numbered bit n. This special-cases
// negative n to return zero. This is a macro because goddamned C99
// doesn't allow static inline functions as constants and I needed
// that.
#define MaskForBit(B)	(((B) < 0 || (B) > 35) ? 0ull : (1ull << 35) >> (B))


// Extract from 36-bit word `w` the PDP-10 numbered bits from `s` to
// `e` and return the result. This also strips off bits to the left of
// the 36-bit real content of `w`.
static inline W36 Extract(W36 w, int s, int e) {
  unsigned toShift = 35 - e;
  if (s > e) return 0ull;
  return (w & ALL1s & (MaskForBit(s - 1) - 1ull)) >> toShift;
}


// Format `w` as standard 123456,,654321 format and return a pointer
// to the buffer it's formatted into (which is passed in from caller).
// This makes printf suck a little less. (Why doesn't printf
// formatting have an extension API for those who want to add more
// formatting types?)
static char *oct36(char *bufP, W36 w) {
  sprintf(bufP, PRI06o32 ",," PRI06o32,
	  (uint32_t) (w >> 18), (uint32_t) (w & 0777777ul));
  return bufP;
}


// Format a PC or other virtual address as octal ,, pair.
static char *octVMA(char *bufP, W36 a) {
  sprintf(bufP, PRI06o32 ",," PRI06o32,
	  (uint32_t) (a >> 18) & 0177, (uint32_t) (a & 0777777ul));
  return bufP;
}

#endif
