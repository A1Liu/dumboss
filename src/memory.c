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

static PageTable *build_page_table(MMap mmap, uint16_t table_level, PageTableIndices min_addr,
                                   PageTableIndices max_addr) {
  assert(table_level < 4);
  for (int i = 0; i < table_level; i++)
    assert(min_addr.indices[i] == max_addr.indices[i]);
  for (int i = table_level; i < 4; i++)
    assert(min_addr.indices[i] == 0);

  PageTable *new_table = alloc_from_entries(mmap, sizeof(PageTable), sizeof(PageTable));
  assert(new_table != MMapEnt__ALLOC_FAILURE);

  const static uint64_t LAZY_FLAG[] = {0, 0, PTE_HUGE_PAGE, PTE_HUGE_PAGE, 0};
  for (uint16_t i = 0; i < PageTable__ENTRY_COUNT; i++) {
    min_addr.indices[table_level] = i;
    uint64_t page_address = indices_to_address(min_addr) & PTE_ADDRESS;
    new_table->entries[i] = page_address | PTE_WRITABLE | PTE_NO_EXECUTE | LAZY_FLAG[table_level];
  }

  return new_table;
}

// static void extend_page_table(MMap mmap, uint64_t min_addr, uint64_t max_addr) {
// }

static MMap sort_entries(MMapEnt *entries, int64_t entry_count);
MMap memory__init(BOOTBOOT *bb) {
  // Calculation described in bootboot specification
  int64_t entry_count = (bb->size - 128) / 16;
  uint64_t min_addr = ~((uint64_t)0), max_addr = 0;
  for (int i = 0; i < entry_count; i++) {
    MMapEnt *entry = &bb->mmap;
    uint64_t ptr = MMapEnt_Ptr(&entry[i]), size = MMapEnt_Size(&entry[i]);

    max_addr = max(max_addr, ptr + size);
    min_addr = min(min_addr, ptr);
  }

  MMap mmap = sort_entries(&bb->mmap, entry_count);

  // First page in memory isn't used right now.
  uint64_t first_ptr = MMapEnt_Ptr(&mmap.entries[0]);
  if (first_ptr < _4KB) {
    int64_t safety_size = _4KB - (int64_t)first_ptr;
    void *safety_alloc = alloc_from_entries(mmap, safety_size, 1);
    memset(safety_alloc, 42, safety_size);
  }

  return mmap;
}

static MMap sort_entries(MMapEnt *entries, int64_t entry_count) {
  // bubble-sort the entries so that the free ones are first
  for (int64_t right_bound = entry_count - 1; right_bound > 0; right_bound--) {
    for (int64_t i = 0; i < right_bound; i++) {
      MMapEnt *left = &entries[i], *right = &entries[i + 1];
      uint64_t l_ptr = MMapEnt_Ptr(left), r_ptr = MMapEnt_Ptr(right);
      uint64_t l_size = MMapEnt_Size(left), r_size = MMapEnt_Size(right);
      uint64_t l_type = MMapEnt_Type(left), r_type = MMapEnt_Type(right);

      bool swap = (l_ptr == 0) & (l_size == 0);
      swap |= (l_type != MMAP_FREE) & (r_type == MMAP_FREE);

      // NOTE: This coalescing is reliant on bootboot behavior, see
      // include/bootboot.h line 89
      //                        - Albert Liu, Oct 21, 2021 Thu 21:33 EDT
      if (l_ptr + l_size == r_ptr && l_type == r_type) {
        left->size = (l_size + r_size) | l_type;
        right->ptr = 0;
        right->size = l_type;
      }

      if (!swap) continue;

      MMapEnt e = entries[i + 1];
      entries[i + 1] = entries[i];
      entries[i] = e;
    }
  }

  // remove weird bit stuff that BOOTBOOT does for the free entries
  int64_t i = 0;
  for (; i < entry_count && MMapEnt_IsFree(&entries[i]); i++) {
    MMapEnt *entry = &entries[i];
    entry->size = MMapEnt_Size(entry);
    log_fmt("entry: free entry size %", entry->size);
  }

  return (MMap){.entries = entries, .count = i};
}

void *alloc_from_entries(MMap mmap, int64_t _size, int64_t _align) {
  if (_size <= 0 || _align < 0) {
    return MMapEnt__ALLOC_FAILURE;
  }

  uint64_t align = max((uint64_t)_align, 1);
  uint64_t size = align_up((uint64_t)_size, align);
  for (int64_t i = 0; i < mmap.count; i++) {
    MMapEnt *cur = &mmap.entries[i];

    uint64_t aligned_ptr = align_up(cur->ptr, align);
    uint64_t aligned_size = cur->size + cur->ptr - aligned_ptr;
    if (aligned_size < size) continue;

    cur->ptr = aligned_ptr + size;
    cur->size = aligned_size - size;
    return (void *)aligned_ptr;
  }

  return MMapEnt__ALLOC_FAILURE;
}
