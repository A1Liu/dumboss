#include "page_tables.h"
#include "logging.h"
#include <efi.h>

#define PT_SIZE 512
const char *efi_memory_type[] = {
    "EfiReservedMemoryType",
    "EfiLoaderCode",
    "EfiLoaderData",
    "EfiBootServicesCode",
    "EfiBootServicesData",
    "EfiRuntimeServicesCode",
    "EfiRuntimeServicesData",
    "EfiConventionalMemory",
    "EfiUnusableMemory",
    "EfiACPIReclaimMemory",
    "EfiACPIMemoryNVS",
    "EfiMemoryMappedIO",
    "EfiMemoryMappedIOPortSpace",
    "EfiPalCode",
};

static inline const EFI_MEMORY_DESCRIPTOR *
MemoryMap__get(const MemoryMap *memory_map, uint64_t elem) {
  uint8_t *buffer_begin = (uint8_t *)memory_map->buffer;
  uint64_t buffer_index = elem * memory_map->descriptor_size;
  if (buffer_index > memory_map->size - memory_map->descriptor_size)
    return NULL;

  return (EFI_MEMORY_DESCRIPTOR *)&buffer_begin[buffer_index];
}

static PTE global_table[PT_SIZE];

void page_tables__init(MemoryMap memory_map) {
  uint64_t index = 0;
  const EFI_MEMORY_DESCRIPTOR *descriptor;
  while ((descriptor = MemoryMap__get(&memory_map, index++)) != NULL) {
    log_fmt(
        "%: EFI_MEMORY_DESCRIPTOR { Type=%, PhysicalStart=%, VirtualStart=%, "
        "NumberOfPages=%, Attribute=% }",
        index - 1, efi_memory_type[descriptor->Type], descriptor->PhysicalStart,
        descriptor->VirtualStart, descriptor->NumberOfPages,
        descriptor->Attribute);

    if (descriptor->Attribute != 15) {
      log_fmt("descriptor->Attribute = %", descriptor->Attribute);
      panic();
    }
  }

  for (int32_t i = 0; i < PT_SIZE; i++)
    global_table[i].address = 0;
}
