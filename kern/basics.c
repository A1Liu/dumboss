#include "basics.h"
#include "alloc.h"
#include <log.h>

// Cobbled together from stack overflow and some previous project.

s64 smallest_greater_power2(s64 _value) {
  assert(_value >= 0);
  u64 value = (u64)_value;

  if (value <= 1) return 0;
  return 64 - __builtin_clzl(value - 1);
}

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

void memcpy(void *_dest, const void *_src, s64 count) {
  u8 *dest = _dest;
  const u8 *src = _src;
  FOR_PTR(src, count) {
    dest[index] = *it;
  }
}

void memset(void *_buffer, u8 byte, s64 len) {
  u8 *buffer = _buffer;
  for (s64 i = 0; i < len; i++)
    buffer[i] = byte;
}
