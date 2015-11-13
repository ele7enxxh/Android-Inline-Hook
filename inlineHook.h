#ifndef _INLINEHOOK_H
#define _INLINEHOOK_H

#include <elf.h>

#ifdef ENABLE_DEBUG
#include <android/log.h>
#define LOG_TAG "secauo_inlineHook"
#define LOGD(fmt, args...) __android_log_print(ANDROID_LOG_DEBUG,LOG_TAG, fmt, ##args)
#define DEBUG_PRINT(fmt,args...) LOGD(fmt, ##args)
#else
#define DEBUG_PRINT(fmt,args...)
#endif

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
	char so_name[128];
	char symbol_name[128];
	uint32_t offset;
	uint32_t target_addr;
	uint32_t new_addr;
	void *orig_instructions;
	void *trampoline_instructions;
};

int inlineUnHook(struct inlineHookInfo *info);
int inlineHook(struct inlineHookInfo *info);

#endif