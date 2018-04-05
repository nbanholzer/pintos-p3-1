#include "vm/frame.h"
#include "threads/palloc.h"
#include <stddef.h>

void *get_frame(enum frame_flags flags)
{
  // TODO: actually build the data structure for this
  return palloc_get_page(PAL_USER | flags);
}

void *get_frame_multiple(enum frame_flags flags, size_t frame_cnt)
{
  return palloc_get_multiple(PAL_USER | flags, frame_cnt);
}

void free_frame(void *frame)
{
  palloc_free_page(frame);
}

void free_frame_multiple(void *frames, size_t frame_cnt)
{
  palloc_free_multiple(frames, frame_cnt);
}