#include <stdio.h>

#include "vm/page.h"

#include "lib/kernel/hash.h"
#include "threads/vaddr.h"
#include "threads/thread.h"
#include "threads/malloc.h"

void
page_init(void)
{

}

bool
mmfile_add_to_page_table (struct file * file, int ofs, int size, void * addr, size_t page_read_bytes)
{
	struct page * pte = malloc(sizeof(struct page));

	pte->vaddr = addr;
	pte->size = size;
	pte->state = ON_DISK;
	pte->f = file;
	pte->f_offset = ofs;
	pte->writable = true;

	if (page_add_entry(pte) == true)
	{
		struct mapped_file *mmfile = malloc(sizeof(struct mapped_file));

		mmfile->mapping = thread_current()->mapid;
		mmfile->p = pte;
		list_push_back(&thread_current()->mappedfiles, &mmfile->elem);	
		return true;
	}

	free(pte);

	return false;
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
	// TODO: Write back to file

	if(debug)
		printf("Freeing page at %p+%d\n", p->vaddr, p->size);
	free(p);
}
