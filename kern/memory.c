#include "memory.h"
#include "alloc.h"
#include <asm.h>
#include <basics.h>
#include <log.h>
#include <macros.h>

// Used Phil Opperman's x86_64 rust code to make these macros
// https://github.com/rust-osdev/x86_64/blob/master/src/structures/paging/page_table.rs

#define PTE_PRESENT         U64(1)
#define PTE_WRITABLE        (U64(1) << 1)
#define PTE_USER_ACCESSIBLE (U64(1) << 2)
#define PTE_WRITE_THROUGH   (U64(1) << 3)
#define PTE_NO_CACHE        (U64(1) << 4)
#define PTE_ACCESSED        (U64(1) << 5)
#define PTE_DIRTY           (U64(1) << 6)
#define PTE_HUGE_PAGE       (U64(1) << 7)
#define PTE_GLOBAL          (U64(1) << 8)
#define PTE_BIT_9           (U64(1) << 9)
#define PTE_BIT_10          (U64(1) << 10)
#define PTE_BIT_11          (U64(1) << 11)
#define PTE_BIT_52          (U64(1) << 52)
#define PTE_BIT_53          (U64(1) << 53)
#define PTE_BIT_54          (U64(1) << 54)
#define PTE_BIT_55          (U64(1) << 55)
#define PTE_BIT_56          (U64(1) << 56)
#define PTE_BIT_57          (U64(1) << 57)
#define PTE_BIT_58          (U64(1) << 58)
#define PTE_BIT_59          (U64(1) << 59)
#define PTE_BIT_60          (U64(1) << 60)
#define PTE_BIT_61          (U64(1) << 61)
#define PTE_BIT_62          (U64(1) << 62)
#define PTE_NO_EXECUTE      (U64(1) << 63)
#define PTE_ADDRESS         U64(0x000ffffffffff000)

const char *const memory__bootboot_mmap_typename[] = {"Used", "Free", "ACPI", "MMIO"};

PageTable4 *read_page_table(void) {
  return read_register(cr3, PageTable4 *, "q");
}

#define PageTable__ENTRY_COUNT 512

typedef struct {
  volatile u64 entries[PageTable__ENTRY_COUNT];
} PageTable;
_Static_assert(sizeof(PageTable) == _4KB, "PageTables should be 4KB");

typedef struct PageTable3 PageTable3;
typedef struct PageTable2 PageTable2;
typedef struct PageTable1 PageTable1;

typedef struct {
  union {
    struct {
      u16 p0;
      u16 p1;
      u16 p2;
      u16 p3;
      u16 p4;
    };
    u16 indices[5];
  };
} PageTableIndices;

static PageTableIndices page_table_indices(u64 address) {
  u64 p1 = address >> 12, p2 = p1 >> 9;
  u64 p3 = p2 >> 9, p4 = p3 >> 9;

  return (PageTableIndices){
      .p0 = (u16)(address % _4KB),
      .p1 = (u16)(p1 % PageTable__ENTRY_COUNT),
      .p2 = (u16)(p2 % PageTable__ENTRY_COUNT),
      .p3 = (u16)(p3 % PageTable__ENTRY_COUNT),
      .p4 = (u16)(p4 % PageTable__ENTRY_COUNT),
  };
}

static u64 indices_to_address(PageTableIndices indices) {
  u64 address = (U64(indices.p4) << 39) | (U64(indices.p3) << 30) | (U64(indices.p2) << 21) |
                (U64(indices.p1) << 12);

  // This sign-extends the address for the top 16 bits, as required by x86_64
  // before Intel Ice Lake
  s64 signed_address_shifted = (s64)(address << 16);
  address = (u64)(signed_address_shifted >> 16);

  return address;
}

void phys_map(PageTable4 *_p4, u64 virtual_begin, u64 physical_begin, s32 page_count) {
  PageTable *p4 = (PageTable *)_p4;

  (void)page_count;
  (void)p4;
  (void)virtual_begin;
  (void)physical_begin;
}

void phys_map3(PageTable3 *_p3, PageTableIndices virtual_indices, u64 physical_begin,
               s32 page_count) {
  (void)_p3;
  (void)virtual_indices;
  (void)physical_begin;
  (void)page_count;
}

static void traverse_table(u64 table_entry, u16 table_level);

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

static void *phys_alloc_from_entries(MMap mmap, s64 _size, s64 _align);
MMap memory__init(BOOTBOOT *bb) {
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

  // First page in physical memory isn't used right now.
  u64 first_ptr = MMapEnt_Ptr(&mmap.data[0]);
  if (first_ptr < _4KB) {
    s64 safety_size = _4KB - (s64)first_ptr;
    void *safety_alloc = phys_alloc_from_entries(mmap, safety_size, 1);
    memset(safety_alloc, 42, safety_size);
  }

  PageTable *table = (PageTable *)read_page_table();

  PageTableIndices indices = page_table_indices(MEMORY__KERNEL_SPACE_BEGIN);
  assert(indices.p3 == 0);
  assert(indices.p2 == 0);
  assert(indices.p1 == 0);
  assert(indices.p0 == 0);
  table->entries[indices.p4] = table->entries[0];
  table->entries[0] = 0;
  write_register(cr3, table);

  return mmap;
}

void *alloc_from_entries(MMap mmap, s64 _size, s64 _align) {
  u64 address = (u64)phys_alloc_from_entries(mmap, _size, _align);
  return (void *)(address + MEMORY__KERNEL_SPACE_BEGIN);
}

static void *phys_alloc_from_entries(MMap mmap, s64 _size, s64 _align) {
  if (_size <= 0 || _align < 0) return MMapEnt__ALLOC_FAILURE;

  u64 align = max((u64)_align, 1);
  u64 size = align_up((u64)_size, align);

  FOR(mmap) {
    u64 aligned_ptr = align_up(it->ptr, align);
    u64 aligned_size = it->size + it->ptr - aligned_ptr;
    if (aligned_size < size) continue;

    it->ptr = aligned_ptr + size;
    it->size = aligned_size - size;
    return (void *)aligned_ptr;
  }

  return MMapEnt__ALLOC_FAILURE;
}

static void traverse_table(u64 table_entry, u16 table_level) {
  const static char *const prefixes[] = {"| | | +-", "| | +-", "| +-", "+-", ""};
  const static char *const page_size_for_entry[] = {"", "4Kb", "2Mb", "1Gb", ""};

  if (!table_level | (table_entry & PTE_HUGE_PAGE)) {
    // Probs wont ever happen
    return;
  }

  u16 count = 0;
  PageTable *table = (PageTable *)(table_entry & PTE_ADDRESS);
  FOR_PTR(table->entries, PageTable__ENTRY_COUNT) {
    u64 entry = *it;
    if (!entry) continue;
    count++;
  }

  log_fmt("%fp%f table with %f children", prefixes[table_level], table_level, count);

  enum Mode { TABLE = 0, EMPTY, PAGE };
  enum Mode mode = TABLE;
  s64 type_count = 0;

#define FINISH_MODE                                                                                \
  switch (mode) {                                                                                  \
  case TABLE:                                                                                      \
    break;                                                                                         \
                                                                                                   \
  case EMPTY:                                                                                      \
    log_fmt("%f%f empty p%f entries", prefixes[table_level - 1], type_count, table_level);         \
    type_count = 0;                                                                                \
    break;                                                                                         \
                                                                                                   \
  case PAGE:                                                                                       \
    log_fmt("%f%f %f page entries", prefixes[table_level - 1], type_count,                         \
            page_size_for_entry[table_level]);                                                     \
    type_count = 0;                                                                                \
    break;                                                                                         \
  }

  FOR_PTR(table->entries, PageTable__ENTRY_COUNT) {
    u64 entry = *it;

    enum Mode new_mode;
    if (!entry) new_mode = EMPTY;
    else if ((table_level == 1) || (entry & PTE_HUGE_PAGE))
      new_mode = PAGE;
    else
      new_mode = TABLE;

    if (mode != new_mode) FINISH_MODE;

    switch (new_mode) {
    case TABLE:
      traverse_table(entry, table_level - 1);
      break;

    case EMPTY:
      type_count++;
      break;

    case PAGE:
      type_count++;
      break;
    }

    mode = new_mode;
  }

  FINISH_MODE;
#undef FINISH_MODE
}
