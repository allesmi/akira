#ifndef VM_PAGE_H
#define VM_PAGE_H

#include <stdint.h>

#include "vm/swap.h"
#include "vm/frame.h"
#include "lib/kernel/list.h"
#include "lib/kernel/hash.h"

enum page_state
{
	FRAMED,						/* Framed page */
	ON_DISK,					/* page on disk */
	ON_SWAP						/* page in swap */
};

enum page_origin
{
	STACK,						/* page comes from stack */
	EXECUTABLE,					/* page comes from executable */
	MMAPPED_FILE				/* page comes from mm-file */
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
	enum page_origin origin;
	block_sector_t swap_slot;		/* When state is ON_SWAP */
	struct file * f;				/* When state is ON_FILE */
	int f_offset;
	bool writable;
	struct frame_entry * fe;

	struct list_elem l_elem;			/* List element */
	struct hash_elem h_elem;			/* List element */
};

void page_init(void);

bool page_add_entry(struct page * p);
void page_delete_entry (struct page * p);
void page_destroy(struct page * p);

struct page * page_get_entry_for_vaddr(const void * vaddr);

unsigned page_hash(const struct hash_elem *e, void * aux);
bool page_less(const struct hash_elem *a, const struct hash_elem *b, void *aux);
void page_hash_destroy(struct hash_elem *e, void* aux UNUSED);

#endif