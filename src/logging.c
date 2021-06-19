#include "logging.h"
#include "basics.h"
#include "serial_communications_port.h"

#define BUF_SIZE 200

static char buffer[BUF_SIZE];

static int64_t write_prefix_to_buffer(String out, sloc loc) {
  const int64_t len = (int64_t)out.size;

  out.data[0] = '[';
  int64_t written = 1 + (int64_t)strcpy_s(string__suffix(out, 1), loc.file);
  if (written >= len)
    return (int64_t)strcpy_s(out, "[source location too long]: ");

  buffer[written] = ':';
  written++;

  written += fmt_u64(string__suffix(out, written), loc.line);
  if (written >= len)
    return (int64_t)strcpy_s(out, "[source location too long]: ");

  written += strcpy_s(string__suffix(out, written), "]: ");
  if (written >= len)
    return (int64_t)strcpy_s(out, "[source location too long]: ");

  return written;
}

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

  for (int64_t i = 0; i < written; i++)
    serial__write(buffer[i]);
  serial__write('\n');
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

  for (int64_t i = 0; i < written; i++)
    serial__write(buffer[i]);
  serial__write('\n');
}

void logging__panic(sloc loc, const char *message) {
  String out = string__new(buffer, BUF_SIZE);
  int64_t written = write_prefix_to_buffer(out, loc);
  written += (int64_t)strcpy_s(string__suffix(out, written), message);
  written = min(written, BUF_SIZE);

  for (int64_t i = 0; i < written; i++)
    serial__write(buffer[i]);
  serial__write('\n');

  for (;;)
    asm volatile("hlt");
}
