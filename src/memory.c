#include "memory.h"
#include "basics.h"
#include "logging.h"
#include <stddef.h>

#define U64_1 ((uint64_t)1)

/// Specifies whether the mapped frame or page table is loaded in memory.
#define PTE_PRESENT U64_1
/// Controls whether writes to the mapped frames are allowed.
///
/// If this bit is unset in a level 1 page table entry, the mapped frame is
/// read-only. If this bit is unset in a higher level page table entry the
/// complete range of mapped pages is read-only.
#define PTE_WRITABLE (U64_1 << 1)
/// Controls whether accesses from userspace (i.e. ring 3) are permitted.
#define PTE_USER_ACCESSIBLE (U64_1 << 2)
/// If this bit is set, a “write-through” policy is used for the cache, else a
/// “write-back” policy is used.
#define PTE_WRITE_THROUGH (U64_1 << 3)
/// Disables caching for the pointed entry is cacheable.
#define PTE_NO_CACHE (U64_1 << 4)
/// Set by the CPU when the mapped frame or page table is accessed.
#define PTE_ACCESSED (U64_1 << 5)
/// Set by the CPU on a write to the mapped frame.
#define PTE_DIRTY (U64_1 << 6)
/// Specifies that the entry maps a huge frame instead of a page table. Only
/// allowed in P2 or P3 tables.
#define PTE_HUGE_PAGE (U64_1 << 7)
/// Indicates that the mapping is present in all address spaces, so it isn't
/// flushed from the TLB on an address space switch.
#define PTE_GLOBAL (U64_1 << 8)
/// Available to the OS, can be used to store additional data, e.g. custom
/// flags.
#define PTE_BIT_9 (U64_1 << 9)
/// Available to the OS, can be used to store additional data, e.g. custom
/// flags.
#define PTE_BIT_10 (U64_1 << 10)
/// Available to the OS, can be used to store additional data, e.g. custom
/// flags.
#define PTE_BIT_11 (U64_1 << 11)
/// Available to the OS, can be used to store additional data, e.g. custom
/// flags.
#define PTE_BIT_52 (U64_1 << 52)
/// Available to the OS, can be used to store additional data, e.g. custom
/// flags.
#define PTE_BIT_53 (U64_1 << 53)
/// Available to the OS, can be used to store additional data, e.g. custom
/// flags.
#define PTE_BIT_54 (U64_1 << 54)
/// Available to the OS, can be used to store additional data, e.g. custom
/// flags.
#define PTE_BIT_55 (U64_1 << 55)
/// Available to the OS, can be used to store additional data, e.g. custom
/// flags.
#define PTE_BIT_56 (U64_1 << 56)
/// Available to the OS, can be used to store additional data, e.g. custom
/// flags.
#define PTE_BIT_57 (U64_1 << 57)
/// Available to the OS, can be used to store additional data, e.g. custom
/// flags.
#define PTE_BIT_58 (U64_1 << 58)
/// Available to the OS, can be used to store additional data, e.g. custom
/// flags.
#define PTE_BIT_59 (U64_1 << 59)
/// Available to the OS, can be used to store additional data, e.g. custom
/// flags.
#define PTE_BIT_60 (U64_1 << 60)
/// Available to the OS, can be used to store additional data, e.g. custom
/// flags.
#define PTE_BIT_61 (U64_1 << 61)
/// Available to the OS, can be used to store additional data, e.g. custom
/// flags.
#define PTE_BIT_62 (U64_1 << 62)
/// Forbid code execution from the mapped frames.
///
/// Can be only used when the no-execute page protection feature is enabled in
/// the EFER register.
#define PTE_NO_EXECUTE (U64_1 << 63)

#define PTE_ADDRESS 0x000ffffffffff000ull

char *memory__bootboot_mmap_typename[] = {"Used", "Free", "ACPI", "MMIO"};

#define PageTable__ENTRY_COUNT 512
typedef struct {
  uint64_t entries[PageTable__ENTRY_COUNT];
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

// NOTE: I'm having trouble finding actual resources online for stuff, so I'm just
// going to make some assumptions.

// static PageTable *build_page_table(MMap mmap, uint16_t table_level, PageTableIndices min_addr,
//                                    PageTableIndices max_addr) {
//   assert(table_level < 4);
//   for (int i = 0; i < table_level; i++)
//     assert(min_addr.indices[i] == max_addr.indices[i]);
//
//   PageTable *new_table = alloc_from_entries(mmap, sizeof(PageTable), sizeof(PageTable));
//   assert(new_table != MMapEnt__ALLOC_FAILURE);
//
//   const static uint64_t LAZY_FLAG[] = {0, 0, PTE_HUGE_PAGE, PTE_HUGE_PAGE};
//   for (uint16_t i = 0; i < max_addr.indices[table_level]; i++) {
//     min_addr.indices[table_level] = i;
//     uint64_t page_address = indices_to_address(min_addr) & PTE_ADDRESS;
//     uint64_t entry = page_address | PTE_WRITABLE | PTE_NO_EXECUTE | LAZY_FLAG[table_level];
//     new_table->entries[i] = entry;
//   }
//
//   return new_table;
// }
//
// static void extend_page_tables(MMap mmap, uint64_t _max_addr) {
//   assert(MEMORY__KERNEL_SPACE_BEGIN > _min_addr);
//
//   PageTableIndices min_addr = page_table_indices(0);
//   PageTableIndices max_addr = page_table_indices(_max_addr);
//   PageTableIndices kernel_offset = page_table_indices(_kernel_offset);
//
//   uint16_t table_level = 4;
//   PageTable *current_table = read_register(cr3, PageTable *);
//   for (uint16_t i = 0; i < max_addr.indices[table_level]; i++) {
//     PageTable *child = build_page_table(mmap, table_level - 1, min_addr);
//     uint64_t address = (uint64_t)child;
//     uint64_t entry = address | PTE_WRITABLE | PTE_NO_EXECUTE;
//     current_table->entries[kernel_offset.indices[table_level] + i] = entry;
//   }
//
//   table_level -= 1;
//   // min_addr[table_level] =
//
//   while (table_level) {
//     uint16_t start_at = kernel_offset[table_level];
//     PageTable *page_entry = build_page_table(mmap, table_level, start_at, min_addr, max_addr);
//     current_table->entries[min_addr] = (uint64_t)page_entry;
//     table_level -= 1;
//   }
// }

MMap memory__init(BOOTBOOT *bb) {
  // Calculation described in bootboot specification
  MMap mmap = {.data = &bb->mmap, .count = (bb->size - 128) / 16};
  uint64_t max_addr = 0;
  FOR(mmap) {
    uint64_t ptr = MMapEnt_Ptr(it), size = MMapEnt_Size(it);
    max_addr = max(max_addr, ptr + size);
  }

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
    log_fmt("entry: free entry size %", it->size);
  }

  // First page in physical memory isn't used right now.
  uint64_t first_ptr = MMapEnt_Ptr(&mmap.data[0]);
  if (first_ptr < _4KB) {
    int64_t safety_size = _4KB - (int64_t)first_ptr;
    void *safety_alloc = alloc_from_entries(mmap, safety_size, 1);
    memset(safety_alloc, 42, safety_size);
  }

  return mmap;
}

void *alloc_from_entries(MMap mmap, int64_t _size, int64_t _align) {
  if (_size <= 0 || _align < 0) return MMapEnt__ALLOC_FAILURE;

  uint64_t align = max((uint64_t)_align, 1);
  uint64_t size = align_up((uint64_t)_size, align);

  FOR(mmap, cur) {
    uint64_t aligned_ptr = align_up(cur->ptr, align);
    uint64_t aligned_size = cur->size + cur->ptr - aligned_ptr;
    if (aligned_size < size) continue;

    cur->ptr = aligned_ptr + size;
    cur->size = aligned_size - size;
    return (void *)aligned_ptr;
  }

  return MMapEnt__ALLOC_FAILURE;
}
