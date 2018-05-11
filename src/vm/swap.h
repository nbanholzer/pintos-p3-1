#ifndef VM_SWAP_H
#define VM_SWAP_H

#include <stddef.h>

void init_swap_table(void);
size_t swap_write(void * frame);
void swap_read(void * frame, size_t swap_idx);
void swap_free(size_t swap_idx);

#endif /* vm/swap.h */
