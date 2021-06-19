#include "main.h"
#include "logging.h"
#include "serial_communications_port.h"
#include <stdint.h>

void main(KernelInfo *info) {
  kernel_entry _kmain =
      main; // statically assert the function signature of main
  (void)_kmain;

  serial__init();

  log("--------------------------------------------------");
  log("                    BOOTING UP");
  log("--------------------------------------------------");

  log_fmt("Kernel starts at % and is % bytes", (uint64_t)info->kernel.data,
          info->kernel.size);
  log("Logging ", 12, " to port ", 1);
  log_fmt("% %", "Hello", "world!");
}
