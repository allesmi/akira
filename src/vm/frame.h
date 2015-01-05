#ifndef VM_FRAME_H
#define VM_FRAME_H

#include "threads/thread.h"
#include "lib/kernel/list.h"

/* An entry in the global farme table */
struct frame_entry
{
	void * frame; /* A pointer to the physical frame */
	struct page * page; /* A pointer to the page data on this frame */
	struct thread * owner; /* Owning thread */

	struct list_elem elem;
};

void frame_init(void);
struct frame_entry *frame_alloc(void);
void frame_free(struct frame_entry *);
void frame_release_all(void);

#endif