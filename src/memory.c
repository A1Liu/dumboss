#include "memory.h"
#include "basics.h"
#include "logging.h"
#include <stddef.h>

// Used Phil Opperman's x86_64 rust code to make these macros
// https://github.com/rust-osdev/x86_64/blob/master/src/structures/paging/page_table.rs

#define PTE_PRESENT         U64_1
#define PTE_WRITABLE        (U64_1 << 1)
#define PTE_USER_ACCESSIBLE (U64_1 << 2)
#define PTE_WRITE_THROUGH   (U64_1 << 3)
#define PTE_NO_CACHE        (U64_1 << 4)
#define PTE_ACCESSED        (U64_1 << 5)
#define PTE_DIRTY           (U64_1 << 6)
#define PTE_HUGE_PAGE       (U64_1 << 7)
#define PTE_GLOBAL          (U64_1 << 8)
#define PTE_BIT_9           (U64_1 << 9)
#define PTE_BIT_10          (U64_1 << 10)
#define PTE_BIT_11          (U64_1 << 11)
#define PTE_BIT_52          (U64_1 << 52)
#define PTE_BIT_53          (U64_1 << 53)
#define PTE_BIT_54          (U64_1 << 54)
#define PTE_BIT_55          (U64_1 << 55)
#define PTE_BIT_56          (U64_1 << 56)
#define PTE_BIT_57          (U64_1 << 57)
#define PTE_BIT_58          (U64_1 << 58)
#define PTE_BIT_59          (U64_1 << 59)
#define PTE_BIT_60          (U64_1 << 60)
#define PTE_BIT_61          (U64_1 << 61)
#define PTE_BIT_62          (U64_1 << 62)
#define PTE_NO_EXECUTE      (U64_1 << 63)
#define PTE_ADDRESS         0x000ffffffffff000ull

const char *const memory__bootboot_mmap_typename[] = {"Used", "Free", "ACPI", "MMIO"};

#define PageTable__ENTRY_COUNT 512
typedef struct {
  volatile uint64_t entries[PageTable__ENTRY_COUNT];
} PageTable;

_Static_assert(sizeof(PageTable) == _4KB, "PageTables should be 4KB");

typedef struct {
  union {
    struct {
      uint16_t p0;
      uint16_t p1;
      uint16_t p2;
      uint16_t p3;
      uint16_t p4;
    };
    uint16_t indices[5];
  };
} PageTableIndices;

static PageTableIndices page_table_indices(uint64_t address) {
  uint64_t p1 = address >> 12;
  uint64_t p2 = p1 >> 9;
  uint64_t p3 = p2 >> 9;
  uint64_t p4 = p3 >> 9;

  return (PageTableIndices){
      .p0 = (uint16_t)(address % _4KB),
      .p1 = (uint16_t)(p1 % PageTable__ENTRY_COUNT),
      .p2 = (uint16_t)(p2 % PageTable__ENTRY_COUNT),
      .p3 = (uint16_t)(p3 % PageTable__ENTRY_COUNT),
      .p4 = (uint16_t)(p4 % PageTable__ENTRY_COUNT),
  };
}

static uint64_t indices_to_address(PageTableIndices indices) {
  uint64_t address = indices.p4;
  address = (address << 9) | indices.p3;
  address = (address << 9) | indices.p2;
  address = (address << 9) | indices.p1;
  address = address << 12;

  // This sign-extends the address for the top 16 bits, as required by x86_64
  // before Intel Ice Lake
  int64_t signed_address_shifted = (int64_t)(address << 16);
  address = (uint64_t)(signed_address_shifted >> 16);

  return address;
}

static void traverse_table(uint64_t table_entry, uint16_t table_level) {
  const static char *const prefixes[] = {"| | | +-", "| | +-", "| +-", "+-", ""};
  const static char *const page_size_for_entry[] = {"", "4Kb", "2Mb", "1Gb", ""};

  if (!table_level | (table_entry & PTE_HUGE_PAGE)) {
    // Probs wont ever happen
    return;
  }

  uint16_t count = 0;
  PageTable *table = (PageTable *)(table_entry & PTE_ADDRESS);
  FOR_PTR(table->entries, PageTable__ENTRY_COUNT) {
    uint64_t entry = *it;
    if (!entry) continue;
    count++;
  }

  log_fmt("%fp%f table with %f children", prefixes[table_level], table_level, count);

  enum Mode { TABLE, EMPTY, PAGE };
  enum Mode mode = 0;
  int64_t type_count = 0;

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
    uint64_t entry = *it;

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

// get physical address from kernel address
uint64_t physical_address(void *ptr) {
  uint64_t address = (uint64_t)ptr;
  assert(address >= MEMORY__KERNEL_SPACE_BEGIN);

  return address - MEMORY__KERNEL_SPACE_BEGIN;
}

// get kernel address from physical address
void *kernel_address(uint64_t address) {
  assert(address < MEMORY__KERNEL_SPACE_BEGIN);

  return (void *)(address + MEMORY__KERNEL_SPACE_BEGIN);
}

void map_region(PageTable *p4, uint64_t virtual_begin, uint64_t physical_begin, uint64_t size) {
  (void)p4;
  (void)virtual_begin;
  (void)physical_begin;
  (void)size;
}

static void *phys_alloc_from_entries(MMap mmap, int64_t _size, int64_t _align);
MMap memory__init(BOOTBOOT *bb) {
  // Calculation described in bootboot specification
  MMap mmap = {.data = &bb->mmap, .count = (bb->size - 128) / 16, .memory_size = 0};
  uint64_t memory_size = 0;
  FOR(mmap) {
    uint64_t ptr = MMapEnt_Ptr(it), size = MMapEnt_Size(it);
    memory_size = max(memory_size, ptr + size);
  }
  mmap.memory_size = memory_size;

  // sort the entries so that the free ones are first
  SLOW_SORT(mmap) {
    uint64_t l_type = MMapEnt_Type(left), r_type = MMapEnt_Type(right);

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
  uint64_t first_ptr = MMapEnt_Ptr(&mmap.data[0]);
  if (first_ptr < _4KB) {
    int64_t safety_size = _4KB - (int64_t)first_ptr;
    void *safety_alloc = phys_alloc_from_entries(mmap, safety_size, 1);
    memset(safety_alloc, 42, safety_size);
  }

  PageTable *table = read_register(cr3, PageTable *, "q");

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

void *alloc_from_entries(MMap mmap, int64_t _size, int64_t _align) {
  uint64_t address = (uint64_t)phys_alloc_from_entries(mmap, _size, _align);
  return (void *)(address + MEMORY__KERNEL_SPACE_BEGIN);
}

static void *phys_alloc_from_entries(MMap mmap, int64_t _size, int64_t _align) {
  if (_size <= 0 || _align < 0) return MMapEnt__ALLOC_FAILURE;

  uint64_t align = max((uint64_t)_align, 1);
  uint64_t size = align_up((uint64_t)_size, align);

  FOR(mmap) {
    uint64_t aligned_ptr = align_up(it->ptr, align);
    uint64_t aligned_size = it->size + it->ptr - aligned_ptr;
    if (aligned_size < size) continue;

    it->ptr = aligned_ptr + size;
    it->size = aligned_size - size;
    return (void *)aligned_ptr;
  }

  return MMapEnt__ALLOC_FAILURE;
}
