#include "memory.h"
#include "page_tables.h"
#include <asm.h>
#include <basics.h>
#include <bitset.h>
#include <macros.h>
#include <types.h>

#define CLASS_COUNT 12
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
  s64 class;
} FreeBlock;

typedef struct {
  s64 buddy;
  s64 bitset_index;
} BuddyInfo;

typedef struct {
  FreeBlock *freelist;
  BitSet buddies;
} ClassInfo;

static struct {
  s64 free_memory;

  // NOTE: This is used for checking correctness. Maybe its not necessary?
  BitSet usable_pages;
  BitSet free_pages;

  // NOTE: The smallest size class is 4kb.
  ClassInfo classes[CLASS_COUNT];
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

static inline BuddyInfo buddy_info(s64 page, s64 class) {
  assert(is_aligned(page, 1 << class));
  s64 buddy = page ^ (S64(1) << class);
  s64 index = page >> (class + 1);
  return (BuddyInfo){.buddy = buddy, .bitset_index = index};
}

static void *alloc_from_entries(MMap mmap, s64 size, s64 align);

static const char *const mmap_typename[] = {"Used", "Free", "ACPI", "MMIO"};
static const char *const sizename[] = {"", " Kb", " Mb", " Gb"};
void memory__init(BOOTBOOT *bb) {
  // Calculation described in bootboot specification
  MMap mmap = {.data = &bb->mmap, .count = (bb->size - 128) / 16};
  u64 mem_upper_bound = 0;
  FOR(mmap) {
    u64 ptr = MMapEnt_Ptr(it), size = MMapEnt_Size(it);
    mem_upper_bound = max(mem_upper_bound, ptr + size);
    const char *const mtype = mmap_typename[MMapEnt_Type(it)];

    u8 class = 0;
    REPEAT(3) {
      if (size < 1024) break;
      class += 1;
      size /= 1024;
    }

    log_fmt("entry: %f entry size %f%f", mtype, size, sizename[class]);
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
      mmap.count = index;
      break;
    }

    it->size = MMapEnt_Size(it);
  }

  // Hacky solution to quickly get everything into a higher-half kernel
  UNSAFE_HACKY_higher_half_init();

  log_fmt("higher-half addressing INIT COMPLETE");

  // Build basic buddy system structure
  const u64 buddy_max = align_up(mem_upper_bound, _4KB << CLASS_COUNT);
  const s64 max_page_idx = buddy_max / _4KB;

  u64 *usable_pages_data = alloc_from_entries(mmap, (max_page_idx - 1) / 8 + 1, 8);
  MemGlobals.usable_pages = BitSet__from_raw(usable_pages_data, max_page_idx);
  BitSet__set_all(MemGlobals.usable_pages, false);

  u64 *free_pages_data = alloc_from_entries(mmap, (max_page_idx - 1) / 8 + 1, 8);
  MemGlobals.free_pages = BitSet__from_raw(free_pages_data, max_page_idx);
  BitSet__set_all(MemGlobals.free_pages, false);

  FOR_PTR(MemGlobals.classes, CLASS_COUNT - 1, info, class) {
    const s64 bitset_size = align_up(buddy_info(max_page_idx, class).bitset_index, 8);
    u64 *const data = alloc_from_entries(mmap, bitset_size / 8, 8);
    info->buddies = BitSet__from_raw(data, bitset_size);
    BitSet__set_all(info->buddies, false);
    info->freelist = NULL;
  }

  MemGlobals.classes[CLASS_COUNT - 1].freelist = NULL;
  MemGlobals.classes[CLASS_COUNT - 1].buddies = BitSet__from_raw(NULL, 0);

  MemGlobals.free_memory = 0;
  s64 available_memory = 0;
  FOR(mmap, entry) {
    u64 begin = align_up(entry->ptr, _4KB);
    u64 end = align_down(entry->ptr + entry->size, _4KB);
    s64 begin_page = begin / _4KB;
    s64 end_page = end / _4KB;
    s64 size = S64(max(end, begin) - begin);

    available_memory += size;

    BitSet__set_range(MemGlobals.usable_pages, begin_page, end_page, true);
    free(kernel_ptr(begin), size / _4KB);
  }

  assert(available_memory == MemGlobals.free_memory);
  alloc__validate_heap();

  log_fmt("global allocator INIT_COMPLETE");

  // Build a new page table using functions that assume higher-half kernel
  PageTable4 *old = get_page_table();
  PageTable4 *new = zeroed_pages(1);
  assert(new);

  // Map higher half code
  void *target = kernel_ptr(0);
  void *result = map_region(new, (u64)target, target, PTE_KERNEL, (s64)mem_upper_bound);
  assert(result);

  // Map kernel code to address listed in the linker script
  const u8 *code_ptr = &code_begin, *code_end_ptr = &code_end, *bss_end_ptr = &bss_end;
  const s64 code_size = S64(code_end_ptr - code_ptr), bss_size = S64(bss_end_ptr - code_end_ptr);

  {
    void *const kern = raw_pages(code_size / _4KB);
    memcpy(kern, code_ptr, code_size);
    result = map_region(new, (u64)code_ptr, kern, PTE_KERNEL_EXE, code_size);
    assert(result);
  }

  void *const bss = raw_pages(bss_size / _4KB);
  result = map_region(new, (u64)code_end_ptr, bss, PTE_KERNEL, bss_size);
  assert(result);

  // Map Bootboot struct, as described in linker script
  {
    void *bb_ptr = translate(old, (u64)bb);
    result = map_page(new, (u64)bb, bb_ptr, PTE_KERNEL);
    assert(result);
  }

  // Map Environment data
  {
    void *env_ptr = translate(old, (u64)&environment);
    result = map_page(new, (u64)&environment, env_ptr, PTE_KERNEL);
    assert(result);
  }

  // Map bootboot kernel stack
  {
    void *stack_bottom = translate(old, (u64)align_down(&result, _4KB));
    result = map_page(new, 0xFFFFFFFFFFFFF000, stack_bottom, PTE_KERNEL);
    assert(result);
  }

  memcpy(bss, code_end_ptr, bss_size);
  set_page_table(new);
  alloc__validate_heap();

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
  FOR_PTR(MemGlobals.classes, CLASS_COUNT, info, class) {
    FreeBlock *block = info->freelist;
    while (block != NULL) {
      if (block == target) return block;
      block = block->next;
    }
  }

  return NULL;
}

static void *pop_freelist(s64 class) {
  assert(class < CLASS_COUNT);

  ClassInfo *info = &MemGlobals.classes[class];
  FreeBlock *block = info->freelist;

  assert(block != NULL);
  assert(block->class == class, "block had class %f in freelist of class %f", block->class, class);
  assert(block->prev == NULL);

  info->freelist = block->next;
  if (info->freelist != NULL) info->freelist->prev = NULL;

  return block;
}

static inline void remove_from_freelist(s64 page, s64 class) {
  assert(class < CLASS_COUNT);
  assert(is_aligned(page, 1 << class));

  FreeBlock *block = kernel_ptr(U64(page) * _4KB);
  assert(block->class == class);

  ClassInfo *info = &MemGlobals.classes[class];
  FreeBlock *prev = block->prev, *next = block->next;
  if (next != NULL) next->prev = prev;
  if (prev != NULL) {
    prev->next = next;
  } else {
    if (info->freelist != block) {
      log_fmt("in size class %f: freelist=%f and block=%f", class, (u64)info->freelist, (u64)block);
      s64 counter = 0;
      for (FreeBlock *i = info->freelist; i != NULL; i = i->next, counter++) {
        log_fmt("%f: block=%f", counter, (u64)i);
      }

      assert(false);
    }

    info->freelist = next;
  }
}

static inline void add_to_freelist(s64 page, s64 class) {
  assert(class < CLASS_COUNT);
  assert(is_aligned(page, 1 << class));

  FreeBlock *block = kernel_ptr(U64(page) * _4KB);
  ClassInfo *info = &MemGlobals.classes[class];

  block->class = class;
  block->prev = NULL;
  block->next = info->freelist;

  if (info->freelist != NULL) info->freelist->prev = block;
  info->freelist = block;
}

void alloc__validate_heap(void) {
  bool success = true;
  s64 calculated_free_memory = 0;
  for (s64 i = 0; i < CLASS_COUNT; i++) {
    s64 size = (s64)((1 << i) * _4KB);
    FreeBlock *block = MemGlobals.classes[i].freelist;
    for (; block != NULL; block = block->next) {
      if (block->class != i) {
        log_fmt("block class = %f but was in class %f", block->class, i);
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

void *zeroed_pages(s64 count) {
  void *data = raw_pages(count);
  ensure(data) return NULL;

  memset(data, 0, count * _4KB);
  return data;
}

void *raw_pages(s64 count) {
  if (count <= 0) return NULL;

  s64 class = smallest_greater_power2(count);
  for (; class < CLASS_COUNT; class ++)
    if (MemGlobals.classes[class].freelist != NULL) break;

  if (class >= CLASS_COUNT) // tried to allocate too much data
    return NULL;

  MemGlobals.free_memory -= count * (s64)_4KB;

  void *const data = pop_freelist(class);
  const u64 addr = physical_address(data);
  const s64 begin_page = addr / _4KB, end_page = begin_page + count;
  assert(BitSet__get_all(MemGlobals.usable_pages, begin_page, end_page));
  assert(BitSet__get_all(MemGlobals.free_pages, begin_page, end_page),
         "%f free of %f expected for class %f data=%f",
         BitSet__get_count(MemGlobals.free_pages, begin_page, end_page), end_page - begin_page,
         class, (u64)data);
  BitSet__set_range(MemGlobals.free_pages, begin_page, end_page, false);

  if (class != CLASS_COUNT - 1) {
    s64 index = buddy_info(begin_page, class).bitset_index;
    assert(BitSet__get(MemGlobals.classes[class].buddies, index));
    BitSet__set(MemGlobals.classes[class].buddies, index, false);
  }

  DECLARE_SCOPED(s64 remaining = count, page = begin_page)
  for (s64 i = class; remaining > 0 && i > 0; i--) {
    const s64 child_class = i - 1;
    ClassInfo *const info = &MemGlobals.classes[child_class];
    const s64 child_size = 1 << child_class;
    const s64 index = buddy_info(page, child_class).bitset_index;
    assert(!BitSet__get(info->buddies, index));

    if (remaining > child_size) {
      remaining -= child_size;
      page += child_size;
      continue;
    }

    add_to_freelist(page + child_size, child_class);
    BitSet__set(info->buddies, index, true);

    if (remaining == child_size) break;
  }

  return data;
}

void free(void *data, s64 count) {
  assert(data != NULL);
  const u64 addr = physical_address(data);
  assert(addr == align_down(addr, _4KB));

  const s64 begin = addr / _4KB, end = begin + count;
  assert(BitSet__get_all(MemGlobals.usable_pages, begin, end));
  assert(!BitSet__get_any(MemGlobals.free_pages, begin, end));

  RANGE(begin, end, page) {
    // TODO should probably do some math here to not have to iterate over every
    // page in data
    FOR_PTR(MemGlobals.classes, CLASS_COUNT - 1, info, class) {
      assert(is_aligned(page, 1 << class));
      const BuddyInfo buds = buddy_info(page, class);

      const bool buddy_is_free = BitSet__get(info->buddies, buds.bitset_index);
      BitSet__set(info->buddies, buds.bitset_index, !buddy_is_free);

      if (!buddy_is_free) {
        add_to_freelist(page, class);
        continue(page);
      }

      remove_from_freelist(buds.buddy, class);
      page = min(page, buds.buddy);
    }

    assert(is_aligned(page, 1 << (CLASS_COUNT - 1)));
    add_to_freelist(page, CLASS_COUNT - 1);
  }

  MemGlobals.free_memory += count * _4KB;
  BitSet__set_range(MemGlobals.free_pages, begin, end, true);
}

void unsafe_mark_memory_usability(void *data, s64 count, bool usable) {
  assert(data != NULL);
  const u64 addr = physical_address(data);
  assert(addr == align_down(addr, _4KB));

  const s64 begin_page = addr / _4KB, end_page = begin_page + count;

  const bool any_are_free = BitSet__get_any(MemGlobals.free_pages, begin_page, end_page);
  assert(!any_are_free, "if you're marking memory usability, the marked pages can't be free");

  const s64 existing_pages = BitSet__get_count(MemGlobals.usable_pages, begin_page, end_page);
  BitSet__set_range(MemGlobals.usable_pages, begin_page, end_page, usable);
}
