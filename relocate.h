#ifndef _RELOCATE_H
#define _RELOCATE_H

#include <stdio.h>

void relocateInstructionInThumb(uint32_t target_addr, uint16_t *orig_instructions, int length, uint16_t *trampoline_instructions);
void relocateInstructionInArm(uint32_t target_addr, uint32_t *orig_instructions, int length, uint32_t *trampoline_instructions);

#endif