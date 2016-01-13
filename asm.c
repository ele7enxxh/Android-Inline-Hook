#include "asm.h"

void inline asmCacheFlush(unsigned int begin, unsigned int end)
{	
	const int syscall = 0xF0002;
	__asm __volatile (
		"push    {r1, r2, r7}\t\n"
		"mov	 r0, %0\t\n"
		"mov	 r1, %1\t\n"
		"mov	 r7, %2\t\n"
		"mov     r2, #0x0\t\n"
		"svc     0x00000000\t\n"
		"pop    {r1, r2, r7}\t\n"
		:
		:	"r" (begin), "r" (end), "r" (syscall)
		:	"r0", "r1", "r2", "r7"
		);
}

void inline *asmMmap2(void *addr, size_t length, int prot, int flags, int fd, off_t pgoffset)
{
	const int syscall = 0xC0;
	__asm __volatile (
		"push    {r1, r2, r3, r4, r5, r7}\t\n"
		"mov	 r0, %0\t\n"
		"mov	 r1, %1\t\n"
		"mov	 r2, %2\t\n"
		"mov	 r3, %3\t\n"
		"mov	 r4, %4\t\n"
		"mov	 r5, %5\t\n"
		"mov     r7, %6\t\n"
		"svc     0x00000000\t\n"
		"pop    {r1, r2, r3, r4, r5, r7}\t\n"
		:
		:	"r" (addr), "r" (length), "r" (prot), "r" (flags), "r" (fd), "r" (pgoffset), "r" (syscall)
		);
}