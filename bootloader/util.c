#include "boot/util.h"
#include "gnu-efi/efi.h"

const EFI_FILE_INFO *LibFileInfo(EFI_FILE_HANDLE FHand) {
  EFI_STATUS Status;
  EFI_FILE_INFO *Buffer;
  UINTN BufferSize;

  //
  // Initialize for GrowBuffer loop
  //

  Status = EFI_SUCCESS;
  Buffer = NULL;
  BufferSize = SIZE_OF_EFI_FILE_INFO + 200;

  //
  // Call the real function
  //
  (void)FHand;

  // while (GrowBuffer(&Status, (VOID **)&Buffer, BufferSize)) {
  //   Status = uefi_call_wrapper(FHand->GetInfo, 4, FHand, &GenericFileInfo,
  //                              &BufferSize, Buffer);
  // }

  return Buffer;
}
