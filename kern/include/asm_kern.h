#pragma once
#include <types.h>

typedef struct {
  u32 eax, ebx, ecx, edx;
} cpuid_result;

#define CPUID_PDPE1GB (U64(1) << 26)
static inline cpuid_result asm_cpuid(u32 code) {
  cpuid_result result;
  asm("cpuid" : "=a"(result.eax), "=b"(result.ebx), "=c"(result.ecx), "=d"(result.edx) : "0"(code));
  return result;
}

static inline u16 core_id(void) {
  return asm_cpuid(1).ebx >> 24;
}
