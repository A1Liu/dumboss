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
  IdtEntry divide_error;
  IdtEntry debug;
  IdtEntry non_maskable_interrupt;
  IdtEntry breakpoint;
  IdtEntry overflow;
  IdtEntry bound_range_exceeded;
  IdtEntry invalid_opcode;
  IdtEntry device_not_available;
  IdtEntry double_fault;
  IdtEntry coprocessor_segment_overrun;
  IdtEntry invalid_tss;
  IdtEntry segment_not_present;
  IdtEntry stack_segment_fault;
  IdtEntry general_protection_fault;
  IdtEntry page_fault;
  IdtEntry reserved_1;
  IdtEntry x87_floating_point;
  IdtEntry alignment_check;
  IdtEntry machine_check;
  IdtEntry simd_floating_point;
  IdtEntry virtualization;
  IdtEntry reserved_2[9];
  IdtEntry security_exception;
  IdtEntry reserved_3;
  IdtEntry interrupts[];
} __attribute__((packed)) Idt;

uint16_t IdtEntry__minimal_options(void);
IdtEntry IdtEntry__missing(void);
