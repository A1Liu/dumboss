#include "kernel/asm.h"
#include "kernel/bootboot.h"
#include "kernel/page_tables.h"
#include "kernel/serial_communications_port.h"
#include "logging.h"
#include <stdint.h>

/******************************************
 * Entry point, called by BOOTBOOT Loader *
 ******************************************/
void _start() {
  /*** NOTE: BOOTBOOT runs _start on all cores in parallel ***/
  cpuid_result result = asm_cpuid(1);

  // ensure only one core does first bit
  if (bootboot.bspid != (result.ebx >> 24)) {
    for (;;)
      asm_hlt();
  }

  log("--------------------------------------------------");
  log("                    BOOTING UP                    ");
  log("--------------------------------------------------");

  log("Logging ", 12, " to port ", 1);
  log_fmt("% %", "Hello", "world!");

  page_tables__init();

  for (;;)
    asm_hlt();
}
