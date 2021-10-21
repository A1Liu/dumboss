#pragma once
#include <stdint.h>

// Used Phil Opperman's x86_64 rust code to figure out how to do this
// https://github.com/rust-osdev/x86_64/blob/master/src/structures/idt.rs

typedef struct {
  uint16_t pointer_low;
  uint16_t gdt_selector;
  uint16_t options;
  uint16_t pointer_middle;
  uint32_t pointer_high;
  uint32_t reserved;
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
  uint64_t instruction_pointer;
  uint64_t code_segment;
  uint64_t cpu_flags;
  uint64_t stack_pointer;
  uint64_t stack_segment;
} ExceptionStackFrame;

typedef void (*Idt__Handler)(ExceptionStackFrame *);
typedef void (*Idt__HandlerExt)(ExceptionStackFrame *, uint64_t);
typedef __attribute__((noreturn, interrupt)) void (*Idt__DivergingHandler)(
    ExceptionStackFrame *);
typedef __attribute__((noreturn, interrupt)) void (*Idt__DivergingHandlerExt)(
    ExceptionStackFrame *, uint64_t);

static inline void load_idt(Idt *base) {
  struct {
    uint16_t size;
    void *base;
  } __attribute__((packed)) IDTR = {.size = sizeof(Idt) - 1, .base = base};

  // let the compiler choose an addressing mode
  asm volatile("lidt %0" : : "m"(IDTR));
}

Idt *Idt__new(void *buffer, int64_t size);
void Idt__log_fmt(ExceptionStackFrame *frame);
uint64_t IdtEntry__handler_addr(IdtEntry entry);

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
void IdtEntry__ForExt__set_handler(IdtEntry__ForExt *entry,
                                   Idt__HandlerExt handler);
void IdtEntry__ForDiverging__set_handler(IdtEntry__ForDiverging *entry,
                                         Idt__DivergingHandler handler);
void IdtEntry__ForDivergingExt__set_handler(IdtEntry__ForDivergingExt *entry,
                                            Idt__DivergingHandlerExt handler);

void divide_by_zero(void);

// Used Phil Opperman's x86_64 rust code to figure out how to do this
// https://github.com/rust-osdev/x86_64/blob/master/src/structures/gdt.rs
// https://github.com/rust-osdev/x86_64/blob/master/src/instructions/segmentation.rs

_Static_assert(sizeof(1ull) == 8, "unsigned long long should be 64 bit");

#define GDT__ACCESSED ((uint64_t)(1ULL << 40))
#define GDT__WRITABLE ((uint64_t)(1ULL << 41))
#define GDT__CONFORMING ((uint64_t)(1ULL << 42))
#define GDT__EXECUTABLE ((uint64_t)(1ULL << 43))
#define GDT__USER_SEGMENT ((uint64_t)(1ULL << 44))
#define GDT__DPL_RING_3 ((uint64_t)(3ULL << 45))
#define GDT__PRESENT ((uint64_t)(1ULL << 47))
#define GDT__AVAILABLE ((uint64_t)(1ULL << 52))
#define GDT__LONG_MODE ((uint64_t)(1ULL << 53))
#define GDT__DEFAULT_SIZE ((uint64_t)(1ULL << 54))
#define GDT__GRANULARITY ((uint64_t)(1ULL << 55))
#define GDT__LIMIT_0_15 ((uint64_t)(0xffffULL))
#define GDT__LIMIT_16_19 ((uint64_t)(0xfULL << 48))
#define GDT__BASE_0_23 ((uint64_t)(0xffffffULL << 16))
#define GDT__BASE_24_31 ((uint64_t)(0xffULL << 56))
#define GDT__COMMON                                                            \
  (GDT__USER_SEGMENT | GDT__ACCESSED | GDT__PRESENT | GDT__WRITABLE |          \
   GDT__LIMIT_0_15 | GDT__LIMIT_16_19 | GDT__GRANULARITY)
#define GDT__KERNEL_CODE (GDT__COMMON | GDT__EXECUTABLE | GDT__LONG_MODE)
#define GDT__USER_CODE (GDT__KERNEL_CODE | GDT__DPL_RING_3)

_Static_assert(GDT__KERNEL_CODE == 0x00af9b000000ffffULL,
               "GDT__KERNEL_CODE has incorrect value");
_Static_assert(GDT__USER_CODE == 0x00affb000000ffffULL,
               "GDT__USER_CODE has incorrect value");

typedef struct {
  uint64_t *gdt;
  uint16_t size;
} GdtInfo;

typedef struct {
  uint64_t table[8];
  uint8_t index;
} Gdt;

GdtInfo current_gdt(void);
void load_gdt(Gdt *base, uint16_t selector);

Gdt Gdt__new(void);
uint16_t Gdt__add_entry(Gdt *gdt, uint64_t entry);
