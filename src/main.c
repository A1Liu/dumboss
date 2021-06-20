#include "main.h"
#include "kernel/serial_communications_port.h"
#include "logging.h"
#include <stdint.h>

void main(const KernelInfo *info) {
  kernel_entry _kmain =
      main; // statically assert the function signature of main
  (void)_kmain;

  serial__init();
  page_tables__init(&info->memory_map);

  log("--------------------------------------------------");
  log("                    BOOTING UP                    ");
  log("--------------------------------------------------");

  log_fmt("Kernel starts at % and is % bytes", (uint64_t)info->kernel.data,
          info->kernel.size);
  log("Logging ", 12, " to port ", 1);
  log_fmt("% %", "Hello", "world!");
}
