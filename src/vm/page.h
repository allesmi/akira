#ifndef VM_PAGE_H
#define VM_PAGE_H

#include <stdint.h>

#include "vm/swap.h"
#include "lib/kernel/list.h"
#include "lib/kernel/hash.h"

enum page_state
{
	FRAMED,
	ON_DISK,
	ON_SWAP
};

struct page_table
{
	struct hash pages;
};


struct page
{
	void * vaddr;
	int size;
	enum page_state state;
	block_sector_t swap_slot;		/* When state is ON_SWAP */
	struct file * f;				/* When state is ON_FILE */
	int f_offset;


	struct list_elem l_elem;			/* List element */
	struct hash_elem h_elem;			/* List element */
};

void page_init(void);
void page_add_to_executabe_segment(struct page * pte);
void page_add_entry(struct page * p);
struct page * page_get_entry_for_vaddr(void * vaddr);

unsigned page_hash(const struct hash_elem *e, void * aux);
bool page_less(const struct hash_elem *a, const struct hash_elem *b, void *aux);

#endif