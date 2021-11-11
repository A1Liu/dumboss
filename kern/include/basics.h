#pragma once
#include "macros.h"
#include "types.h"

typedef struct {
  const char *file;
  u32 line;
} sloc;

typedef struct {
  u8 *data;
  s64 count;
} Buffer;

typedef struct {
  char *data;
  s64 count;
} String;

typedef struct {
  u64 *data;
  s64 count;
} BitSet;

typedef enum __attribute__((packed)) {
  type_id_bool,
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
    u64 u64_value;
    s64 i64_value;
    char char_value;
    bool bool_value;
    void *ptr;
  };
  type_id type;
} any;

// clang-format off
#define make_any(value)                                                        \
  _Generic((value),                                                            \
          bool  : basics__make_any_bool,                                       \
            u8  : basics__make_any_u64,                                        \
            s8  : basics__make_any_i64,                                        \
           u16  : basics__make_any_u64,                                        \
           s16  : basics__make_any_i64,                                        \
           u32  : basics__make_any_u64,                                        \
           s32  : basics__make_any_i64,                                        \
           u64  : basics__make_any_u64,                                        \
           s64  : basics__make_any_i64,                                        \
          char  : basics__make_any_char,                                       \
          char* : basics__make_any_char_ptr,                                   \
    const char* : basics__make_any_char_ptr,                                   \
           any  : basics__make_any_any                                         \
  )(value)
// clang-format on

#define make_any_array(...)                                                                        \
  (any[]) {                                                                                        \
    FOR_ARGS(make_any, __VA_ARGS__)                                                                \
  }

s64 smallest_greater_power2(s64 value);
#define align_up(value, _align)                                                                    \
  ({                                                                                               \
    u64 M_align = _align;                                                                          \
    assert(M_align && (__builtin_popcountl(M_align) == 1), "alignment wasn't a power of 2");       \
    __builtin_align_up(value, M_align);                                                            \
  })
#define align_down(value, _align)                                                                  \
  ({                                                                                               \
    u64 M_align = _align;                                                                          \
    assert(M_align && (__builtin_popcountl(M_align) == 1), "alignment wasn't a power of 2");       \
    __builtin_align_down(value, M_align);                                                          \
  })
#define is_aligned(value, _align)                                                                  \
  ({                                                                                               \
    u64 M_align = _align;                                                                          \
    assert(M_align && (__builtin_popcountl(M_align) == 1), "alignment wasn't a power of 2");       \
    __builtin_is_aligned(value, M_align);                                                          \
  })

String Str__new(char *data, s64 size);
bool Str__is_null(String str);
String Str__slice(String str, s64 begin, s64 end);
String Str__suffix(String str, s64 begin);

BitSet BitSet__from_raw(u64 *data, s64 size);
bool BitSet__get(BitSet bits, s64 idx);
bool BitSet__get_all(BitSet bits, s64 begin, s64 end);
bool BitSet__get_any(BitSet bits, s64 begin, s64 end);
void BitSet__set(BitSet bits, s64 idx, bool value);
void BitSet__set_all(BitSet bits, bool value);
void BitSet__set_range(BitSet bits, s64 begin, s64 end, bool value);

static inline void asm_hlt(void) {
  asm volatile("hlt");
}

static any inline basics__make_any_bool(bool value) {
  return (any){.bool_value = value, .type = type_id_bool};
}

static any inline basics__make_any_u64(u64 value) {
  return (any){.u64_value = (u64)value, .type = type_id_u64};
}

static any inline basics__make_any_i64(s64 value) {
  return (any){.i64_value = (s64)value, .type = type_id_i64};
}

static any inline basics__make_any_char_ptr(const char *value) {
  return (any){.ptr = (void *)value, .type = type_id_char_ptr};
}

static any inline basics__make_any_char(char value) {
  return (any){.char_value = value, .type = type_id_char};
}

static any inline basics__make_any_any(any value) {
  return value;
}

// If return value is positive, formatter tried to write that many bytes to
// provided buffer; If negative, the formatter to format the argument
s64 basics__fmt_any(String out, any value);

// If return value is positive, formatter tried to write that many bytes to
// provided buffer; If negative, the formatter failed on that argument
// (one-indexed)
s64 basics__fmt(String out, const char *fmt, s32 count, const any *args);

// Panic the kernel
_Noreturn void shutdown(void);
void memcpy(void *dest, const void *src, s64 count);
void memset(void *buffer, u8 value, s64 len);

// Formats a u64. Returns the length of buffer needed to output this number. If
// return value is smaller than `size`, then the writing succedded
s64 fmt_u64(String out, u64 value);

// Formats a i64. Returns the length of buffer needed to output this number. If
// return value is smaller than size parameter, then the writing succedded
s64 fmt_i64(String out, s64 value);

// Returns the length of a null-terminated string
s64 strlen(const char *str);

// Tries to write null-terminated string `src` into `dest`
// char *strcpy(char *dest, const char *src);

// Tries to write null-terminated string `src` into `dest`, stopping when
// `dest.size` is reached, and returning the amount of data written
s64 strcpy_s(String dest, const char *src);
