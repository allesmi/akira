#ifndef VM_PAGE_H
#define VM_PAGE_H

#include <stdint.h>

#include "vm/swap.h"
#include "lib/kernel/list.h"

enum page_state
{
	FRAMED,
	ON_DISK,
	ON_SWAP
};

enum page_segment
{
	STACK,
	EXECUTABLE,
	MMAPED_FILE
};

struct page_table_segment
{
	void * start;		/* Start address of this segment */
	int size;		/* Number of pages for this segment (may grow) */

	struct list page_list;
};

struct page_table_entry
{
	void * vaddr;
	int size;
	enum page_state state;
	block_sector_t swap_slot;		/* When state is ON_SWAP */
	struct file * f;				/* When state is ON_FILE */
	int f_offset;
	bool writable;

	struct list_elem elem;			/* List element */
};

void page_init(void);
void page_add_to_executabe_segment(struct page_table_entry * pte);
struct page_table_entry * page_get_entry_for_vaddr(void * vaddr);
void mmfile_add_to_page_table (struct file * f, int ofs, int size, void * addr,
	size_t page_read_bytes, size_t page_zero_bytes);

#endif