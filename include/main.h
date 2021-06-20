#pragma once

#include "basics.h"
#include "gnu-efi/efi.h"
#include "kernel/page_tables.h"

typedef struct {
  Buffer kernel;
  MemoryMap memory_map;
} KernelInfo;

typedef void (*kernel_entry)(const KernelInfo *info);
