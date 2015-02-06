#include "filesys/inode.h"
#include <list.h>
#include <debug.h>
#include <stddef.h>
#include <round.h>
#include <string.h>
#include "filesys/filesys.h"
#include "filesys/free-map.h"
#include "threads/malloc.h"
#include "filesys/cache.h"

/* Identifies an inode. */
#define INODE_MAGIC 0x494e4f44
#define DIRECTS 121
#define INDIRECTS (BLOCK_SECTOR_SIZE/(sizeof(block_sector_t)))
#define MAX_SECTORS ((unsigned)(DIRECTS + INDIRECTS + INDIRECTS * INDIRECTS))

/* On-disk inode.
   Must be exactly BLOCK_SECTOR_SIZE bytes long. */
struct inode_disk
  {
    block_sector_t start;               /* First data sector. */
    off_t length;                       /* File size in bytes. */
    block_sector_t parent;              /* Block of parent inode */
    uint32_t is_dir;                    /* Positive, if this inode is a dir */
    unsigned magic;                     /* Magic number. */
    block_sector_t sectors[DIRECTS];    /* Direct pointers */
    block_sector_t i_inode;             /* Pointer to single-indirect inode */
    block_sector_t di_inode;            /* Pointer to double-indirect inode */
  };

/* On-disk indirect inode */
struct indirect_inode_disk
{
  block_sector_t sectors[INDIRECTS];
};

/* Returns the number of sectors to allocate for an inode SIZE
   bytes long. */
static inline size_t
bytes_to_sectors (off_t size)
{
  return DIV_ROUND_UP (size, BLOCK_SECTOR_SIZE);
}

/* In-memory inode. */
struct inode 
  {
    struct list_elem elem;              /* Element in inode list. */
    block_sector_t sector;              /* Sector number of disk location. */
    int open_cnt;                       /* Number of openers. */
    bool removed;                       /* True if deleted, false otherwise. */
    int deny_write_cnt;                 /* 0: writes ok, >0: deny writes. */
    struct inode_disk data;             /* Inode content. */
  };

/* Returns the block device sector that contains byte offset POS
   within INODE.
   Returns -1 if INODE does not contain data for a byte at offset
   POS. */
static block_sector_t
byte_to_sector (struct inode *inode, off_t pos, bool extend) 
{
  ASSERT (inode != NULL);
  block_sector_t * t;
  block_sector_t sector_idx = pos / BLOCK_SECTOR_SIZE;
  if (sector_idx < MAX_SECTORS)
  {
    if(sector_idx < DIRECTS)
    {
      if(inode->data.sectors[sector_idx] == 0)
      {
        if(!extend)
          return -1;

        if(!free_map_allocate(1, &inode->data.sectors[sector_idx]))
          return -1;
      }
      return inode->data.sectors[sector_idx];
    }
    else if(sector_idx < (DIRECTS + INDIRECTS))
    {
      sector_idx -= DIRECTS;
      struct indirect_inode_disk id;

      if(inode->data.i_inode == 0)
      {
        if(!extend)
          return -1;

        if(!free_map_allocate(1, &inode->data.i_inode))
          return -1;

        cache_write(inode->sector, &inode->data);
      }

      cache_read(inode->data.i_inode, &id);
      t = &id.sectors[sector_idx];
      if(*t == 0)
      {
        if(!extend)
          return -1;

        if(!free_map_allocate(1, t))
          return -1;

        cache_write(inode->data.i_inode, &id);
      }
      return *t;
    }
    else
    {
      struct indirect_inode_disk did, id;

      // If the double-indirect inode is not yet allocated
      if(inode->data.di_inode == 0)
      {
        if(!extend)
          return -1;

        if(!free_map_allocate(1, &inode->data.di_inode))
          return -1;

        cache_write(inode->sector, &inode->data);
      }

      unsigned di_idx = (sector_idx - DIRECTS - INDIRECTS) / INDIRECTS;

      // If the single-indirect node is not yet allocated
      cache_read(inode->data.di_inode, &did);
      if(did.sectors[di_idx] == 0)
      {
        if(!extend)
          return -1;

        if(!free_map_allocate(1, &did.sectors[di_idx]))
          return -1;

        cache_write(inode->data.di_inode, &did);
      }

      unsigned i_idx = (sector_idx - DIRECTS - INDIRECTS) % INDIRECTS;

      cache_read(did.sectors[di_idx], &id);
      if(id.sectors[i_idx] == 0)
      {
        if(!extend)
          return -1;

        if(!free_map_allocate(1, &id.sectors[i_idx]))
          return -1;

        cache_write(did.sectors[di_idx], &id);
      }

      return id.sectors[i_idx];
    }
    return -1;
  }
  else
    return -1;
}

/* List of open inodes, so that opening a single inode twice
   returns the same `struct inode'. */
static struct list open_inodes;

/* Initializes the inode module. */
void
inode_init (void) 
{
  list_init (&open_inodes);
}

/* Initializes an inode with LENGTH bytes of data and
   writes the new inode to sector SECTOR on the file system
   device.
   Returns true if successful.
   Returns false if memory or disk allocation fails. */
bool
inode_create (block_sector_t sector, off_t length, bool is_dir, struct inode * parent)
{
  struct inode in;
  struct inode_disk *disk_inode = NULL;
  bool success = false;

  /* If this assertion fails, the inode structure is not exactly
     one sector in size, and you should fix that. */
  ASSERT (sizeof *disk_inode == BLOCK_SECTOR_SIZE);

  ASSERT ((unsigned)length/BLOCK_SECTOR_SIZE < MAX_SECTORS);

  disk_inode = calloc (1, sizeof *disk_inode);
  if (disk_inode != NULL)
    {
      in.sector = sector;
      disk_inode->length = length;
      disk_inode->magic = INODE_MAGIC;
      disk_inode->is_dir = is_dir;
      if(parent != NULL)
      {
        disk_inode->parent = parent->sector;
      }
      else
        disk_inode->parent = sector;
      
      success = true;

      size_t sectors = bytes_to_sectors(length);
      if(sectors > 0)
        free_map_allocate(1, &disk_inode->sectors[0]);

      
      in.data = *disk_inode;
      unsigned i;
      for(i = 1; i < sectors; i++)
      {
        /* This function has the side effect of allocating a sector 
        if the position is not found */
        success &= (byte_to_sector(&in, i * BLOCK_SECTOR_SIZE, true) != -1);
      }
      if(success)
      {
        cache_write(sector, &in.data);
      }  
      free (disk_inode);
    }
  return success;
}

/* Reads an inode from SECTOR
   and returns a `struct inode' that contains it.
   Returns a null pointer if memory allocation fails. */
struct inode *
inode_open (block_sector_t sector)
{
  struct list_elem *e;
  struct inode *inode;

  /* Check whether this inode is already open. */
  for (e = list_begin (&open_inodes); e != list_end (&open_inodes);
       e = list_next (e)) 
    {
      inode = list_entry (e, struct inode, elem);
      if (inode->sector != ROOT_DIR_SECTOR && inode->sector == sector) 
        {
          inode_reopen (inode);
          return inode; 
        }
    }

  /* Allocate memory. */
  inode = malloc (sizeof *inode);
  if (inode == NULL)
    return NULL;

  /* Initialize. */
  list_push_front (&open_inodes, &inode->elem);
  inode->sector = sector;
  inode->open_cnt = 1;
  inode->deny_write_cnt = 0;
  inode->removed = false;
  cache_read (inode->sector, &inode->data);
  return inode;
}

/* Reopens and returns INODE. */
struct inode *
inode_reopen (struct inode *inode)
{
  if (inode != NULL)
    inode->open_cnt++;
  return inode;
}

/* Returns INODE's inode number. */
block_sector_t
inode_get_inumber (const struct inode *inode)
{
  return inode->sector;
}

/* Closes INODE and writes it to disk.
   If this was the last reference to INODE, frees its memory.
   If INODE was also a removed inode, frees its blocks. */
void
inode_close (struct inode *inode) 
{
  /* Ignore null pointer. */
  if (inode == NULL)
    return;

  /* Release resources if this was the last opener. */
  if (--inode->open_cnt == 0)
    {
      /* Remove from inode list and release lock. */
      list_remove (&inode->elem);
 
      /* Deallocate blocks if removed. */
      if (inode->removed) 
        {
          struct inode_disk in;
          struct indirect_inode_disk s_in, d_in;
          cache_read(inode->sector, &in);
          unsigned i, k;
          printf("About to remove\n");
          /* Deallocate direct blocks ... */
          for(i = 0; i < DIRECTS; i++)
          {
            if(in.sectors[i] != 0)
              free_map_release(in.sectors[i], 1);
          }

          /* ... single-indirect blocks ... */
          if(in.i_inode != 0)
          {
            cache_read(in.i_inode, &s_in);
            for(i = 0; i < INDIRECTS; i++)
            {
              if(s_in.sectors[i] != 0)
                free_map_release(s_in.sectors[i], 1);
            }
            free_map_release(in.i_inode, 1);
          }

          /* ... and double-indirect blocks. */
          if(in.di_inode != 0)
          {
            cache_read(in.di_inode, &d_in);
            for(i = 0; i < INDIRECTS; i++)
            {
              if(d_in.sectors[i] != 0)
              {
                cache_read(d_in.sectors[i], &s_in);
                for(k = 0; k < INDIRECTS; k++)
                {
                  if(s_in.sectors[k] != 0)
                    free_map_release(s_in.sectors[k], 1);
                }
                free_map_release(d_in.sectors[i], 1);
              }
            }
            free_map_release(in.di_inode, 1);
          }
          
          free_map_release (inode->sector, 1);
        }

      free (inode); 
    }
}

/* Marks INODE to be deleted when it is closed by the last caller who
   has it open. */
void
inode_remove (struct inode *inode) 
{
  ASSERT (inode != NULL);
  inode->removed = true;
}

/* Reads SIZE bytes from INODE into BUFFER, starting at position OFFSET.
   Returns the number of bytes actually read, which may be less
   than SIZE if an error occurs or end of file is reached. */
off_t
inode_read_at (struct inode *inode, void *buffer_, off_t size, off_t offset) 
{
  uint8_t *buffer = buffer_;
  off_t bytes_read = 0;
  uint8_t *bounce = NULL;
  while (size > 0) 
    {
      block_sector_t sector_idx;
      /* Disk sector to read, starting byte offset within sector. */
      if(offset <= inode->data.length)
        sector_idx = byte_to_sector (inode, offset, true);
      else
        return 0;
      int sector_ofs = offset % BLOCK_SECTOR_SIZE;

      /* Bytes left in inode, bytes left in sector, lesser of the two. */
      off_t inode_left = inode_length (inode) - offset;
      int sector_left = BLOCK_SECTOR_SIZE - sector_ofs;
      int min_left = inode_left < sector_left ? inode_left : sector_left;

      /* Number of bytes to actually copy out of this sector. */
      int chunk_size = size < min_left ? size : min_left;
      if (chunk_size <= 0)
        break;

      if (sector_ofs == 0 && chunk_size == BLOCK_SECTOR_SIZE)
        {
          /* Read full sector directly into caller's buffer. */
          cache_read (sector_idx, buffer + bytes_read);
        }
      else 
        {
          /* Read sector into bounce buffer, then partially copy
             into caller's buffer. */
          if (bounce == NULL) 
            {
              bounce = malloc (BLOCK_SECTOR_SIZE);
              if (bounce == NULL)
                break;
            }
          cache_read (sector_idx, bounce);
          memcpy (buffer + bytes_read, bounce + sector_ofs, chunk_size);
        }
      
      /* Advance. */
      size -= chunk_size;
      offset += chunk_size;
      bytes_read += chunk_size;
    }
  free (bounce);

  return bytes_read;
}

/* Writes SIZE bytes from BUFFER into INODE, starting at OFFSET.
   Returns the number of bytes actually written, which may be
   less than SIZE if end of file is reached or an error occurs.
   (Normally a write at end of file would extend the inode, but
   growth is not yet implemented.) */
off_t
inode_write_at (struct inode *inode, const void *buffer_, off_t size,
                off_t offset) 
{
  const uint8_t *buffer = buffer_;
  off_t bytes_written = 0;
  uint8_t *bounce = NULL;
  unsigned new_length = inode->data.length > offset+size?
                          inode->data.length : 
                          offset+size;
  if (inode->deny_write_cnt)
    return 0;
  while (size > 0) 
    {
      /* Sector to write, starting byte offset within sector. */
      block_sector_t sector_idx = byte_to_sector (inode, offset, true);
      int sector_ofs = offset % BLOCK_SECTOR_SIZE;

      /* Bytes left in inode, bytes left in sector, lesser of the two. */
      off_t inode_left = BLOCK_SECTOR_SIZE; /* Ana geht nu! */
      int sector_left = BLOCK_SECTOR_SIZE - sector_ofs;
      int min_left = inode_left < sector_left ? inode_left : sector_left;

      /* Number of bytes to actually write into this sector. */
      int chunk_size = size < min_left ? size : min_left;
      if (chunk_size <= 0)
        break;

      if (sector_ofs == 0 && chunk_size == BLOCK_SECTOR_SIZE)
        {
          /* Write full sector directly to disk. */
          cache_write (sector_idx, buffer + bytes_written);
        }
      else 
        {
          /* We need a bounce buffer. */
          if (bounce == NULL) 
            {
              bounce = malloc (BLOCK_SECTOR_SIZE);
              if (bounce == NULL)
                break;
            }

          /* If the sector contains data before or after the chunk
             we're writing, then we need to read in the sector
             first.  Otherwise we start with a sector of all zeros. */
          if (sector_ofs > 0 || chunk_size < sector_left) 
            cache_read (sector_idx, bounce);
          else
            memset (bounce, 0, BLOCK_SECTOR_SIZE);
          memcpy (bounce + sector_ofs, buffer + bytes_written, chunk_size);
          cache_write (sector_idx, bounce);
        }

      /* Advance. */
      size -= chunk_size;
      offset += chunk_size;
      bytes_written += chunk_size;
    }
  free (bounce);
  if(inode->data.length != new_length)
  {
    inode->data.length = new_length;
    cache_write(inode->sector, &inode->data);
  }
  return bytes_written;
}

/* Disables writes to INODE.
   May be called at most once per inode opener. */
void
inode_deny_write (struct inode *inode) 
{
  inode->deny_write_cnt++;
  ASSERT (inode->deny_write_cnt <= inode->open_cnt);
}

/* Re-enables writes to INODE.
   Must be called once by each inode opener who has called
   inode_deny_write() on the inode, before closing the inode. */
void
inode_allow_write (struct inode *inode) 
{
  ASSERT (inode->deny_write_cnt > 0);
  ASSERT (inode->deny_write_cnt <= inode->open_cnt);
  inode->deny_write_cnt--;
}

/* Returns the length, in bytes, of INODE's data. */
off_t
inode_length (const struct inode *inode)
{
  return inode->data.length;
}

/* Returns the sector of the parent inode */
block_sector_t
inode_parent (const struct inode *inode)
{
  return inode->data.parent;
}

/* Returns true if this inode is a directory */
bool
inode_is_dir(const struct inode *inode)
{
  return inode->data.is_dir > 0;
}

void
inode_print(const struct inode *inode)
{
  printf("Inode %p at sector %d\n", inode, inode->sector);
}

bool
inode_is_open(const struct inode *inode)
{
  return inode->open_cnt > 0;
}