#include "filesys/filesys.h"
#include <debug.h>
#include <stdio.h>
#include <string.h>
#include "filesys/file.h"
#include "filesys/free-map.h"
#include "filesys/inode.h"
#include "filesys/directory.h"
#include "filesys/cache.h"

/* Partition that contains the file system. */
struct block *fs_device;

static void do_format (void);

/* Initializes the file system module.
   If FORMAT is true, reformats the file system. */
void
filesys_init (bool format) 
{
  fs_device = block_get_role (BLOCK_FILESYS);
  if (fs_device == NULL)
    PANIC ("No file system device found, can't initialize file system.");

  inode_init ();
  free_map_init ();
  cache_init();

  if (format) 
    do_format ();

  free_map_open ();
}

/* Shuts down the file system module, writing any unwritten data
   to disk. */
void
filesys_done (void) 
{
  free_map_close ();
  cache_flush();
}

/* Creates a file named NAME with the given INITIAL_SIZE.
   Returns true if successful, false otherwise.
   Fails if a file named NAME already exists,
   or if internal memory allocation fails. */
bool
filesys_create (struct dir * parent, const char *name, off_t initial_size) 
{
  block_sector_t inode_sector = 0;
  bool success = (parent != NULL
                  && free_map_allocate (1, &inode_sector)
                  && inode_create (inode_sector, initial_size, false, dir_get_inode(parent))
                  && dir_add (parent, name, inode_sector));
  if (!success && inode_sector != 0) 
    free_map_release (inode_sector, 1);
  // dir_close (parent);
  return success;
}

/* Creates a directory named NAME with the given INITIAL_SIZE.
   Returns true if successful, false otherwise.
   Fails if a directory named NAME already exists,
   or if internal memory allocation fails. */
bool
filesys_create_dir (struct dir * parent, const char *name)
{
  block_sector_t inode_sector = 0;
  bool success = (parent != NULL
                  && free_map_allocate (1, &inode_sector)
                  && inode_create (inode_sector, 1, true, dir_get_inode(parent))
                  && dir_add (parent, name, inode_sector));
  if (!success && inode_sector != 0) 
    free_map_release (inode_sector, 1);
  // dir_close (parent);

  return success;
}

/* Opens the file with the given NAME.
   Returns the new file if successful or a null pointer
   otherwise.
   Fails if no file named NAME exists,
   or if an internal memory allocation fails. */
struct file *
filesys_open (const char *name)
{
  struct dir *dir = dir_resolve(name);
  struct inode *inode = NULL;
  char * last_segment = strrchr(name, '/');
  if(last_segment == NULL)
    last_segment = name;
  else
    last_segment += 1;


  if (dir != NULL)
    dir_lookup (dir, last_segment, &inode);
  // dir_close (dir);

  return file_open (inode);
}

/* Deletes the file named NAME.
   Returns true if successful, false on failure.
   Fails if no file named NAME exists,
   or if an internal memory allocation fails. */
bool
filesys_remove (const char *name) 
{
  struct inode * inode;
  if(dir_resolve_deep(name, &inode) == 0)
  {
    block_sector_t ps = inode_parent(inode);
    struct inode * pi = inode_open(ps);
    struct dir * parent = dir_open(pi);
    char * last_segment = strrchr(name, '/');
    if(last_segment == NULL)
      last_segment = name;
    else
      last_segment += 1;

    if(inode_is_dir(inode) && !dir_is_empty(dir_open(inode)))
      return false;
    
    bool ret = dir_remove (parent, last_segment);
    return ret;
  }
  return false;
}

/* Formats the file system. */
static void
do_format (void)
{
  printf ("Formatting file system...");
  free_map_create ();
  if (!dir_create (ROOT_DIR_SECTOR, 16))
    PANIC ("root directory creation failed");
  free_map_close ();
  printf ("done.\n");
}
