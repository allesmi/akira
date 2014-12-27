#include <inttypes.h>
#include <debug.h>
#include <stdio.h>

#include "vm/swap.h"
#include "devices/block.h"
#include "threads/palloc.h"

static struct block * swap_device;

static block_sector_t free_list;
static block_sector_t unused;

struct free_slot
{
	block_sector_t next;
};

void
swap_init(void)
{
	swap_device = block_get_role(BLOCK_SWAP);

	free_list = 0;
	unused = 0;

	// // A small test program
	// char * a = "First slot";
	// char * b = "Second slot";
	// char * c = "Third slot";
	// char * d = "Fourth slot";

	// block_sector_t s1 = swap_store(a);
	// block_sector_t s2 = swap_store(b);
	// block_sector_t s3 = swap_store(c);

	// char * bo = swap_retrieve(s2);
	// printf("Retrieved '%s'\n", bo);

	// swap_store(d);

	// int i;
	// for(i = 0; i < 3; i++)
	// {
	// 	char * o = swap_retrieve((block_sector_t)i);
	// 	printf("Retrieved from slot %d: '%s'.\n", i, o);
	// }
}

block_sector_t
swap_store(void * page)
{
	if (free_list == unused && unused == block_size(swap_device))
		PANIC ("Swap is full!");
	
	block_sector_t f = free_list;

	void * p = palloc_get_page(0);
	block_read(swap_device, f, p);
	if(free_list != unused)
		free_list = ((struct free_slot *)p)->next;
	else
	{
		unused = unused + 1;
		free_list = unused;
	}
	palloc_free_page(p);

	block_write(swap_device, f, page);

	return f;
}

void
swap_retrieve(block_sector_t slot_no, void * page)
{
	block_read(swap_device, slot_no, page);

	struct free_slot f;
	f.next = free_list;

	block_write(swap_device, slot_no, &f);
	free_list = slot_no;
}