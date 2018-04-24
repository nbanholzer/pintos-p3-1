#include "vm/page.h"
#include "lib/kernel/hash.h"
#include "vm/frame.h"
#include "threads/palloc.h"
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <bitmap.h>
#include <debug.h>
#include <string.h>
#include "threads/synch.h"
#include "threads/vaddr.h"
#include "threads/malloc.h"

struct s_page_entry*
init_s_page_entry(uint8_t *u_addr, struct file *file, size_t read_bytes)
{
  struct s_page_entry *spe;
  //TODO:fix malloc size
  spe = (struct s_page_entry *)malloc(sizeof(struct frame)*user_pool_size);
  spe->addr = u_addr;
  spe->file = file;
  spe->read_bytes = read_bytes;
  return spte;
}

unsigned
page_hash(const struct hash_elem *p_, void *aux UNUSED)
{
  const struct s_page_entry *spe = hash_entry(p_, struct s_page_entry, hash_elem);
  //TODO: Can we switch this sizeof to 8bytes since addresses are always the same size?
  return hash_bytes(&spe->addr, sizeof(p->addr));
}

bool
page_less(const struct hash_elem *a_, const struct hash_elem *b_, void *aux UNUSED)
{
  const struct s_page_entry *a = hash_entry(a_, struct s_page_entry, hash_elem);
  const struct s_page_entry *b = hash_entry(b_, struct s_page_entry, hash_elem);

  return a->addr < b->addr;

}
