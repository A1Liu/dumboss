#include "any.h"
#include "basics.h"
#include "logging.h"

uint64_t any__fmt(any value, char *out, uint64_t size) {
  if (out == NULL) // TODO what should we do here?
    panic();

  switch (value.type) {
  case type_id_u8:
    return fmt_u64(*(uint8_t *)value.data, out, size);
  case type_id_i8:
    return fmt_i64(*(int8_t *)value.data, out, size);
  case type_id_u16:
    return fmt_u64(*(uint16_t *)value.data, out, size);
  case type_id_i16:
    return fmt_i64(*(int16_t *)value.data, out, size);
  case type_id_u32:
    return fmt_u64(*(uint32_t *)value.data, out, size);
  case type_id_i32:
    return fmt_i64(*(int32_t *)value.data, out, size);
  case type_id_u64:
    return fmt_u64(*(uint64_t *)value.data, out, size);
  case type_id_i64:
    return fmt_i64(*(int64_t *)value.data, out, size);

  case type_id_char: {
    if (size >= 1)
      *out = *(char *)value.data;
    return 1;
  }

  case type_id_char_ptr: {
    char *src = *(char **)value.data;
    if (src == NULL)
      return 0;

    uint64_t written = strcpy_s(out, src, size);
    return written + strlen(src + written);
  }

  default: // TODO what should we do here?
    panic();
  }
}
