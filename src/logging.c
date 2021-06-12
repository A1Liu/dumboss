#include "logging.h"
#include "basics.h"
#include "serial_communications_port.h"

#define BUF_SIZE 200

char buffer[BUF_SIZE];

static uint64_t write_prefix_to_buffer(sloc loc) {
  buffer[0] = '[';

  uint64_t written = 1 + strcpy_s(buffer + 1, loc.file, BUF_SIZE - 1);
  if (written == BUF_SIZE)
    panic();

  buffer[written] = ':';
  written++;

  written += fmt_u64(loc.line, buffer + written, BUF_SIZE - written);
  if (written == BUF_SIZE)
    panic();

  written += strcpy_s(buffer + written, "]: ", BUF_SIZE - written);
  if (written == BUF_SIZE)
    panic();

  return written;
}

void logging__log(sloc loc, uint32_t count, any *args) {
  (void)loc;
  panic();

  uint64_t written = 0;
  for (uint32_t i = 0; i < count; i++) {
    panic();
    written += any__fmt(args[i], buffer + written, BUF_SIZE - written);
    if (written > BUF_SIZE) // TODO expand buffer instead of crashing
      panic();
  }

  for (uint32_t i = 0; i < written; i++)
    serial__write(buffer[i]);
  serial__write('\n');
}

void logging__log_fmt(sloc loc, const char *fmt, uint32_t count, any *args) {
  (void)loc;
  uint64_t written = 0, format_count = 0;
  while (written < BUF_SIZE && *fmt) {
    if (*fmt != '%') {
      buffer[written] = *fmt;
      written++;
      fmt++;
      continue;
    }

    fmt++;
    if (*fmt == '%') {
      buffer[written] = '%';
      written++;
      fmt++;
      continue;
    }

    if (format_count == count)
      panic(); // TODO how should we handle this? It's definitely a bug, and
               // this case is the scary one we don't ever want to happen

    written +=
        any__fmt(args[format_count], buffer + written, BUF_SIZE - written);
    format_count++;
  }

  if (written > BUF_SIZE || *fmt) // TODO expand buffer instead of crashing
    panic();

  if (format_count != count)
    panic(); // TODO how should we handle this? It's a bug, but it's kinda
             // fine

  for (uint32_t i = 0; i < written; i++)
    serial__write(buffer[i]);
  serial__write('\n');
}

void logging__panic(sloc loc) {
  (void)loc;
  (void)write_prefix_to_buffer;
  uint64_t written = write_prefix_to_buffer(loc);

  for (uint64_t i = 0; i < written; i++)
    serial__write(buffer[i]);

  char *message = "Panicking!\n";
  for (; *message; message++)
    serial__write(*message);

  for (;;)
    ;
}
