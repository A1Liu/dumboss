#include "memory.h"
#include "page_tables.h"
#include <asm.h>
#include <basics.h>
#include <bitset.h>
#include <macros.h>
#include <types.h>

#define SIZE_CLASS_COUNT 12

typedef struct {
  MMapEnt *data;
  s64 count;
  u64 memory_size;
} MMap;

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
const char *const memory__bootboot_mmap_typename[] = {"Used", "Free", "ACPI", "MMIO"};

// get physical address from kernel address
u64 physical_address(void *ptr) {
  u64 address = (u64)ptr;
  assert(address >= MEMORY__KERNEL_SPACE_BEGIN);

  return address - MEMORY__KERNEL_SPACE_BEGIN;
}

// get kernel address from physical address
void *kernel_address(u64 address) {
  assert(address < MEMORY__KERNEL_SPACE_BEGIN);

  return (void *)(address + MEMORY__KERNEL_SPACE_BEGIN);
}

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

static void *alloc_from_entries(MMap mmap, s64 size, s64 align);

void memory__init(BOOTBOOT *bb) {
  // Calculation described in bootboot specification
  MMap mmap = {.data = &bb->mmap, .count = (bb->size - 128) / 16, .memory_size = 0};
  u64 memory_size = 0;
  FOR(mmap) {
    u64 ptr = MMapEnt_Ptr(it), size = MMapEnt_Size(it);
    memory_size = max(memory_size, ptr + size);
  }
  mmap.memory_size = memory_size;

  // sort the entries so that the free ones are first
  SLOW_SORT(mmap) {
    u64 l_type = MMapEnt_Type(left), r_type = MMapEnt_Type(right);

    bool swap = (l_type != MMAP_FREE) & (r_type == MMAP_FREE);
    if (swap) SWAP(left, right);
  }

  // remove weird bit stuff that BOOTBOOT does for the free entries
  FOR(mmap) {
    if (!MMapEnt_IsFree(it)) {
      mmap.count = it_index;
      break;
    }

    it->size = MMapEnt_Size(it);
    log_fmt("entry: free entry size %f", it->size);
  }

  // Hacky solution to quickly get everything into a higher-half kernel
  UNSAFE_HACKY_higher_half_init();

  // Initialize allocator
  GLOBAL = alloc_from_entries(mmap, sizeof(*GLOBAL), 8);
  memset(GLOBAL, 0, sizeof(*GLOBAL));

  // Build basic buddy system structure
  mmap.memory_size = align_up(mmap.memory_size, _4KB << SIZE_CLASS_COUNT);
  s64 max_page_idx = address_to_page(mmap.memory_size);

  u64 *usable_pages_data = alloc_from_entries(mmap, (max_page_idx - 1) / 8 + 1, 8);
  GLOBAL->usable_pages = BitSet__from_raw(usable_pages_data, max_page_idx);
  BitSet__set_all(GLOBAL->usable_pages, false);

  u64 *free_pages_data = alloc_from_entries(mmap, (max_page_idx - 1) / 8 + 1, 8);
  GLOBAL->free_pages = BitSet__from_raw(free_pages_data, max_page_idx);
  BitSet__set_all(GLOBAL->free_pages, false);

  FOR_PTR(GLOBAL->size_classes, SIZE_CLASS_COUNT, info, class) {
    BitSet buddies = BitSet__from_raw(NULL, 0);

    if (class != SIZE_CLASS_COUNT - 1) {
      s64 num_buddy_pairs = page_to_buddy(max_page_idx, class);
      u64 *data = alloc_from_entries(mmap, (num_buddy_pairs - 1) / 8 + 1, 8);
      buddies = BitSet__from_raw(data, num_buddy_pairs);
      BitSet__set_all(buddies, false);
    }

    info->freelist = NULL;
    info->buddies = buddies;
  }

  s64 available_memory = 0;
  FOR(mmap, entry) {
    u64 begin = align_up(entry->ptr, _4KB);
    u64 end = align_down(entry->ptr + entry->size, _4KB);
    s64 begin_page = address_to_page(begin);
    s64 end_page = address_to_page(end);
    u64 size = max(end, begin) - begin;

    log_fmt("%fk bytes at %f", size / 1024, begin);
    available_memory += (s64)size;

    BitSet__set_range(GLOBAL->usable_pages, begin_page, end_page, true);
    free(kernel_address(begin), size / _4KB);

    entry->size = size;
    entry->ptr = begin;
  }

  assert(available_memory == GLOBAL->free_memory);
  GLOBAL->heap_size = available_memory;
  alloc__validate_heap();
  log_fmt("heap validated");

  // Build a new page table using functions that assume higher-half kernel
}

static void *alloc_from_entries(MMap mmap, s64 _size, s64 _align) {
  assert(_size > 0 && _align >= 0);

  u64 align = max((u64)_align, 1);
  u64 size = align_up((u64)_size, align);

  FOR(mmap) {
    u64 aligned_ptr = align_up(it->ptr, align);
    u64 aligned_size = it->size + it->ptr - aligned_ptr;
    if (aligned_size < size) continue;

    it->ptr = aligned_ptr + size;
    it->size = aligned_size - size;
    return (void *)(aligned_ptr + MEMORY__KERNEL_SPACE_BEGIN);
  }

  assert(false);
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

void unsafe_mark_memory_usability(void *data, s64 count, bool usable) {
  assert(data != NULL);
  u64 addr = physical_address(data);
  assert(addr == align_down(addr, _4KB));

  const s64 begin_page = address_to_page(addr), end_page = begin_page + count;

  bool any_are_free = BitSet__get_any(GLOBAL->free_pages, begin_page, end_page);
  assert(!any_are_free, "if you're marking memory usability, the marked pages can't be free");

  BitSet__set_range(GLOBAL->usable_pages, begin_page, end_page, usable);
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
