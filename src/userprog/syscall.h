#ifndef USERPROG_SYSCALL_H
#define USERPROG_SYSCALL_H

#include "threads/thread.h"

void syscall_init (void);
void syscall_exit (int status);

#endif /* userprog/syscall.h */
