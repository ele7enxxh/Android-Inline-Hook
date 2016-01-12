/*
author: ele7enxxh
mail: ele7enxxh@qq.com
website: ele7enxxh.com
modified time: 2016-1-10
created time: 2016-1-10
*/

#include <stdio.h>
#include <dirent.h>
#include <string.h>
#include <errno.h>
#include <asm/signal.h>

#include "utils.h"

#define ENABLE_DEBUG
#include "log.h"

void contAllThreads(pid_t pid, pid_t *tids)
{
	pid_t tid;
	int i;

	i = 0;
	for (tid = tids[i]; tid != 0; tid = tids[++i]) {
		tkill(pid, tid, SIGCONT);
	}	
}

void stopAllThreads(pid_t pid, pid_t *tids)
{
	pid_t tid;
	int i;

	i = 0;
	for (tid = tids[i]; tid != 0; tid = tids[++i]) {
		tkill(pid, tid, SIGSTOP);
	}
}

int getAllTids(pid_t pid, pid_t *tids)
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
