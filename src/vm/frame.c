#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>

#include "lib/kernel/bitmap.h"

#include "threads/loader.h"
#include "threads/synch.h"
#include "threads/malloc.h"
#include "vm/frame.h"
#include "threads/vaddr.h"
#include "threads/palloc.h"
#include "userprog/pagedir.h"

static void evict_frame(struct frame_entry * fe, bool skip_swap);

static struct list frame_list;
static struct lock frame_lock;

void
frame_init(void)
{
	list_init(&frame_list);
	lock_init(&frame_lock);
}

struct frame_entry *
frame_alloc(void)
{
	lock_acquire(&frame_lock);
	void * page = palloc_get_page(PAL_USER);
	int i = 0;
	struct frame_entry * fe = NULL;
	int sz = list_size(&frame_list);
	while(page == NULL && i < sz + 1)
	{
		fe = list_entry(list_pop_front(&frame_list), struct frame_entry, elem);

		if(pagedir_is_accessed(fe->owner->pagedir, fe->frame))
		{
			pagedir_set_accessed(fe->owner->pagedir, fe->frame, false);
			list_push_back(&frame_list, &fe->elem);
		}
		else
		{
			evict_frame(fe, false);
			page = palloc_get_page(PAL_USER);
		}
		i++;
	}

	if(page == NULL)
		PANIC("Allocation of user frame failed.");

	if(fe == NULL)
	{
		fe = (struct frame_entry *)malloc(sizeof(struct frame_entry));
		if(fe == NULL)
			PANIC("Allocation of frame data failed.");
	}

	fe->frame = page;
	fe->owner = thread_current();
	list_push_back(&frame_list, &fe->elem);
	pagedir_set_accessed(fe->owner->pagedir, fe->frame, true);

	lock_release(&frame_lock);
	return fe;
}

void
frame_free(struct frame_entry * fe)
{
	evict_frame(fe, true);
	list_remove(&fe->elem);
	free(fe);
}

void
frame_release_all(void)
{
	int i, freecnt = 0;
	lock_acquire(&frame_lock);
	int sz = list_size(&frame_list);

	for(i = 0; i < sz; i++)
	{
		struct list_elem *e = list_pop_front(&frame_list);
		struct frame_entry * fe = list_entry(e, struct frame_entry, elem);

		if(fe->owner == thread_current())
		{
			evict_frame(fe, true);
			free(fe);
			freecnt++;
		}
		else
		{
			list_push_back(&frame_list, &fe->elem);
		}
	}
	lock_release(&frame_lock);
	if(debug)
		printf("Released all %d frames of thread %d\n", freecnt, thread_tid());
}

void
evict_frame(struct frame_entry * fe, bool skip_swap)
{
	struct page * p = fe->page;
	if(p->origin == STACK || (p->origin == EXECUTABLE && p->writable))
	{
		if(!skip_swap)
		{
			p->swap_slot = swap_store(fe->frame);
			p->state = ON_SWAP;
		}
	}
	else if(p->origin == MMAPPED_FILE)
	{
		if(pagedir_is_dirty(thread_current()->pagedir, p->vaddr))
		{
			file_write_at (p->f, p->vaddr, p->size, p->f_offset);
		}
		p->state = ON_DISK;
	}
	else if(p->origin == EXECUTABLE && !p->writable)
	{
		p->state = ON_DISK;
	}
	if(debug)
		printf("Evicting physical frame %p from virtual address %p owned by %d\n", fe->frame, fe->page->vaddr, fe->owner->tid);

	if(fe->owner != NULL)
		pagedir_clear_page(fe->owner->pagedir, p->vaddr);

	p->fe = NULL;

	palloc_free_page(fe->frame);
}