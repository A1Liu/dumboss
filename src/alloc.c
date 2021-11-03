#include "alloc.h"
#include "logging.h"

#define SIZE_CLASS_COUNT 12

typedef struct memory__FreeBlock {
  struct memory__FreeBlock *next;
  struct memory__FreeBlock *prev;
  s64 size_class;
} FreeBlock;

typedef struct {
  FreeBlock *freelist;
  BitSet buddies;
} SizeClassInfo;

typedef struct {
  s64 heap_size;
  s64 free_memory;

  // NOTE: This is used for checking correctness. Maybe its not necessary?
  BitSet usable_pages;
  BitSet free_pages;

  // NOTE: The smallest size class is 4kb.
  SizeClassInfo size_classes[SIZE_CLASS_COUNT];
} GlobalState;

static GlobalState *GLOBAL;

static inline s64 address_to_page(u64 address) {
  return (s64)(address / _4KB);
}

static inline u64 page_to_address(s64 page) {
  return ((u64)page) * _4KB;
}

static inline bool valid_page_for_size_class(s64 page, s64 size_class) {
  return (page >> size_class << size_class) == page;
}

static inline s64 buddy_page_for_page(s64 page_index, s64 size_class) {
  assert(valid_page_for_size_class(page_index, size_class));
  return page_index ^ (((s64)1) << size_class);
}

static inline s64 page_to_buddy(s64 page_index, s64 size_class) {
  return page_index >> (size_class + 1);
}

void alloc__init(MMap mmap) {
  GLOBAL = alloc_from_entries(mmap, sizeof(*GLOBAL), 8);
  memset(GLOBAL, 0, sizeof(*GLOBAL));

  // Build basic buddy system structure
  u64 memory_size = align_up(mmap.memory_size, _4KB << SIZE_CLASS_COUNT);
  s64 max_page_idx = address_to_page(memory_size);

  u64 *usable_pages_data = alloc_from_entries(mmap, max_page_idx / 8, 8);
  assert(usable_pages_data != MMapEnt__ALLOC_FAILURE);
  GLOBAL->usable_pages = BitSet__from_raw(usable_pages_data, max_page_idx);

  u64 *free_pages_data = alloc_from_entries(mmap, max_page_idx / 8, 8);
  assert(free_pages_data != MMapEnt__ALLOC_FAILURE);
  GLOBAL->free_pages = BitSet__from_raw(free_pages_data, max_page_idx);
  BitSet__set_all(GLOBAL->free_pages, false);

  for (s64 i = 0; i < SIZE_CLASS_COUNT - 1; i++) {
    s64 num_buddy_pairs = page_to_buddy(max_page_idx, i);
    u64 *data = alloc_from_entries(mmap, num_buddy_pairs / 8, 8);
    assert(data != MMapEnt__ALLOC_FAILURE);
    BitSet buddies = BitSet__from_raw(data, num_buddy_pairs);
    BitSet__set_all(buddies, false);

    GLOBAL->size_classes[i].freelist = NULL;
    GLOBAL->size_classes[i].buddies = buddies;
  }
  GLOBAL->size_classes[SIZE_CLASS_COUNT - 1].freelist = NULL;
  GLOBAL->size_classes[SIZE_CLASS_COUNT - 1].buddies = BitSet__from_raw(NULL, 0);

  for (s64 i = 0, previous_end = 0; i < mmap.count; i++) {
    MMapEnt *entry = &mmap.data[i];
    u64 end_address = align_down(entry->ptr + entry->size, _4KB);
    entry->ptr = align_up(entry->ptr, _4KB);
    s64 begin = address_to_page(entry->ptr);
    assert(begin >= previous_end);

    if (begin != previous_end) BitSet__set_range(GLOBAL->usable_pages, previous_end, begin, false);

    s64 end = address_to_page(end_address);
    entry->size = end_address - entry->ptr;
    BitSet__set_range(GLOBAL->usable_pages, begin, end, true);
    previous_end = end;
  }

  log_fmt("After setting up allocator");
  s64 available_memory = 0;
  for (s64 i = 0; i < mmap.count; i++) {
    log_fmt("%fk bytes at %f", mmap.data[i].size / 1024, mmap.data[i].ptr);
    available_memory += (s64)mmap.data[i].size;
  }
  log_fmt("");

  for (s64 i = 0; i < mmap.count; i++)
    free(kernel_address(mmap.data[i].ptr), mmap.data[i].size / _4KB);

  log_fmt("after adding memory to allocator");

  assert(available_memory == GLOBAL->free_memory);
  GLOBAL->heap_size = available_memory;
  alloc__validate_heap();
  log_fmt("heap validated");
}

static void *pop_freelist(s64 size_class) {
  assert(size_class < SIZE_CLASS_COUNT);

  SizeClassInfo *info = &GLOBAL->size_classes[size_class];
  FreeBlock *block = info->freelist;

  assert(block != NULL);
  assert(block->size_class == size_class);
  assert(block->prev == NULL);

  info->freelist = block->next;
  if (info->freelist != NULL) info->freelist->prev = NULL;
  return block;
}

static inline void remove_from_freelist(s64 page, s64 size_class) {
  assert(size_class < SIZE_CLASS_COUNT);
  assert(valid_page_for_size_class(page, size_class));

  FreeBlock *block = kernel_address(page_to_address(page));
  assert(block->size_class == size_class);

  SizeClassInfo *info = &GLOBAL->size_classes[size_class];
  FreeBlock *prev = block->prev, *next = block->next;
  if (next != NULL) next->prev = prev;
  if (prev != NULL) {
    prev->next = next;
  } else {
    if (info->freelist != block) {
      log_fmt("in size class %f: freelist=%f and block=%f", size_class, (u64)info->freelist,
              (u64)block);
      s64 counter = 0;
      for (FreeBlock *i = info->freelist; i != NULL; i = i->next, counter++) {
        log_fmt("%f: block=%f", counter, (u64)i);
      }

      assert(false);
    }
    info->freelist = next;
  }
}

static inline void add_to_freelist(s64 page, s64 size_class) {
  assert(size_class < SIZE_CLASS_COUNT);
  assert(valid_page_for_size_class(page, size_class));

  FreeBlock *block = kernel_address(page_to_address(page));
  SizeClassInfo *info = &GLOBAL->size_classes[size_class];
  block->size_class = size_class;
  block->prev = NULL;
  block->next = info->freelist;
  if (block->next != NULL) block->next->prev = block;
  info->freelist = block;
}

void alloc__validate_heap(void) {
  s64 calculated_free_memory = 0;
  for (s64 i = 0; i < SIZE_CLASS_COUNT; i++) {
    s64 size = (s64)((1 << i) * _4KB);
    FreeBlock *block = GLOBAL->size_classes[i].freelist;
    for (; block != NULL; block = block->next) {
      assert(block->size_class == i);
      calculated_free_memory += size;
    }
  }

  assert(calculated_free_memory == GLOBAL->free_memory);
}

void *alloc(s64 count) {
  if (count <= 0) return NULL;

  s64 size_class = smallest_greater_power2(count);
  for (; size_class < SIZE_CLASS_COUNT; size_class++)
    if (GLOBAL->size_classes[size_class].freelist != NULL) break;

  if (size_class >= SIZE_CLASS_COUNT) // tried to allocate too much data
    return NULL;

  GLOBAL->free_memory -= count * (s64)_4KB;

  void *const data = pop_freelist(size_class);
  const u64 addr = physical_address(data);
  const s64 begin_page = address_to_page(addr), end_page = begin_page + count;
  assert(BitSet__get_all(GLOBAL->usable_pages, begin_page, end_page));
  assert(BitSet__get_all(GLOBAL->free_pages, begin_page, end_page));
  BitSet__set_range(GLOBAL->free_pages, begin_page, end_page, false);

  if (size_class != SIZE_CLASS_COUNT - 1) {
    s64 buddy_index = page_to_buddy(begin_page, size_class);
    assert(BitSet__get(GLOBAL->size_classes[size_class].buddies, buddy_index));
    BitSet__set(GLOBAL->size_classes[size_class].buddies, buddy_index, false);
  }

  if (size_class == 0) return data;

  for (s64 i = size_class - 1, page = begin_page; i >= 0; i--) {
    if ((1 << (i + 1)) == count) return data;

    SizeClassInfo *info = &GLOBAL->size_classes[i];
    s64 size = 1 << i, buddy_index = page_to_buddy(page, i);
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

void unsafe_mark_memory_usability(void *data, s64 count, bool usable) {
  assert(data != NULL);
  u64 addr = physical_address(data);
  assert(addr == align_down(addr, _4KB));

  const s64 begin_page = address_to_page(addr), end_page = begin_page + count;
  BitSet__set_range(GLOBAL->usable_pages, begin_page, end_page, usable);
}

static void free_at_size_class(s64 page, s64 size_class);

void free(void *data, s64 count) {
  assert(data != NULL);
  const u64 addr = physical_address(data);
  assert(addr == align_down(addr, _4KB));

  const s64 begin_page = address_to_page(addr), end_page = begin_page + count;
  for (s64 i = begin_page; i < end_page; i++) {
    assert(!BitSet__get(GLOBAL->free_pages, i), "index: %f", i - begin_page);
  }
  assert(BitSet__get_all(GLOBAL->usable_pages, begin_page, end_page));
  assert(!BitSet__get_any(GLOBAL->free_pages, begin_page, end_page));

  // TODO should probably do some math here to not have to iterate over every
  // page in data
  for (s64 page = begin_page; page < end_page; page++)
    free_at_size_class(page, 0);

  BitSet__set_range(GLOBAL->free_pages, begin_page, end_page, true);
}

static void free_at_size_class(s64 page, s64 size_class) {
  assert(valid_page_for_size_class(page, size_class));
  GLOBAL->free_memory += (1 << size_class) * _4KB;

  for (s64 i = size_class; i < SIZE_CLASS_COUNT - 1; i++) {
    assert(valid_page_for_size_class(page, i));

    SizeClassInfo *info = &GLOBAL->size_classes[i];
    const s64 buddy_index = page_to_buddy(page, i);
    const s64 buddy_page = buddy_page_for_page(page, i);
    const bool buddy_is_free = BitSet__get(info->buddies, buddy_index);
    BitSet__set(info->buddies, buddy_index, !buddy_is_free);

    if (!buddy_is_free) return add_to_freelist(page, i);

    remove_from_freelist(buddy_page, i);
    page = min(page, buddy_page);
  }

  assert(valid_page_for_size_class(page, SIZE_CLASS_COUNT - 1));
  add_to_freelist(page, SIZE_CLASS_COUNT - 1);
}
