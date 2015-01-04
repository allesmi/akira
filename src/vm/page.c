#include <stdio.h>

#include "vm/page.h"

#include "lib/kernel/hash.h"
#include "threads/vaddr.h"
#include "threads/thread.h"
#include "threads/malloc.h"
#include "userprog/pagedir.h"

void
page_init(void)
{

}

bool
page_add_entry(struct page * p)
{
	if(debug)
		printf("Added to page table of thread %d: %p+%d %s.\n", thread_tid(), p->vaddr, p->size,
			p->f != NULL? "backed by file": "");
	struct thread * t = thread_current();

	return hash_insert(&t->pages, &p->h_elem) == NULL;
}

struct page *
page_get_entry_for_vaddr(const void * vaddr)
{
	void * page_bound = pg_round_down(vaddr);

	struct thread * t = thread_current();
	struct page pte;
	struct hash_elem *e;

	pte.vaddr = page_bound;

	e = hash_find(&t->pages, &pte.h_elem);

	// if(e!=NULL) printf("Found entry for %p\n", vaddr);

	return e != NULL ? hash_entry(e, struct page, h_elem) :
		NULL;
}

unsigned
page_hash(const struct hash_elem *e, void * aux UNUSED)
{
	struct page * pte = hash_entry(e, struct page, h_elem);

	return hash_bytes(&pte->vaddr, sizeof pte->vaddr);
}

bool
page_less(const struct hash_elem *a, const struct hash_elem *b, void *aux UNUSED)
{
	struct page * ptea = hash_entry(a, struct page, h_elem);
	struct page * pteb = hash_entry(b, struct page, h_elem);

	return ptea->vaddr < pteb->vaddr;
}

void
page_hash_destroy(struct hash_elem *e, void* aux UNUSED)
{
	struct page * p = hash_entry(e, struct page, h_elem);
	page_destroy(p);
}

void
page_destroy(struct page * p)
{
	struct thread * t = thread_current();
	if (p->state == FRAMED && p->origin == MMAPPED_FILE && pagedir_is_dirty (t->pagedir, p->vaddr))
	{
		if(debug)
			printf("Writing from %p back to file at %x with %d bytes\n", p->vaddr, p->f_offset, p->size);
		file_write_at (p->f, p->vaddr, p->size, p->f_offset);
	}
	else if(p->state == ON_SWAP)
	{
		swap_retrieve(p->swap_slot, NULL);
	}

	if(debug)
		printf("Freeing page at %p+%d\n", p->vaddr, p->size);
	
	hash_delete (&t->pages, &p->h_elem);
	free(p);
}
