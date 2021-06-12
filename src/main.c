#include "logging.h"
#include "serial_communications_port.h"
#include <stdint.h>

void main(void) {
  serial__init();

  const char *hello = "Hello world!\n";
  for (int32_t i = 0; hello[i]; i++)
    serial__write(hello[i]);

  any a = make_any("Logging");
  panic();
  (void)a;
  logging__log(__LOC__, 1, &a);

  // log("Logging");
  // panic();

  // log_fmt("Hello peeps");
  // log_fmt("Hello %", "world");
  // panic();
  // log_fmt("% %", "Hello", "world!");
}
