#ifndef VM_FRAME_H
#define VM_FRAME_H

#include <stddef.h>
#include "threads/thread.h"
#include <bitmap.h>
#include "threads/synch.h"

/* Set up to mirror palloc_flags */
enum frame_flags
{
  FRAME_ZERO = 002
};

struct f_table
{
  struct lock lock;                   /* Mutual exclusion. */
  struct bitmap *used_map;            /* Bitmap of free pages. */
  struct frame *frame_list;           /* Array of frames. */
};

struct frame
{
  void *upage;
  void *kpage;
  struct thread *t;
  struct lock lock;
};

void init_frame_table(void);
void *get_frame (enum frame_flags, void* upage, struct thread *t);
void *get_frame_multiple(enum frame_flags, size_t frame_cnt, void* upage, struct thread *t);
struct lock *get_frame_lock(void *frame);
void free_frame (void *frame);
void free_frame_multiple(void *frames, size_t frame_cnt);
void evict(struct frame *frame);
int eviction_routine(void);

#endif /* vm/frame.h */
