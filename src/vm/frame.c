#include "vm/frame.h"
#include "threads/palloc.h"
#include <stddef.h>
#include <bitmap.h>

struct f_table
{
  struct lock lock;                   /* Mutual exclusion. */
  struct bitmap *used_map;            /* Bitmap of free pages. */
  struct frame *frame_list;           /* Bitmap of free pages. */
}

struct frame
{
  void *upage;
  void *kpage;
}

static struct f_table frame_table;

void init_frame_table()
{
  //To calculate number of user pages
  //Taken from palloc.c
  //I'm worried about this peing incorrect because palloc puts the map
  //in the pool, perhaps lowering this number
  size_t user_pages = bitmap_size(user_pool->used_map);

  //initialize used_map and frame_list with size user_pages
  frame_table->used_map = bitmap_create(user_pages);
  frame_table->frame_list = (struct frame *)malloc(sizeof(struct frame)*user_pages);

  //Allocates user pool and stores page address in kpage
  //TODO are we guaranteed these are sequential?
  for (size_t i = 0; i < user_pages; i++) {
    frame_table->frame_list->kpage = palloc_get_page(PAL_USER);
  }
}

void *get_frame(enum frame_flags flags)
{
  return get_frame_multiple(flags, 1)
}

void *get_frame_multiple(enum frame_flags flags, size_t frame_cnt)
{
  void *frames;
  size_t frame_idx;

  lock_acquire(frame_table->lock);
  frame_idx = bitmap_scan_and_flip (frame_table->used_map, 0, page_cnt, false);
  lock_release(frame_table->);

  //would like to update the upage somehow?
  return frame_table->frame_list[frame_idx]->kpage;
}

void free_frame(void *frame)
{
  palloc_free_page(frame);
}

void free_frame_multiple(void *frames, size_t frame_cnt)
{
  palloc_free_multiple(frames, frame_cnt);
}
