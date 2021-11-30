#include "init.h"
#include "memory.h"
#include <basics.h>
#include <macros.h>

// Used Phil Opperman's x86_64 rust code to figure out how to do this
// https://github.com/rust-osdev/x86_64/blob/master/src/structures/gdt.rs
// https://github.com/rust-osdev/x86_64/blob/master/src/instructions/segmentation.rs

typedef struct {
  u32 reserved_1;
  u64 privilege_stack_table[3];
  u64 reserved_2;
  u64 interrupt_stack_table[7];
  u64 reserved_3;
  u16 reserved_4;
  u16 iomap_base;
} __attribute__((packed)) Tss;

typedef struct {
  u64 *gdt;
  u16 size;
} GdtInfo;

// TODO make this actualy work with more than one cpu
typedef struct {
  u16 index;
  u64 table[8];
} Gdt;

#define GDT__ACCESSED     (U64(1) << 40)
#define GDT__WRITABLE     (U64(1) << 41)
#define GDT__CONFORMING   (U64(1) << 42)
#define GDT__EXECUTABLE   (U64(1) << 43)
#define GDT__USER_SEGMENT (U64(1) << 44)
#define GDT__DPL_RING_3   (U64(3) << 45)
#define GDT__PRESENT      (U64(1) << 47)
#define GDT__AVAILABLE    (U64(1) << 52)
#define GDT__LONG_MODE    (U64(1) << 53)
#define GDT__DEFAULT_SIZE (U64(1) << 54)
#define GDT__GRANULARITY  (U64(1) << 55)
#define GDT__LIMIT_0_15   (U64(0xffff))
#define GDT__LIMIT_16_19  (U64(0xf) << 48)
#define GDT__BASE_0_23    (U64(0xffffff) << 16)
#define GDT__BASE_24_31   (U64(0xff) << 56)
#define GDT__COMMON                                                                                \
  (GDT__USER_SEGMENT | GDT__ACCESSED | GDT__PRESENT | GDT__WRITABLE | GDT__LIMIT_0_15 |            \
   GDT__LIMIT_16_19 | GDT__GRANULARITY)
#define GDT__KERNEL_CODE (GDT__COMMON | GDT__EXECUTABLE | GDT__LONG_MODE)
#define GDT__USER_CODE   (GDT__KERNEL_CODE | GDT__DPL_RING_3)

_Static_assert(GDT__KERNEL_CODE == 0x00af9b000000ffffULL, "GDT__KERNEL_CODE has incorrect value");
_Static_assert(GDT__USER_CODE == 0x00affb000000ffffULL, "GDT__USER_CODE has incorrect value");

static void Gdt__init(Gdt *gdt);
static u16 Gdt__add_entry(Gdt *gdt, u64 entry);
static void Gdt__add_tss(Gdt *gdt, const Tss *tss, s64 core_idx);
static inline void load_gdt(Gdt *base, u16 selector);
static GdtInfo current_gdt(void);

void descriptor__init() {
  Gdt *gdt = Bump__bump(&InitAlloc, Gdt);
  assert(gdt);
  Gdt__init(gdt);

  Tss *tss = Bump__bump(&InitAlloc, Tss);
  assert(tss);

  // TODO make this safer
  //      - Albert Liu, Nov 18, 2021 Thu 23:04 EST
  void *new_stack = zeroed_pages(2);
  tss->interrupt_stack_table[0] = U64(new_stack) + 2 * _4KB;

  u16 segment = Gdt__add_entry(gdt, GDT__KERNEL_CODE);
  Gdt__add_tss(gdt, tss, 0);

  load_gdt(gdt, segment);

  log_fmt("global descriptor table INIT_COMPLETE");
}

u16 tss_segment(s64 core_idx) {
  u16 tss_idx = (U16(core_idx) * 2) + 2;
  return U16(tss_idx << 3);
}

/*
--------------------------------------------------------------------------------


                                   UTILITIES


--------------------------------------------------------------------------------
*/

static GdtInfo current_gdt(void) {
  struct {
    u16 size;
    void *base;
  } __attribute__((packed)) GDTR;

  asm volatile("sgdt %0" : "=m"(GDTR));
  return (GdtInfo){
      .gdt = GDTR.base,
      .size = GDTR.size,
  };
}

static inline void load_gdt(Gdt *base, u16 selector) {
  u16 size = base->index * sizeof(u64) - 1;
  struct {
    u16 size;
    void *base;
  } __attribute__((packed)) GDTR = {.size = size, .base = base->table};

  // let the compiler choose an addressing mode
  u64 tmp = selector;
  asm volatile("lgdt %0" : : "m"(GDTR));
  asm volatile("pushq %0; pushq %1; lretq" : : "r"(tmp), "r"(&&exit));
exit:
  return;
}

static void Gdt__init(Gdt *gdt) {
  gdt->table[0] = 0;
  gdt->index = 1;
}

static u16 Gdt__add_entry(Gdt *gdt, u64 entry) {
  u16 index = gdt->index;
  assert(index < 8);

  gdt->table[index] = entry;
  gdt->index = index + 1;

  u16 priviledge_level = (entry & GDT__DPL_RING_3) ? 3 : 0;
  return U16((index << 3) | priviledge_level);
}

static void Gdt__add_tss(Gdt *gdt, const Tss *tss, s64 core_idx) {
  const u64 BYTE = 255;
  const u64 BYTES_0_16 = BYTE | (BYTE << 8);
  const u64 BYTES_0_24 = BYTE | (BYTE << 8) | (BYTE << 16);
  const u64 BYTES_24_32 = (BYTE << 24);
  const u64 BYTES_32_64 = (BYTE << 32) | (BYTE << 40) | (BYTE << 48) | (BYTE << 56);

  const u64 addr = U64(tss);
  u64 low = GDT__PRESENT;
  low |= (addr & BYTES_0_24) << 16;
  low |= (addr & BYTES_24_32) << 32;
  low |= sizeof(Tss) - 1;
  low |= U64(9) << 40;

  u64 high = 0;
  high |= (addr & BYTES_32_64) >> 32;

  const u16 index = gdt->index;
  assert(index + 1 < 8);

  gdt->table[index] = low;
  gdt->table[index + 1] = high;
  gdt->index += 2;

  // priviledge level is zero so there's no need to BIT_OR with anything
  u16 selector = U16(index << 3);
  assert(selector == tss_segment(core_idx));
}
