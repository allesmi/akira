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
#include "threads/thread.h"
#include "threads/malloc.h"
#include "threads/synch.h"
#include "devices/input.h"
#include "vm/page.h"

static void syscall_handler (struct intr_frame *);
static void sys_halt (void);
mapid_t mmap (int fd, void *addr);
void munmap (mapid_t mapping);
struct thread_file * get_thread_file (int fd);

struct lock syscall_lock;	/* A lock for system calls */

void
syscall_init (void) 
{
  intr_register_int (0x30, 3, INTR_ON, syscall_handler, "syscall");
  lock_init(&syscall_lock);
}

static bool
is_valid_user_pointer (const void * charlie)
{
	return is_user_vaddr (charlie) && page_get_entry_for_vaddr(charlie) != NULL;
}

static bool
is_valid_user_pointer_range(const void * charlie, unsigned size)
{
	unsigned i;

	for(i = (unsigned)charlie; i < (unsigned)charlie + size; i += PGSIZE)
	{
		if(!is_valid_user_pointer((const void *)i))
		{
			return 0;
		}
	}

	return 1;
}

static void
userprog_fail (struct intr_frame *f)
{
	f->eax = -1;
	syscall_exit(-1);
}

static void
syscall_handler (struct intr_frame *f) 
{
	if (f->esp == NULL || !is_valid_user_pointer ((const void *) f->esp))
	{
		userprog_fail (f);
	}

	struct thread *current = thread_current ();

	if (current->last_fd == 0)
	{
		current->last_fd = 2;
		list_init (&current->thread_files);
	}

	// printf("Syscall %d by '%s'.\n", *(int *)f->esp, thread_name());

	switch (* (int *)(f->esp))
	{
		case SYS_HALT:
		{
			sys_halt ();
			break;
		}
		case SYS_EXIT:
		{
			if (!is_valid_user_pointer ((int *)f->esp + 1))
				userprog_fail (f);

			int status = *((int *) f->esp + 1);
			f->eax = status;
			syscall_exit (status);

			break;
		}
		case SYS_EXEC:
		{
			if(!is_valid_user_pointer((char **) f->esp + 1))
				userprog_fail (f);

			char * file = *((char **) f->esp + 1);
			if (file == NULL || !is_valid_user_pointer ((const void *) file))
				userprog_fail (f);

			lock_acquire (&syscall_lock);
			pid_t pid = process_execute (file);
			f->eax = pid;
			lock_release (&syscall_lock);
			
			break;
		}
		case SYS_WAIT:
		{
			if (!is_valid_user_pointer ((int *)f->esp + 1))
				userprog_fail (f);

			pid_t pid = *((int *) f->esp + 1);

			int status = process_wait (pid);
			f->eax = status;
			
			break;
		}
		case SYS_CREATE:
		{
			if(!is_valid_user_pointer((char **) f->esp + 1))
				userprog_fail (f);

			char * file = *((char **) f->esp + 1);
			if (file == NULL || !is_valid_user_pointer ((const void *) file))
				userprog_fail (f);

			if(!is_valid_user_pointer((unsigned *) f->esp + 2))
				userprog_fail (f);
			unsigned initial_size = *((unsigned *) f->esp + 2);
			lock_acquire (&syscall_lock);
			bool ret = filesys_create (file, initial_size);
			lock_release (&syscall_lock);
			f->eax = ret;

			break;
		}
		case SYS_REMOVE:
		{
			if(!is_valid_user_pointer((char **) f->esp + 1))
				userprog_fail (f);
			char * file = *((char **)f->esp + 1);

			if (file == NULL || !is_valid_user_pointer (file))
				userprog_fail (f);

			lock_acquire (&syscall_lock);
			bool ret = filesys_remove (file);
			lock_release (&syscall_lock);
			f->eax = ret;

			break;
		}
		case SYS_OPEN:
		{
			if(!is_valid_user_pointer((char **) f->esp + 1))
				userprog_fail (f);
			char * file_name = *((char **) f->esp + 1);

			if (file_name == NULL || !is_valid_user_pointer (file_name))
				userprog_fail (f);

			struct thread_file * tf = malloc(sizeof (struct thread_file));
			struct file *file;

			lock_acquire (&syscall_lock);
			file = filesys_open (file_name);

			if(file == NULL)
			{
				f->eax = -1;
				lock_release (&syscall_lock);
			}
			else
			{
				tf->fdfile = file;

				tf->fd = current->last_fd;
				current->last_fd++;

				list_push_back (&current->thread_files, &tf->elem);
				lock_release (&syscall_lock);

				f->eax = tf->fd;
			}

			break;
		}
		case SYS_FILESIZE:
		{
			if(!is_valid_user_pointer((int *) f->esp + 1))
				userprog_fail (f);

			int fd = *((int *) f->esp + 1);

			lock_acquire (&syscall_lock);
			struct thread_file * current_tf = get_thread_file (fd);

			if (current_tf != NULL)
				f->eax = file_length (current_tf->fdfile);

			lock_release (&syscall_lock);

			break;
		}
		case SYS_READ:
		{
			if(!is_valid_user_pointer((int *) f->esp + 1))
				userprog_fail (f);
			int fd = *((int *)f->esp + 1);

			if(!is_valid_user_pointer((void **) f->esp + 2))
				userprog_fail (f);
			void * buf = *((void **)f->esp + 2);

			if(!is_valid_user_pointer (buf))
				userprog_fail (f);

			if(!is_valid_user_pointer((unsigned *) f->esp + 3))
				userprog_fail (f);
			unsigned size = *((unsigned *)f->esp + 3);

			if(!is_valid_user_pointer_range(buf, size))
				userprog_fail(f);


			lock_acquire (&syscall_lock);
			struct thread_file * current_tf = get_thread_file (fd);

			if (current_tf != NULL)
			{
				f->eax = file_read (current_tf->fdfile, buf, size);
			}
			else if(fd == STDIN_FILENO)
			{
				unsigned i;
				uint8_t * input_buffer = buf;

				for (i = 0; i < size; i++)
				{
					input_buffer[i] = input_getc ();
				}

				f->eax = size;
			}
			// printf("\tRead %d bytes\n", f->eax);

			lock_release (&syscall_lock);

			break;
		}
		case SYS_WRITE:
		{
			if(!is_valid_user_pointer((int *) f->esp + 1))
				userprog_fail (f);
			int fd = *((int *)f->esp + 1);

			if(!is_valid_user_pointer((void **) f->esp + 2))
				userprog_fail (f);
			void * buf = *((void **)f->esp + 2);

			if(!is_valid_user_pointer (buf))
				userprog_fail (f);

			if(!is_valid_user_pointer((unsigned *) f->esp + 3))
				userprog_fail (f);
			unsigned size = *((unsigned *)f->esp + 3);

			if(!is_valid_user_pointer_range(buf, size))
				userprog_fail(f);

			lock_acquire (&syscall_lock);
			struct thread_file * current_tf = get_thread_file (fd);

			if (current_tf != NULL)
			{
				f->eax = file_write (current_tf->fdfile, buf, size);
			}
			else if(fd == STDOUT_FILENO)
			{
				putbuf(buf, size);
				f->eax = size;
			}

			lock_release (&syscall_lock);

			break;
		}
		case SYS_SEEK:
		{
			if(!is_valid_user_pointer((int *) f->esp + 1))
				userprog_fail (f);
			int fd = *((int *)f->esp + 1);

			if(!is_valid_user_pointer((unsigned *) f->esp + 2))
				userprog_fail (f);
			unsigned position = *((unsigned *) f->esp + 2);

			lock_acquire (&syscall_lock);
			struct thread_file * current_tf = get_thread_file (fd);

			if (current_tf != NULL)
				file_seek (current_tf->fdfile, position);

			lock_release (&syscall_lock);
			
			break;
		}
		case SYS_TELL:
		{
			if(!is_valid_user_pointer((int *) f->esp + 1))
				userprog_fail (f);
			int fd = *((int *)f->esp + 1);

			lock_acquire (&syscall_lock);
			struct thread_file * current_tf = get_thread_file (fd);

			if (current_tf != NULL)
				f->eax = file_tell (current_tf->fdfile);

			lock_release (&syscall_lock);

			break;
		}
		case SYS_CLOSE:
		{
			if(!is_valid_user_pointer((int *) f->esp + 1))
				userprog_fail (f);
			int fd = *((int *)f->esp + 1);

			lock_acquire (&syscall_lock);
			struct thread_file * current_tf = get_thread_file (fd);

			if (current_tf != NULL)
			{
				list_remove (&current_tf->elem);
				file_close (current_tf->fdfile);
				free (current_tf);
			}

			lock_release (&syscall_lock);

			break;
		}
		case SYS_MMAP:
		{
			if(!is_valid_user_pointer((int *) f->esp + 1))
			{
				f->eax = -1;
				break;
			}
				

			int fd = *((int *)f->esp + 1);

			if(!is_valid_user_pointer((void **) f->esp + 2))
			{
				f->eax = -1;
				break;
			}

			void * addr = *((void **)f->esp + 2);

			if(((uint32_t) addr % PGSIZE) != 0)
			{
				f->eax = -1;
				break;
			}

			f->eax = mmap(f, addr);

			if (f->eax != -1)
				thread_current()->mapid++;

			break;
		}
		case SYS_MUNMAP:
		{                                
			if(!is_valid_user_pointer((int *) f->esp + 1))
			{
				f->eax = -1;
				break;
			}

			mapid_t mapping = *((int *)f->esp + 1);
			munmap(mapping);

			break;
		}
	}
}

static void
sys_halt (void)
{
	shutdown_power_off ();
}

void
syscall_exit (int status)
{
	printf ("%s: exit(%d)\n", thread_name(), status);

	thread_current()->return_value = status;

	thread_exit ();
}

struct
thread_file * get_thread_file (int fd)
{
	struct thread *current = thread_current ();
	struct list_elem * e;
	struct thread_file * current_tf;

	for (e = list_begin (&current->thread_files); e != list_end (&current->thread_files);
		e = list_next (e))
	{
		current_tf = list_entry (e, struct thread_file, elem);

		if (current_tf->fd == fd)
			return current_tf;
	}

	return NULL;
}

mapid_t 
mmap (int fd, void *addr)
{
	struct thread_file * current_tf = get_thread_file (fd);

	if (current_tf == NULL || current_tf->fdfile == NULL ||
		!is_valid_user_pointer(current_tf->fdfile) || addr == NULL)
		return -1;
	

	struct file * f = current_tf->fdfile;
	struct file * reopen_file = file_reopen (f);

	if (reopen_file == NULL || file_length (reopen_file) == 0)
		return -1;

	int offset = 0;
	int read_bytes = file_length(reopen_file);
	
	
	while (read_bytes > 0)
	{
		size_t page_read_bytes = read_bytes < PGSIZE ? read_bytes : PGSIZE;

		if (!mmfile_add_to_page_table (reopen_file, offset, read_bytes, addr, page_read_bytes))
		{
			return -1;
		}

		read_bytes -= page_read_bytes;
		offset += page_read_bytes;
		addr += PGSIZE;	
	}

	return thread_current()->mapid;
}

void 
munmap (mapid_t mapping)
{
	remove_mmap (mapping, false);
}

void remove_mmap (mapid_t mapping, bool all)
{
	struct thread * t = thread_current ();
	struct list_elem *e;

	for (e = list_begin (&t->mappedfiles); e != list_end (&t->mappedfiles);
		 e = list_next (e))
	{
		struct mapped_file * mmfile = list_entry (e, struct mapped_file, elem);

		if (mmfile->mapping == mapping || all == true)
		{
			if (pagedir_is_dirty (t->pagedir ,mmfile->p->vaddr))
			{
				file_write_at (mmfile->p->f, mmfile->p->vaddr, 
					mmfile->p->size, mmfile->p->f_offset);
			}	

			frame_free(pagedir_get_page(t->pagedir, mmfile->p->vaddr));
			pagedir_clear_page(t->pagedir, mmfile->p->vaddr);	
		}
	}
	
}

