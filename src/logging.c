#include "logging.h"

#define BUF_SIZE 200

static char buffer[BUF_SIZE];
void serial__write(char a);

void logging__log(sloc loc, int32_t count, any *args) {
  String out = Str__new(buffer, BUF_SIZE);
  int64_t written = write_prefix_to_buffer(out, loc);

  for (int32_t i = 0; i < count; i++) {
    int64_t fmt_try =
        basics__fmt_any(Str__suffix(out, min(written, BUF_SIZE)), args[i]);
    assert(fmt_try >= 0);
    written += fmt_try;
  }

  if (written > BUF_SIZE) { // TODO expand buffer
    const char *suffix = "... [clipped]";
    strcpy_s(Str__suffix(out, (int64_t)(out.size - strlen(suffix))), suffix);
    written = BUF_SIZE;
  }

  for (int64_t i = 0; i < written; i++)
    serial__write(buffer[i]);
  serial__write('\n');
}

void logging__log_fmt(sloc loc, const char *fmt, int32_t count, any *args) {
  String out = Str__new(buffer, BUF_SIZE);
  int64_t written = write_prefix_to_buffer(out, loc);
  int64_t fmt_try = basics__fmt(Str__suffix(out, written), fmt, count, args);
  if (fmt_try < 0)
    logging__panic(loc, "failed to log data due to invalid parameters");

  written += fmt_try;

  if (written > BUF_SIZE) { // TODO expand buffer
    const char *suffix = "... [clipped]";
    strcpy_s(Str__suffix(out, (int64_t)(out.size - strlen(suffix))), suffix);
    written = BUF_SIZE;
  }

  for (int64_t i = 0; i < written; i++)
    serial__write(buffer[i]);
  serial__write('\n');
}

void logging__panic(sloc loc, const char *message) {
  String out = Str__new(buffer, BUF_SIZE);
  int64_t written = write_prefix_to_buffer(out, loc);
  written += (int64_t)strcpy_s(Str__suffix(out, written), message);
  written = min(written, BUF_SIZE);

  for (int64_t i = 0; i < written; i++)
    serial__write(buffer[i]);
  serial__write('\n');

  for (;;)
    asm_hlt();
}

// Largely copy-pasted from
// https://wiki.osdev.org/Serial_Ports
#define COM1 ((uint16_t)0x3f8)

static inline void asm_outb(uint16_t port, uint8_t val) {
  asm volatile("outb %0, %1" : : "a"(val), "Nd"(port));
}

static inline uint8_t asm_inb(uint16_t port) {
  uint8_t ret;
  asm volatile("inb %1, %0" : "=a"(ret) : "Nd"(port));
  return ret;
}

int is_transmit_empty() { return asm_inb(COM1 + 5) & 0x20; }

void serial__write(char a) {
  while (is_transmit_empty() == 0)
    ;

  asm_outb(COM1, (uint8_t)a);
}
