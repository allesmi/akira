#include <stdint.h>
#include <stdbool.h>

#include "vm/frame.h"
#include "threads/vaddr.h"
#include "threads/palloc.h"

// TODO: find out number of frames in physical memory
static struct frame_entry frame_table[1000];

static void evict_next_frame(void);
static bool frames_available(void);

void
frame_init(void)
{
}

void *
frame_alloc(void)
{
	if (!frames_available())
	{
		evict_next_frame();
	}

	void * page = palloc_get_page(PAL_USER);
	uintptr_t phys_address = vtop(page);

	// Set pointer of frame_entry at PHYS_ADDRESS/PGSIZE to
	// PAGE.

	return 0;
}

void
frame_free(void * frame)
{

}

static void
evict_next_frame(void)
{

}

static bool
frames_available(void)
{
	return true;
}