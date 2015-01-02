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

void
page_add_to_executabe_segment(struct page * pte)
{
	page_add_entry(pte);
}

bool
page_add_entry(struct page * p)
{
	if(debug)
		printf("Added to page table: %p+%d\n", p->vaddr, p->size);
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

	if (p->state == FRAMED && p->f != NULL && pagedir_is_dirty (t->pagedir, p->vaddr))
	{	
		file_write_at (p->f, p->vaddr, p->size, p->f_offset);
	}

	if(debug)
		printf("Freeing page at %p+%d\n", p->vaddr, p->size);
	
	hash_delete (&t->pages, &p->h_elem);
	free(p);
}
