#include "userprog/syscall.h"
#include <stdio.h>
#include <string.h>
#include <syscall-nr.h>
#include <user/syscall.h>
#include "devices/shutdown.h"
#include "threads/interrupt.h"
#include "threads/thread.h"
#include "threads/vaddr.h"
#include "userprog/pagedir.h"
#include "userprog/process.h"
#include "filesys/filesys.h"
#include "threads/malloc.h"
#include "threads/synch.h"

static void syscall_handler (struct intr_frame *);
static void sys_halt (void);
static void sys_exit (int status);

void
syscall_init (void) 
{
  intr_register_int (0x30, 3, INTR_ON, syscall_handler, "syscall");
}

static bool
is_valid_user_pointer (const void * charlie)
{
	return is_user_vaddr (charlie) && pagedir_get_page (thread_current()->pagedir, charlie) != NULL;
}

static bool
validate_arguments (int arg_count, struct  intr_frame *f)
{
	int i;

	for (i = 1; i <= arg_count; i++)
	{
		if (!is_valid_user_pointer (f->esp + i))
	    	return false;
	}

	return true;
}

static void
userprog_fail (struct intr_frame *f)
{
	f->eax = -1;
	sys_exit(-1);
}

static void
syscall_handler (struct intr_frame *f) 
{
	if (f->esp == NULL || !is_valid_user_pointer ((const void *) f->esp))
	{
		userprog_fail (f);
	}

	switch (* (int *)(f->esp))
	{
		case SYS_HALT:
		{
			sys_halt ();
			break;
		}
		case SYS_EXIT:
		{
			if (!is_valid_user_pointer (f->esp + 4))
				userprog_fail (f);
			int status = *((int *) f->esp + 1);
			f->eax = status;
			sys_exit (status);
			break;
		}
		case SYS_EXEC:
		{
			char * file = *((char **) f->esp + 1);
			pid_t pid = process_execute (file);
			f->eax = pid;
			break;
		}
		case SYS_WAIT:
		{
			if (!validate_arguments (1, f))
				userprog_fail (f);

			pid_t pid = *((int *) f->esp + 1);

			int status = process_wait (pid);
			f->eax = status;
			
			break;
		}
		case SYS_CREATE:
		{
			/*char * file = *((char **)f->esp + 1);
			if(file == NULL || !is_valid_user_pointer((const void *) file))
				userprog_fail(f);
			unsigned initial_size = *((unsigned *)f->esp + 2);
			*/
			if (!validate_arguments (2, f))
				userprog_fail (f);

			char * file = *((char **) f->esp + 1);
			if (file == NULL || !is_valid_user_pointer ((const void *) file))
				userprog_fail (f);

			unsigned initial_size = *((unsigned *) f->esp + 2);
			bool ret = filesys_create (file, initial_size);
			f->eax = ret;

			break;
		}
		case SYS_REMOVE:
		{
			if (!validate_arguments (1, f))
				userprog_fail (f);

			char * file = *((char **)f->esp + 1);

			if (file == NULL || !is_valid_user_pointer (file))
				userprog_fail (f);

			bool ret = filesys_remove(file);
			f->eax = ret;

			break;
		}
		case SYS_OPEN:
		{
			if (!validate_arguments (1, f))
				userprog_fail (f);

			char * file_name = *((char **) f->esp + 1);

			if (!is_valid_user_pointer (file_name))
				userprog_fail (f);


			struct thread_file * tf = (struct thread_file *)malloc(sizeof (struct thread_file));
			tf->fdfile = filesys_open(file_name);

			if(!is_valid_user_pointer(tf->fdfile))
				userprog_fail(f);

			tf->pos = 0;
			struct thread * t = thread_current();
			lock_acquire(t->last_fd_lock);
			t->last_fd++;
			tf->fd = t->last_fd;
			lock_release(t->last_fd_lock);

			// TODO: Do we have to sync here?
			list_push_back(&t->files, &tf->elem);

			f->eax = tf->fd;

			break;
		}
		case SYS_FILESIZE:
		{
			break;
		}
		case SYS_READ:
		{
			break;
		}
		case SYS_WRITE:
		{
			if(!validate_arguments(3, f))
				userprog_fail(f);

			int fd = *((int *)f->esp + 1);
			void * buf = *((void **)f->esp + 2);

			if(!is_valid_user_pointer(buf))
				userprog_fail(f);

			unsigned size = *((unsigned *)f->esp + 3);
			// printf("%d: %x, %x, %d\n", SYS_WRITE, fd, &buf, size);
			if(fd == STDOUT_FILENO)
			{
				putbuf(buf, size);
			}

			break;
		}
		case SYS_SEEK:
		{
			break;
		}
		case SYS_TELL:
		{
			break;
		}
		case SYS_CLOSE:
		{
			break;
		}
	}
}

static void
sys_halt (void)
{
	shutdown_power_off ();
}

static void
sys_exit (int status)
{
	printf ("%s: exit(%d)\n", thread_current()->name, status);
	thread_exit ();
}

/*
pid_t exec (const char *file);
int wait (pid_t);
bool create (const char *file, unsigned initial_size);
bool remove (const char *file);
int open (const char *file);
int filesize (int fd);
int read (int fd, void *buffer, unsigned length);
int write (int fd, const void *buffer, unsigned length);
void seek (int fd, unsigned position);
unsigned tell (int fd);
void close (int fd);
 */
