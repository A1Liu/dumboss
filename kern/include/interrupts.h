#pragma once
#include <types.h>

typedef struct {
  u64 instruction_pointer;
  u64 code_segment;
  u64 cpu_flags;
  u64 stack_pointer;
  u64 stack_segment;
} ExceptionStackFrame;

#define HANDLER       __attribute__((interrupt)) void
#define NORET_HANDLER __attribute__((noreturn, interrupt)) void

void load_idt(void);
