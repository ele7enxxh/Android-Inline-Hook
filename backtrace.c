/*
detection thread function call stack
author: ele7enxxh
mail: ele7enxxh@qq.com
website: ele7enxxh.com
modified time: 2015-11-30
created time: 2015-11-30
*/

#include <stdio.h>
#include <dirent.h>
#include <dlfcn.h>
#include <string.h>
#include <errno.h>
#include <asm/signal.h>

#define ENABLE_DEBUG
#include "log.h"

#define MAX_DEPATH  32

struct backtrace_frame {
    uintptr_t *absolute_pc;
    uintptr_t *stack_top;
    size_t stack_size;
};

typedef ssize_t (*t_unwind_backtrace_thread) (pid_t, struct backtrace_frame *, size_t, size_t);

static t_unwind_backtrace_thread unwind_backtrace_thread = NULL;

static const char *corkscrew_path = "/system/lib/libcorkscrew.so";

static int registerUnwindFunction()
{
	void *handle;

	if (unwind_backtrace_thread != NULL) {
		return 0;
	}

	handle = dlopen(corkscrew_path, RTLD_NOW);
	if (handle == NULL) {
		LOGD("dlopen(): %s", strerror(errno));
		return -1;
	}

	unwind_backtrace_thread = (t_unwind_backtrace_thread) dlsym(handle, "unwind_backtrace_thread");
	if (unwind_backtrace_thread == NULL) {
		LOGD("dlsym(): %s", strerror(errno));
		return -1;
	}

	return 0;
}

static ssize_t backtracePtrace(pid_t tid, struct backtrace_frame *backtrace)
{

	LOGD("unwinding thread: %d from process: %d", tid, getpid());
	return unwind_backtrace_thread(tid, backtrace, 1, MAX_DEPATH);
}

static void contAllThreads(pid_t pid, pid_t *tids)
{
	pid_t tid;
	int i;

	i = 0;
	for (tid = tids[i]; tid != 0; tid = tids[++i]) {
		tkill(pid, tid, SIGCONT);
	}	
}

static void stopAllThreads(pid_t pid, pid_t *tids)
{
	pid_t tid;
	int i;

	i = 0;
	for (tid = tids[i]; tid != 0; tid = tids[++i]) {
		tkill(pid, tid, SIGSTOP);
	}
}

static int getAllTid(pid_t pid, pid_t *tids)
{
	char dir_path[32];
	DIR *dir;
	int i;
	struct dirent *entry;
	pid_t tid;

	if (pid < 0) {
		snprintf(dir_path, sizeof(dir_path), "/proc/self/task");
	}
	else {
		snprintf(dir_path, sizeof(dir_path), "/proc/%d/task", pid);
	}

	dir = opendir(dir_path);
    if (dir == NULL) {
        LOGD("opendir(): %s", strerror(errno));
        return -1;
    }

    i = 0;
    while((entry = readdir(dir)) != NULL) {
    	tid = atoi(entry->d_name);
    	if (tid != 0 && tid != getpid()) {
    		tids[i++] = tid;
    	}
    }
    closedir(dir);
    tids[i] = 0;
    return 0;
}

static int doCheckThreadsafety(pid_t tid, int status, uint32_t *addrs, int length)
{
	struct backtrace_frame backtrace[MAX_DEPATH];
	ssize_t count;
	int i;

	count = backtracePtrace(tid, backtrace);

	for (i = 0; i < count; ++i) {
		int idx;
		uint32_t addr;

		idx = 0;
		for (addr = addrs[idx]; addr != 0; addr = addrs[++idx]) {
			if ((uint32_t) backtrace[i].absolute_pc >= addr && (uint32_t) backtrace[i].absolute_pc <= (addr + length)) {
				LOGD("thread function call stack contains the hook address(0x%x)", addr);
				return -1;
			}
		}
	}

	return 0;
}

int checkThreadsafety(int status, uint32_t *addrs, int length)
{
	pid_t pid;
	pid_t tids[1024];
	pid_t tid;
	int i;

	if (registerUnwindFunction()== -1) {
		return -1;
	}

	pid = getpid();
	if (getAllTid(pid, tids) == -1) {
		return -1;
	}

	stopAllThreads(pid, tids);

	i = 0;
	for (tid = tids[i]; tid != 0; tid = tids[++i]) {
		if (doCheckThreadsafety(tid, status, addrs, length) == -1) {
			contAllThreads(pid, tids);
			return -1;
		}
	}

	contAllThreads(pid, tids);

	LOGD("check thread safety success");

	return 0;
}