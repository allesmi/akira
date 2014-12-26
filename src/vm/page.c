#include <stdio.h>

#include "vm/page.h"

#include "lib/kernel/bitmap.h"
#include "lib/kernel/list.h"
#include "threads/vaddr.h"

static struct bitmap * page_bm;

/* Supplemental page table is seperated in segments. */
static struct page_table_segment eseg;
static struct page_table_segment sseg;

void
page_init(void)
{
	page_bm = bitmap_create(382);

	eseg.start = (void*)0x8048000;
	eseg.size = 0;
	list_init(&eseg.page_list);

	sseg.start = (void*) PHYS_BASE;
	sseg.size = 0; // remember stack grows downwards
	list_init(&sseg.page_list);
}

void
page_add_to_executabe_segment(struct page_table_entry * pte)
{
	list_push_back(&eseg.page_list, &pte->elem);
	eseg.size++;
}

struct page_table_entry *
page_get_entry_for_vaddr(void * vaddr)
{
	void * page_bound = pg_round_down(vaddr);

	// search in executable segment
	if(page_bound >= eseg.start && 
		page_bound < (void *)(eseg.start + PGSIZE * eseg.size))
	{
		struct list_elem *e;
		for (e = list_begin (&eseg.page_list); e != list_end (&eseg.page_list);
			e = list_next (e))
		{
			struct page_table_entry *p = list_entry (e, struct page_table_entry, elem);
			if(page_bound == p->vaddr)
			{
				return p;
			}
		}
	}

	// search in stack segment
	if(page_bound <= sseg.start && 
		page_bound > (void *)(sseg.start - PGSIZE * sseg.size))
	{
		struct list_elem *e;
		for (e = list_begin (&sseg.page_list); e != list_end (&sseg.page_list);
			e = list_next (e))
		{
			struct page_table_entry *p = list_entry (e, struct page_table_entry, elem);
			if(page_bound == p->vaddr)
			{
				return p;
			}
		}
	}

	// TODO: search in segments for mmaped files

	return NULL;
}