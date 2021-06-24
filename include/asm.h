#pragma once
#include <stdint.h>

// https://wiki.osdev.org/Inline_Assembly/Examples
typedef struct {
  uint32_t eax, ebx, ecx, edx;
} cpuid_result;

static inline cpuid_result asm_cpuid(int32_t code) {
  cpuid_result result;
  asm("cpuid"
      : "=a"(result.eax), "=b"(result.ebx), "=c"(result.ecx), "=d"(result.edx)
      : "0"(code));
  return result;
}

static inline void asm_outb(uint16_t port, uint8_t val) {
  asm volatile("outb %0, %1" : : "a"(val), "Nd"(port));
}

static inline void asm_outw(uint16_t port, uint8_t val) {
  asm volatile("outw %0, %1" : : "a"(val), "Nd"(port));
}

static inline void asm_outl(uint16_t port, uint32_t val) {
  asm volatile("outl %0, %1" : : "a"(val), "Nd"(port));
}

static inline uint8_t asm_inb(uint16_t port) {
  uint8_t ret;
  asm volatile("inb %1, %0" : "=a"(ret) : "Nd"(port));
  return ret;
}

static inline uint16_t asm_inw(uint16_t port) {
  uint8_t ret;
  asm volatile("inw %1, %0" : "=a"(ret) : "Nd"(port));
  return ret;
}

static inline uint32_t asm_inl(uint16_t port) {
  uint8_t ret;
  asm volatile("inl %1, %0" : "=a"(ret) : "Nd"(port));
  return ret;
}

static inline void asm_hlt(void) { asm volatile("hlt"); }
