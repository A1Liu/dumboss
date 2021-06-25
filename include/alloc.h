#pragma once
#include "bootboot.h"
#include "logging.h"
#include <stddef.h>
#include <stdint.h>

#define _4KB 4096LL
#define _2MB 2097152LL
#define _1GB 1073741824LL

typedef struct alloc__PageBlock *PageBlock;

typedef enum {
  PageBlockSize__4kb = 0,
  // PageBlockSize__2mb = 1,
  // PageBlockSize__1gb = 2,
} __attribute__((packed)) PageBlockSize;

static inline void *PageBlock__ptr(PageBlock *block) {
  return (void *)(((uint64_t)block) & 0xFFFFFFFFFFFFFFF0);
}

static inline int64_t PageBlock__size_class(PageBlock *block) {
  switch (((uint64_t)block) & 0xF) {
  case PageBlockSize__4kb:
    return _4KB;
  // case PageBlockSize__2mb:
  //   return _2MB;
  // case PageBlockSize__1gb:
  //   return _1GB;
  default:
    panic();
    return 0;
  }
}

int64_t alloc__init(MMapEnt *entries, int64_t entry_count);

// Allocate `count` contiguous pages, each of size 4kb
PageBlock *alloc_4kb(int64_t count);

// Allocate `count` contiguous pages, each of size 2mb
// PageBlock *alloc_2mb(int64_t count);

void free(PageBlock *block);
