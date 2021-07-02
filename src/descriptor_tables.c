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

// This disables IRQs by default
static uint16_t IdtEntry__minimal_options(void) {
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

#undef IdtEntry__set_handler
static void IdtEntry__set_handler(IdtEntry *entry, void *handler) {
  uint64_t addr = (uint64_t)handler;
  entry->pointer_low = (uint16_t)addr;
  entry->pointer_middle = (uint16_t)(addr >> 16);
  entry->pointer_high = (uint32_t)(addr >> 32);
  entry->gdt_selector = read_register(cs, uint16_t);
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

void load_gdt(Gdt *base, uint16_t selector) {
  uint16_t size = base->index * sizeof(uint64_t) - 1;
  struct {
    uint16_t size;
    void *base;
  } __attribute__((packed)) GDTR = {.size = size, .base = base};

  // let the compiler choose an addressing mode
  asm volatile("lgdt %0" : : "m"(GDTR));

  extern void _x86_64_asm_set_cs(uint64_t);
  _x86_64_asm_set_cs(selector);
  // asm volatile("pushq %0; leaq 1f(%%rip), %%rax; pushq %%rax; lretq; 1:"
  //              :
  //              : "r"(tmp)
  //              : "rax");
}

Gdt Gdt__new(void) {
  Gdt gdt;
  gdt.table[0] = 0;
  gdt.index = 1;
  return gdt;
}

uint16_t Gdt__add_entry(Gdt *gdt, uint64_t entry) {
  uint8_t index = gdt->index;
  assert(index < 8);

  gdt->table[index] = entry;
  gdt->index = index + 1;

  uint8_t priviledge_level = (entry & GDT__DPL_RING_3) ? 3 : 0;
  return (uint16_t)((index << 3) | priviledge_level);
}

void divide_by_zero(void) {
  asm volatile("movq $0, %rdx; divq %rdx");
}
