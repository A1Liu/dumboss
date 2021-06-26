#pragma once
#include "bootboot.h"
#include "logging.h"
#include <stddef.h>
#include <stdint.h>

#define _4KB ((uint64_t)4096)
// #define _2MB 2097152LL
// #define _1GB 1073741824LL

int64_t alloc__init(MMapEnt *entries, int64_t entry_count);

// Allocate `count` contiguous pages, each of size 4kb
void *alloc(int64_t count);

// Free contiguous pages starting at data
void free(void *data, int64_t count);

void alloc__validate_heap(void);

void alloc__assert_heap_empty(void);
