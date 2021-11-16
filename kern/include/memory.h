#pragma once
#include "bootboot.h"
#include <types.h>

extern struct {
} Globals;

#define MEMORY__KERNEL_SPACE_BEGIN ((u64)0xffff800000000000ull)

#define _KB (1024)
#define _MB (1024 * 1024)
#define _GB (1024 * 1024 * 1024)

#define _4KB (4096)
#define _2MB (2 * 1024 * 1024)
#define _1GB (1024 * 1024 * 1024)

void memory__init(BOOTBOOT *bb);

// get physical address from kernel address
u64 physical_address(void *ptr);

// get kernel address from physical address
u64 kernel_address(u64 address);

// get kernel address from physical address
void *kernel_ptr(u64 address);

// Allocate `count` contiguous pages
// Pages are zeroed.
void *alloc(s64 count);

// Copy the given memory to a new allocation
Buffer alloc_copy(const void *src, s64 size);

// Free contiguous pages starting at data
void free(void *data, s64 count);

// Free contiguous pages starting at data that were not usable before
void unsafe_mark_memory_usability(void *data, s64 count, bool usable);

// Check that the heap is in a valid state
void alloc__validate_heap(void);
