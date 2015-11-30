#ifndef _INLINEHOOK_H
#define _INLINEHOOK_H

#include <elf.h>

#include "list.h"

struct soinfo {
	char name[128];
	const Elf32_Phdr* phdr;
	size_t phnum;
	Elf32_Addr entry;
	Elf32_Addr base;
	unsigned size;

	uint32_t unused1;  // DO NOT USE, maintained for compatibility.

	Elf32_Dyn* dynamic;

	uint32_t unused2; // DO NOT USE, maintained for compatibility
	uint32_t unused3; // DO NOT USE, maintained for compatibility

	struct _soinfo* next;
	unsigned flags;

	const char* strtab;
	Elf32_Sym* symtab;

	size_t nbucket;
	size_t nchain;
	unsigned* bucket;
	unsigned* chain;

	unsigned* plt_got;

	Elf32_Rel* plt_rel;
	size_t plt_rel_count;

	Elf32_Rel* rel;
	size_t rel_count;
};

struct inlineHookInfo {
	struct list_head list;
	char so_name[128];
	char function_name[128];
	uint32_t target_addr;
	uint32_t new_addr;
	uint32_t **proto_addr;
	void *orig_instructions;
	void *trampoline_instructions;
	int status;
};

int unregisterInlineHookByName(const char *function_name, const char *so_name);
int unregisterInlineHookByAddr(uint32_t target_addr);
int registerInlineHookByName(const char *function_name, const char *so_name, uint32_t offset, uint32_t new_addr, uint32_t **proto_addr);
int registerInlineHookByAddr(uint32_t target_addr, uint32_t new_addr, uint32_t **proto_addr);
int inlineUnHook();
int inlineHook();

#endif