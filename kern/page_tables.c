#include "page_tables.h"
#include "memory.h"
#include <asm.h>
#include <basics.h>
#include <macros.h>

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

static u64 make_pte(void *ptr, u64 flags) {
  if (ptr == NULL) return 0;

  u64 addr_bits = physical_address(ptr) & PTE_ADDRESS;
  return addr_bits | flags;
}

static void *pte_address(u64 entry) {
  if (entry == 0) return NULL;

  const u64 addr_bits = entry & PTE_ADDRESS;
  const s64 signed_address_shifted = (s64)(addr_bits << 16);
  const u64 address = (u64)(signed_address_shifted >> 16);
  return kernel_ptr(address);
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

PageTable4 *get_page_table(void) {
  return kernel_ptr(read_register(cr3, u64, "q"));
}

void set_page_table(PageTable4 *p4) {
  write_register(cr3, physical_address(p4));
}

// Hacky solution to quickly get everything into a higher-half kernel. Called in
// memory.c, before allocator is initialized.
void UNSAFE_HACKY_higher_half_init() {
  PageTable *table = read_register(cr3, PageTable *, "q");
  PageTableIndices indices = page_table_indices(MEMORY__KERNEL_SPACE_BEGIN);
  assert(indices.p3 == 0);
  assert(indices.p2 == 0);
  assert(indices.p1 == 0);
  assert(indices.p0 == 0);
  table->entries[indices.p4] = table->entries[0];
  table->entries[0] = 0;
  write_register(cr3, table);
}

void *map_region(PageTable4 *p4, u64 virtual, void *_kernel, u64 flags, s64 size) {
  ensure(p4 != NULL && is_aligned(p4, _4KB)) return NULL;
  ensure(_kernel && is_aligned(_kernel, _4KB)) return NULL;
  ensure(virtual && is_aligned(virtual, _4KB)) return NULL;
  ensure(is_aligned(size, _4KB)) return NULL;

  void *const output = (void *)virtual;

  DECLARE_SCOPED(u64 diff = 0)
  for (u8 *ptr = (u8 *)_kernel, *res = NULL; size > 0; virtual += diff, ptr += diff, size -= diff) {
    if (is_aligned(virtual, _2MB) && is_aligned(ptr, _2MB) && size >= _2MB) {
      res = map_2MB_page(p4, virtual, ptr, flags);
      ensure(res) return NULL;
      diff = _2MB;
      continue;
    }

    res = map_page(p4, virtual, ptr, flags);
    ensure(res) return NULL;
    diff = _4KB;
  }

  return output;
}

void *translate(PageTable4 *_p4, u64 virtual) {
  ensure(_p4) return NULL;

  PageTable *p4 = (PageTable *)_p4;
  PageTableIndices indices = page_table_indices(virtual);

  PageTable *p3 = pte_address(p4->entries[indices.p4]);
  ensure(p3) return NULL;

  PageTable *p2 = pte_address(p3->entries[indices.p3]);
  ensure(p2) return NULL;

  const u64 p2_entry = p2->entries[indices.p2];
  u8 *page = pte_address(p2_entry);
  ensure(page) return NULL;

  if (p2_entry & PTE_HUGE_PAGE) {
    return page + (U64(indices.p1) << 12) + indices.p0;
  }

  PageTable *p1 = (PageTable *)page;
  return pte_address(p1->entries[indices.p1]) + indices.p0;
}

void *map_page(PageTable4 *_p4, u64 virtual, void *kernel, u64 flags) {
  PageTable *p4 = (PageTable *)_p4;
  PageTableIndices indices = page_table_indices(virtual);

  ensure(p4 != NULL && is_aligned(p4, _4KB)) return NULL;
  ensure(kernel && is_aligned(kernel, _4KB)) return NULL;
  ensure(virtual && is_aligned(virtual, _4KB)) return NULL;

  PageTable *p3 = pte_address(p4->entries[indices.p4]);
  ensure(p3) {
    p3 = zeroed_pages(1);
    ensure(p3) return NULL;

    p4->entries[indices.p4] = make_pte(p3, flags);
  }

  PageTable *p2 = pte_address(p3->entries[indices.p3]);
  ensure(p2) {
    p2 = zeroed_pages(1);
    ensure(p2) return NULL;

    p3->entries[indices.p3] = make_pte(p2, flags);
  }

  PageTable *p1 = pte_address(p2->entries[indices.p2]);
  ensure(p1) {
    p1 = zeroed_pages(1);
    ensure(p1) return NULL;

    p2->entries[indices.p2] = make_pte(p1, flags);
  }

  ensure(p1->entries[indices.p1] == 0) return NULL;
  p1->entries[indices.p1] = make_pte(kernel, flags);

  return (void *)virtual;
}

void *map_2MB_page(PageTable4 *_p4, u64 virtual, void *kernel, u64 flags) {
  PageTable *p4 = (PageTable *)_p4;
  PageTableIndices indices = page_table_indices(virtual);

  ensure(p4 != NULL && is_aligned(p4, _4KB)) return NULL;
  ensure(kernel && is_aligned(kernel, _2MB)) return NULL;
  ensure(virtual && is_aligned(virtual, _2MB)) return NULL;

  PageTable *p3 = pte_address(p4->entries[indices.p4]);
  ensure(p3) {
    p3 = zeroed_pages(1);
    ensure(p3) return NULL;

    p4->entries[indices.p4] = make_pte(p3, flags);
  }

  PageTable *p2 = pte_address(p3->entries[indices.p3]);
  ensure(p2) {
    p2 = zeroed_pages(1);
    ensure(p2) return NULL;

    p3->entries[indices.p3] = make_pte(p2, flags);
  }

  ensure(p2->entries[indices.p2] == 0) return NULL;
  p2->entries[indices.p2] = make_pte(kernel, flags | PTE_HUGE_PAGE);

  return (void *)virtual;
}

static void traverse_table_inner(u64 table_entry, u16 table_level);
void traverse_table(PageTable4 *p4) {
  traverse_table_inner(make_pte(p4, 0), 4);
}

static void traverse_table_inner(u64 table_entry, u16 table_level) {
  const static char *const prefixes[] = {"| | | +-", "| | +-", "| +-", "+-", ""};
  const static char *const page_size_for_entry[] = {"", "4Kb", "2Mb", "1Gb", ""};

  if (!table_level | (table_entry & PTE_HUGE_PAGE)) {
    // Probs wont ever happen
    return;
  }

  u16 count = 0;
  PageTable *table = pte_address(table_entry);
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
      traverse_table_inner(entry, table_level - 1);
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
