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
    void* kp;
    kp = frame_table.frame_list[i].kpage = palloc_get_page(PAL_USER);
    lock_init(&frame_table.frame_list[i].lock);
  }
  init_swap_table();
}

struct lock *get_frame_lock(void *frame)
{
  // NOTE: this should be what you need to get a frame index
  unsigned frame_idx = pg_no(frame) - pg_no(frame_table.frame_list[0].kpage);
  return &frame_table.frame_list[frame_idx].lock;
}

void *get_frame(enum frame_flags flags, void* upage, struct thread * t)
{
  return get_frame_multiple(flags, 1, upage, t);
}

void *get_frame_multiple(enum frame_flags flags, size_t frame_cnt, void* upage, struct thread * t)
{
  int frame_idx;

  lock_acquire(&frame_table.lock);
  frame_idx = bitmap_scan_and_flip (frame_table.used_map, 0, frame_cnt, false);
  if (frame_idx == BITMAP_ERROR){
    //printf("call eviction_routine");
    frame_idx = eviction_routine();
    if (frame_idx == -1) {
      lock_release(&frame_table.lock);
      return NULL;
    }
    else{
      frame_idx = bitmap_scan_and_flip (frame_table.used_map, 0, frame_cnt, false);
    }
  }
  lock_release(&frame_table.lock);

  //update upage
  struct frame *f = &frame_table.frame_list[frame_idx];
  lock_acquire(&f->lock);
  f->upage = upage;
  f->t = t;
  lock_release(&f->lock);
  //do we ever call get_frame_multiple?
  return f->kpage;
}

void free_frame(void *frame)
{
  free_frame_multiple(frame, 1);
}

void free_frame_multiple(void *frames, size_t frame_cnt)
{
  size_t frame_idx = pg_no (frames) - pg_no (frame_table.frame_list[0].kpage);
  frame_table.frame_list[0].upage = NULL;

#ifndef NDEBUG
  memset (frames, 0xcc, PGSIZE * frame_cnt);
#endif
  //lock_acquire(&frame_table.lock);
  ASSERT (bitmap_all (frame_table.used_map, frame_idx, frame_cnt));
  bitmap_set_multiple (frame_table.used_map, frame_idx, frame_cnt, false);
  //lock_release(&frame_table.lock);

}

void
evict(struct frame *frame)
{
  struct s_page_entry * spe = find_page_entry(frame->t, frame->upage);
  uint32_t * pd = frame->t->pagedir;

  //want to set to read from file if not directory
  if(!pagedir_is_dirty(pd, frame->upage) && (spe->file != NULL)){
    pagedir_clear_page(pd, frame->upage);
    free_frame(frame->kpage);
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
    pagedir_clear_page(pd, frame->upage);
    free_frame(frame->kpage);
  }
  return;
}

int
eviction_routine(){
  size_t size= bitmap_size(frame_table.used_map);
  uint32_t * pd;
  void * page;
  for (size_t i = 0; i < size; i++) {
    bool lock_check = lock_try_acquire(&frame_table.frame_list[i].lock);
    pd = frame_table.frame_list[i].t->pagedir;
    page = frame_table.frame_list[i].upage;
    printf("debug1\n");
    printf("%p\n", page);
    if (lock_check && !pagedir_is_accessed(pd, page)) {
      evict(&frame_table.frame_list[i]);
      lock_release(&frame_table.frame_list[i].lock);
      return (int)i;
    }
    lock_release(&frame_table.frame_list[i].lock);
    printf("debug2\n");
  }
  //no unaccessed frames found
  for (size_t i = 0; i < size; i++) {
    pd = frame_table.frame_list[i].t->pagedir;
    page = frame_table.frame_list[i].upage;
    printf("debug3\n");
    pagedir_set_accessed(pd, page, false);
    printf("debug4\n");
  }
  return -1;
}
