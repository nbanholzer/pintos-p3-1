#include "vm/frame.h"
#include "threads/palloc.h"
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <debug.h>
#include <string.h>
#include "threads/vaddr.h"
#include "threads/malloc.h"
#include "vm/swap.h"
#include "userprog/pagedir.h"
#include "page.h"


static struct f_table frame_table;

void init_frame_table()
{
  //To calculate number of user pages
  //Taken from palloc.c
  extern size_t user_pool_size;

  //initialize used_map and frame_list with size user_pages
  lock_init (&frame_table.lock);
  frame_table.used_map = bitmap_create(user_pool_size);
  frame_table.frame_list = (struct frame *)malloc(sizeof(struct frame)*user_pool_size);

  //Allocates user pool and stores page address in kpage
  for (size_t i = 0; i < user_pool_size; i++) {
    frame_table.frame_list[i].kpage = palloc_get_page(PAL_USER);
  }
  //init_swap_table();
}

void *get_frame(enum frame_flags flags, void* upage, struct thread * t)
{
  return get_frame_multiple(flags, 1, upage, t);
}

void *get_frame_multiple(enum frame_flags flags, size_t frame_cnt, void* upage, struct thread * t)
{
  size_t frame_idx;

  lock_acquire(&frame_table.lock);
  frame_idx = bitmap_scan_and_flip (frame_table.used_map, 0, frame_cnt, false);
  if (frame_idx == BITMAP_ERROR){
    PANIC("No frames remaining");
  }
  //update upage
  frame_table.frame_list[frame_idx].upage = upage;
  frame_table.frame_list[frame_idx].t = t;
  lock_release(&frame_table.lock);
  //do we ever call get_frame_multiple?
  return frame_table.frame_list[frame_idx].kpage;
}

void free_frame(void *frame)
{
  free_frame_multiple(frame, 1);
}

void free_frame_multiple(void *frames, size_t frame_cnt)
{
  size_t frame_idx = pg_no (frames) - pg_no (frame_table.frame_list[0].kpage);

#ifndef NDEBUG
  memset (frames, 0xcc, PGSIZE * frame_cnt);
#endif

  ASSERT (bitmap_all (frame_table.used_map, frame_idx, frame_cnt));
  bitmap_set_multiple (frame_table.used_map, frame_idx, frame_cnt, false);
}

void
evict(struct frame *frame)
{
  struct s_page_entry * spe = find_page_entry(frame->t, frame->upage);
  uint32_t * pd = frame->t->pagedir;

  //want to set to read from file if not directory
  if(!pagedir_is_dirty(pd, frame->upage) && (spe->file != NULL)){
    free_frame(frame);
    spe->in_file = true;
    spe->in_frame = false;
    spe->frame_addr = NULL;
  }
  else{
    size_t swap_idx= swap_write(frame);
    spe->in_swap = true;
    spe->in_frame = false;
    spe->frame_addr = NULL;
    spe->swap_idx = swap_idx;
    free_frame(frame);
  }
  return;
}

int
eviction_routine(){
  size_t size= bitmap_size(frame_table.used_map);
  uint32_t * pd;
  void * page;
  for (size_t i = 0; i < size; i++) {
    pd = frame_table.frame_list[i].t->pagedir;
    page = frame_table.frame_list[i].upage;
    if (!pagedir_is_accessed(pd, page)) {
      evict(&frame_table.frame_list[i]);
      return (int)i;
    }
  }
  //no unaccessed frames found
  for (size_t i = 0; i < size; i++) {
    pd = frame_table.frame_list[i].t->pagedir;
    page = frame_table.frame_list[i].upage;
    pagedir_set_accessed(pd, page, false);
  }
  return -1;
}
