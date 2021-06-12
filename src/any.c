#include "any.h"
#include "basics.h"
#include "logging.h"

uint64_t any__fmt(any value, char *out, uint64_t size) {
  if (out == NULL) // TODO what should we do here?
    panic();

  switch (value.type) {
  case type_id_u8:
    return fmt_u64(value.u64_value, out, size);
  case type_id_i8:
    return fmt_i64(value.i64_value, out, size);
  case type_id_u16:
    return fmt_u64(value.u64_value, out, size);
  case type_id_i16:
    return fmt_i64(value.i64_value, out, size);
  case type_id_u32:
    return fmt_u64(value.u64_value, out, size);
  case type_id_i32:
    return fmt_i64(value.i64_value, out, size);
  case type_id_u64:
    return fmt_u64(value.u64_value, out, size);
  case type_id_i64:
    return fmt_i64(value.i64_value, out, size);

  case type_id_char: {
    if (size >= 1)
      *out = value.char_value;
    return 1;
  }

  case type_id_char_ptr: {
    char *src = (char *)value.ptr;
    if (src == NULL)
      return 0;

    uint64_t written = strcpy_s(out, src, size);
    return written + strlen(src + written);
  }

  default: // TODO what should we do here?
    panic();
  }
}
