#include "descriptor_tables.h"
#include <asm.h>
#include <basics.h>
#include <macros.h>

// Used Phil Opperman's x86_64 rust code to figure out how to do this
// https://github.com/rust-osdev/x86_64/blob/master/src/structures/idt.rs

Idt *Idt__new(void *buffer, s64 size) {
  assert(size >= 0);
  assert((u64)size >= sizeof(Idt));
  assert(sizeof(Idt) / sizeof(IdtEntry) == 256);

  IdtEntry *entries = (IdtEntry *)buffer;
  for (s64 i = 0; i < 256; i++) {
    entries[i] = IdtEntry__missing();
  }

  return (Idt *)buffer;
}

void Idt__log_fmt(ExceptionStackFrame *frame) {
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

// Used Phil Opperman's x86_64 rust code to figure out how to do this
// https://github.com/rust-osdev/x86_64/blob/master/src/structures/gdt.rs
// https://github.com/rust-osdev/x86_64/blob/master/src/instructions/segmentation.rs

// TODO the full definition of the GDT should probably be entirely in this file.

GdtInfo current_gdt(void) {
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

void load_gdt(Gdt *base, u16 selector) {
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

Gdt Gdt__new(void) {
  Gdt gdt;
  gdt.table[0] = 0;
  gdt.index = 1;
  return gdt;
}

u16 Gdt__add_entry(Gdt *gdt, u64 entry) {
  uint8_t index = gdt->index;
  assert(index < 8);

  gdt->table[index] = entry;
  gdt->index = index + 1;

  uint8_t priviledge_level = (entry & GDT__DPL_RING_3) ? 3 : 0;
  return (u16)((index << 3) | priviledge_level);
}

void divide_by_zero(void) {
  asm volatile("movq $0, %rdx; divq %rdx");
}
