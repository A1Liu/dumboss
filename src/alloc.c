#include "alloc.h"

#define ENTRY_ALLOC_FAILURE ((void *)~0ULL)

typedef struct alloc__FreeBlock {
  struct alloc__FreeBlock *next;
  struct alloc__FreeBlock *prev;
} FreeBlock;

typedef struct {
  FreeBlock *freelist;
  BitSet buddies;
} SizeClassInfo;

#define SIZE_CLASS_COUNT 10
static int64_t free_memory = 0;
static FreeBlock *max_size_freelist = NULL;
static SizeClassInfo size_classes[SIZE_CLASS_COUNT];
static BitSet usable_pages;

static inline int64_t address_to_page(uint64_t address) {
  return (int64_t)(address / _4KB);
}

static inline uint64_t page_to_address(int64_t page) {
  return ((uint64_t)page) * _4KB;
}

static inline int64_t buddy_page_for_page(int64_t page_index,
                                          int64_t size_class) {
  return page_index ^ (((int64_t)1) << size_class);
}

static inline int64_t page_to_buddy(int64_t page_index, int64_t size_class) {
  return page_index >> (size_class + 1);
}

static inline uint64_t *alloc_from_entries(MMapEnt *entries,
                                           int64_t entry_count, int64_t _size) {
  if (_size < 0)
    return ENTRY_ALLOC_FAILURE;

  uint64_t size = align_up((uint64_t)_size, 8);
  for (int64_t i = 0; i < entry_count; i++) {
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

  return ENTRY_ALLOC_FAILURE;
}

int64_t alloc__init(MMapEnt *entries, int64_t entry_count) {
  // bubble-sort the entries so that the free ones are last
  for (int64_t right_bound = entry_count - 1; right_bound > 0; right_bound--) {
    for (int64_t i = 0; i < right_bound; i++) {
      if (MMapEnt_IsFree(&entries[i]) && !MMapEnt_IsFree(&entries[i + 1])) {
        MMapEnt e = entries[i + 1];
        entries[i + 1] = entries[i];
        entries[i] = e;
      }
    }
  }

  // Turn entries into an array of only free data
  int64_t start = 0;
  for (; start < entry_count && !MMapEnt_IsFree(&entries[start]); start++)
    ;
  // remove weird bit stuff that BOOTBOOT does
  for (int64_t i = start; i < entry_count; i++)
    entries[i].size = MMapEnt_Size(&entries[i]);

  entry_count -= start;
  entries += start;

  log_fmt("After filtering to only free memory");
  for (int64_t i = 0; i < entry_count; i++) {
    MMapEnt *entry = &entries[i];
    log_fmt("%k bytes at %", entry->size / 1024, entry->ptr);
  }
  log();

  // Build basic buddy system structure
  MMapEnt *last_entry = &entries[entry_count - 1];
  uint64_t max_address = last_entry->ptr + last_entry->size;
  max_address = align_up(max_address, _4KB << SIZE_CLASS_COUNT);
  int64_t max_page_idx = address_to_page(max_address);
  uint64_t *data = alloc_from_entries(entries, entry_count, max_page_idx);
  assert(data != ENTRY_ALLOC_FAILURE);

  usable_pages = BitSet__new(data, max_page_idx);

  for (int64_t i = 0; i < SIZE_CLASS_COUNT; i++) {
    int64_t num_buddy_pairs = page_to_buddy(max_page_idx, i);
    uint64_t *data = alloc_from_entries(entries, entry_count, num_buddy_pairs);
    assert(data != ENTRY_ALLOC_FAILURE);

    size_classes[i].freelist = NULL;
    size_classes[i].buddies = BitSet__new(data, num_buddy_pairs);
    BitSet__set_all(size_classes[i].buddies, false);
  }

  for (int64_t i = 0, previous_end = 0; i < entry_count; i++) {
    MMapEnt *entry = &entries[i];
    uint64_t end_address = align_down(entry->ptr + entry->size, _4KB);
    entry->ptr = align_up(entry->ptr, _4KB);
    int64_t begin = address_to_page(entry->ptr);
    assert(begin >= previous_end);

    if (begin != previous_end)
      BitSet__set_range(usable_pages, previous_end, begin, false);

    int64_t end = address_to_page(end_address);
    entry->size = end_address - entry->ptr;
    BitSet__set_range(usable_pages, begin, end, true);
    previous_end = end;
  }

  log_fmt("After allocating page information");
  int64_t available_memory = 0;
  for (int64_t i = 0; i < entry_count; i++) {
    log_fmt("%k bytes at %", entries[i].size / 1024, entries[i].ptr);
    available_memory += (int64_t)entries[i].size;
  }

  for (int64_t i = 0; i < entry_count; i++)
    free((void *)entries[i].ptr, entries[i].size / _4KB);

  assert(available_memory == free_memory);

  return start;
}

void *alloc(int64_t count) {
  (void)count;
  return NULL;
}

static void free_at_size_class(int64_t page, int64_t size_class);

void free(void *data, int64_t count) {
  assert(data != NULL);
  uint64_t addr = (uint64_t)data;
  assert(addr == align_down(addr, _4KB));

  int64_t begin_page = address_to_page(addr), end_page = begin_page + count;
  // TODO should probably do some math here to not have to iterate over every
  // page in data
  for (int64_t page = begin_page; page < end_page; page++)
    free_at_size_class(page, 0);
}

static void free_at_size_class(int64_t page, int64_t size_class) {
  assert(BitSet__get(usable_pages, page));

  for (int64_t i = size_class; i < SIZE_CLASS_COUNT; i++) {
    assert((page >> i << i) == page);

    int64_t buddy_index = page_to_buddy(page, i);
    SizeClassInfo *info = &size_classes[i];
    int64_t buddy_page = buddy_page_for_page(page, i);
    bool buddy_is_valid = BitSet__get(usable_pages, buddy_page);
    bool buddy_is_free = BitSet__get(info->buddies, buddy_index);
    BitSet__set(info->buddies, buddy_index, !buddy_is_valid || !buddy_is_free);

    if (!buddy_is_valid || !buddy_is_free) {
      FreeBlock *block = (void *)page_to_address(page);
      block->prev = NULL;
      block->next = info->freelist;
      info->freelist = block;
      break;
    }

    FreeBlock *buddy = (void *)page_to_address(buddy_page);
    FreeBlock *prev = buddy->prev;
    FreeBlock *next = buddy->next;
    if (next != NULL)
      next->prev = prev;
    if (prev != NULL) {
      prev->next = next;
    } else {
      assert(info->freelist == buddy);
      info->freelist = next;
    }

    page = min(page, buddy_page);
  }

  (void)max_size_freelist;
  free_memory += (1 << size_class) * _4KB;
}
