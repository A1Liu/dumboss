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
  debug(13, bootboot.bspid, 15);

  for (uint64_t i = 0; i < 66; i++) {
    debug(i, align_up(i, 2));
  }

  // Calculation described in bootboot specification
  int64_t entry_count = (bootboot.size - 128) / 16;
  entry_count = alloc__init(&bootboot.mmap, entry_count);

  log_fmt("Done initializing alloc!");

  void *data_1 = alloc(1);
  assert(data_1 != NULL);

  void *data_2 = alloc(16);
  assert(data_2 != NULL);
  log_fmt("% at % and % at %", 1 * _4KB, (uint64_t)data_1, 16 * _4KB,
          (uint64_t)data_2);

  alloc__validate_heap();
  free(data_1, 1);
  alloc__validate_heap();
  free(data_2, 16);
  alloc__validate_heap();

  alloc__assert_heap_empty();

  log_fmt("Kernel main ened");
  for (;;)
    asm_hlt();
}
