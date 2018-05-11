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
#include "threads/malloc.h"
#include <debug.h>
#include "userprog/pagedir.h"
#include "lib/round.h"
#include "vm/swap.h"

struct s_page_entry*
init_s_page_entry(uint8_t *u_addr, struct file *file, off_t ofs, size_t read_bytes, bool writable)
{
  struct s_page_entry *spe;
  spe = (struct s_page_entry *)malloc(sizeof(struct s_page_entry));
  spe->addr = u_addr;
  spe->file = file;
  spe->ofs = ofs;
  spe->read_bytes = read_bytes;
  spe->in_frame = false;
  spe->in_swap = false;
  spe->in_file = true;
  spe->writable = writable;
  return spe;
}

struct s_page_entry*
init_stack_entry(uint8_t *u_addr, uint8_t *frame_addr)
{
  struct s_page_entry *spe;
  spe = (struct s_page_entry *)malloc(sizeof(struct s_page_entry));
  spe->addr = u_addr;
  spe->frame_addr = frame_addr;
  spe->in_frame = true;
  spe->in_swap = false;
  spe->in_file = false;
  spe->writable = true;
  return spe;
}

unsigned
page_hash(const struct hash_elem *p_, void *aux UNUSED)
{
  const struct s_page_entry *spe = hash_entry(p_, struct s_page_entry, hash_elem);
  return hash_int((int)spe->addr);
}

bool
page_less(const struct hash_elem *a_, const struct hash_elem *b_, void *aux UNUSED)
{
  const struct s_page_entry *a = hash_entry(a_, struct s_page_entry, hash_elem);
  const struct s_page_entry *b = hash_entry(b_, struct s_page_entry, hash_elem);

  return a->addr < b->addr;

}

void
deallocate_page(struct hash_elem *element, void *aux)
{
  struct thread *t = aux;
  struct s_page_entry *spe = hash_entry(element ,struct s_page_entry, hash_elem);
  //printf("debug 4\n");
  if(spe->in_frame){
    //printf("debug 5\n");
    pagedir_clear_page(t->pagedir, spe->addr);
    //printf("Acquire, deallocate page\n");
    lock_acquire(get_frame_lock(spe->frame_addr));
    free_frame(spe->frame_addr);
    //printf("Release, deallocate page\n");
    lock_release(get_frame_lock(spe->frame_addr));
    //printf("debug 6\n");
  }
  if(spe->in_swap){
    swap_free(spe->swap_idx);
  }
  hash_delete(&t->s_page_table, &spe->hash_elem);
  free(spe);
  //printf("debug 7\n");
}

void
delete_s_page_table(struct hash *table)
{
  //printf("debug 3\n");
  hash_destroy(table, deallocate_page);
}

struct s_page_entry *
find_page_entry(struct thread *t, void * addr){
  struct s_page_entry temp_spe;
  struct hash_elem *e;

  temp_spe.addr = (void*)ROUND_DOWN((unsigned)addr, (unsigned)PGSIZE);
  e = hash_find(&t->s_page_table, &temp_spe.hash_elem);
  if (e)
  {
    struct s_page_entry *spe = hash_entry(e ,struct s_page_entry, hash_elem);
    return spe;
  }
  else{
    return NULL;
  }
}
