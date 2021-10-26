#pragma once
#include "memory.h"

void alloc__init(MMap mmap);

// Allocate `count` contiguous pages, each of size 4kb
void *alloc(s64 count);

// Free contiguous pages starting at data
void free(void *data, s64 count);

// Check that the heap is in a valid state
void alloc__validate_heap(void);
