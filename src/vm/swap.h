#ifndef VM_SWAP_H
#define VM_SWAP_H

void swap_init(void);
unsigned swap_store(void*);
void * swap_retrieve(unsigned);

#endif