#ifndef VM_SWAP_H
#define VM_SWAP_H

#include <devices/block.h>

void swap_init(void);
block_sector_t swap_store(void*);
void swap_retrieve(block_sector_t, void *);

#endif