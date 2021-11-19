#pragma once
#include <types.h>

#define MEMORY__KERNEL_SPACE_BEGIN ((u64)0xffff800000000000ull)

// get physical address from kernel address
u64 physical_address(const void *ptr);

// get kernel address from physical address
u64 kernel_address(u64 address);

// get kernel address from physical address
void *kernel_ptr(u64 address);

// Allocate `count` physically contiguous pages. Pages will be uninitialized.
Buffer try_raw_pages(s64 count);

// Allocate `count` physically contiguous pages. Pages will be uninitialized.
void *raw_pages(s64 count);

// Allocate `count` contiguous pages
// Pages are zeroed.
void *zeroed_pages(s64 count);

// Release contiguous pages starting at data
void release_pages(void *data, s64 count);

void unsafe_mark_memory_usability(const void *data, s64 count, bool usable);

// Check that the heap is in a valid state
void validate_heap(void);
