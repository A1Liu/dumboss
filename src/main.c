#include "main.h"
#include "logging.h"
#include "serial_communications_port.h"
#include <stdint.h>

void main(void) {
  asm("cli"); // clear interrupts

  serial__init();

  log("Logging ", 12, " to port ", 1);
  log_fmt("Hello peeps");
  log_fmt("Hello %", "world");
  log_fmt("% %", "Hello", "world!");
}
