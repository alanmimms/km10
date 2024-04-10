#include <stdio.h>
#include <stdint.h>

#include "kl10.h"
#include "disasm.h"
#include "loada10.h"







int main(int argc, char *argv[]) {
  static W36 memory[256*1024];
  W36 startAddr, lowestAddr, highestAddr;
  char *fileNameP;

  if (argc == 2) {
    fileNameP = argv[1];
  } else {
    return -1;
  }

  int st = LoadA10(fileNameP, memory, &startAddr, &lowestAddr, &highestAddr);
  fprintf(stderr, "[Loaded %s  st=%d  start=" PRI06o64 "]\n", fileNameP, st, startAddr);
  return 0;
}
