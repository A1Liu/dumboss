#pragma once

#include "basics.h"
#include "gnu-efi/efi.h"
#include "page_tables.h"

typedef struct {
  Buffer kernel;
} KernelInfo;

typedef void (*kernel_entry)(KernelInfo *info);
