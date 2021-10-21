#include "alloc.h"
#include "logging.h"
#include <stddef.h>

#define SIZE_CLASS_COUNT 12

typedef struct memory__FreeBlock {
  struct memory__FreeBlock *next;
  struct memory__FreeBlock *prev;
  int64_t size_class;
} FreeBlock;

typedef struct {
  FreeBlock *freelist;
  BitSet buddies;
} SizeClassInfo;

typedef struct {
  int64_t heap_size;
  int64_t free_memory;

  // NOTE: This is used for checking correctness. Maybe its not necessary?
  BitSet usable_pages;

  // NOTE: The smallest size class is 4kb.
  SizeClassInfo size_classes[SIZE_CLASS_COUNT];
} GlobalState;

static GlobalState *GLOBAL;

static inline int64_t address_to_page(uint64_t address) {
  return (int64_t)(address / _4KB);
}

static inline uint64_t page_to_address(int64_t page) {
  return ((uint64_t)page) * _4KB;
}

static inline bool valid_page_for_size_class(int64_t page, int64_t size_class) {
  return (page >> size_class << size_class) == page;
}

static inline int64_t buddy_page_for_page(int64_t page_index,
                                          int64_t size_class) {
  assert(valid_page_for_size_class(page_index, size_class));
  return page_index ^ (((int64_t)1) << size_class);
}

static inline int64_t page_to_buddy(int64_t page_index, int64_t size_class) {
  return page_index >> (size_class + 1);
}

void alloc__init(MMapEnt *entries, int64_t entry_count) {
  GLOBAL = alloc_from_entries(entries, entry_count, sizeof(*GLOBAL), 8);
  memset(GLOBAL, 0, sizeof(*GLOBAL));

  // Build basic buddy system structure
  MMapEnt *last_entry = &entries[entry_count - 1];
  uint64_t max_address = last_entry->ptr + last_entry->size;
  max_address = align_up(max_address, _4KB << SIZE_CLASS_COUNT);
  int64_t max_page_idx = address_to_page(max_address);
  uint64_t *data = alloc_from_entries(entries, entry_count, max_page_idx, 8);
  assert(data != MMapEnt_ALLOC_FAILURE);

  GLOBAL->usable_pages = BitSet__new(data, max_page_idx);

  for (int64_t i = 0; i < SIZE_CLASS_COUNT - 1; i++) {
    int64_t num_buddy_pairs = page_to_buddy(max_page_idx, i);
    uint64_t *data =
        alloc_from_entries(entries, entry_count, num_buddy_pairs, 8);
    assert(data != MMapEnt_ALLOC_FAILURE);

    GLOBAL->size_classes[i].freelist = NULL;
    GLOBAL->size_classes[i].buddies = BitSet__new(data, num_buddy_pairs);
    BitSet__set_all(GLOBAL->size_classes[i].buddies, false);
  }
  GLOBAL->size_classes[SIZE_CLASS_COUNT - 1].freelist = NULL;
  GLOBAL->size_classes[SIZE_CLASS_COUNT - 1].buddies = BitSet__new(NULL, 0);

  for (int64_t i = 0, previous_end = 0; i < entry_count; i++) {
    MMapEnt *entry = &entries[i];
    uint64_t end_address = align_down(entry->ptr + entry->size, _4KB);
    entry->ptr = align_up(entry->ptr, _4KB);
    int64_t begin = address_to_page(entry->ptr);
    assert(begin >= previous_end);

    if (begin != previous_end)
      BitSet__set_range(GLOBAL->usable_pages, previous_end, begin, false);

    int64_t end = address_to_page(end_address);
    entry->size = end_address - entry->ptr;
    BitSet__set_range(GLOBAL->usable_pages, begin, end, true);
    previous_end = end;
  }

  log_fmt("After setting up allocator");
  int64_t available_memory = 0;
  for (int64_t i = 0; i < entry_count; i++) {
    log_fmt("%k bytes at %", entries[i].size / 1024, entries[i].ptr);
    available_memory += (int64_t)entries[i].size;
  }

  for (int64_t i = 0; i < entry_count; i++)
    free((void *)entries[i].ptr, entries[i].size / _4KB);

  assert(available_memory == GLOBAL->free_memory);
  GLOBAL->heap_size = available_memory;
  alloc__validate_heap();
}

static void *pop_freelist(int64_t size_class) {
  assert(size_class < SIZE_CLASS_COUNT);

  SizeClassInfo *info = &GLOBAL->size_classes[size_class];
  FreeBlock *block = info->freelist;

  assert(block != NULL);
  assert(block->size_class == size_class);
  assert(block->prev == NULL);

  info->freelist = block->next;
  if (info->freelist != NULL)
    info->freelist->prev = NULL;
  return block;
}

static inline void remove_from_freelist(int64_t page, int64_t size_class) {
  assert(size_class < SIZE_CLASS_COUNT);
  assert(valid_page_for_size_class(page, size_class));

  FreeBlock *block = (void *)page_to_address(page);
  assert(block->size_class == size_class);

  SizeClassInfo *info = &GLOBAL->size_classes[size_class];
  FreeBlock *prev = block->prev, *next = block->next;
  if (next != NULL)
    next->prev = prev;
  if (prev != NULL) {
    prev->next = next;
  } else {
    if (info->freelist != block) {
      log_fmt("in size class %: freelist=% and block=%", size_class,
              (uint64_t)info->freelist, (uint64_t)block);
      int64_t counter = 0;
      for (FreeBlock *i = info->freelist; i != NULL; i = i->next, counter++) {
        log_fmt("%: block=%", counter, (uint64_t)i);
      }

      assert(false);
    }
    info->freelist = next;
  }
}

static inline void add_to_freelist(int64_t page, int64_t size_class) {
  assert(size_class < SIZE_CLASS_COUNT);
  assert(valid_page_for_size_class(page, size_class));

  FreeBlock *block = (void *)page_to_address(page);
  SizeClassInfo *info = &GLOBAL->size_classes[size_class];
  block->size_class = size_class;
  block->prev = NULL;
  block->next = info->freelist;
  if (block->next != NULL)
    block->next->prev = block;
  info->freelist = block;
}

void alloc__validate_heap(void) {
  int64_t calculated_free_memory = 0;
  for (int64_t i = 0; i < SIZE_CLASS_COUNT; i++) {
    int64_t size = (int64_t)((1 << i) * _4KB);
    FreeBlock *block = GLOBAL->size_classes[i].freelist;
    for (; block != NULL; block = block->next) {
      assert(block->size_class == i);
      calculated_free_memory += size;
    }
  }

  assert(calculated_free_memory == GLOBAL->free_memory);
}

void *alloc(int64_t count) {
  if (count <= 0)
    return NULL;

  int64_t size_class = smallest_greater_power2(count);
  for (; size_class < SIZE_CLASS_COUNT; size_class++)
    if (GLOBAL->size_classes[size_class].freelist != NULL)
      break;

  if (size_class >= SIZE_CLASS_COUNT) // tried to allocate too much data
    return NULL;

  GLOBAL->free_memory -= count * (int64_t)_4KB;

  void *const data = pop_freelist(size_class);
  int64_t page = address_to_page((uint64_t)data);
  assert(BitSet__get(GLOBAL->usable_pages, page));

  if (size_class != SIZE_CLASS_COUNT - 1) {
    int64_t buddy_index = page_to_buddy(page, size_class);
    assert(BitSet__get(GLOBAL->size_classes[size_class].buddies, buddy_index));
    BitSet__set(GLOBAL->size_classes[size_class].buddies, buddy_index, false);
  }

  if (size_class == 0)
    return data;

  for (int64_t i = size_class - 1; i >= 0; i--) {
    if ((1 << (i + 1)) == count)
      return data;

    SizeClassInfo *info = &GLOBAL->size_classes[i];
    int64_t size = 1 << i, buddy_index = page_to_buddy(page, i);
    assert(!BitSet__get(info->buddies, buddy_index));

    if (count <= size) {
      add_to_freelist(page + size, i);
      BitSet__set(info->buddies, buddy_index, true);
    } else {
      count -= size;
      page += size;
    }
  }

  // TODO this is probably unreachable
  return data;
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
  assert(BitSet__get(GLOBAL->usable_pages, page));
  assert(valid_page_for_size_class(page, size_class));
  GLOBAL->free_memory += (1 << size_class) * _4KB;

  for (int64_t i = size_class; i < SIZE_CLASS_COUNT - 1; i++) {
    assert(valid_page_for_size_class(page, i));

    SizeClassInfo *info = &GLOBAL->size_classes[i];
    const int64_t buddy_index = page_to_buddy(page, i);
    const int64_t buddy_page = buddy_page_for_page(page, i);
    const bool buddy_is_free = BitSet__get(info->buddies, buddy_index);
    BitSet__set(info->buddies, buddy_index, !buddy_is_free);

    if (!buddy_is_free)
      return add_to_freelist(page, i);

    remove_from_freelist(buddy_page, i);
    page = min(page, buddy_page);
  }

  assert(valid_page_for_size_class(page, SIZE_CLASS_COUNT - 1));
  add_to_freelist(page, SIZE_CLASS_COUNT - 1);
}
