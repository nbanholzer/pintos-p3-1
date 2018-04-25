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
init_s_page_entry(uint8_t *u_addr, struct file *file, off_t ofs, size_t read_bytes)
{
  struct s_page_entry *spe;
  spe = (struct s_page_entry *)malloc(sizeof(struct s_page_entry);
  spe->addr = u_addr;
  spe->file = file;
  spe->ofs = ofs;
  spe->read_bytes = read_bytes;
  spe->in_frame = false;
  spe->in_swap = false;
  spe->in_file = true;
  return spte;
}

struct s_page_entry*
init_stack_entry(uint8_t *u_addr, uint8_t *frame_addr)
{
  struct s_page_entry *spe;
  spe = (struct s_page_entry *)malloc(sizeof(struct s_page_entry);
  spe->addr = u_addr;
  spe->frame_addr = frame_addr;
  spe->in_frame = true;
  spe->in_swap = false;
  spe->in_file = false;
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
