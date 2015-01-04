#include <inttypes.h>
#include <debug.h>
#include <stdio.h>

#include "vm/swap.h"
#include "devices/block.h"
#include "threads/palloc.h"
#include "threads/vaddr.h"

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
}

block_sector_t
swap_store(void * page)
{
	int i;
	if (free_list == unused && unused == block_size(swap_device))
		PANIC ("Swap is full!");
	
	block_sector_t f = free_list;

	void * p = palloc_get_page(0);
	block_read(swap_device, f, p);
	if(free_list != unused)
		free_list = ((struct free_slot *)p)->next;
	else
	{
		unused = unused + PGSIZE/BLOCK_SECTOR_SIZE;
		free_list = unused;
	}
	palloc_free_page(p);

	for(i = 0; i < PGSIZE/BLOCK_SECTOR_SIZE; i++)
	{
		block_write(swap_device, f + i, page + i * BLOCK_SECTOR_SIZE);
	}

	return f;
}

void
swap_retrieve(block_sector_t slot_no, void * page)
{
	if(slot_no < 0)
		return;

	int i;
	if(page != NULL)
	{
		for(i = 0; i < PGSIZE/BLOCK_SECTOR_SIZE; i++)
		{
			block_read(swap_device, slot_no + i, page + i * BLOCK_SECTOR_SIZE);
		}
	}

	struct free_slot f;
	f.next = free_list;

	block_write(swap_device, slot_no, &f);
	free_list = slot_no;
}