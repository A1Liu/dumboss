#include "alloc.h"

typedef struct {
  uint8_t data[_4KB];
} Page4kb;

typedef struct alloc__FreeBlock {
  struct alloc__FreeBlock *next;
} FreeBlock;

typedef struct {
  FreeBlock *first;
  BitSet buddies;
} SizeClassInfo;

#define SIZE_CLASS_COUNT 10
static SizeClassInfo info[SIZE_CLASS_COUNT];
static BitSet usable_pages;

static inline uint64_t align_up(uint64_t value, uint64_t alignment) {
  int bits = 64 - __builtin_clzl(alignment - 1);
  return (((value - 1) >> bits) + 1) << bits;
}

// static inline uint64_t align_down(uint64_t value, uint64_t alignment) {
//   int bits = 64 - __builtin_clzl(alignment - 1);
//   return value >> bits << bits;
// }

static inline uint64_t *alloc_from_entries(MMapEnt *entries,
                                           int64_t entry_count, int64_t _size) {
  if (_size < 0)
    return NULL;

  uint64_t size = align_up((uint64_t)_size, 8);
  for (int i = 0; i < entry_count; i++) {
    MMapEnt *cur = &entries[i];

    uint64_t aligned = align_up(cur->ptr, 8);
    cur->size -= aligned - cur->ptr;
    cur->ptr = aligned;

    if (cur->size < size)
      continue;

    uint64_t ptr = cur->ptr;
    cur->ptr += size;
    cur->size -= size;
    return (void *)ptr;
  }

  return NULL;
}

int64_t alloc__init(MMapEnt *entries, int64_t entry_count) {
  // bubble-sort the entries so that the free ones are last
  for (int64_t right_bound = entry_count - 1; right_bound > 0; right_bound--) {
    for (int i = 0; i < right_bound; i++) {
      if (MMapEnt_IsFree(&entries[i]) && !MMapEnt_IsFree(&entries[i + 1])) {
        MMapEnt e = entries[i + 1];
        entries[i + 1] = entries[i];
        entries[i] = e;
      }
    }
  }

  // Turn entries into an array of only free data
  int start = 0;
  for (; start < entry_count && !MMapEnt_IsFree(&entries[start]); start++)
    ;
  // First page shouldnt be used for data
  for (; start < entry_count; start++) {
    MMapEnt *cur = &entries[start];
    cur->size = MMapEnt_Size(cur); // remove weird bit stuff that BOOTBOOT does

    if (cur->ptr >= _4KB)
      break;

    const uint64_t end = cur->ptr + cur->size;
    if (end > _4KB) {
      cur->ptr = _4KB;
      cur->size = end - _4KB;
      break;
    }
  }
  // remove weird bit stuff that BOOTBOOT does
  for (int i = start + 1; i < entry_count; i++)
    entries[i].size = MMapEnt_Size(&entries[i]);

  entry_count -= start;
  entries += start;

  log_fmt("After filtering to only free memory");
  for (int i = 0; i < entry_count; i++) {
    MMapEnt *entry = &entries[i];
    log_fmt("%k bytes at %", entry->size / 1024, entry->ptr);
  }
  log();

  // Build basic buddy system structure
  MMapEnt *last_entry = &entries[entry_count - 1];
  uint64_t max_address = last_entry->ptr + last_entry->size;
  int64_t max_page_idx = (int64_t)(max_address / _4KB);
  uint64_t *data = alloc_from_entries(entries, entry_count, max_page_idx);
  if (data == NULL) // how do we handle this case?
    panic();
  usable_pages = BitSet__new(data, max_page_idx);

  for (int64_t i = 0; i < SIZE_CLASS_COUNT; i++) {
    int64_t num_buddy_pairs = max_page_idx >> (i + 1);
    uint64_t *data = alloc_from_entries(entries, entry_count, num_buddy_pairs);
    if (data == NULL) // how do we handle this case?
      panic();
    info[i].first = NULL;
    info[i].buddies = BitSet__new(data, num_buddy_pairs);
    BitSet__set_all(info[i].buddies, false);
  }

  log_fmt("After allocating page information");
  for (int i = 0; i < entry_count; i++) {
    MMapEnt *entry = &entries[i];
    log_fmt("%k bytes at %", entry->size / 1024, entry->ptr);
  }

  return start;
}

PageBlock *alloc_4kb(int64_t count) {
  (void)count;
  return NULL;
}

PageBlock *alloc_2mb(int64_t count) {
  (void)count;
  return NULL;
}

void free(PageBlock *block) { (void)block; }
