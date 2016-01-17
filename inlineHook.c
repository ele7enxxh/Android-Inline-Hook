/*
thumb16 thumb32 arm32 inlineHook
author: ele7enxxh
mail: ele7enxxh@qq.com
website: ele7enxxh.com
modified time: 2015-01-17
created time: 2015-11-30
*/

#include <string.h>
#include <stdbool.h>
#include <sys/mman.h>

#include "relocate.h"
#include "inlineHook.h"

#ifndef PAGE_SIZE
#define PAGE_SIZE 4096
#endif

#define PAGE_START(addr) (~(PAGE_SIZE - 1) & (addr))

enum hook_status {
	REGISTERED,
	HOOKED,
};

struct inlineHookItem {
	char so_name[128];
	char function_name[128];
	uint32_t target_addr;
	uint32_t new_addr;
	uint32_t **proto_addr;
	void *orig_instructions;
	void *trampoline_instructions;
	int length;
	int status;
};

struct inlineHookInfo {
	struct inlineHookItem item[1024];
	int size;
};

static struct inlineHookInfo info = {0};

static bool isExecutableAddr(uint32_t addr)
{
	return true;
}

static struct inlineHookItem *findInlineHookItem(uint32_t target_addr)
{
	int i;

	for (i = 0; i < info.size; ++i) {
		if (info.item[i].target_addr == target_addr) {
			return &info.item[i];
		}
	}

	return NULL;
}

static struct inlineHookItem *addInlineHookItem() {
	struct inlineHookItem *item;

	if (info.size >= 1024) {
		return NULL;
	}

	item = &info.item[info.size];
	++info.size;

	return item;
}

static void deleteInlineHookItem(int pos)
{
	info.item[pos] = info.item[info.size - 1];
	--info.size;
}

static void registerInlineHookInThumb(struct inlineHookItem *item)
{
	item->target_addr -= 1;
	item->length = 10;
	item->orig_instructions = malloc(item->length);

	memcpy(item->orig_instructions, (void *) item->target_addr, item->length);

	if (item->proto_addr != NULL) {
		item->trampoline_instructions = mmap(NULL, PAGE_SIZE, PROT_READ | PROT_WRITE | PROT_EXEC, MAP_ANONYMOUS | MAP_PRIVATE, 0, 0);
		relocateInstructionInThumb(item->target_addr, (uint16_t *) item->orig_instructions, item->length, (uint16_t *) item->trampoline_instructions);
	}

	item->target_addr += 1;
}

static void registerInlineHookInArm(struct inlineHookItem *item)
{
	item->length = 8;
	item->orig_instructions = malloc(item->length);

	memcpy(item->orig_instructions, (void *) item->target_addr, item->length);

	if (item->proto_addr != NULL) {
		item->trampoline_instructions = mmap(NULL, PAGE_SIZE, PROT_READ | PROT_WRITE | PROT_EXEC, MAP_ANONYMOUS | MAP_PRIVATE, 0, 0);
		relocateInstructionInArm(item->target_addr, (uint32_t *) item->orig_instructions, item->length, (uint32_t *) item->trampoline_instructions);
	}
}

enum ele7en_status registerInlineHook(uint32_t target_addr, uint32_t new_addr, uint32_t **proto_addr)
{
	struct inlineHookItem *item;

	if (!isExecutableAddr(target_addr) || !isExecutableAddr(new_addr)) {
		return ELE7EN_ERROR_NOT_EXECUTABLE;
	}

	item = findInlineHookItem(target_addr);
	if (item != NULL) {
		if (item->status == REGISTERED) {
			return ELE7EN_ERROR_ALREADY_REGISTERED;
		}
		else if (item->status == HOOKED) {
			return ELE7EN_ERROR_ALREADY_HOOKED;
		}
		else {
			return ELE7EN_ERROR_UNKNOWN;
		}
	}

	item = addInlineHookItem();

	item->target_addr = target_addr;
	item->new_addr = new_addr;
	item->proto_addr = proto_addr;

	if (item->target_addr % 4 != 0) {
		registerInlineHookInThumb(item);
	}
	else {
		registerInlineHookInArm(item);
	}

	item->status = REGISTERED;

	return ELE7EN_OK;
}

static void doInlineUnHook(struct inlineHookItem *item, int pos)
{
	if (item->target_addr % 4 != 0) {
		item->target_addr -= 1;
	}
	mprotect((void *) PAGE_START(item->target_addr), PAGE_SIZE, PROT_READ | PROT_WRITE | PROT_EXEC);
	memcpy((void *) item->target_addr, item->orig_instructions, item->length);
	mprotect((void *) PAGE_START(item->target_addr), PAGE_SIZE, PROT_READ | PROT_EXEC);
	munmap(item->trampoline_instructions, PAGE_SIZE);
	free(item->orig_instructions);

	deleteInlineHookItem(pos);

	cacheflush(item->target_addr, item->target_addr + item->length, 0);
}

enum ele7en_status inlineUnHook(uint32_t target_addr)
{
	int i;

	for (i = 0; i < info.size; ++i) {
		if (info.item[i].target_addr == target_addr && info.item[i].status == HOOKED) {
			doInlineUnHook(&info.item[i], i);
			return ELE7EN_OK;
		}
	}

	return ELE7EN_ERROR_NOT_HOOKED;
}

void inlineUnHookAll()
{
	int i;

	for (i = 0; i < info.size; ++i) {
		if (info.item[i].status == HOOKED) {
			doInlineUnHook(&info.item[i], i);
		}
	}
}

static void doInlineHookInThumb(struct inlineHookItem *item)
{
	mprotect((void *) PAGE_START(item->target_addr), PAGE_SIZE, PROT_READ | PROT_WRITE | PROT_EXEC);

	((uint16_t *) item->target_addr)[0] = 0xF8DF;
	((uint16_t *) item->target_addr)[1] = 0xF000;	// LDR.W PC, [PC]
	((uint16_t *) item->target_addr)[2] = item->new_addr & 0xFFFF;
	((uint16_t *) item->target_addr)[3] = item->new_addr >> 16;

	mprotect((void *) PAGE_START(item->target_addr), PAGE_SIZE, PROT_READ | PROT_EXEC);

	if (item->trampoline_instructions != NULL) {
		*(item->proto_addr) = item->trampoline_instructions + 1;
	}
}

static void doInlineHookInArm(struct inlineHookItem *item)
{
	mprotect((void *) PAGE_START(item->target_addr), PAGE_SIZE, PROT_READ | PROT_WRITE | PROT_EXEC);

	((uint32_t *) (item->target_addr))[0] = 0xe51ff004;	// LDR PC, [PC, #-4]
	((uint32_t *) (item->target_addr))[1] = item->new_addr;

	mprotect((void *) PAGE_START(item->target_addr), PAGE_SIZE, PROT_READ | PROT_EXEC);

	if (item->trampoline_instructions != NULL) {
		*(item->proto_addr) = item->trampoline_instructions;
	}
}

static void doInlineHook(struct inlineHookItem *item)
{
	if (item->target_addr % 4 != 0) {
		item->target_addr -= 1;
		doInlineHookInThumb(item);
		item->target_addr += 1;
	}
	else {
		doInlineHookInArm(item);
	}

	item->status = HOOKED;

	// cacheFlush(item->target_addr, item->target_addr + item->length);
	cacheflush(item->target_addr, item->target_addr + item->length, 0);
}

enum ele7en_status inlineHook(uint32_t target_addr)
{
	int i;
	struct inlineHookItem *item;

	item = NULL;
	for (i = 0; i < info.size; ++i) {
		if (info.item[i].target_addr == target_addr) {
			item = &info.item[i];
			break;
		}
	}

	if (item == NULL) {
		return ELE7EN_ERROR_NOT_REGISTERED;
	}

	if (item->status == REGISTERED) {
		doInlineHook(item);
		return ELE7EN_OK;
	}
	else if (item->status == HOOKED) {
		return ELE7EN_ERROR_ALREADY_HOOKED;
	}
	else {
		return ELE7EN_ERROR_UNKNOWN;
	}
}

void inlineHookAll()
{
	int i;

	for (i = 0; i < info.size; ++i) {
		if (info.item[i].status == REGISTERED) {
			doInlineHook(&info.item[i]);
		}
	}
}