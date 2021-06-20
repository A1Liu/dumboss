#pragma once

#include "basics.h"
#include "gnu-efi/efi.h"

typedef struct {
  Buffer kernel;
} KernelInfo;

typedef void (*kernel_entry)(KernelInfo *info);
