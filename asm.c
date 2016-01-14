#include "asm.h"

int inline asmCacheFlush(unsigned int begin, unsigned int end)
{	
	const int syscall = 0xF0002;
	int ret;

	__asm __volatile (
		"push    {r1, r2, r7}\n\t"
		"mov	 r0, %[begin]\n\t"
		"mov	 r1, %[end]\n\t"
		"mov     r2, #0x0\n\t"
		"mov	 r7, %[syscall]\n\t"
		"svc     0x00000000\n\t"
		"mov     %[ret], r0\n\t"
		"pop    {r1, r2, r7}\n\t"

		:   [ret] "=r" (ret)
		:	[begin] "r" (begin), [end] "r" (end), [syscall] "r" (syscall)
		:	"r0", "r1", "r2", "r7"
	);

	return ret;
}

void inline *asmMmap2(void *addr, size_t length, int prot, int flags, int fd, off_t pgoffset)
{
	const int syscall = 0xC0;
	void *ret;

	__asm __volatile (
		"push    {r1, r2, r3, r4, r5, r7}\n\t"
		"mov	 r0, %[addr]\n\t"
		"mov	 r1, %[length]\n\t"
		"mov	 r2, %[prot]\n\t"
		"mov	 r3, %[flags]\n\t"
		"mov	 r4, %[fd]\n\t"
		"mov	 r5, %[pgoffset]\n\t"
		"mov	 r7, %[syscall]\n\t"
		"svc     0x00000000\n\t"
		"mov     %[ret], r0\n\t"
		"pop    {r1, r2, r3, r4, r5, r7}\n\t"

		:   [ret] "=r" (ret)
		:	[addr] "r" (addr), [length] "r" (length), [prot] "r" (prot), [flags] "r" (flags), [fd] "r" (fd), [pgoffset] "r" (pgoffset), [syscall] "r" (syscall)

	);

	return ret;
}
