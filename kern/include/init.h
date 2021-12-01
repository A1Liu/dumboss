#include <basics.h>

// Defined in memory.c
extern Bump InitAlloc;

void memory__init(void);
void descriptor__init(void);
void tasks__init(void);

// Defined in descriptor_tables.c
u16 tss_segment(s64 core_idx);
