#include "basics.h"
#include "boot/util.h"
#include "gnu-efi/efi.h"
#include "gnu-efi/efilib.h"
#include "main.h"
#include "page_tables.h"

#define BUF_SIZE 100

static CHAR16 wchar_buffer[BUF_SIZE];
static char temp_buffer[BUF_SIZE];

uint64_t wfmt_u64(uint64_t value, uint64_t index) {

  uint64_t written = fmt_u64(value, temp_buffer, BUF_SIZE - index);
  if (written + index > BUF_SIZE)
    return written;
  for (uint64_t i = 0; i < written; i++)
    wchar_buffer[i + index] = (CHAR16)temp_buffer[i];

  return written;
}

EFI_STATUS efi_main(EFI_HANDLE image_handle, EFI_SYSTEM_TABLE *system_table) {
  ST = system_table;
  BS = system_table->BootServices;

  EFI_FILE_HANDLE volume = get_volume(image_handle);
  Buffer kernel = read_file(volume, L"kernel");
  // uint8_t values[] = {0x12, 0x13, 0x11, 0x15};
  // for (uint32_t i = 0; i < sizeof(values) && values[i] == kernel.data[i];
  // i++) {
  //   print(L"Bruh\r\n");
  // }

  uint64_t written = wfmt_u64(kernel.data[0], 0);
  wchar_buffer[written++] = L'\r';
  wchar_buffer[written++] = L'\n';
  wchar_buffer[written++] = L'\0';
  print(wchar_buffer);
  print(L"Hello\r\n");
  print(L"Goodbye\r\n");

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

  // print(L"Passedena\r\n");
  result = uefi_call_wrapper(system_table->BootServices->ExitBootServices, 2,
                             image_handle, map_key);
  if (result != EFI_SUCCESS)
    return result;

  main_func f = (void *)kernel.data;
  f();
  // goto *(void *)kernel.data;
  // asm volatile("jmpq *%0" : : "X"(kernel.data));

  // goto *(void *)kernel.data;

  for (;;)
    asm("hlt");
}
