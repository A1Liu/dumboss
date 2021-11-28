#include <basics.h>

// Defined in memory.c
extern Bump InitAlloc;

void memory__init(void);
void descriptor__init(void);
void tasks__init(void);

// Defined in descriptor_tables.c
u16 tss_segment(s64 core_idx);
void load_idt(void);

typedef struct {
  u64 instruction_pointer;
  u64 code_segment;
  u64 cpu_flags;
  u64 stack_pointer;
  u64 stack_segment;
} ExceptionStackFrame;

#define HANDLER       __attribute__((interrupt)) void
#define NORET_HANDLER __attribute__((noreturn, interrupt)) void
