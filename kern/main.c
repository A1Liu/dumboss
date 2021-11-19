#include "bootboot.h"
#include "init.h"
#include "multitasking.h"
#include <asm.h>
#include <macros.h>

typedef struct {
  u32 eax, ebx, ecx, edx;
} cpuid_result;

#define CPUID_PDPE1GB (U64(1) << 26)
static inline cpuid_result asm_cpuid(u32 code) {
  cpuid_result result;
  asm("cpuid" : "=a"(result.eax), "=b"(result.ebx), "=c"(result.ecx), "=d"(result.edx) : "0"(code));
  return result;
}

static void divide_by_zero(void) {
  asm volatile("movq $0, %rdx; divq %rdx");
}

static void init(void) {
  log("--------------------------------------------------");
  log("                    BOOTING UP                    ");
  log("--------------------------------------------------");

  u32 gb_pages = asm_cpuid(0x80000001).edx & CPUID_PDPE1GB;
  if (gb_pages) log("gb pages are enabled");

  memory__init();

  descriptor__init();

  // divide_by_zero();

  return task_main();
}

/******************************************
 * Entry point, called by BOOTBOOT Loader *
 ******************************************/
void _start(void) {
  /*** NOTE: BOOTBOOT runs _start on all cores in parallel ***/
  const uint16_t apic_id = asm_cpuid(1).ebx >> 24, bspid = bb.bspid;

  if (apic_id == bspid) init();

  // ensure only one core is running
  while (true)
    asm_hlt();

  return task_main();
}
