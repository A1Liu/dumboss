#include "main.h"
#include <efi.h>

EFI_STATUS efi_main(EFI_HANDLE image_handle, EFI_SYSTEM_TABLE *system_table) {
  UINTN mapSize = 0, mapKey, descriptorSize;
  EFI_MEMORY_DESCRIPTOR *memoryMap = NULL;
  UINT32 descriptorVersion;
  // Get the required memory pool size for the memory map...
  EFI_STATUS result =
      uefi_call_wrapper(system_table->BootServices->GetMemoryMap, 5, &mapSize,
                        memoryMap, NULL, &descriptorSize, NULL);
  if (result != EFI_BUFFER_TOO_SMALL)
    return result;

  // Allocating the pool creates at least one new descriptor... for the chunk of
  // memory changed to EfiLoaderData Not sure that UEFI firmware must allocate
  // on a memory type boundry... if not, then two descriptors might be created
  mapSize += 2 * descriptorSize;
  // Get a pool of memory to hold the map...
  result = uefi_call_wrapper(system_table->BootServices->AllocatePool, 3,
                             EfiLoaderData, mapSize, (void **)&memoryMap);
  if (result != EFI_SUCCESS)
    return result;

  // Get the actual memory map...
  result = uefi_call_wrapper(system_table->BootServices->GetMemoryMap, 5,
                             &mapSize, memoryMap, &mapKey, &descriptorSize,
                             &descriptorVersion);
  if (result != EFI_SUCCESS)
    return result;

  result = uefi_call_wrapper(system_table->BootServices->ExitBootServices, 2,
                             image_handle, mapKey);
  if (result != EFI_SUCCESS)
    return result;

  main();

  for (;;)
    ;

  return EFI_SUCCESS;
}
