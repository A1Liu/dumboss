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
} __attribute__((packed)) IdtEntry;

typedef struct {
  IdtEntry inner;
} __attribute__((packed)) IdtEntry__ForExt;
typedef struct {
  IdtEntry inner;
} __attribute__((packed)) IdtEntry__ForDiverging;
typedef struct {
  IdtEntry inner;
} __attribute__((packed)) IdtEntry__ForDivergingExt;

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
} __attribute__((packed)) Idt;

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

static inline void asm_lidt(Idt *base) {
  struct {
    uint16_t size;
    void *base;
  } __attribute__((packed)) IDTR = {.size = sizeof(Idt) - 1, .base = base};

  asm("lidt %0" : : "m"(IDTR)); // let the compiler choose an addressing mode
}

Idt *Idt__new(void *buffer, int64_t size);
void Idt__log_fmt(ExceptionStackFrame *frame);

IdtEntry IdtEntry__missing(void);
uint64_t IdtEntry__handler_addr(IdtEntry entry);

// clang-format off
#define IdtEntry__set_handler(entry, handler)                                  \
  _Generic((entry),                                                            \
      IdtEntry*                   : IdtEntry__Default__set_handler,            \
      IdtEntry__ForExt*           : IdtEntry__ForExt__set_handler,             \
      IdtEntry__ForDiverging*     : IdtEntry__ForDiverging__set_handler,       \
      IdtEntry__ForDivergingExt*  : IdtEntry__ForDivergingExt__set_handler     \
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
