#include "alloc.h"
#include "asm.h"
#include "bootboot.h"
#include "logging.h"
#include "page_tables.h"
#include "serial_communications_port.h"
#include "sync.h"

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
