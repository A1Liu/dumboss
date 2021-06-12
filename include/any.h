#pragma once
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

// clang-format off
#define make_any(value)                                                        \
  (_Generic((value),                                                            \
       uint8_t  : (any){.u64_value = (uint64_t)value, .type = type_id_u8},      \
        int8_t  : (any){.i64_value = (int64_t)value, .type = type_id_i8},      \
      uint16_t  : (any){.u64_value = (uint64_t)value, .type = type_id_u16},     \
       int16_t  : (any){.i64_value = (int64_t)value, .type = type_id_i16},     \
      uint32_t  : (any){.u64_value = (uint64_t)value, .type = type_id_u32},     \
       int32_t  : (any){.i64_value = (int64_t)value, .type = type_id_i32},     \
      uint64_t  : (any){.u64_value = (uint64_t)value, .type = type_id_u64},     \
       int64_t  : (any){.i64_value = (int64_t)value, .type = type_id_i64},     \
          char  : (any){.char_value = (char)value, .type = type_id_char},   \
          char* : (any){.ptr = (void*)value, .type = type_id_char_ptr},     \
           any  : value                                                        \
  ))
// clang-format on

uint64_t any__fmt(any value, char *out, uint64_t size);
