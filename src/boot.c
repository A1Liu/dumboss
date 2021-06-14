#include "main.h"
#include "page_tables.h"
#include <efi.h>

EFI_STATUS efi_main(EFI_HANDLE image_handle, EFI_SYSTEM_TABLE *system_table) {
  UINTN map_size = 0, descriptor_size;
  // Get the required memory pool size for the memory map...
  EFI_STATUS result =
      uefi_call_wrapper(system_table->BootServices->GetMemoryMap, 5, &map_size,
                        NULL, NULL, &descriptor_size, NULL);
  if (result != EFI_BUFFER_TOO_SMALL)
    return result;

  // Allocating the pool creates at least one new descriptor... for the chunk of
  // memory changed to EfiLoaderData Not sure that UEFI firmware must allocate
  // on a memory type boundry... if not, then two descriptors might be created
  map_size += 2 * descriptor_size;

  // Get a pool of memory to hold the map...
  EFI_MEMORY_DESCRIPTOR *buffer = NULL;
  result = uefi_call_wrapper(system_table->BootServices->AllocatePool, 3,
                             EfiLoaderData, map_size, (void **)&buffer);
  if (result != EFI_SUCCESS)
    return result;

  // Get the actual memory map...
  UINTN map_key;
  UINT32 descriptor_version;
  result = uefi_call_wrapper(system_table->BootServices->GetMemoryMap, 5,
                             &map_size, buffer, &map_key, &descriptor_size,
                             &descriptor_version);
  if (result != EFI_SUCCESS)
    return result;

  result = uefi_call_wrapper(system_table->BootServices->ExitBootServices, 2,
                             image_handle, map_key);
  if (result != EFI_SUCCESS)
    return result;

  main(MemoryMap__new(map_size, descriptor_size, buffer));

  for (;;)
    asm("hlt");

  return EFI_SUCCESS;
}
