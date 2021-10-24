#pragma once
#include "bootboot.h"
#include <stdint.h>

#define MMapEnt__ALLOC_FAILURE     ((void *)~(uint64_t)0)
#define MEMORY__KERNEL_SPACE_BEGIN ((uint64_t)0xffff800000000000ull)

#define _4KB 4096
// #define _2MB ((uint64_t)2097152)
// #define _1GB ((uint64_t)1073741824)

typedef struct {
  MMapEnt *data;
  int64_t count;
  uint64_t memory_size;
} MMap;

extern const char *const memory__bootboot_mmap_typename[];

void *alloc_from_entries(MMap mmap, int64_t size, int64_t align);

MMap memory__init(BOOTBOOT *bb);

// get physical address from kernel address
uint64_t physical_address(void *ptr);

// get kernel address from physical address
void *kernel_address(uint64_t address);
