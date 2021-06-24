#pragma once

#include "basics.h"
#include <stdint.h>

typedef struct {
  const char *file;
  uint32_t line;
} sloc;

#define __LOC__                                                                \
  (sloc) { .file = __FILE__, .line = __LINE__ }

typedef enum __attribute__((packed)) {
  type_id_u8,
  type_id_i8,
  type_id_u16,
  type_id_i16,
  type_id_u32,
  type_id_i32,
  type_id_u64,
  type_id_i64,
  type_id_char,
  type_id_char_ptr,
} type_id;

typedef struct {
  union {
    uint64_t u64_value;
    int64_t i64_value;
    char char_value;
    void *ptr;
  };
  type_id type;
} any;

static any inline fmt__make_any_u64(uint64_t value) {
  return (any){.u64_value = (uint64_t)value, .type = type_id_u64};
}

static any inline fmt__make_any_i64(int64_t value) {
  return (any){.i64_value = (int64_t)value, .type = type_id_i64};
}

static any inline fmt__make_any_char_ptr(const char *value) {
  return (any){.ptr = (void *)value, .type = type_id_char_ptr};
}

static any inline fmt__make_any_char(char value) {
  return (any){.char_value = value, .type = type_id_char};
}

static any inline fmt__make_any_any(any value) { return value; }

// clang-format off
#define make_any(value)                                                        \
  _Generic((value),                                                            \
       uint8_t  : fmt__make_any_u64,                                           \
        int8_t  : fmt__make_any_i64,                                           \
      uint16_t  : fmt__make_any_u64,                                           \
       int16_t  : fmt__make_any_i64,                                           \
      uint32_t  : fmt__make_any_u64,                                           \
       int32_t  : fmt__make_any_i64,                                           \
      uint64_t  : fmt__make_any_u64,                                           \
       int64_t  : fmt__make_any_i64,                                           \
          char  : fmt__make_any_char,                                          \
          char* : fmt__make_any_char_ptr,                                      \
    const char* : fmt__make_any_char_ptr,                                      \
           any  : fmt__make_any_any                                            \
  )(value)
// clang-format on

int64_t write_prefix_to_buffer(String out, sloc loc);

// If return value is positive, formatter tried to write that many bytes to
// provided buffer; If negative, the formatter to format the argument
int64_t fmt__fmt_any(String out, any value);

// If return value is positive, formatter tried to write that many bytes to
// provided buffer; If negative, the formatter failed on that argument
// (one-indexed)
int64_t fmt__fmt(String out, const char *fmt, int32_t count, any *args);
