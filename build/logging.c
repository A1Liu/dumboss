#include "logging.h"
#include <stdio.h>

#define BUF_SIZE 200

static char buffer[BUF_SIZE];

void logging__log(sloc loc, uint32_t count, any *args) {
  String out = string__new(buffer, BUF_SIZE);
  int64_t written = write_prefix_to_buffer(out, loc);

  for (uint32_t i = 0; i < count; i++) {
    int64_t fmt_try =
        fmt__fmt_any(string__suffix(out, min(written, BUF_SIZE)), args[i]);
    if (fmt_try < 0)
      panic();
    written += fmt_try;
  }

  if (written > BUF_SIZE) { // TODO expand buffer
    const char *suffix = "... [clipped]";
    strcpy_s(string__suffix(out, (int64_t)(out.size - strlen(suffix))), suffix);
    written = BUF_SIZE;
  }

  printf("%.*s\n", (int)out.size, out.data);
}

void logging__log_fmt(sloc loc, const char *fmt, uint32_t count, any *args) {
  String out = string__new(buffer, BUF_SIZE);
  int64_t written = write_prefix_to_buffer(out, loc);
  int64_t fmt_try = fmt__fmt(string__suffix(out, written), fmt, count, args);
  if (fmt_try < 0)
    logging__panic(loc, "failed to log data due to invalid parameters");

  written += fmt_try;

  if (written > BUF_SIZE) { // TODO expand buffer
    const char *suffix = "... [clipped]";
    strcpy_s(string__suffix(out, (int64_t)(out.size - strlen(suffix))), suffix);
    written = BUF_SIZE;
  }

  printf("%.*s\n", (int)out.size, out.data);
}

void logging__panic(sloc loc, const char *message) {
  String out = string__new(buffer, BUF_SIZE);
  int64_t written = write_prefix_to_buffer(out, loc);
  written += (int64_t)strcpy_s(string__suffix(out, written), message);
  if (written > BUF_SIZE) { // TODO expand buffer
    const char *suffix = "... [clipped]";
    strcpy_s(string__suffix(out, (int64_t)(out.size - strlen(suffix))), suffix);
    written = BUF_SIZE;
  }

  printf("%.*s\n", (int)out.size, out.data);
  __builtin_trap();
}
