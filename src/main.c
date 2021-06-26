#include "bootboot.h"
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

/******************************************
 * Entry point, called by BOOTBOOT Loader *
 ******************************************/
void _start(void) {
  /*** NOTE: BOOTBOOT runs _start on all cores in parallel ***/
  const cpuid_result result = asm_cpuid(1);

  // ensure only one core does first bit
  for (; bootboot.bspid != (result.ebx >> 24);)
    asm_hlt();

  log("--------------------------------------------------");
  log("                    BOOTING UP                    ");
  log("--------------------------------------------------");

  // Calculation described in bootboot specification
  int64_t entry_count = (bootboot.size - 128) / 16;
  entry_count = alloc__init(&bootboot.mmap, entry_count);

  void *hello = alloc(5);
  alloc__validate_heap();
  free(hello, 3);
  alloc__validate_heap();

  log_fmt("Kernel main end");

  for (;;)
    asm_hlt();
}
