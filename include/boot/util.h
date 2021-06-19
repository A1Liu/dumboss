#pragma once
#include "basics.h"
#include "gnu-efi/efi.h"
#include <stdbool.h>
#include <stdint.h>

EFI_FILE_HANDLE get_volume(EFI_HANDLE image);
Buffer read_file(EFI_FILE_HANDLE volume, CHAR16 *file_name);
void print(CHAR16 *str);
