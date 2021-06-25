#include "fmt.h"
#include "basics.h"

int64_t write_prefix_to_buffer(String out, sloc loc) {
  any args[] = {make_any(loc.file), make_any(loc.line)};
  return fmt__fmt(out, "[%:%]: ", 2, args);
}

int64_t fmt__fmt_any(String out, any value) {
  switch (value.type) {
  case type_id_bool: {
    const char *const src = value.bool_value ? "true" : "false";

    // @Safety there better not be any C-strings that are over 2^63 bytes long
    int64_t written = (int64_t)strcpy_s(out, src);
    return written + (int64_t)strlen(src + written);
  }
    return (int64_t)fmt_u64(out, value.u64_value);
  case type_id_u8:
    return (int64_t)fmt_u64(out, value.u64_value);
  case type_id_i8:
    return (int64_t)fmt_i64(out, value.i64_value);
  case type_id_u16:
    return (int64_t)fmt_u64(out, value.u64_value);
  case type_id_i16:
    return (int64_t)fmt_i64(out, value.i64_value);
  case type_id_u32:
    return (int64_t)fmt_u64(out, value.u64_value);
  case type_id_i32:
    return (int64_t)fmt_i64(out, value.i64_value);
  case type_id_u64:
    return (int64_t)fmt_u64(out, value.u64_value);
  case type_id_i64:
    return (int64_t)fmt_i64(out, value.i64_value);

  case type_id_char: {
    if (out.size >= 1)
      *out.data = value.char_value;
    return 1;
  }

  case type_id_char_ptr: {
    char *src = (char *)value.ptr;
    if (src == NULL)
      return 0;

    // @Safety there better not be any C-strings that are over 2^63 bytes long
    int64_t written = (int64_t)strcpy_s(out, src);
    return written + (int64_t)strlen(src + written);
  }

  default: // TODO what should we do here?
    return -1;
  }
}

int64_t fmt__fmt(String out, const char *fmt, int32_t count, any *args) {
  // TODO why would a string be larger than int64_t's max value?
  const int64_t len = (int64_t)out.size;
  int64_t format_count = 0, written = 0;

  while (*fmt) {
    if (*fmt != '%') {
      if (written < len)
        out.data[written] = *fmt;
      written++;
      fmt++;
      continue;
    }

    fmt++;
    if (*fmt == '%') {
      if (written < len)
        out.data[written] = '%';
      written++;
      fmt++;
      continue;
    }

    // TODO how should we handle this? It's definitely a bug, and this case is
    // the scary one we don't ever want to happen
    if (format_count >= count)
      return -format_count - 1;

    int64_t fmt_try = fmt__fmt_any(string__suffix(out, min(written, len)),
                                   args[format_count]);
    if (fmt_try < 0)
      return -format_count - 1;
    written += fmt_try;
    format_count++;
  }

  // TODO how should we handle this? It's a bug, but it's kinda fine
  if (format_count != count)
    return -format_count - 1;
  return written;
}
