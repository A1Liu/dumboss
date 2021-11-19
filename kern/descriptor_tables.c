#include "init.h"
#include "memory.h"
#include "types.h"
#include <asm.h>
#include <basics.h>
#include <macros.h>

// Used Phil Opperman's x86_64 rust code to figure out how to do this
// https://github.com/rust-osdev/x86_64/blob/master/src/structures/gdt.rs
// https://github.com/rust-osdev/x86_64/blob/master/src/instructions/segmentation.rs

typedef struct {
  u16 pointer_low;
  u16 gdt_selector;
  u16 options;
  u16 pointer_middle;
  u32 pointer_high;
  u32 reserved;
} IdtEntry;

typedef struct {
  IdtEntry inner;
} IdtEntry__ForExt;
typedef struct {
  IdtEntry inner;
} IdtEntry__ForDiverging;
typedef struct {
  IdtEntry inner;
} IdtEntry__ForDivergingExt;

typedef struct {
  IdtEntry divide_error;
  IdtEntry debug;
  IdtEntry non_maskable_interrupt;
  IdtEntry breakpoint;
  IdtEntry overflow;
  IdtEntry bound_range_exceeded;
  IdtEntry invalid_opcode;
  IdtEntry device_not_available;
  IdtEntry__ForDivergingExt double_fault;
  IdtEntry coprocessor_segment_overrun;
  IdtEntry__ForExt invalid_tss;
  IdtEntry__ForExt segment_not_present;
  IdtEntry__ForExt stack_segment_fault;
  IdtEntry__ForExt general_protection_fault;
  IdtEntry page_fault;
  IdtEntry reserved_1;
  IdtEntry x87_floating_point;
  IdtEntry__ForExt alignment_check;
  IdtEntry__ForDiverging machine_check;
  IdtEntry simd_floating_point;
  IdtEntry virtualization;
  IdtEntry reserved_2[9];
  IdtEntry__ForExt security_exception;
  IdtEntry reserved_3;
  IdtEntry user_defined[256 - 32];
} Idt;

typedef struct {
  u64 instruction_pointer;
  u64 code_segment;
  u64 cpu_flags;
  u64 stack_pointer;
  u64 stack_segment;
} ExceptionStackFrame;

#define HANDLER       __attribute__((interrupt)) void
#define NORET_HANDLER __attribute__((noreturn, interrupt)) void

typedef HANDLER (*Idt__Handler)(ExceptionStackFrame *);
typedef HANDLER (*Idt__HandlerExt)(ExceptionStackFrame *, u64);
typedef NORET_HANDLER (*Idt__DivergingHandler)(ExceptionStackFrame *);
typedef NORET_HANDLER (*Idt__DivergingHandlerExt)(ExceptionStackFrame *, u64);

// clang-format off
#define IdtEntry__set_handler(entry, handler)                                  \
  _Generic((entry),                                                            \
                        IdtEntry* : IdtEntry__Default__set_handler,            \
                IdtEntry__ForExt* : IdtEntry__ForExt__set_handler,             \
          IdtEntry__ForDiverging* : IdtEntry__ForDiverging__set_handler,       \
       IdtEntry__ForDivergingExt* : IdtEntry__ForDivergingExt__set_handler     \
  )(entry, handler)
// clang-format on

void IdtEntry__Default__set_handler(IdtEntry *entry, Idt__Handler handler);
void IdtEntry__ForExt__set_handler(IdtEntry__ForExt *entry, Idt__HandlerExt handler);
void IdtEntry__ForDiverging__set_handler(IdtEntry__ForDiverging *entry,
                                         Idt__DivergingHandler handler);
void IdtEntry__ForDivergingExt__set_handler(IdtEntry__ForDivergingExt *entry,
                                            Idt__DivergingHandlerExt handler);

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

typedef struct {
  u64 table[8];
  u8 index;
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

// Used Phil Opperman's x86_64 rust code to figure out how to do this
// https://github.com/rust-osdev/x86_64/blob/master/src/structures/idt.rs
static void Idt__init(Idt *idt);
static void Idt__log_fmt(ExceptionStackFrame *frame);
static u64 IdtEntry__handler_addr(IdtEntry entry);
static inline IdtEntry IdtEntry__missing(void);
static inline void load_idt(Idt *base);

static void Gdt__init(Gdt *gdt);
static u16 Gdt__add_entry(Gdt *gdt, u64 entry);
static u16 Gdt__add_tss(Gdt *gdt, const Tss *tss);
static inline void load_gdt(Gdt *base, u16 selector);
static GdtInfo current_gdt(void);

static NORET_HANDLER Idt__double_fault(ExceptionStackFrame *frame, u64 error_code);

void descriptor__init() {
  // NOTE: this leaks intentionally. The GDT and IDT need to exist until shutdown,
  // at which time it does not matter whether they are freed.
  Bump bump = Bump__new(2);

  Idt *idt = Bump__bump(&bump, Idt);
  assert(idt);
  Idt__init(idt);

  Gdt *gdt = Bump__bump(&bump, Gdt);
  assert(gdt);
  Gdt__init(gdt);

  Tss *tss = Bump__bump(&bump, Tss);
  assert(tss);

  // TODO make this safer
  //      - Albert Liu, Nov 18, 2021 Thu 23:04 EST
  void *new_stack = zeroed_pages(2);
  tss->interrupt_stack_table[0] = U64(new_stack) + 2 * _4KB;

  u16 segment = Gdt__add_entry(gdt, GDT__KERNEL_CODE);
  Gdt__add_tss(gdt, tss);

  load_gdt(gdt, segment);

  log_fmt("global descriptor table INIT_COMPLETE");

  IdtEntry__set_handler(&idt->double_fault, Idt__double_fault);
  load_idt(idt);

  log_fmt("interrupt descriptor table INIT_COMPLETE");

  log_fmt("descriptor INIT_COMPLETE");
}

/*
--------------------------------------------------------------------------------


                      The actual implementations of handlers


--------------------------------------------------------------------------------
*/

static NORET_HANDLER Idt__double_fault(ExceptionStackFrame *frame, u64 error_code) {
  log_fmt("double fault error_code: %f", error_code);
  Idt__log_fmt(frame);
  panic();
}

/*
--------------------------------------------------------------------------------


                                   UTILITIES


--------------------------------------------------------------------------------
*/

static void Idt__init(Idt *idt) {
  IdtEntry *entries = (IdtEntry *)idt;
  for (s64 i = 0; i < 256; i++) {
    entries[i] = IdtEntry__missing();
  }
}

static void Idt__log_fmt(ExceptionStackFrame *frame) {
  log_fmt("ExceptionStackFrame{ip=%f,cs=%f,flags=%f,sp=%f,ss=%f}", frame->instruction_pointer,
          frame->code_segment, frame->cpu_flags, frame->stack_pointer, frame->stack_segment);
}

static u16 IdtEntry__set_present(u16 opts) {
  return opts | (1 << 15);
}

u64 IdtEntry__handler_addr(IdtEntry entry) {
  return ((u64)entry.pointer_low) | (((u64)entry.pointer_middle) << 16) |
         (((u64)entry.pointer_high) << 32);
}

#undef IdtEntry__set_handler
static void IdtEntry__set_handler(IdtEntry *entry, void *handler) {
  u64 addr = (u64)handler;
  entry->pointer_low = (u16)addr;
  entry->pointer_middle = (u16)(addr >> 16);
  entry->pointer_high = (uint32_t)(addr >> 32);
  entry->gdt_selector = read_register(cs, u16);
  entry->options = IdtEntry__set_present(entry->options);
}

void IdtEntry__Default__set_handler(IdtEntry *entry, Idt__Handler handler) {
  IdtEntry__set_handler(entry, handler);
}

void IdtEntry__ForExt__set_handler(IdtEntry__ForExt *entry, Idt__HandlerExt handler) {
  IdtEntry__set_handler(&entry->inner, handler);
}
void IdtEntry__ForDiverging__set_handler(IdtEntry__ForDiverging *entry,
                                         Idt__DivergingHandler handler) {
  IdtEntry__set_handler(&entry->inner, handler);
}
void IdtEntry__ForDivergingExt__set_handler(IdtEntry__ForDivergingExt *entry,
                                            Idt__DivergingHandlerExt handler) {
  IdtEntry__set_handler(&entry->inner, handler);
}

static inline IdtEntry IdtEntry__missing(void) {
  // Options disable IRQs by default

  return (IdtEntry){
      .pointer_low = 0,
      .gdt_selector = 0,
      .options = 0xe00,
      .pointer_middle = 0,
      .pointer_high = 0,
      .reserved = 0,
  };
}

static inline void load_idt(Idt *base) {
  struct {
    u16 size;
    void *base;
  } __attribute__((packed)) IDTR = {.size = sizeof(Idt) - 1, .base = base};

  // let the compiler choose an addressing mode
  asm volatile("lidt %0" : : "m"(IDTR));
}

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
  } __attribute__((packed)) GDTR = {.size = size, .base = base};

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
  gdt->index = U8(index + 1);

  u16 priviledge_level = (entry & GDT__DPL_RING_3) ? 3 : 0;
  return U16((index << 3) | priviledge_level);
}

static u16 Gdt__add_tss(Gdt *gdt, const Tss *tss) {
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
  return U16(index << 3);
}
