#include "alloc.h"
#include "bootboot.h"
#include "descriptor_tables.h"
#include "memory.h"
#include <asm.h>
#include <basics.h>
#include <log.h>
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

__attribute__((interrupt, noreturn)) void Idt__double_fault(ExceptionStackFrame *frame,
                                                            u64 error_code) {
  log_fmt("double fault error_code: %f", error_code);
  Idt__log_fmt(frame);
  panic();
}

/******************************************
 * Entry point, called by BOOTBOOT Loader *
 ******************************************/
void _start(void) {
  /*** NOTE: BOOTBOOT runs _start on all cores in parallel ***/
  const uint16_t apic_id = asm_cpuid(1).ebx >> 24, bspid = bootboot.bspid;

  // ensure only one core does first bit
  while (apic_id != bspid)
    asm_hlt();

  log("--------------------------------------------------");
  log("                    BOOTING UP                    ");
  log("--------------------------------------------------");

  u32 gb_pages = asm_cpuid(0x80000001).edx & CPUID_PDPE1GB;
  if (gb_pages) log("Gb pages are enabled");

  MMap mmap = memory__init(&bootboot);
  alloc__init(mmap);

  GdtInfo gdt_info = current_gdt();

  log_fmt("GDT: %f %f", gdt_info.size, (u64)gdt_info.gdt);

  Gdt *gdt = alloc(1);
  *gdt = Gdt__new();
  u16 segment = Gdt__add_entry(gdt, GDT__KERNEL_CODE);
  load_gdt(gdt, segment);

  Idt *idt = Idt__new(alloc(1), 1 * _4KB);
  IdtEntry__set_handler(&idt->double_fault, Idt__double_fault);
  load_idt(idt);

  // divide_by_zero();

  void *hello = alloc(5);
  alloc__validate_heap();
  free(hello, 5);
  alloc__validate_heap();

  log_fmt("Kernel main end");
  shutdown();
}
