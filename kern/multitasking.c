#include "multitasking.h"
#include <macros.h>

_Noreturn void task_main(void) {
  log_fmt("Kernel main end");
  exit(0);
}
