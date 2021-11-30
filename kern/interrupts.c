#include "asm.h"
#include "init.h"
#include "memory.h"
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

// Used Phil Opperman's x86_64 rust code to figure out how to do this
// https://github.com/rust-osdev/x86_64/blob/master/src/structures/idt.rs
static void Idt__log_fmt(ExceptionStackFrame *frame);
static u64 IdtEntry__handler_addr(IdtEntry entry);
static inline IdtEntry IdtEntry__missing(void);
static u16 IdtEntry__set_IST_index(u16 opts, u8 idx);

static void cpu_set_apic_base(void *ptr);
static void *cpu_get_apic_base(void);

static NORET_HANDLER Idt__double_fault(ExceptionStackFrame *frame, u64 error_code);

void load_idt(void) {
  Idt *idt = Bump__bump(&InitAlloc, Idt);
  assert(idt);

  IdtEntry *entries = (IdtEntry *)idt;
  for (s64 i = 0; i < 256; i++) {
    entries[i] = IdtEntry__missing();
  }

  IdtEntry__set_handler(&idt->double_fault, Idt__double_fault);

  struct {
    u16 size;
    void *idt;
  } __attribute__((packed)) IDTR = {.size = sizeof(Idt) - 1, .idt = idt};

  // let the compiler choose an addressing mode
  asm volatile("lidt %0" : : "m"(IDTR));

  // OSDev describes how to do this: https://wiki.osdev.org/APIC

  // TODO remap interrupts

  // TODO set up local APIC stuff I guess (first step is serial interrupts)
}

#define IA32_APIC_BASE_MSR        0x1B
#define IA32_APIC_BASE_MSR_BSP    0x100 // Processor is a BSP
#define IA32_APIC_BASE_MSR_ENABLE 0x800
static void cpu_set_apic_base(void *ptr) {
  u64 apic = physical_address(ptr);
  cpuSetMSR(IA32_APIC_BASE_MSR, (apic & 0xffffff000) | IA32_APIC_BASE_MSR_ENABLE);
}

static void *cpu_get_apic_base() {
  u64 apic = cpuGetMSR(IA32_APIC_BASE_MSR) & 0xffffff000;
  return kernel_ptr(apic);
}

/*
--------------------------------------------------------------------------------


                      The actual implementations of handlers


--------------------------------------------------------------------------------
*/

static NORET_HANDLER Idt__double_fault(ExceptionStackFrame *frame, u64 error_code) {
  log_fmt("double fault error_code: %f", error_code);
  Idt__log_fmt(frame);
  log_fmt("double fault stack location: %f", (u64)&error_code);
  panic();
}

static void Idt__log_fmt(ExceptionStackFrame *frame) {
  log_fmt("ExceptionStackFrame{ip=%f,cs=%f,flags=%f,sp=%f,ss=%f}", frame->instruction_pointer,
          frame->code_segment, frame->cpu_flags, frame->stack_pointer, frame->stack_segment);
}

static u16 IdtEntry__set_present(u16 opts) {
  return opts | (1 << 15);
}

static u16 IdtEntry__set_IST_index(u16 opts, u8 idx) {
  assert(idx < 8);
  return (opts & ~U16(7)) | U16(idx);
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
  entry->options = IdtEntry__set_IST_index(entry->options, 1);
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
