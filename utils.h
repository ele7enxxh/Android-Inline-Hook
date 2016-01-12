#ifndef _UTILS_H
#define _UTILS_H

void contAllThreads(pid_t pid, pid_t *tids);
void stopAllThreads(pid_t pid, pid_t *tids);
int getAllTids(pid_t pid, pid_t *tids);

#endif