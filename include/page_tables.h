#pragma once
#include <efi.h>
#include <stdint.h>

typedef struct {
  const uint64_t size;
  const uint64_t descriptor_size;
  const EFI_MEMORY_DESCRIPTOR *buffer;
} MemoryMap;

static inline MemoryMap
MemoryMap__new(uint64_t size, uint64_t descriptor_size,
               const EFI_MEMORY_DESCRIPTOR *memory_map) {
  return (MemoryMap){
      .size = size, .descriptor_size = descriptor_size, .buffer = memory_map};
}

typedef struct {
  uint64_t address;
} PTE;

void page_tables__init(MemoryMap memory_map);

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
