#pragma once
#include "gnu-efi/efi.h"
#include <stdint.h>

typedef struct {
  const uint64_t size;
  const uint64_t descriptor_size;
  const EFI_MEMORY_DESCRIPTOR *buffer;
} MemoryMap;

static inline MemoryMap
MemoryMap__new(uint64_t size, uint64_t descriptor_size,
               const EFI_MEMORY_DESCRIPTOR *memory_map) {
  return (MemoryMap){
      .size = size, .descriptor_size = descriptor_size, .buffer = memory_map};
}

void page_tables__init(const MemoryMap *memory_map);
