#include "boot/util.h"
#include "gnu-efi/efi.h"
#include "gnu-efi/efilib.h"

void print(CHAR16 *str) {
  ST->ConOut->OutputString(ST->ConOut, str);
  ST->ConIn->Reset(ST->ConIn, FALSE);
}

// https://wiki.osdev.org/Loading_files_under_UEFI
EFI_FILE_HANDLE get_volume(EFI_HANDLE image) {
  EFI_LOADED_IMAGE *loaded_image = NULL;             /* image interface */
  EFI_GUID lipGuid = EFI_LOADED_IMAGE_PROTOCOL_GUID; /* image interface GUID */
  EFI_FILE_IO_INTERFACE *IOVolume;                   /* file system interface */
  EFI_GUID fsGuid =
      EFI_SIMPLE_FILE_SYSTEM_PROTOCOL_GUID; /* file system interface GUID */
  EFI_FILE_HANDLE Volume;                   /* the volume's interface */

  /* get the loaded image protocol interface for our "image" */
  uefi_call_wrapper(BS->HandleProtocol, 3, image, &lipGuid,
                    (void **)&loaded_image);
  /* get the volume handle */
  uefi_call_wrapper(BS->HandleProtocol, 3, loaded_image->DeviceHandle, &fsGuid,
                    (VOID *)&IOVolume);
  uefi_call_wrapper(IOVolume->OpenVolume, 2, IOVolume, &Volume);
  return Volume;
}

Buffer read_file(EFI_FILE_HANDLE Volume, CHAR16 *file_name) {
  EFI_FILE_HANDLE file_handle;

  /* open the file */
  EFI_STATUS status = uefi_call_wrapper(
      Volume->Open, 5, Volume, &file_handle, file_name, EFI_FILE_MODE_READ,
      EFI_FILE_READ_ONLY | EFI_FILE_HIDDEN | EFI_FILE_SYSTEM);
  // TODO what do we do if the status isn't correct?

  status = EFI_SUCCESS;
  EFI_FILE_INFO *info_buffer = NULL;
  uint64_t buffer_size = sizeof(EFI_FILE_INFO) + 200;

  while (GrowBuffer(&status, (void **)&info_buffer, buffer_size)) {
    status = uefi_call_wrapper(file_handle->GetInfo, 4, file_handle,
                               &GenericFileInfo, &buffer_size, info_buffer);
  }

  uint64_t file_size = info_buffer->FileSize;
  uint8_t *file_buffer = AllocatePool(file_size);

  uefi_call_wrapper(file_handle->Read, 3, file_handle, &file_size, file_buffer);
  return (Buffer){.data = file_buffer, .size = file_size};
}

/*++
Below is Copyright (c) 1998  Intel Corporation
--*/

BOOLEAN GrowBuffer(EFI_STATUS *Status, VOID **Buffer, UINTN BufferSize) {
  BOOLEAN TryAgain;
  //
  // If this is an initial request, buffer will be null with a new buffer size
  //
  if (!*Buffer && BufferSize) {
    *Status = EFI_BUFFER_TOO_SMALL;
  }
  //
  // If the status code is "buffer too small", resize the buffer
  //

  TryAgain = FALSE;
  if (*Status == EFI_BUFFER_TOO_SMALL) {
    if (*Buffer) {
      FreePool(*Buffer);
    }
    *Buffer = AllocatePool(BufferSize);
    if (*Buffer) {
      TryAgain = TRUE;
    } else {
      *Status = EFI_OUT_OF_RESOURCES;
    }
  }
  //
  // If there's an error, free the buffer
  //
  if (!TryAgain && EFI_ERROR(*Status) && *Buffer) {
    FreePool(*Buffer);
    *Buffer = NULL;
  }
  return TryAgain;
}

void *AllocatePool(uint64_t size) {
  EFI_STATUS Status;
  VOID *p;
  Status = uefi_call_wrapper(BS->AllocatePool, 3, PoolAllocationType, size, &p);
  if (EFI_ERROR(Status))
    return NULL;

  return p;
}

void FreePool(void *Buffer) { uefi_call_wrapper(BS->FreePool, 1, Buffer); }
