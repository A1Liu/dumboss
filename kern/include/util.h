#pragma once
#include <macros.h>
#include <types.h>

static inline void asm_hlt(void) {
  asm volatile("hlt");
}

// Panic the kernel
_Noreturn void shutdown(void);
