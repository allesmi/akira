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

void
frame_init(void)
{

}

void *
frame_alloc(void)
{
	void * page = palloc_get_page(PAL_USER);
	
	return page;
}

void
frame_free(void * page)
{
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
	true;
}