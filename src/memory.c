#include "memory.h"

#define ENTRY_ALLOC_FAILURE ((void *)~0ULL)

/// Specifies whether the mapped frame or page table is loaded in memory.
#define PTE_PRESENT ((uint64_t)(1))
/// Controls whether writes to the mapped frames are allowed.
///
/// If this bit is unset in a level 1 page table entry, the mapped frame is
/// read-only. If this bit is unset in a higher level page table entry the
/// complete range of mapped pages is read-only.
#define PTE_WRITABLE ((uint64_t)(1 << 1))
/// Controls whether accesses from userspace (i.e. ring 3) are permitted.
#define PTE_USER_ACCESSIBLE ((uint64_t)(1 << 2))
/// If this bit is set, a “write-through” policy is used for the cache, else a
/// “write-back” policy is used.
#define PTE_WRITE_THROUGH ((uint64_t)(1 << 3))
/// Disables caching for the pointed entry is cacheable.
#define PTE_NO_CACHE ((uint64_t)(1 << 4))
/// Set by the CPU when the mapped frame or page table is accessed.
#define PTE_ACCESSED ((uint64_t)(1 << 5))
/// Set by the CPU on a write to the mapped frame.
#define PTE_DIRTY ((uint64_t)(1 << 6))
/// Specifies that the entry maps a huge frame instead of a page table. Only
/// allowed in P2 or P3 tables.
#define PTE_HUGE_PAGE ((uint64_t)(1 << 7))
/// Indicates that the mapping is present in all address spaces, so it isn't
/// flushed from the TLB on an address space switch.
#define PTE_GLOBAL ((uint64_t)(1 << 8))
/// Available to the OS, can be used to store additional data, e.g. custom
/// flags.
#define PTE_BIT_9 ((uint64_t)(1 << 9))
/// Available to the OS, can be used to store additional data, e.g. custom
/// flags.
#define PTE_BIT_10 ((uint64_t)(1 << 10))
/// Available to the OS, can be used to store additional data, e.g. custom
/// flags.
#define PTE_BIT_11 ((uint64_t)(1 << 11))
/// Available to the OS, can be used to store additional data, e.g. custom
/// flags.
#define PTE_BIT_52 ((uint64_t)(1 << 52))
/// Available to the OS, can be used to store additional data, e.g. custom
/// flags.
#define PTE_BIT_53 ((uint64_t)(1 << 53))
/// Available to the OS, can be used to store additional data, e.g. custom
/// flags.
#define PTE_BIT_54 ((uint64_t)(1 << 54))
/// Available to the OS, can be used to store additional data, e.g. custom
/// flags.
#define PTE_BIT_55 ((uint64_t)(1 << 55))
/// Available to the OS, can be used to store additional data, e.g. custom
/// flags.
#define PTE_BIT_56 ((uint64_t)(1 << 56))
/// Available to the OS, can be used to store additional data, e.g. custom
/// flags.
#define PTE_BIT_57 ((uint64_t)(1 << 57))
/// Available to the OS, can be used to store additional data, e.g. custom
/// flags.
#define PTE_BIT_58 ((uint64_t)(1 << 58))
/// Available to the OS, can be used to store additional data, e.g. custom
/// flags.
#define PTE_BIT_59 ((uint64_t)(1 << 59))
/// Available to the OS, can be used to store additional data, e.g. custom
/// flags.
#define PTE_BIT_60 ((uint64_t)(1 << 60))
/// Available to the OS, can be used to store additional data, e.g. custom
/// flags.
#define PTE_BIT_61 ((uint64_t)(1 << 61))
/// Available to the OS, can be used to store additional data, e.g. custom
/// flags.
#define PTE_BIT_62 ((uint64_t)(1 << 62))
/// Forbid code execution from the mapped frames.
///
/// Can be only used when the no-execute page protection feature is enabled in
/// the EFER register.
#define PTE_NO_EXECUTE ((uint64_t)(1 << 63))

typedef struct memory__FreeBlock {
  struct memory__FreeBlock *next;
  struct memory__FreeBlock *prev;
  int64_t size_class;
} FreeBlock;

typedef struct {
  FreeBlock *freelist;
  BitSet buddies;
} SizeClassInfo;

#define SIZE_CLASS_COUNT 12
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

static void *alloc_from_entries(MMapEnt *entries, int64_t entry_count,
                                int64_t _size) {
  if (_size <= 0)
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

char *memory__bootboot_mmap_typename[] = {"Used", "Free", "ACPI", "MMIO"};

int64_t memory__init(MMapEnt *entries, int64_t entry_count) {
  assert(GLOBAL == NULL);
  for (int64_t i = 0; i < entry_count; i++) {
    char *typename = memory__bootboot_mmap_typename[MMapEnt_Type(&entries[i])];

    log_fmt("entry: % entry with size %", typename,
            MMapEnt_Size(&entries[i]) / _4KB);
  }

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

  // Safety bytes
  GLOBAL = alloc_from_entries(entries, entry_count, sizeof(*GLOBAL));
  if (GLOBAL == NULL) {
    memset(GLOBAL, 42, sizeof(*GLOBAL));
    GLOBAL = alloc_from_entries(entries, entry_count, sizeof(*GLOBAL));
  }
  assert(GLOBAL != NULL);
  memset(GLOBAL, 0, sizeof(*GLOBAL));

  // Build basic buddy system structure
  MMapEnt *last_entry = &entries[entry_count - 1];
  uint64_t max_address = last_entry->ptr + last_entry->size;
  max_address = align_up(max_address, _4KB << SIZE_CLASS_COUNT);
  int64_t max_page_idx = address_to_page(max_address);
  uint64_t *data = alloc_from_entries(entries, entry_count, max_page_idx);
  assert(data != ENTRY_ALLOC_FAILURE);

  GLOBAL->usable_pages = BitSet__new(data, max_page_idx);

  for (int64_t i = 0; i < SIZE_CLASS_COUNT - 1; i++) {
    int64_t num_buddy_pairs = page_to_buddy(max_page_idx, i);
    uint64_t *data = alloc_from_entries(entries, entry_count, num_buddy_pairs);
    assert(data != ENTRY_ALLOC_FAILURE);

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

  log_fmt("After allocating page information");
  int64_t available_memory = 0;
  for (int64_t i = 0; i < entry_count; i++) {
    log_fmt("%k bytes at %", entries[i].size / 1024, entries[i].ptr);
    available_memory += (int64_t)entries[i].size;
  }

  for (int64_t i = 0; i < entry_count; i++)
    free((void *)entries[i].ptr, entries[i].size / _4KB);

  assert(available_memory == GLOBAL->free_memory);
  GLOBAL->heap_size = available_memory;
  memory__validate_heap();

  return start;
}

void memory__validate_heap(void) {

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
