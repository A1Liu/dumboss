#include "logging.h"

int main(int argc, char *argv[]) {
  log_fmt("argc=%", argc);
  for (int i = 0; i < argc; i++)
    log_fmt("argv[%]=%", i, argv[i]);

  // panic();
  logging__panic(__LOC__, "Dockerfile");
}
