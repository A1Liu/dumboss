#include "alloc.h"
#include "asm.h"
#include "bootboot.h"
#include "logging.h"
#include "page_tables.h"
#include "serial_communications_port.h"
#include <stdint.h>

/******************************************
 * Entry point, called by BOOTBOOT Loader *
 ******************************************/
void _start() {
  /*** NOTE: BOOTBOOT runs _start on all cores in parallel ***/
  const cpuid_result result = asm_cpuid(1);

  // ensure only one core does first bit
  for (; bootboot.bspid != (result.ebx >> 24);)
    asm_hlt();

  log("--------------------------------------------------");
  log("                    BOOTING UP                    ");
  log("--------------------------------------------------");

  log("Logging ", 12, " to port ", 1);
  log_fmt("% %", "Hello", "world!");

  debug(12);
  debug(13, 14, 15);

  // Calculation described in bootboot specification
  int64_t entry_count = (bootboot.size - 128) / 16;
  entry_count = alloc__init(&bootboot.mmap, entry_count);

  for (;;)
    asm_hlt();
}
