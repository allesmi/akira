#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>

#include "lib/kernel/bitmap.h"

#include "threads/loader.h"
#include "threads/malloc.h"
#include "vm/frame.h"
#include "threads/vaddr.h"
#include "threads/palloc.h"
#include "userprog/pagedir.h"

static void evict_frame(struct frame_entry * fe);

static struct list frame_list;

void
frame_init(void)
{
	list_init(&frame_list);
}

struct frame_entry *
frame_alloc(void)
{
	printf("frame_alloc start\n");
	void * page = palloc_get_page(PAL_USER);
	int i = 0;
	struct frame_entry * fe = NULL;

	while(page == NULL || i < list_size(&frame_list))
	{
		fe = list_entry(list_pop_front(&frame_list), struct frame_entry, elem);
		if(pagedir_is_accessed(fe->owner->pagedir, fe->frame))
		{
			pagedir_set_accessed(fe->owner->pagedir, fe->frame, false);
			list_push_back(&frame_list, &fe->elem);
		}
		else
		{
			evict_frame(fe);
			page = palloc_get_page(PAL_USER);
		}
		i++;
	}

	if(fe == NULL)
		fe = (struct frame_entry *)malloc(sizeof(struct frame_entry));

	if(fe == NULL)
		PANIC("Allocation of frame data failed.");

	fe->frame = page;
	fe->owner = thread_current();
	list_push_back(&frame_list, &fe->elem);
	printf("frame_alloc end\n");

	return fe;
}

void
frame_free(struct frame_entry * fe)
{
	palloc_free_page(fe->frame);
}

static void
evict_frame(struct frame_entry * fe)
{
	struct page * p = fe->page;
	if(p->origin == STACK || (p->origin == EXECUTABLE && p->writable))
	{
		p->swap_slot = swap_store(p->vaddr);
		p->state = ON_SWAP;
	}
	else if(p->origin == MMAPPED_FILE)
	{
		file_write_at (p->f, p->vaddr, p->size, p->f_offset);
		p->state = ON_DISK;
	}
	else if(p->origin == EXECUTABLE && !p->writable)
	{
		p->state = ON_DISK;
	}

	pagedir_clear_page(fe->owner->pagedir, p->vaddr);
}