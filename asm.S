.text

.align 0
.global asm_mmap2
.type asm_mmap2, %function
asm_mmap2:
.fnstart
    mov     ip, sp
    stmfd   sp!, {r4, r5, r6, r7}
    ldmfd   ip, {r4, r5, r6}
    ldr     r7, =#0xC0
    swi     #0
    ldmfd   sp!, {r4, r5, r6, r7}
    bx    lr
.fnend;
.size asm_mmap2, .-asm_mmap2;

.align 0
.global asm_cacheflush
.type asm_cacheflush, %function
asm_cacheflush:
.fnstart
    mov     ip, r7
    ldr     r7, =#0xF0002
    swi     #0
    mov     r7, ip
    bx    lr
.fnend;
.size asm_cacheflush, .-asm_cacheflush;
