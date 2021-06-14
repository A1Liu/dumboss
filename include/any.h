#pragma once

#include "gnu-efi/efi.h"
#include <stdint.h>

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

any inline any__make_any_u64(uint64_t value) {
  return (any){.u64_value = (uint64_t)value, .type = type_id_u64};
}

any inline any__make_any_i64(int64_t value) {
  return (any){.i64_value = (int64_t)value, .type = type_id_i64};
}

any inline any__make_any_char_ptr(const char *value) {
  return (any){.ptr = (void *)value, .type = type_id_char_ptr};
}

any inline any__make_any_char(char value) {
  return (any){.char_value = value, .type = type_id_char};
}

any inline any__make_any_any(any value) { return value; }

// clang-format off
#define make_any(value)                                                        \
  _Generic((value),                                                            \
       uint8_t  : any__make_any_u64,                                           \
        int8_t  : any__make_any_i64,                                           \
      uint16_t  : any__make_any_u64,                                           \
       int16_t  : any__make_any_i64,                                           \
      uint32_t  : any__make_any_u64,                                           \
       int32_t  : any__make_any_i64,                                           \
      uint64_t  : any__make_any_u64,                                           \
       int64_t  : any__make_any_i64,                                           \
          char  : any__make_any_char,                                          \
          char* : any__make_any_char_ptr,                                      \
    const char* : any__make_any_char_ptr,                                      \
           any  : any__make_any_any                                            \
  )(value)
// clang-format on

uint64_t any__fmt(any value, char *out, uint64_t size);
