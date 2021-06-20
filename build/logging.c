#include "logging.h"
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>

void logging__log(sloc loc, int32_t count, any *args) {
  String out = string__new(NULL, 0);
  int64_t needed = write_prefix_to_buffer(out, loc);

  for (int32_t i = 0; i < count; i++) {
    int64_t fmt_try = fmt__fmt_any(string__suffix(out, needed), args[i]);
    if (fmt_try < 0)
      logging__panic(loc, "failed to log data due to invalid parameters");
    needed += fmt_try;
  }

  out = string__new(malloc((uint64_t)needed), needed);
  int64_t written = write_prefix_to_buffer(out, loc);

  for (int32_t i = 0; i < count; i++) {
    int64_t fmt_try = fmt__fmt_any(string__suffix(out, written), args[i]);
    if (fmt_try < 0)
      logging__panic(loc, "failed to log data due to invalid parameters");
    written += fmt_try;
  }

  assert(written == needed);

  printf("%.*s\n", (int)written, out.data);
  fflush(stdout);
}

void logging__log_fmt(sloc loc, const char *fmt, int32_t count, any *args) {
  String out = string__new(NULL, 0);
  int64_t needed = write_prefix_to_buffer(out, loc);
  int64_t fmt_try = fmt__fmt(string__suffix(out, needed), fmt, count, args);
  if (fmt_try < 0)
    logging__panic(loc, "failed to log data due to invalid parameters");
  needed += fmt_try;

  out = string__new(malloc((uint64_t)needed), needed);
  int64_t written = write_prefix_to_buffer(out, loc);
  fmt_try = fmt__fmt(string__suffix(out, written), fmt, count, args);
  if (fmt_try < 0)
    logging__panic(loc, "failed to log data due to invalid parameters");
  written += fmt_try;

  assert(written == needed);

  printf("%.*s\n", (int)written, out.data);
  fflush(stdout);
}

void logging__panic(sloc loc, const char *message) {
  int64_t needed = write_prefix_to_buffer(string__new(NULL, 0), loc);
  needed += (int64_t)strlen(message);

  String out = string__new(malloc((uint64_t)needed), needed);
  int64_t written = write_prefix_to_buffer(out, loc);
  written += (int64_t)strcpy_s(string__suffix(out, written), message);
  assert(written == needed);

  printf("%.*s\n", (int)written, out.data);
  fflush(stdout);

  __builtin_trap();
}
