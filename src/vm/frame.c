#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>

#include "lib/kernel/bitmap.h"

#include "threads/loader.h"
#include "vm/frame.h"
#include "threads/vaddr.h"
#include "threads/palloc.h"

static void evict_next_frame(void);
static bool frames_available(void);

struct bitmap * frames_bm;

void
frame_init(void)
{
	// Assumption: When init_ram_pages is the number of pages
	// in physical memory, init_ram_pages/2 is the number available
	// for user programs
	frames_bm = bitmap_create(init_ram_pages / 2);
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

	size_t index = (size_t)(phys_address/PGSIZE - init_ram_pages/2);

	bitmap_set(frames_bm, index, true);

	// TODO: Set pointer of frame_entry at PHYS_ADDRESS/PGSIZE to
	// PAGE.

	return page;
}

void
frame_free(void * page)
{
	// TODO: delete entry in frame table
	size_t index = (size_t)page/PGSIZE;
	bitmap_set(frames_bm, index, false);

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
	return !bitmap_all(frames_bm, 0, init_ram_pages/2);
}