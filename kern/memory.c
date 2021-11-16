#include "memory.h"
#include "page_tables.h"
#include <asm.h>
#include <basics.h>
#include <bitset.h>
#include <macros.h>
#include <types.h>

#define SIZE_CLASS_COUNT 12
extern const u8 code_begin;
extern const u8 code_end;
extern const u8 bss_end;

typedef struct {
  MMapEnt *data;
  s64 count;
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

static struct {
  s64 heap_size;
  s64 free_memory;

  // NOTE: This is used for checking correctness. Maybe its not necessary?
  BitSet usable_pages;
  BitSet free_pages;

  // NOTE: The smallest size class is 4kb.
  SizeClassInfo size_classes[SIZE_CLASS_COUNT];
} MemGlobals;

// get physical address from kernel address
u64 physical_address(void *ptr) {
  u64 address = (u64)ptr;
  assert(address >= MEMORY__KERNEL_SPACE_BEGIN);

  return address - MEMORY__KERNEL_SPACE_BEGIN;
}

// get kernel address from physical address
u64 kernel_address(u64 address) {
  assert(address < MEMORY__KERNEL_SPACE_BEGIN);

  return address + MEMORY__KERNEL_SPACE_BEGIN;
}

void *kernel_ptr(u64 address) {
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

const char *const memory__bootboot_mmap_typename[] = {"Used", "Free", "ACPI", "MMIO"};
const char *const sizename[] = {"", " Kb", " Mb", " Gb"};
void memory__init(BOOTBOOT *bb) {
  // Calculation described in bootboot specification
  MMap mmap = {.data = &bb->mmap, .count = (bb->size - 128) / 16};
  u64 mem_upper_bound = 0;
  FOR(mmap) {
    u64 ptr = MMapEnt_Ptr(it), size = MMapEnt_Size(it);
    mem_upper_bound = max(mem_upper_bound, ptr + size);
    const char *const mtype = memory__bootboot_mmap_typename[MMapEnt_Type(it)];

    u8 size_class = 0;
    REPEAT(3) {
      if (size < 1024) break;
      size_class++;
      size /= 1024;
    }

    log_fmt("entry: %f entry size %f%f", mtype, size, sizename[size_class]);
  }

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
  }

  // Hacky solution to quickly get everything into a higher-half kernel
  UNSAFE_HACKY_higher_half_init();

  log_fmt("higher-half addressing INIT COMPLETE");

  // Build basic buddy system structure
  const u64 buddy_max = align_up(mem_upper_bound, _4KB << SIZE_CLASS_COUNT);
  s64 max_page_idx = address_to_page(buddy_max);

  u64 *usable_pages_data = alloc_from_entries(mmap, (max_page_idx - 1) / 8 + 1, 8);
  MemGlobals.usable_pages = BitSet__from_raw(usable_pages_data, max_page_idx);
  BitSet__set_all(MemGlobals.usable_pages, false);

  u64 *free_pages_data = alloc_from_entries(mmap, (max_page_idx - 1) / 8 + 1, 8);
  MemGlobals.free_pages = BitSet__from_raw(free_pages_data, max_page_idx);
  BitSet__set_all(MemGlobals.free_pages, false);

  FOR_PTR(MemGlobals.size_classes, SIZE_CLASS_COUNT - 1, info, class) {
    s64 num_buddy_pairs = page_to_buddy(max_page_idx, class);
    u64 *data = alloc_from_entries(mmap, (num_buddy_pairs - 1) / 8 + 1, 8);
    info->buddies = BitSet__from_raw(data, num_buddy_pairs);
    BitSet__set_all(info->buddies, false);
    info->freelist = NULL;
  }

  MemGlobals.size_classes[SIZE_CLASS_COUNT - 1].freelist = NULL;
  MemGlobals.size_classes[SIZE_CLASS_COUNT - 1].buddies = BitSet__from_raw(NULL, 0);

  MemGlobals.free_memory = 0;
  s64 available_memory = 0;
  FOR(mmap, entry) {
    u64 begin = align_up(entry->ptr, _4KB);
    u64 end = align_down(entry->ptr + entry->size, _4KB);
    s64 begin_page = address_to_page(begin);
    s64 end_page = address_to_page(end);
    s64 size = S64(max(end, begin) - begin);

    available_memory += size;

    BitSet__set_range(MemGlobals.usable_pages, begin_page, end_page, true);
    free(kernel_ptr(begin), size / _4KB);
  }

  assert(available_memory == MemGlobals.free_memory);
  MemGlobals.heap_size = available_memory;
  alloc__validate_heap();
  log_fmt("global allocator INIT_COMPLETE");

  // Build a new page table using functions that assume higher-half kernel
  PageTable4 *old = get_page_table();
  PageTable4 *new = alloc(1);
  assert(new);

  // Map higher half code
  void *result = map_region(new, MEMORY__KERNEL_SPACE_BEGIN, (void *)MEMORY__KERNEL_SPACE_BEGIN,
                            PTE_KERNEL, (s64)mem_upper_bound);
  assert(result);

  // Map kernel code to address listed in the linker script
  const u8 *code_ptr = &code_begin, *code_end_ptr = &code_end, *bss_end_ptr = &bss_end;
  const s64 code_size = S64(code_end_ptr - code_ptr), bss_size = S64(bss_end_ptr - code_end_ptr);

  Buffer kern_text = alloc_copy(code_ptr, code_size);
  assert(kern_text.data);
  Buffer bss = alloc_copy(code_end_ptr, bss_size);
  assert(bss.data);

  result = map_region(new, (u64)code_ptr, kern_text.data, PTE_KERNEL_EXE, kern_text.count);
  assert(result);
  result = map_region(new, (u64)code_end_ptr, bss.data, PTE_KERNEL_EXE, bss.count);
  assert(result);

  // Map Bootboot struct, as described in linker script
  void *bb_ptr = translate(old, (u64)bb);
  result = map_page(new, (u64)bb, bb_ptr, PTE_KERNEL);
  assert(result);

  // Map Environment data
  void *env_ptr = translate(old, (u64)&environment);
  result = map_page(new, (u64)&environment, env_ptr, PTE_KERNEL);
  assert(result);

  // Map kernel stack
  void *stack_bottom = translate(old, (u64)align_down(&result, _4KB));
  result = map_page(new, 0xFFFFFFFFFFFFF000, stack_bottom, PTE_KERNEL);
  assert(result);

  set_page_table(new);

  log_fmt("memory INIT_COMPLETE");
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

static inline FreeBlock *find_block(FreeBlock *target) {
  FOR_PTR(MemGlobals.size_classes, SIZE_CLASS_COUNT, info, class) {
    FreeBlock *block = info->freelist;
    while (block != NULL) {
      if (block == target) return block;
      block = block->next;
    }
  }

  return NULL;
}

static void *pop_freelist(s64 size_class) {
  assert(size_class < SIZE_CLASS_COUNT);

  SizeClassInfo *info = &MemGlobals.size_classes[size_class];
  FreeBlock *block = info->freelist;

  assert(block != NULL);
  assert(block->size_class == size_class, "block had size_class %f in freelist of class %f",
         block->size_class, size_class);
  assert(block->prev == NULL);

  info->freelist = block->next;
  if (info->freelist != NULL) info->freelist->prev = NULL;

  assert(find_block(block) == NULL);

  return block;
}

static inline void remove_from_freelist(s64 page, s64 size_class) {
  assert(size_class < SIZE_CLASS_COUNT);
  assert(valid_page_for_size_class(page, size_class));

  FreeBlock *block = kernel_ptr(page_to_address(page));
  assert(block->size_class == size_class);
  assert(find_block(block) != NULL);

  SizeClassInfo *info = &MemGlobals.size_classes[size_class];
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

  assert(find_block(block) == NULL);
}

static inline void add_to_freelist(s64 page, s64 size_class) {
  assert(size_class < SIZE_CLASS_COUNT);
  assert(valid_page_for_size_class(page, size_class));

  FreeBlock *block = kernel_ptr(page_to_address(page));
  assert(find_block(block) == NULL);
  SizeClassInfo *info = &MemGlobals.size_classes[size_class];

  block->size_class = size_class;
  block->prev = NULL;
  block->next = info->freelist;

  if (info->freelist != NULL) info->freelist->prev = block;
  info->freelist = block;
}

void alloc__validate_heap(void) {
  bool success = true;
  s64 calculated_free_memory = 0;
  for (s64 i = 0; i < SIZE_CLASS_COUNT; i++) {
    s64 size = (s64)((1 << i) * _4KB);
    FreeBlock *block = MemGlobals.size_classes[i].freelist;
    for (; block != NULL; block = block->next) {
      if (block->size_class != i) {
        log_fmt("block class = %f but was in class %f", block->size_class, i);
        success = false;
      }

      calculated_free_memory += size;
    }
  }

  if (calculated_free_memory != MemGlobals.free_memory) {
    log_fmt("calculated was %f but free_memory was %f", calculated_free_memory,
            MemGlobals.free_memory);
    success = false;
  }

  assert(success);
}

static void *alloc_raw(s64 count);

Buffer alloc_copy(const void *src, s64 size) {
  Buffer buffer;
  buffer.count = align_up(size, _4KB);
  buffer.data = alloc_raw(buffer.count / _4KB);

  memcpy(buffer.data, src, size);

  return buffer;
}

void *alloc(s64 count) {
  void *data = alloc_raw(count);
  ensure(data) return NULL;

  memset(data, 0, count * _4KB);
  return data;
}

static void *alloc_raw(s64 count) {
  if (count <= 0) return NULL;

  s64 size_class = smallest_greater_power2(count);
  for (; size_class < SIZE_CLASS_COUNT; size_class++)
    if (MemGlobals.size_classes[size_class].freelist != NULL) break;

  if (size_class >= SIZE_CLASS_COUNT) // tried to allocate too much data
    return NULL;

  MemGlobals.free_memory -= count * (s64)_4KB;

  void *const data = pop_freelist(size_class);
  const u64 addr = physical_address(data);
  const s64 begin_page = address_to_page(addr), end_page = begin_page + count;
  assert(BitSet__get_all(MemGlobals.usable_pages, begin_page, end_page));
  assert(BitSet__get_all(MemGlobals.free_pages, begin_page, end_page),
         "%f free of %f expected for class %f data=%f",
         BitSet__get_count(MemGlobals.free_pages, begin_page, end_page), end_page - begin_page,
         size_class, (u64)data);
  BitSet__set_range(MemGlobals.free_pages, begin_page, end_page, false);

  s64 buddy_index = page_to_buddy(begin_page, size_class);
  assert(BitSet__get(MemGlobals.size_classes[size_class].buddies, buddy_index));
  BitSet__set(MemGlobals.size_classes[size_class].buddies, buddy_index, false);

  DECLARE_SCOPED(s64 remaining = count, page = begin_page)
  for (s64 i = size_class; remaining > 0 && i > 0; i--) {
    const s64 child_class = i - 1;
    SizeClassInfo *const info = &MemGlobals.size_classes[child_class];
    const s64 child_size = 1 << child_class, buddy_index = page_to_buddy(page, child_class);
    assert(!BitSet__get(info->buddies, buddy_index));

    if (remaining > child_size) {
      remaining -= child_size;
      page += child_size;
      continue;
    }

    add_to_freelist(page + child_size, child_class);
    BitSet__set(info->buddies, buddy_index, true);

    if (remaining == child_size) break;
  }

  return data;
}

static void free_at_size_class(s64 page, s64 size_class);

void free(void *data, s64 count) {
  assert(data != NULL);
  const u64 addr = physical_address(data);
  assert(addr == align_down(addr, _4KB));

  const s64 begin_page = address_to_page(addr), end_page = begin_page + count;
  for (s64 i = begin_page; i < end_page; i++) {
    assert(!BitSet__get(MemGlobals.free_pages, i), "index: %f", i - begin_page);
  }
  assert(BitSet__get_all(MemGlobals.usable_pages, begin_page, end_page));
  assert(!BitSet__get_any(MemGlobals.free_pages, begin_page, end_page));

  // TODO should probably do some math here to not have to iterate over every
  // page in data
  for (s64 page = begin_page; page < end_page; page++)
    free_at_size_class(page, 0);

  MemGlobals.free_memory += count * _4KB;
  BitSet__set_range(MemGlobals.free_pages, begin_page, end_page, true);
}

void unsafe_mark_memory_usability(void *data, s64 count, bool usable) {
  assert(data != NULL);
  const u64 addr = physical_address(data);
  assert(addr == align_down(addr, _4KB));

  const s64 begin_page = address_to_page(addr), end_page = begin_page + count;

  const bool any_are_free = BitSet__get_any(MemGlobals.free_pages, begin_page, end_page);
  assert(!any_are_free, "if you're marking memory usability, the marked pages can't be free");

  const s64 existing_pages = BitSet__get_count(MemGlobals.usable_pages, begin_page, end_page);
  MemGlobals.heap_size -= existing_pages * _4KB;
  if (usable) {
    MemGlobals.heap_size += count * _4KB;
  }

  BitSet__set_range(MemGlobals.usable_pages, begin_page, end_page, usable);
}

static void free_at_size_class(s64 page, s64 size_class) {

  for (s64 i = size_class; i < SIZE_CLASS_COUNT - 1; i++) {
    assert(valid_page_for_size_class(page, i));

    SizeClassInfo *info = &MemGlobals.size_classes[i];
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
