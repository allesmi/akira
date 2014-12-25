#include <stdint.h>
#include <stdbool.h>

#include "vm/frame.h"
#include "threads/vaddr.h"
#include "threads/palloc.h"

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

	// TODO: Set pointer of frame_entry at PHYS_ADDRESS/PGSIZE to
	// PAGE.

	return page;
}

void
frame_free(void * page)
{
	// TODO: delete entry in frame table

	palloc_free_page(page);
}

static void
evict_next_frame(void)
{
	// TODO: panic when there is no frame to evict
}

static bool
frames_available(void)
{
	return true;
}