#pragma once
#include "bootboot.h"
#include <stdint.h>

#define MMapEnt_ALLOC_FAILURE ((void *)~0ULL)
#define MEMORY__KERNEL_BEGIN 0x2000000000000000ull

#define _4KB 4096
// #define _2MB ((uint64_t)2097152)
// #define _1GB ((uint64_t)1073741824)

extern char *memory__bootboot_mmap_typename[];

void *alloc_from_entries(MMapEnt *entries, int64_t entry_count, int64_t size,
                         int64_t align);

int64_t memory__init(MMapEnt *entries, int64_t entry_count);

// get physical address from kernel address
uint64_t physical_address(void *ptr);

// get kernel address from physical address
void *kernel_address(uint64_t);
