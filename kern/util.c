#include "util.h"
#include "alloc.h"
#include <basics.h>
#include <log.h>
#include <macros.h>

static inline void asm_outw(u16 port, u16 val) {
  asm volatile("outw %0, %1" : : "a"(val), "Nd"(port));
}

_Noreturn void shutdown(void) {
  // Cause immediate shutdown when in a virtual machine
  // https://wiki.osdev.org/Shutdown
  asm_outw(0xB004, 0x2000);
  asm_outw(0x604, 0x2000);
  asm_outw(0x4004, 0x3400);

  for (;;)
    asm_hlt();
}
