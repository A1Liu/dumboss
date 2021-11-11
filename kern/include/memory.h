#pragma once
#include "bootboot.h"
#include <types.h>

#define MMapEnt__ALLOC_FAILURE     ((void *)~(u64)0)
#define MEMORY__KERNEL_SPACE_BEGIN ((u64)0xffff800000000000ull)

#define _4KB 4096
// #define _2MB ((u64)2097152)
// #define _1GB ((u64)1073741824)

typedef struct PageTable4 PageTable4;

typedef struct {
  MMapEnt *data;
  s64 count;
  u64 memory_size;
} MMap;

extern const char *const memory__bootboot_mmap_typename[];

void *alloc_from_entries(MMap mmap, s64 size, s64 align);

MMap memory__init(BOOTBOOT *bb);

// get physical address from kernel address
u64 physical_address(void *ptr);

// get kernel address from physical address
void *kernel_address(u64 address);

// Read the value of cr3
PageTable4 *read_page_table(void);

// map a single page of memory
void map_physical_page(PageTable4 *p4, u64 virtual_begin, u64 physical_begin);
