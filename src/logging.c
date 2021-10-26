#include "logging.h"

#define BUF_SIZE 200

static char buffer[BUF_SIZE];
void serial__write(char a);

static s64 write_prefix_to_buffer(String out, sloc loc) {
  any args[] = {make_any(loc.file), make_any(loc.line)};
  return basics__fmt(out, "[%f:%f]: ", 2, args);
}

void logging__log(sloc loc, s32 count, const any *args) {
  String out = Str__new(buffer, BUF_SIZE);
  s64 written = write_prefix_to_buffer(out, loc);

  for (s32 i = 0; i < count; i++) {
    s64 fmt_try = basics__fmt_any(Str__suffix(out, min(written, BUF_SIZE)), args[i]);
    assert(fmt_try >= 0);
    written += fmt_try;
  }

  if (written > BUF_SIZE) { // TODO expand buffer
    const char *suffix = "... [clipped]";
    strcpy_s(Str__suffix(out, (s64)(out.count - strlen(suffix))), suffix);
    written = BUF_SIZE;
  }

  for (s64 i = 0; i < written; i++)
    serial__write(buffer[i]);
  serial__write('\n');
}

void logging__log_fmt(sloc loc, const char *fmt, s32 count, const any *args) {
  String out = Str__new(buffer, BUF_SIZE);
  s64 written = write_prefix_to_buffer(out, loc);
  s64 fmt_try = basics__fmt(Str__suffix(out, written), fmt, count, args);
  if (fmt_try < 0) {
    if (fmt_try < -count - 1) {
      logging__log_fmt(loc, "too many parameters (expected %f, got %f)", 2,
                       make_any_array(-fmt_try, count));
      shutdown();
    } else {
      logging__log_fmt(loc, "failed to log parameter at index %f", 1, make_any_array(-fmt_try - 1));
      shutdown();
    }
  }

  written += fmt_try;

  if (written > BUF_SIZE) { // TODO expand buffer
    const char *suffix = "... [clipped]";
    strcpy_s(Str__suffix(out, (s64)(out.count - strlen(suffix))), suffix);
    written = BUF_SIZE;
  }

  for (s64 i = 0; i < written; i++)
    serial__write(buffer[i]);
  serial__write('\n');
}

// Largely copy-pasted from
// https://wiki.osdev.org/Serial_Ports
#define COM1 ((uint16_t)0x3f8)

int is_transmit_empty() {
  return in8(COM1 + 5) & 0x20;
}

void serial__write(char a) {
  while (is_transmit_empty() == 0)
    ;

  out8(COM1, (uint8_t)a);
}
