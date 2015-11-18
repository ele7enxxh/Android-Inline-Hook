/*
thumb16 thumb32 arm32 inlineHook in Android
author: ele7enxxh
mail: ele7enxxh@qq.com
website: ele7enxxh.com
modified time: 2015-11-18
created time: 2015-11-12
*/

#include <stdio.h>
#include <string.h>
#include <dlfcn.h>
#include <sys/mman.h>
#include <asm/signal.h>

#define ENABLE_DEBUG
#include "inlineHook.h"

#ifndef PAGE_SIZE
#define PAGE_SIZE 4096
#endif

#define PAGE_START(addr) (~(PAGE_SIZE - 1) & (addr))
#define ALIGN_PC(pc)	(pc & 0xFFFFFFFC)

// THUMB16
#define B1_THUMB16		0	// B <label>
#define B2_THUMB16		1	// B <label>
#define BX_THUMB16		2	// BX PC
#define ADD_THUMB16		3	// ADD <Rdn>, PC (Rd != PC, Rn != PC) 在对ADD进行修正时，采用了替换PC为Rr的方法，当Rd也为PC时，由于之前更改了Rr的值，可能会影响跳转后的正常功能。
#define MOV_THUMB16		4	// MOV Rd, PC
#define ADR_THUMB16		5	// ADR Rd, <label>
#define LDR_THUMB16		6 	// LDR Rt, <label>
// THUMB32
#define BLX_THUMB32		7	// BLX <label>
#define BL_THUMB32 		8	// BL <label>
#define B1_THUMB32		9 	// B.W <label>
#define B2_THUMB32		10 	// B.W <label>
#define ADR1_THUMB32	11 	// ADR.W Rd, <label>
#define ADR2_THUMB32	12 	// ADR.W Rd, <label>
#define LDR_THUMB32		13	// LDR.W Rt, <label>
#define TBB_THUMB32		14	// TBB [PC, Rm]
#define TBH_THUMB32		15	// TBH [PC, Rm, LSL #1]
// ARM
#define BLX_ARM			16	// BLX <label>
#define BL_ARM 			17	// BL <label>
#define B_ARM			18	// B <label>
#define BX_ARM			19 	// BX PC
#define ADD_ARM			20	// ADD Rd, PC, Rm (Rd != PC, Rm != PC) 在对ADD进行修正时，采用了替换PC为Rr的方法，当Rd也为PC时，由于之前更改了Rr的值，可能会影响跳转后的正常功能;实际汇编中没有发现Rm也为PC的情况，故未做处理。
#define ADR1_ARM		21 	// ADR Rd, <label>
#define ADR2_ARM		22 	// ADR Rd, <label>
#define MOV_ARM			23	// MOV Rd, PC
#define LDR_ARM			24	// LDR Rt, <label>

#define UNDEFINE	99

struct inlineHookInfo *head_info = NULL;

void inline cacheFlush(unsigned int begin, unsigned int end)
{	
	const int syscall = 0xf0002;
	__asm __volatile (
		"mov	 r0, %0\n"			
		"mov	 r1, %1\n"
		"mov	 r7, %2\n"
		"mov     r2, #0x0\n"
		"svc     0x00000000\n"
		:
		:	"r" (begin), "r" (end), "r" (syscall)
		:	"r0", "r1", "r7"
		);
}

static int getTypeInThumb16(uint16_t instruction)
{
	if ((instruction & 0xF000) == 0xD000) {
		DEBUG_PRINT("B1_THUMB16\n");
		return B1_THUMB16;
	}
	if ((instruction & 0xF800) == 0xE000) {
		DEBUG_PRINT("B2_THUMB16\n");
		return B2_THUMB16;
	}
	if ((instruction & 0xFFF8) == 0x4778) {
		DEBUG_PRINT("BX_THUMB16\n");
		return BX_THUMB16;
	}
	if ((instruction & 0xFF78) == 0x4478) {
		DEBUG_PRINT("ADD_THUMB16\n");
		return ADD_THUMB16;
	}
	if ((instruction & 0xFF78) == 0x4678) {
		DEBUG_PRINT("MOV_THUMB16\n");
		return MOV_THUMB16;
	}
	if ((instruction & 0xF800) == 0xA000) {
		DEBUG_PRINT("ADR_THUMB16\n");
		return ADR_THUMB16;
	}
	if ((instruction & 0xF800) == 0x4800) {
		DEBUG_PRINT("LDR_THUMB16\n");
		return LDR_THUMB16;
	}
	return UNDEFINE;
}

static int getTypeInThumb32(uint32_t instruction)
{
	if ((instruction & 0xF800D000) == 0xF000C000) {
		DEBUG_PRINT("BLX_THUMB32\n");
		return BLX_THUMB32;
	}
	if ((instruction & 0xF800D000) == 0xF000D000) {
		DEBUG_PRINT("BL_THUMB32\n");
		return BL_THUMB32;
	}
	if ((instruction & 0xF800D000) == 0xF0008000) {
		DEBUG_PRINT("B1_THUMB32\n");
		return B1_THUMB32;
	}
	if ((instruction & 0xF800D000) == 0xF0009000) {
		DEBUG_PRINT("B2_THUMB32\n");
		return B2_THUMB32;
	}
	if ((instruction & 0xFBFF8000) == 0xF2AF0000) {
		DEBUG_PRINT("ADR1_THUMB32\n");
		return ADR1_THUMB32;
	}
	if ((instruction & 0xFBFF8000) == 0xF20F0000) {
		DEBUG_PRINT("ADR2_THUMB32\n");
		return ADR2_THUMB32;		
	}
	if ((instruction & 0xFF7F0000) == 0xF85F0000) {
		DEBUG_PRINT("LDR_THUMB32\n");
		return LDR_THUMB32;
	}
	if ((instruction & 0xFFFF00F0) == 0xE8DF0000) {
		return TBB_THUMB32;
	}
	if ((instruction & 0xFFFF00F0) == 0xE8DF0010) {
		return TBH_THUMB32;
	}
	return UNDEFINE;
}

static int getTypeInArm(uint32_t instruction)
{
	if ((instruction & 0xFE000000) == 0xFA000000) {
		DEBUG_PRINT("BLX_ARM\n");
		return BLX_ARM;
	}
	if ((instruction & 0xF000000) == 0xB000000) {
		DEBUG_PRINT("BL_ARM\n");
		return BL_ARM;
	}
	if ((instruction & 0xF000000) == 0xA000000) {
		DEBUG_PRINT("B_ARM\n");
		return B_ARM;
	}
	if ((instruction & 0xFF000FF) == 0x120001F) {
		DEBUG_PRINT("BX_ARM\n");
		return BX_ARM;
	}
	if ((instruction & 0xFEF0010) == 0x8F0000) {
		DEBUG_PRINT("ADD_ARM\n");
		return ADD_ARM;
	}
	if ((instruction & 0xFFF0000) == 0x28F0000) {
		DEBUG_PRINT("ADR1_ARM\n");
		return ADR1_ARM;
	}
	if ((instruction & 0xFFF0000) == 0x24F0000) {
		DEBUG_PRINT("ADR2_ARM\n");
		return ADR2_ARM;		
	}
	if ((instruction & 0xE5F0000) == 0x41F0000) {
		DEBUG_PRINT("LDR_ARM\n");
		return LDR_ARM;
	}
	if ((instruction & 0xFE00FFF) == 0x1A0000F) {
		DEBUG_PRINT("MOV_ARM\n");
		return MOV_ARM;
	}
	return UNDEFINE;
}

static int relocateInstructionInThumb16(uint32_t pc, uint16_t instruction, uint16_t *trampoline_instructions)
{
	int type;
	int offset;
	
	type = getTypeInThumb16(instruction);
	if (type == B1_THUMB16 || type == B2_THUMB16 || type == BX_THUMB16) {
		uint32_t x;
		int top_bit;
		uint32_t imm32;
		uint32_t value;
		int idx;
		
		idx = 0;
		if (type == B1_THUMB16) {
			x = (instruction & 0xFF) << 1;
			top_bit = x >> 8;
			imm32 = top_bit ? (x | (0xFFFFFFFF << 8)) : x;
			value = pc + imm32;
			trampoline_instructions[idx++] = instruction & 0xFF00;
			trampoline_instructions[idx++] = 0xE003;	// B PC, #6
		}
		else if (type == B2_THUMB16) {
			x = (instruction & 0x7FF) << 1;
			top_bit = x >> 11;
			imm32 = top_bit ? (x | (0xFFFFFFFF << 11)) : x;
			value = pc + imm32;
		}
		else if (type == BX_THUMB16) {
			value = pc;
		}
		
		trampoline_instructions[idx++] = 0xF8DF;
		trampoline_instructions[idx++] = 0xF000;	// LDR.W PC, [PC]
		trampoline_instructions[idx++] = value & 0xFFFF;
		trampoline_instructions[idx++] = value >> 16;
		offset = idx;
	}
	else if (type == ADD_THUMB16) {
		int rdn;
		int rm;
		int r;
		
		rdn = ((instruction & 0x80) >> 4) | (instruction & 0x7);
		
		for (r = 7; ; --r) {
			if (r != rdn) {
				break;
			}
		}
		
		trampoline_instructions[0] = 0xB400 | (1 << r);	// PUSH {Rr}
		trampoline_instructions[1] = 0x4802 | (r << 8);	// LDR Rr, [PC, #8]
		trampoline_instructions[2] = (instruction & 0xFF87) | (r << 3);
		trampoline_instructions[3] = 0xBC00 | (1 << r);	// POP {Rr}
		trampoline_instructions[4] = 0xE002;	// B PC, #4
		trampoline_instructions[5] = 0xBF00;
		trampoline_instructions[6] = pc & 0xFFFF;
		trampoline_instructions[7] = pc >> 16;
		offset = 8;
	}
	else if (type == MOV_THUMB16 || type == ADR_THUMB16 || type == LDR_THUMB16) {
		int r;
		uint32_t value;
		
		if (type == MOV_THUMB16) {
			r = instruction & 0x7;
			value = pc;
		}
		else if (type == ADR_THUMB16) {
			r = (instruction & 0x700) >> 8;
			value = ALIGN_PC(pc) + (instruction & 0xFF) << 2;
		}
		else {
			r = (instruction & 0x700) >> 8;
			value = ((uint32_t *) (ALIGN_PC(pc) + ((instruction & 0xFF) << 2)))[0];
		}

		trampoline_instructions[0] = 0x4800 | (r << 8);	// LDR Rd, [PC]
		trampoline_instructions[1] = 0xE001;	// B PC, #2
		trampoline_instructions[2] = value & 0xFFFF;
		trampoline_instructions[3] = value >> 16;
		offset = 4;
	}
	else {
		trampoline_instructions[0] = instruction;
		offset = 1;
	}
	
	return offset;
}

static int relocateInstructionInThumb32(uint32_t pc, uint16_t high_instruction, uint16_t low_instruction, uint16_t *trampoline_instructions)
{
	uint32_t instruction;
	int type;
	int idx;
	int offset;
	
	instruction = (high_instruction << 16) | low_instruction;
	type = getTypeInThumb32(instruction);
	idx = 0;
	if (type == BLX_THUMB32 || type == BL_THUMB32 || type == B1_THUMB32 || type == B2_THUMB32) {
		uint32_t j1;
		uint32_t j2;
		uint32_t s;
		uint32_t i1;
		uint32_t i2;
		uint32_t x;
		uint32_t imm32;
		uint32_t value;

		j1 = (low_instruction & 0x2000) >> 13;
		j2 = (low_instruction & 0x800) >> 11;
		s = (high_instruction & 0x400) >> 10;
		i1 = !(j1 ^ s);
		i2 = !(j2 ^ s);

		if (type == BLX_THUMB32 || type == BL_THUMB32) {
			trampoline_instructions[idx++] = 0xF20F;
			trampoline_instructions[idx++] = 0x0E09;	// ADD.W LR, PC, #9
		}
		else if (type == B1_THUMB32) {
			trampoline_instructions[idx++] = 0xD000 | ((high_instruction & 0x3C0) << 2);
			trampoline_instructions[idx++] = 0xE003;	// B PC, #6
		}
		trampoline_instructions[idx++] = 0xF8DF;
		trampoline_instructions[idx++] = 0xF000;	// LDR.W PC, [PC]
		if (type == BLX_THUMB32) {
			x = (s << 24) | (i1 << 23) | (i2 << 22) | ((high_instruction & 0x3FF) << 12) | ((low_instruction & 0x7FE) << 1);
			imm32 = s ? (x | (0xFFFFFFFF << 25)) : x;
			value = pc + imm32;
		}
		else if (type == BL_THUMB32) {
			x = (s << 24) | (i1 << 23) | (i2 << 22) | ((high_instruction & 0x3FF) << 12) | ((low_instruction & 0x7FF) << 1);
			imm32 = s ? (x | (0xFFFFFFFF << 25)) : x;
			value = pc + imm32 + 1;
		}
		else if (type == B1_THUMB32) {
			x = (s << 20) | (j2 << 19) | (j1 << 18) | ((high_instruction & 0x3F) << 12) | ((low_instruction & 0x7FF) << 1);
			imm32 = s ? (x | (0xFFFFFFFF << 21)) : x;
			value = pc + imm32 + 1;
		}
		else if (type == B2_THUMB32) {
			x = (s << 24) | (i1 << 23) | (i2 << 22) | ((high_instruction & 0x3FF) << 12) | ((low_instruction & 0x7FF) << 1);
			imm32 = s ? (x | (0xFFFFFFFF << 25)) : x;
			value = pc + imm32 + 1;
		}
		trampoline_instructions[idx++] = value & 0xFFFF;
		trampoline_instructions[idx++] = value >> 16;
		offset = idx;
	}
	else if (type == ADR1_THUMB32 || type == ADR2_THUMB32 || type == LDR_THUMB32) {
		int r;
		uint32_t imm32;
		uint32_t value;
		
		if (type == ADR1_THUMB32 || type == ADR2_THUMB32) {
			uint32_t i;
			uint32_t imm3;
			uint32_t imm8;
		
			r = (low_instruction & 0xF00) >> 8;
			i = (high_instruction & 0x400) >> 10;
			imm3 = (low_instruction & 0x7000) >> 12;
			imm8 = instruction & 0xFF;
			
			imm32 = (i << 31) | (imm3 << 30) | (imm8 << 27);
			
			if (type == ADR1_THUMB32) {
				value = ALIGN_PC(pc) + imm32;
			}
			else {
				value = ALIGN_PC(pc) - imm32;
			}
		}
		else {
			int is_add;
			uint32_t *addr;
			
			is_add = (high_instruction & 0x80) >> 7;
			r = low_instruction >> 12;
			imm32 = low_instruction & 0xFFF;
			
			if (is_add) {
				addr = (uint32_t *) (ALIGN_PC(pc) + imm32);
			}
			else {
				addr = (uint32_t *) (ALIGN_PC(pc) - imm32);
			}
			
			value = addr[0];
		}
		
		trampoline_instructions[0] = 0x4800 | (r << 8);	// LDR Rr, [PC]
		trampoline_instructions[1] = 0xE001;	// B PC, #2
		trampoline_instructions[2] = value & 0xFFFF;
		trampoline_instructions[3] = value >> 16;
		offset = 4;
	}

	else if (type == TBB_THUMB32 || type == TBH_THUMB32) {
		int rm;
		int r;
		int rx;
		
		rm = low_instruction & 0xF;
		
		for (r = 7;; --r) {
			if (r != rm) {
				break;
			}
		}
		
		for (rx = 7; ; --rx) {
			if (rx != rm && rx != r) {
				break;
			}
		}
		
		trampoline_instructions[0] = 0xB400 | (1 << rx);	// PUSH {Rx}
		trampoline_instructions[1] = 0x4805 | (r << 8);	// LDR Rr, [PC, #20]
		trampoline_instructions[2] = 0x4600 | (rm << 3) | rx;	// MOV Rx, Rm
		if (type == TBB_THUMB32) {
			trampoline_instructions[3] = 0xEB00 | r;
			trampoline_instructions[4] = 0x0000 | (rx << 8) | rx;	// ADD.W Rx, Rr, Rx
			trampoline_instructions[5] = 0x7800 | (rx << 3) | rx; 	// LDRB Rx, [Rx]
		}
		else if (type == TBH_THUMB32) {
			trampoline_instructions[3] = 0xEB00 | r;
			trampoline_instructions[4] = 0x0040 | (rx << 8) | rx;	// ADD.W Rx, Rr, Rx, LSL #1
			trampoline_instructions[5] = 0x8800 | (rx << 3) | rx; 	// LDRH Rx, [Rx]
		}
		trampoline_instructions[6] = 0xEB00 | r;
		trampoline_instructions[7] = 0x0040 | (r << 8) | rx;	// ADD Rr, Rr, Rx, LSL #1
		trampoline_instructions[8] = 0x3001 | (r << 8);	// ADD Rr, #1
		trampoline_instructions[9] = 0xBC00 | (1 << rx);	// POP {Rx}
		trampoline_instructions[10] = 0x4700 | (r << 3);	// BX Rr
		trampoline_instructions[11] = 0xBF00;
		trampoline_instructions[12] = pc & 0xFFFF;
		trampoline_instructions[13] = pc >> 16;
		offset = 14;
	}
	else {
		trampoline_instructions[0] = high_instruction;
		trampoline_instructions[1] = low_instruction;
		offset = 2;
	}

	return offset;
}

static void relocateInstructionInThumb(uint32_t target_addr, uint16_t *orig_instructions, int length, uint16_t *trampoline_instructions)
{
	int i;
	uint32_t pc;
	uint32_t lr;

	i = 0;
	pc = target_addr + 4;
	while (1) {
		int offset;
		
		if ((int) (&trampoline_instructions[0]) % 4 != 0) {
			trampoline_instructions[0] = 0xBF00;	// NOP
			trampoline_instructions += 1;
		}
		
		if ((orig_instructions[i] >> 11) >= 0x1D && (orig_instructions[i] >> 11) <= 0x1F) {
			offset = relocateInstructionInThumb32(pc, orig_instructions[i], orig_instructions[i + 1], trampoline_instructions);
			pc += sizeof(uint32_t);
			trampoline_instructions += offset;
			i += 2;
		}
		else {
			offset = relocateInstructionInThumb16(pc, orig_instructions[i], trampoline_instructions);
			pc += sizeof(uint16_t);
			trampoline_instructions += offset;
			++i;
		}
		
		if ((i + 1) >= length / sizeof(uint16_t)) {
			break;
		}
	}
	
	if ((int) (&trampoline_instructions[0]) % 4 != 0) {
		trampoline_instructions[0] = 0xBF00;	// NOP
		trampoline_instructions += 1;
	}
	
	lr = target_addr + i * sizeof(uint16_t) + 1;
	trampoline_instructions[0] = 0xF8DF;
	trampoline_instructions[1] = 0xF000;	// LDR.W PC, [PC]
	trampoline_instructions[2] = lr & 0xFFFF;
	trampoline_instructions[3] = lr >> 16;
}

static void relocateInstructionInArm(uint32_t target_addr, uint32_t *orig_instructions, int length, uint32_t *trampoline_instructions)
{
	uint32_t pc;
	uint32_t lr;
	int i;
	int idx;

	pc = target_addr + 8;
	lr = target_addr + length;

	idx = 0;
	for (i = 0; i < length / sizeof(uint32_t); ++i) {
		uint32_t instruction;
		int type;

		instruction = orig_instructions[i];
		type = getTypeInArm(instruction);
		if (type == BLX_ARM || type == BL_ARM || type == B_ARM || type == BX_ARM) {
			uint32_t x;
			int top_bit;
			uint32_t imm32;
			uint32_t value;

			if (type == BLX_ARM || type == BL_ARM) {
				trampoline_instructions[idx++] = 0xE28FE004;	// ADD LR, PC, #4
			}
			trampoline_instructions[idx++] = 0xE51FF004;  	// LDR PC, [PC, #-4]
			if (type == BLX_ARM) {
				x = ((instruction & 0xFFFFFF) << 2) | ((instruction & 0x1000000) >> 23);
			}
			else if (type == BL_ARM || type == B_ARM) {
				x = (instruction & 0xFFFFFF) << 2;
			}
			else {
				x = 0;
			}
			
			top_bit = x >> 25;
			imm32 = top_bit ? (x | (0xFFFFFFFF << 26)) : x;
			if (type == BLX_ARM) {
				value = pc + imm32 + 1;
			}
			else {
				value = pc + imm32;
			}
			trampoline_instructions[idx++] = value;
			
		}
		else if (type == ADD_ARM) {
			int rd;
			int rm;
			int r;
			
			rd = (instruction & 0xF000) >> 12;
			rm = instruction & 0xF;
			
			for (r = 12; ; --r) {
				if (r != rd && r != rm) {
					break;
				}
			}
			
			trampoline_instructions[idx++] = 0xE52D0004 | (r << 12);	// PUSH {Rr}
			trampoline_instructions[idx++] = 0xE59F0008 | (r << 12);	// LDR Rr, [PC, #8]
			trampoline_instructions[idx++] = (instruction & 0xFFF0FFFF) | (r << 16);
			trampoline_instructions[idx++] = 0xE49D0004 | (r << 12);	// POP {Rr}
			trampoline_instructions[idx++] = 0xE28FF000;	// ADD PC, PC
			trampoline_instructions[idx++] = pc;
		}
		else if (type == ADR1_ARM || type == ADR2_ARM || type == LDR_ARM || type == MOV_ARM) {
			int r;
			uint32_t value;
			
			r = (instruction & 0xF000) >> 12;
			
			if (type == ADR1_ARM || type == ADR2_ARM || type == LDR_ARM) {
				uint32_t imm32;
				
				imm32 = instruction & 0xFFF;
				if (type == ADR1_ARM) {
					value = pc + imm32;
				}
				else if (type == ADR2_ARM) {
					value = pc - imm32;
				}
				else if (type == LDR_ARM) {
					int is_add;
					
					is_add = (instruction & 0x800000) >> 23;
					if (is_add) {
						value = ((uint32_t *) (pc + imm32))[0];
					}
					else {
						value = ((uint32_t *) (pc - imm32))[0];
					}
				}
			}
			else {
				value = pc;
			}
				
			trampoline_instructions[idx++] = 0xE51F0000 | (r << 12);	// LDR Rr, [PC]
			trampoline_instructions[idx++] = 0xE28FF000;	// ADD PC, PC
			trampoline_instructions[idx++] = value;
		}
		else {
			trampoline_instructions[idx++] = instruction;
		}
		pc += sizeof(uint32_t);
	}
	
	trampoline_instructions[idx++] = 0xe51ff004;	// LDR PC, [PC, #-4]
	trampoline_instructions[idx++] = lr;
}

static void inlineHookInThumb(struct inlineHookInfo *info, uint32_t new_addr, uint32_t **proto_addr)
{
	int idx;
	
	info->orig_instructions = malloc(10);
	info->target_addr -= 1;
	memcpy(info->orig_instructions, (void *) info->target_addr, 10);

	mprotect((void *) PAGE_START(info->target_addr), PAGE_SIZE, PROT_READ | PROT_WRITE | PROT_EXEC);
	
	idx = 0;
	if (info->target_addr % 4 != 0) {
		((uint16_t *) info->target_addr)[idx++] = 0xBF00;	// NOP
	}

	((uint16_t *) info->target_addr)[idx++] = 0xF8DF;
	((uint16_t *) info->target_addr)[idx++] = 0xF000;	// LDR.W PC, [PC]
	((uint16_t *) info->target_addr)[idx++] = new_addr & 0xFFFF;
	((uint16_t *) info->target_addr)[idx++] = new_addr >> 16;

	mprotect((void *) PAGE_START(info->target_addr), PAGE_SIZE, PROT_READ | PROT_EXEC);

	info->trampoline_instructions = mmap(NULL, PAGE_SIZE, PROT_READ | PROT_WRITE | PROT_EXEC, MAP_ANONYMOUS | MAP_PRIVATE, 0, 0);
	relocateInstructionInThumb(info->target_addr, (uint16_t *) info->orig_instructions, 10, (uint16_t *) info->trampoline_instructions);
	if (proto_addr != NULL) {
		*proto_addr = info->trampoline_instructions + 1;
	}
}

static void inlineHookInArm(struct inlineHookInfo *info, uint32_t new_addr, uint32_t **proto_addr)
{
	info->orig_instructions = malloc(8);
	memcpy(info->orig_instructions, (void *) info->target_addr, 8);

	mprotect((void *) PAGE_START(info->target_addr), PAGE_SIZE, PROT_READ | PROT_WRITE | PROT_EXEC);

	((uint32_t *) (info->target_addr))[0] = 0xe51ff004;	// LDR PC, [PC, #-4]
	((uint32_t *) (info->target_addr))[1] = new_addr;

	mprotect((void *) PAGE_START(info->target_addr), PAGE_SIZE, PROT_READ | PROT_EXEC);

	info->trampoline_instructions = mmap(NULL, PAGE_SIZE, PROT_READ | PROT_WRITE | PROT_EXEC, MAP_ANONYMOUS | MAP_PRIVATE, 0, 0);
	relocateInstructionInArm(info->target_addr, (uint32_t *) info->orig_instructions, 8, (uint32_t *) info->trampoline_instructions);
	if (proto_addr != NULL) {
		*proto_addr = info->trampoline_instructions;
	}
}

static unsigned elfhash(const char *symbol_name)
{
	const unsigned char *name = (const unsigned char *) symbol_name;
	unsigned h = 0, g;

	while(*name) {
		h = (h << 4) + *name++;
		g = h & 0xf0000000;
		h ^= g;
		h ^= g >> 24;
	}
	return h;
}

static uint32_t findSymbolAddr(struct soinfo *si, const char *symbol_name)
{
	Elf32_Sym *symtab;
	const char *strtab;
	size_t i;

	symtab = si->symtab;
	strtab = si->strtab;
	
	int n = 1;
	unsigned hash = elfhash(symbol_name);
	for (i = si->bucket[hash % si->nbucket]; i != 0; i = si->chain[i]) {
		Elf32_Sym* s = symtab + i;
		n++;
		if (strcmp(strtab + s->st_name, symbol_name) == 0 && ELF32_ST_TYPE(s->st_info) == STT_FUNC) {
			return (s->st_value + si->base);
		}
	}

	/*
	for (i = 0; i < si->nchain; ++i) {
		if (strcmp((char *) (strtab + symtab[i].st_name), symbol_name) == 0 && ELF32_ST_TYPE(symtab[i].st_info) == STT_FUNC) {
			return (symtab[i].st_value + si->base);
		}
	}
	*/

	return 0;	
}

static int doInlineUnHook(struct inlineHookInfo *info)
{
	int is_arm;
	int length;
	
	if (info->target_addr %4 == 0) {
		is_arm = 1;
		length = 8;
	}
	else {
		is_arm = 0;
		info->target_addr -= 1;
		length = 10;
	}
	
	mprotect((void *) PAGE_START(info->target_addr), PAGE_SIZE, PROT_READ | PROT_WRITE | PROT_EXEC);
	memcpy((void *) info->target_addr, info->orig_instructions , length);
	mprotect((void *) PAGE_START(info->target_addr), PAGE_SIZE, PROT_READ | PROT_EXEC);
	
	free(info->orig_instructions);
	munmap(info->trampoline_instructions, PAGE_SIZE);
	
	DEBUG_PRINT("end inline unhooking, target_addr: 0x%x\n", info->target_addr);
	
	return 0;
}

int inlineUnHookByName(const char *function_name, const char *so_name)
{
	struct inlineHookInfo *info;
	
	if (function_name == NULL || so_name == NULL) {
		DEBUG_PRINT("illegal parameter\n");
		return -1;
	}
	
	if (head_info == NULL) {
		DEBUG_PRINT("no function is hooked\n");
		return -1;
	}
	
	DEBUG_PRINT("start inline unhooking, function_name: %s, so_name: %s\n", function_name, so_name);
	
	for (info = head_info; info != NULL; info = info->next)
	{
		if (strcmp(function_name, info->function_name) == 0 && strcmp(so_name, info->so_name) == 0) {
			return doInlineUnHook(info);
		}
	}
	
	DEBUG_PRINT("we do not need to unhook, function_name: %s, so_name: %s\n", function_name, so_name);
	return -1;
}

int inlineUnHookByAddr(uint32_t target_addr)
{
	struct inlineHookInfo *info;
	
	if (!target_addr) {
		DEBUG_PRINT("illegal parameter\n");
		return -1;
	}
	
	if (head_info == NULL) {
		DEBUG_PRINT("no function is hooked\n");
		return -1;
	}
	
	DEBUG_PRINT("start inline unhooking, target_addr: 0x%x\n", target_addr);
	
	for (info = head_info; info != NULL; info = info->next)
	{
		if (target_addr == info->target_addr) {
			return doInlineUnHook(info);
		}
	}
	
	DEBUG_PRINT("we do not need to unhook, target_addr: 0x%x\n", target_addr);
	return -1;
}

static int doInlineHook(struct inlineHookInfo *info, uint32_t new_addr, uint32_t **proto_addr)
{
	kill(-1, SIGSTOP);

	if (info->target_addr % 4 == 0) {
		inlineHookInArm(info, new_addr, proto_addr);
		cacheFlush(info->target_addr, info->target_addr + 8);
	}
	else {
		inlineHookInThumb(info, new_addr, proto_addr);
		cacheFlush(info->target_addr, info->target_addr + 10);
		info->target_addr += 1;
	}
	
	kill(-1, SIGCONT);
	
	DEBUG_PRINT("end inline hooking, target_addr: 0x%x, new_addr: 0x%x, *proto_addr: 0x%x\n", info->target_addr, new_addr, (uint32_t) *proto_addr);
	
	return 0;
}


int inlineHookByName(const char *function_name, const char *so_name, uint32_t offset, uint32_t new_addr, uint32_t **proto_addr)
{
	struct soinfo *si;
	struct inlineHookInfo *info;
	
	if (function_name == NULL || so_name == NULL || !new_addr) {
		DEBUG_PRINT("illegal parameter\n");
		return -1;
	}
	
	DEBUG_PRINT("start inline hooking, function_name: %s, so_name: %s, offset: 0x%x\n", function_name, so_name, offset);
	
	if (head_info == NULL) {
		head_info = (struct inlineHookInfo *) malloc(sizeof(struct inlineHookInfo));
		info = head_info;
	}
	else {
		head_info->next = (struct inlineHookInfo *) malloc(sizeof(struct inlineHookInfo));
		info = head_info->next;
	}
	
	strncpy(info->so_name, so_name, strlen(so_name));
	strncpy(info->function_name, function_name, strlen(function_name));
	
	si = (struct soinfo *) dlopen(info->so_name, RTLD_NOW);
	if (si == NULL) {
		DEBUG_PRINT("dlopen %s failed\n", info->so_name);
		return -1;
	}
		
	info->target_addr = findSymbolAddr(si, info->function_name);
	if (!info->target_addr) {
		DEBUG_PRINT("can not find %s in %s\n", info->function_name, info->so_name);
		return -1;
	}
	info->target_addr += offset;
	
	return doInlineHook(info, new_addr, proto_addr);
}

int inlineHookByAddr(uint32_t target_addr, uint32_t new_addr, uint32_t **proto_addr)
{
	struct inlineHookInfo *info;
	
	if (!target_addr || !new_addr) {
		DEBUG_PRINT("illegal parameter\n");
		return -1;
	}
	
	DEBUG_PRINT("start inline hooking, target_addr: 0x%x, new_addr: 0x%x\n", target_addr, new_addr);

	if (head_info == NULL) {
		head_info = (struct inlineHookInfo *) malloc(sizeof(struct inlineHookInfo));
		info = head_info;
	}
	else {
		head_info->next = (struct inlineHookInfo *) malloc(sizeof(struct inlineHookInfo));
		info = head_info->next;
	}

	info->target_addr = target_addr;
	
	return doInlineHook(info, new_addr, proto_addr);
}