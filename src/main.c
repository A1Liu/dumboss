#include "bootboot.h"
#include "descriptor_tables.h"
#include "logging.h"
#include "memory.h"

typedef struct {
  uint32_t eax, ebx, ecx, edx;
} cpuid_result;

static inline cpuid_result asm_cpuid(int32_t code) {
  cpuid_result result;
  asm("cpuid"
      : "=a"(result.eax), "=b"(result.ebx), "=c"(result.ecx), "=d"(result.edx)
      : "0"(code));
  return result;
}

__attribute__((interrupt, noreturn)) void
Idt__double_fault(ExceptionStackFrame *frame, uint64_t error_code) {
  log_fmt("double fault error_code: %", error_code);
  Idt__log_fmt(frame);
  panic();
}

/******************************************
 * Entry point, called by BOOTBOOT Loader *
 ******************************************/
void _start(void) {
  /*** NOTE: BOOTBOOT runs _start on all cores in parallel ***/
  const cpuid_result result = asm_cpuid(1);

  // ensure only one core does first bit
  while (bootboot.bspid != (result.ebx >> 24))
    asm_hlt();

  log("--------------------------------------------------");
  log("                    BOOTING UP                    ");
  log("--------------------------------------------------");

  // Calculation described in bootboot specification
  int64_t entry_count = (bootboot.size - 128) / 16;
  entry_count = alloc__init(&bootboot.mmap, entry_count);

  uint64_t cs = read_register(cs);
  dbg(cs, 1);
  dbg(write_register(rax, 12));
  dbg(read_register(rax));

  Idt *idt = Idt__new(alloc(1), 1 * _4KB);
  IdtEntry__set_handler(&idt->double_fault, Idt__double_fault);
  asm_lidt(idt);

  // divide_by_zero();

  void *hello = alloc(5);
  alloc__validate_heap();
  free(hello, 3);
  alloc__validate_heap();

  log_fmt("Kernel main end");
  shutdown();
}
