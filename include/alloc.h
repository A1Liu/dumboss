#pragma once
#include "memory.h"

void alloc__init(MMapEnt *entries, int64_t entry_count);

// Allocate `count` contiguous pages, each of size 4kb
void *alloc(int64_t count);

// Free contiguous pages starting at data
void free(void *data, int64_t count);

// Check that the heap is in a valid state
void alloc__validate_heap(void);
