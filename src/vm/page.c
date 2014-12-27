#include <stdio.h>

#include "vm/page.h"

#include "lib/kernel/hash.h"
#include "threads/vaddr.h"
#include "threads/thread.h"

void
page_init(void)
{

}

void
page_add_to_executabe_segment(struct page * pte)
{
	struct thread * t = thread_current();
	hash_insert(&t->pages, &pte->h_elem);
}

struct page *
page_get_entry_for_vaddr(void * vaddr)
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
