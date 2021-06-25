#pragma once
#include "bootboot.h"
#include <stdint.h>

void page_tables__init(MMapEnt *entries, int64_t entry_count);
