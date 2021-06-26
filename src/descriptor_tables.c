#include "descriptor_tables.h"
#include "asm.h"

// Used Phil Opperman's x86_64 rust code to figure out how to do this
// https://github.com/rust-osdev/x86_64/blob/master/src/structures/idt.rs

uint16_t IdtEntry__minimal_options(void) { return 0xe00; }

static uint16_t IdtEntry__set_present(uint16_t opts) {
  return opts | (1 << 15);
}

uint64_t handler_addr(IdtEntry entry) {
  return ((uint64_t)entry.pointer_low) |
         (((uint64_t)entry.pointer_middle) << 16) |
         (((uint64_t)entry.pointer_high) << 32);
}

IdtEntry IdtEntry__set_handler(IdtEntry entry, void *handler) {
  uint64_t addr = (uint64_t)handler;
  entry.pointer_low = (uint16_t)addr;
  entry.pointer_middle = (uint16_t)(addr >> 16);
  entry.pointer_high = (uint16_t)(addr >> 32);
  // entry.gdt_selector = CS::get_reg() .0;
  entry.options = IdtEntry__set_present(entry.options);

  return entry;
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
