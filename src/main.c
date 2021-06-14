#include "main.h"
#include "logging.h"
#include "serial_communications_port.h"
#include <stdint.h>

void main(void) {
  serial__init();

  const char *hello = "Hello world!\n";
  for (int32_t i = 0; hello[i]; i++)
    serial__write(hello[i]);

  log("Logging");

  log_fmt("Hello peeps");
  log_fmt("Hello %", "world");
  log_fmt("% %", "Hello", "world!");
}
