#ifndef _ASM_H
#define _ASM_H

#include <stdio.h>

int inline asmCacheFlush(unsigned int begin, unsigned int end);
void inline *asmMmap2(void *addr, size_t length, int prot, int flags, int fd, off_t pgoffset);

#endif