#include <asm.h>
#include <types.h>

#define __DUMBOSS_IMPL__
#include <basics.h>
#include <bitset.h>
#include <sync.h>

static inline void asm_outw(u16 port, u16 val) {
  asm volatile("outw %0, %1" : : "a"(val), "Nd"(port));
}

_Noreturn void shutdown(void) {
  // Cause immediate shutdown when in a virtual machine
  // https://wiki.osdev.org/Shutdown
  asm_outw(0xB004, 0x2000);
  asm_outw(0x604, 0x2000);
  asm_outw(0x4004, 0x3400);

  for (;;)
    asm_hlt();
}

#define BUF_SIZE 200

static char buffer[BUF_SIZE];
static void serial__write(char a);

static s64 write_prefix_to_buffer(String out, sloc loc) {
  any args[] = {make_any(loc.file), make_any(loc.line)};
  return any__fmt(out, "[%f:%f]: ", 2, args);
}

void logging__log(sloc loc, s32 count, const any *args) {
  String out = Str__new(buffer, BUF_SIZE);
  s64 written = write_prefix_to_buffer(out, loc);

  for (s32 i = 0; i < count; i++) {
    s64 fmt_try = any__fmt_any(Str__suffix(out, min(written, BUF_SIZE)), args[i]);
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
  s64 fmt_try = any__fmt(Str__suffix(out, written), fmt, count, args);
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

static void serial__write(char a) {
  while (is_transmit_empty() == 0)
    ;

  out8(COM1, (uint8_t)a);
}
