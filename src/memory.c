#include "memory.h"
#include "basics.h"
#include "logging.h"
#include <stddef.h>

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

char *memory__bootboot_mmap_typename[] = {"Used", "Free", "ACPI", "MMIO"};

static int64_t sort_entries(MMapEnt *entries, int64_t entry_count) {

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
      if (l_ptr + l_size == r_ptr && l_type == r_type) {
        left->size = (l_size + r_size) | l_type;
        right->ptr = 0;
        right->size = l_type;
      }

      if (!swap)
        continue;

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

  return i;
}

void *alloc_from_entries(MMapEnt *entries, int64_t entry_count, int64_t _size,
                         int64_t _align) {
  if (_size <= 0 || _align < 0) {
    log("wtf");
    return MMapEnt_ALLOC_FAILURE;
  }

  uint64_t align = max((uint64_t)_align, 1);
  uint64_t size = align_up((uint64_t)_size, align);
  for (int64_t i = 0; i < entry_count; i++) {
    MMapEnt *cur = &entries[i];

    uint64_t aligned_ptr = align_up(cur->ptr, align);
    uint64_t aligned_size = cur->size + cur->ptr - aligned_ptr;
    if (aligned_size < size)
      continue;

    cur->ptr = aligned_ptr + size;
    cur->size = aligned_size - size;
    return (void *)aligned_ptr;
  }

  log("none found");
  return MMapEnt_ALLOC_FAILURE;
}

int64_t memory__init(MMapEnt *entries, int64_t entry_count) {
  entry_count = sort_entries(entries, entry_count);

  uint64_t first_ptr = MMapEnt_Ptr(&entries[0]);
  if (first_ptr < _4KB) {
    int64_t safety_size = _4KB - (int64_t)first_ptr;
    void *safety_alloc =
        alloc_from_entries(entries, entry_count, safety_size, 1);
    memset(safety_alloc, 42, safety_size);
  }

  return entry_count;
}
