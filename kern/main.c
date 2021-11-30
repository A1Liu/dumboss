#include "asm.h"
#include "bootboot.h"
#include "init.h"
#include "multitasking.h"
#include <macros.h>

static void init(void) {
  log("--------------------------------------------------");
  log("                    BOOTING UP                    ");
  log("--------------------------------------------------");

  u32 gb_pages = asm_cpuid(0x80000001).edx & CPUID_EDX_PDPE1GB;
  if (gb_pages) log("gb pages are enabled");

  u32 apic_enabled = asm_cpuid(1).edx & CPUID_EDX_APIC;
  assert(apic_enabled, "APIC was not enabled");
  log("APIC is enabled");

  memory__init();

  descriptor__init();

  tasks__init();

  return task_begin();
}

/******************************************
 * Entry point, called by BOOTBOOT Loader *
 ******************************************/
void _start(void) {
  /*** NOTE: BOOTBOOT runs _start on all cores in parallel ***/
  if (core_id() == bb.bspid) init();

  // ensure only one core is running
  while (true) {
    asm_hlt();
    pause();
  }

  return task_begin();
}
