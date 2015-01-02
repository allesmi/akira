#ifndef VM_FRAME_H
#define VM_FRAME_H

#include "threads/thread.h"
#include "lib/kernel/list.h"

struct frame_entry
{
	void * frame;
	struct page * page;
	struct thread * owner;

	struct list_elem elem;
};

void frame_init(void);
struct frame_entry *frame_alloc(void);
void frame_free(struct frame_entry *);

#endif