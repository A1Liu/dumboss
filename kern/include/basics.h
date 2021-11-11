#pragma once
#include <macros.h>
#include <types.h>

s64 smallest_greater_power2(s64 value);

static inline void asm_hlt(void) {
  asm volatile("hlt");
}

// Panic the kernel
_Noreturn void shutdown(void);
void memcpy(void *dest, const void *src, s64 count);
void memset(void *buffer, u8 value, s64 len);
