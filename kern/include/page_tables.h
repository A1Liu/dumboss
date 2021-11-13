#pragma once
#include <types.h>

typedef struct PageTable4 PageTable4;

// Called in memory.c
void UNSAFE_HACKY_higher_half_init(void);

// Read the value of cr3
PageTable4 *get_page_table(void);

void *map_page(PageTable4 *_p4, u64 virtual_begin, void *kernel_begin);
