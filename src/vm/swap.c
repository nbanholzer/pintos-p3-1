#include "vm/swap.h"
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
#include "vm/frame.h"
#include "devices/block.h"



struct swap_table
{
  struct lock lock;
  struct bitmap *used_map;
  struct block *swap_block;
  size_t swap_size; //in pages
};

static struct swap_table swap_table;

void
init_swap_table()
{
  swap_table.swap_block = block_get_role(BLOCK_SWAP);
  swap_table.swap_size = (size_t)(block_size(swap_table.swap_block)/8);
  swap_table.used_map = bitmap_create(swap_table.swap_size);
  lock_init(&swap_table.lock);
}

size_t
swap_write(void * frame)
{
  lock_acquire(&swap_table.lock);
  size_t write_idx;
  write_idx = bitmap_scan_and_flip(swap_table.used_map, 0, 1, false);
  if (write_idx == BITMAP_ERROR) {
    PANIC("No More Swap Slots");
  }
  for (size_t i = 0; i < 8; i++) {
    block_write(swap_table.swap_block, (write_idx * 8) + i, frame + (512 * i));
  }
  lock_release(&swap_table.lock);
}

void
swap_read(void * frame, size_t swap_idx) {
  lock_acquire(&swap_table.lock);
  for (size_t i = 0; i < 8; i++) {
    block_read(swap_table.swap_block, (swap_idx * 8) + i, frame + (512 * i));
  }
  bitmap_set_multiple(swap_table.used_map, swap_idx, 1, false);
  lock_release(&swap_table.lock);
}
