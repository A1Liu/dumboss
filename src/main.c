#include "serial_communications_port.h"
#include <stdint.h>

void main(void) {
  init_serial();
  const char *hello = "Hello world!\n";
  for (int32_t i = 0; i < 16; i++)
    write_serial(hello[i]);
}
