#pragma once
#include <types.h>

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

#define PTE_NOT_EMPTY  (PTE_BIT_9)
#define PTE_KERNEL     (PTE_WRITABLE | PTE_NO_EXECUTE | PTE_PRESENT | PTE_NOT_EMPTY)
#define PTE_KERNEL_EXE (PTE_PRESENT | PTE_NOT_EMPTY)
#define PTE_USER       (PTE_WRITABLE | PTE_NO_EXECUTE | PTE_PRESENT | PTE_USER_ACCESSIBLE | PTE_NOT_EMPTY)

typedef struct PageTable4 PageTable4;

// Called in memory.c
void UNSAFE_HACKY_higher_half_init(void);

// Read the value of cr3
PageTable4 *get_page_table(void);
void set_page_table(PageTable4 *p4);

void traverse_table(PageTable4 *p4);

void destroy_table(PageTable4 *p4);
void destroy_bootboot_table(PageTable4 *p4);

bool copy_mapping(PageTable4 *dest, PageTable4 *src, u64 virt, s64 count, u64 flags);

// Get the size of the underlying contiguous physical region
s64 region_size(PageTable4 *p4, u64 ptr);

// Translates virtual address to physical address
void *translate(PageTable4 *p4, u64 virt);

bool map_region(PageTable4 *p4, u64 virt, const void *kernel, u64 flags, s64 size);

bool map_page(PageTable4 *p4, u64 virt, const void *kernel, u64 flags);

bool map_2MB_page(PageTable4 *p4, u64 virt, const void *kernel, u64 flags);
