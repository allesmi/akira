#ifndef USERPROG_SYSCALL_H
#define USERPROG_SYSCALL_H

#include "threads/thread.h"

void syscall_init (void);
void syscall_exit (int status);
void remove_mmap (mapid_t mapping, bool all);

#endif /* userprog/syscall.h */
