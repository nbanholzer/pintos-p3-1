#ifndef VM_FRAME_H
#define VM_FRAME_H

#include <stddef.h>

/* Set up to mirror palloc_flags */
enum frame_flags
{
  FRAME_ZERO = 002
};

void init_frame_table();
void *get_frame (enum frame_flags);
void *get_frame_multiple(enum frame_flags, size_t frame_cnt);
void free_frame (void *frame);
void free_frame_multiple(void *frames, size_t frame_cnt);

#endif /* vm/frame.h */
