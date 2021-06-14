#include "main.h"
#include "logging.h"
#include "serial_communications_port.h"
#include <stdint.h>

void main(MemoryMap memory_map) {
  asm("cli"); // clear interrupts

  page_tables__init(memory_map);
  serial__init();

  log("Logging ", 12, " to port ", 1);
  log_fmt("Hello peeps");
  log_fmt("Hello %", "world");
  log_fmt("% %", "Hello", "world!");
}
