#ifndef __DISASM_H__
#define __DISASM_H__ 1

#include "kl10.h"

extern void Disassemble(W36 iw,
			const char **mnemonicPP,
			const char **ioDevPP,
			unsigned *acP,
			unsigned *iP,
			unsigned *xP,
			unsigned *yP);

extern void DisassembleToString(W36 iw, char *bufferP);

#endif
