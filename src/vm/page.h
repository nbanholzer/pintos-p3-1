#ifndef VM_PAGE_H
#define VM_PAGE_H

#include "lib/kernel/hash.h"
#include "filesys/directory.h"
#include "filesys/file.h"
#include "filesys/filesys.h"

//should these have a lock?
struct s_page_entry
{
  struct hash_elem hash_elem;
  uint8_t *addr;
  bool in_frame;
  bool in_swap;
  bool in_file;
  uint8_t *frame_addr;
  struct *file file;
  off_t ofs;
  size_t read_bytes;
};

struct s_page_entry* init_s_page_entry(uint8_t *u_addr, struct *file file, off_t ofs, size_t read_bytes);

struct s_page_entry* init_stack_entry(uint8_t *u_addr, uint8_t *frame_addr);

unsigned page_hash(const struct hash_elem *p_, void *aux UNUSED);

bool page_less (const struct hash_elem *a_, const struct hash_elem *b_, void aux* UNUSED);

//TODO: Deallocate s_page_entry function

#endif /* vm/page.h */
