#ifndef __LOADA10_H__
#define  __LOADA10_H__ 1

#include "kl10.h"

extern int LoadA10(const char *fileNameP,
		   W36 *memP,
		   W36 *startAddrP,
		   W36 *lowestAddrP,
		   W36 *highestAddrP);

#endif
