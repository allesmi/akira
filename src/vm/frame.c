#include "vm/frame.h"
#include "lib/kernel/bitmap.h"
#include "threads/vaddr.h"

static struct bitmap * frames;

void
frame_init(void)
{
	frames = bitmap_create(1000 * PGSIZE);
}
