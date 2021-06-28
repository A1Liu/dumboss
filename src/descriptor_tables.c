#include "descriptor_tables.h"
#include "logging.h"

// Used Phil Opperman's x86_64 rust code to figure out how to do this
// https://github.com/rust-osdev/x86_64/blob/master/src/structures/idt.rs

Idt *Idt__new(void *buffer, int64_t size) {
  assert(size >= 0);
  assert((uint64_t)size >= sizeof(Idt));
  assert(sizeof(Idt) / sizeof(IdtEntry) == 256);

  IdtEntry *entries = (IdtEntry *)buffer;
  for (int64_t i = 0; i < 256; i++) {
    entries[i] = IdtEntry__missing();
  }

  return (Idt *)buffer;
}

void Idt__log_fmt(ExceptionStackFrame *frame) {
  log_fmt("ExceptionStackFrame{ip=%,cs=%,flags=%,sp=%,ss=%}",
          frame->instruction_pointer, frame->code_segment, frame->cpu_flags,
          frame->stack_pointer, frame->stack_segment);
}

uint16_t IdtEntry__minimal_options(void) {
  return 0xe00;
}

static uint16_t IdtEntry__set_present(uint16_t opts) {
  return opts | (1 << 15);
}

uint64_t IdtEntry__handler_addr(IdtEntry entry) {
  return ((uint64_t)entry.pointer_low) |
         (((uint64_t)entry.pointer_middle) << 16) |
         (((uint64_t)entry.pointer_high) << 32);
}

static inline uint16_t asm_cs_reg(void) {
  uint16_t value = 0;
  asm("mov %%cs, %0" : "=r"(value));
  return value;
}

#undef IdtEntry__set_handler
static void IdtEntry__set_handler(IdtEntry *entry, void *handler) {
  uint64_t addr = (uint64_t)handler;
  entry->pointer_low = (uint16_t)addr;
  entry->pointer_middle = (uint16_t)(addr >> 16);
  entry->pointer_high = (uint32_t)(addr >> 32);
  entry->gdt_selector = asm_cs_reg();
  entry->options = IdtEntry__set_present(entry->options);
}

void IdtEntry__Default__set_handler(IdtEntry *entry, Idt__Handler handler) {
  IdtEntry__set_handler(entry, handler);
}

void IdtEntry__ForExt__set_handler(IdtEntry__ForExt *entry,
                                   Idt__HandlerExt handler) {
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

IdtEntry IdtEntry__missing(void) {
  return (IdtEntry){
      .pointer_low = 0,
      .gdt_selector = 0,
      .options = IdtEntry__minimal_options(),
      .pointer_middle = 0,
      .pointer_high = 0,
      .reserved = 0,
  };
}

void divide_by_zero(void) {
  asm volatile("movq $0, %rdx; divq %rdx");
}
