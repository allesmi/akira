#ifndef CACHE_H
#define CACHE_H

#include <stddef.h>
#include <stdbool.h>
#include <inttypes.h>
#include "devices/block.h"
#include "threads/synch.h"

#define CACHE_SIZE 64

/* Metadata for one entry in the buffer cache */
struct cache_entry
{
	struct lock l;		/* Lock for this cache entry */
	block_sector_t sector; 	/* Sector that backs this entry */
	bool dirty; 			/* Written recently */
	bool accessed; 			/* Read/written recently */
};

void cache_init(void);
void cache_read(block_sector_t, void*);
void cache_write(block_sector_t, const void*);

#endif