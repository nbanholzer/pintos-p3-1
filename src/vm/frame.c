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
  size_t frame_idx;
  //printf("Acquire, get_frame\n");
  lock_acquire(&frame_table.lock);
  frame_idx = bitmap_scan_and_flip (frame_table.used_map, 0, frame_cnt, false);
  //printf("test_frame_bool: %u\n", bitmap_test(&frame_table.used_map, 2));
  //printf("get_frame_idx: %u\n", frame_idx);
  if (frame_idx == BITMAP_ERROR){
    //printf("call eviction_routine\n");
    int evict_check = eviction_routine();
    //printf("Eviction return: %i\n", evict_check);
    if (evict_check == -1) {
      //printf("Release, get_frame\n");
      lock_release(&frame_table.lock);
      return NULL;
    }
    else{
      frame_idx = bitmap_scan_and_flip (frame_table.used_map, 0, frame_cnt, false);
      //printf("frame idx: %u\n", frame_idx);
    }
  }
  struct frame *f = &frame_table.frame_list[frame_idx];
  lock_acquire(&f->lock);
  //printf("Release, get_frame\n");

  //update upage
  f->upage = upage;
  //printf("put upage: %p\n", upage );
  f->t = t;
  lock_release(&frame_table.lock);
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
  //printf("Old Upage: %p\n", );
  frame_table.frame_list[frame_idx].upage = NULL;

#ifndef NDEBUG
  memset (frames, 0xcc, PGSIZE * frame_cnt);
#endif
  //lock_acquire(&frame_table.lock);
  ASSERT (bitmap_all (frame_table.used_map, frame_idx, frame_cnt));
  bitmap_set_multiple (frame_table.used_map, frame_idx, frame_cnt, false);
  //printf("free_frame1: %u\n", frame_idx);
  //lock_release(&frame_table.lock);

}

void
evict(struct frame *frame)
{
  //printf("debug1\n");
  struct s_page_entry * spe = find_page_entry(frame->t, frame->upage);
  //printf("debug2\n");
  uint32_t * pd = frame->t->pagedir;
  //printf("debug3\n");
  //printf("evict upage: %p\n", frame->upage);
  //want to set to read from file if not directory
  if(spe->is_mmap && pagedir_is_dirty(pd, frame->upage)){
    lock_acquire(frame->t->filesys_lock);
    // TODO: this is gonna blow up if this thing isn't in a frame
    unsigned bytes_written = file_write_at(spe->file, spe->frame_addr, spe->read_bytes, spe->ofs);
    lock_release(frame->t->filesys_lock);
    pagedir_clear_page(pd, frame->upage);
    pagedir_set_accessed(pd, frame->upage, false);
    pagedir_set_dirty(pd, frame->upage, false);
    free_frame(frame->kpage);
    spe->in_file = true;
    spe->in_frame = false;
    spe->frame_addr = NULL;
  }
  else if(!pagedir_is_dirty(pd, frame->upage) && (spe->file != NULL)){
    //printf("debug4\n");
    pagedir_clear_page(pd, frame->upage);
    pagedir_set_accessed(pd, frame->upage, false);
    pagedir_set_dirty(pd, frame->upage, false);
    free_frame(frame->kpage);
    spe->in_file = true;
    spe->in_frame = false;
    spe->frame_addr = NULL;

    //printf("debug5\n");
  }
  else{
    //printf("debug6\n");
    size_t swap_idx= swap_write(frame);
    pagedir_clear_page(pd, frame->upage);
    pagedir_set_accessed(pd, frame->upage, false);
    pagedir_set_dirty(pd, frame->upage, false);
    spe->in_swap = true;
    spe->in_frame = false;
    spe->frame_addr = NULL;
    spe->swap_idx = swap_idx;
    free_frame(frame->kpage);
    //printf("debug7\n");
  }
  return;
}

int
eviction_routine(){
  size_t size= bitmap_size(frame_table.used_map);
  uint32_t * pd;
  void * page;
  //printf("in eviction routine\n");
  for (size_t i = 0; i < size; i++) {
    bool lock_check = lock_try_acquire(&frame_table.frame_list[i].lock);
    pd = frame_table.frame_list[i].t->pagedir;
    page = frame_table.frame_list[i].upage;
    //printf("debug1\n");
    //printf("%p\n", page);
    if (lock_check && !pagedir_is_accessed(pd, page)) {
      evict(&frame_table.frame_list[i]);
      lock_release(&frame_table.frame_list[i].lock);
      return (int)i;
    }
    lock_release(&frame_table.frame_list[i].lock);
    //printf("debug2\n");
  }
  //no unaccessed frames found
  for (size_t i = 0; i < size; i++) {
    pd = frame_table.frame_list[i].t->pagedir;
    page = frame_table.frame_list[i].upage;
    //printf("debug3\n");
    pagedir_set_accessed(pd, page, false);
    //printf("debug4\n");
  }
  return -1;
}
