#pragma once
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#define U64_1 ((uint64_t)1)

typedef struct {
  const char *file;
  uint32_t line;
} sloc;

typedef struct {
  uint8_t *data;
  int64_t count;
} Buffer;

typedef struct {
  char *data;
  int64_t count;
} String;

typedef struct {
  uint64_t *data;
  int64_t count;
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
    uint64_t u64_value;
    int64_t i64_value;
    char char_value;
    bool bool_value;
    void *ptr;
  };
  type_id type;
} any;

#define SWAP(left, right)                                                                          \
  ({                                                                                               \
    typeof(*left) value = *left;                                                                   \
    *left = *right;                                                                                \
    *right = value;                                                                                \
  })

#define __LOC__                                                                                    \
  (sloc) {                                                                                         \
    .file = __FILE__, .line = __LINE__                                                             \
  }

#define NARG(...)                                                                                  \
  NARG_INTERNAL_PRIVATE(0, ##__VA_ARGS__, 70, 69, 68, 67, 66, 65, 64, 63, 62, 61, 60, 59, 58, 57,  \
                        56, 55, 54, 53, 52, 51, 50, 49, 48, 47, 46, 45, 44, 43, 42, 41, 40, 39,    \
                        38, 37, 36, 35, 34, 33, 32, 31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21,    \
                        20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0)
#define NARG_INTERNAL_PRIVATE(_0, _1_, _2_, _3_, _4_, _5_, _6_, _7_, _8_, _9_, _10_, _11_, _12_,   \
                              _13_, _14_, _15_, _16_, _17_, _18_, _19_, _20_, _21_, _22_, _23_,    \
                              _24_, _25_, _26_, _27_, _28_, _29_, _30_, _31_, _32_, _33_, _34_,    \
                              _35_, _36, _37, _38, _39, _40, _41, _42, _43, _44, _45, _46, _47,    \
                              _48, _49, _50, _51, _52, _53, _54, _55, _56, _57, _58, _59, _60,     \
                              _61, _62, _63, _64, _65, _66, _67, _68, _69, _70, count, ...)        \
  count

#define _STRINGIFY(x) #x
#define STRINGIFY(x)  _STRINGIFY(x)
#define _PASTE(a, b)  a##b
#define PASTE(a, b)   _PASTE(a, b)

// function, separator, capture
#define _FOR_ARGS0(func, ...)
#define _FOR_ARGS1(func, a)                func(a)
#define _FOR_ARGS2(func, a, b)             func(a), func(b)
#define _FOR_ARGS3(func, a, b, c)          func(a), func(b), func(c)
#define _FOR_ARGS4(func, a, b, c, d)       func(a), func(b), func(c), func(d)
#define _FOR_ARGS5(func, a, b, c, d, e)    func(a), func(b), func(c), func(d), func(e)
#define _FOR_ARGS6(func, a, b, c, d, e, f) func(a), func(b), func(c), func(d), func(e), func(f)
#define _FOR_ARGS7(func, a, b, c, d, e, f, g)                                                      \
  func(a), func(b), func(c), func(d), func(e), func(f), func(g)
#define _FOR_ARGS8(func, _a0, _a1, _a2, _a3, _a4, _a5, _a6, _a7)                                   \
  func(_a0), func(_a1), func(_a2), func(_a3), func(_a4), func(_a5), func(_a6), func(_a6)
#define FOR_ARGS(func, ...)                                                                        \
  PASTE(_FOR_ARGS, NARG(__VA_ARGS__))                                                              \
  (func, ##__VA_ARGS__)

#define _FOR_ARGS_SEP0(func, sep, ...)
#define _FOR_ARGS_SEP1(func, sep, a)          func(a)
#define _FOR_ARGS_SEP2(func, sep, a, b)       func(a) sep() func(b)
#define _FOR_ARGS_SEP3(func, sep, a, b, c)    func(a) sep() func(b) sep() func(c)
#define _FOR_ARGS_SEP4(func, sep, a, b, c, d) func(a) sep() func(b) sep() func(c) sep() func(d)
#define _FOR_ARGS_SEP5(func, sep, a, b, c, d, e)                                                   \
  func(a) sep() func(b) sep() func(c) sep() func(d) sep() func(e)
#define _FOR_ARGS_SEP6(func, sep, a, b, c, d, e, f)                                                \
  func(a) sep() func(b) sep() func(c) sep() func(d) sep() func(e) sep() func(f)
#define _FOR_ARGS_SEP7(func, sep, a, b, c, d, e, f, g)                                             \
  func(a) sep() func(b) sep() func(c) sep() func(d) sep() func(e) sep() func(f) sep() func(g)
#define _FOR_ARGS_SEP8(func, sep, _a0, _a1, _a2, _a3, _a4, _a5, _a6, _a7)                          \
  func(_a0) sep() func(_a1) sep() func(_a2) sep() func(_a3) sep() func(_a4) sep() func(_a5) sep()  \
      func(_a6) sep() func(_a6)
#define FOR_ARGS_SEP(func, sep, ...)                                                               \
  PASTE(_FOR_ARGS_SEP, NARG(__VA_ARGS__))                                                          \
  (func, sep, ##__VA_ARGS__)

// NOTE: This breaks compatibility with GCC. I guess that's fine, but like, seems
// weird and a bit uncomfy.
//                                    - Albert Liu, Oct 24, 2021 Sun 04:20 EDT
#define DECLARE_SCOPED(...) for (__VA_ARGS__;; ({ break; }))

#define _FOR(array, it, it_index)                                                                  \
  DECLARE_SCOPED(typeof(array) M_array = array)                                                    \
  DECLARE_SCOPED(int64_t it_index = 0, M_len = M_array.count)                                      \
  DECLARE_SCOPED(typeof(&array.data[0]) M_ptr = M_array.data, it = NULL)                           \
  for (it = M_ptr; it_index < M_len; it++, it_index++)

#define _FOR_PTR(ptr, len, it, it_index)                                                           \
  DECLARE_SCOPED(int64_t it_index = 0, M_len = len)                                                \
  DECLARE_SCOPED(typeof(&ptr[0]) M_ptr = ptr, it = NULL)                                           \
  for (it = M_ptr; it_index < M_len; it++, it_index++)

#define _FOR1(array)               _FOR(array, it, it_index)
#define _FOR2(array, it)           _FOR(array, it, it_index)
#define _FOR3(array, it, it_index) _FOR(array, it, it_index)
#define FOR(...)                   PASTE(_FOR, NARG(__VA_ARGS__))(__VA_ARGS__)

#define _FOR_PTR2(ptr, len)               _FOR_PTR(ptr, len, it, it_index)
#define _FOR_PTR3(ptr, len, it)           _FOR_PTR(ptr, len, it, it_index)
#define _FOR_PTR4(ptr, len, it, it_index) _FOR_PTR(ptr, len, it, it_index)
#define FOR_PTR(...)                      PASTE(_FOR_PTR, NARG(__VA_ARGS__))(__VA_ARGS__)

// Does bubble sort stuff; user has to make the swaps themself
#define SLOW_SORT(array)                                                                           \
  DECLARE_SCOPED(typeof(array) M_array = array)                                                    \
  for (int64_t M_right_bound = M_array.count - 1, M_left_index = 0; M_right_bound > 0;             \
       M_right_bound--, M_left_index = 0)                                                          \
    for (typeof(M_array.data) left = M_array.data + M_left_index,                                  \
                              right = M_array.data + M_left_index + 1;                             \
         M_left_index < M_right_bound; M_left_index++, left++, right++)

// clang-format off
#define make_any(value)                                                        \
  _Generic((value),                                                            \
          bool  : basics__make_any_bool,                                       \
       uint8_t  : basics__make_any_u64,                                        \
        int8_t  : basics__make_any_i64,                                        \
      uint16_t  : basics__make_any_u64,                                        \
       int16_t  : basics__make_any_i64,                                        \
      uint32_t  : basics__make_any_u64,                                        \
       int32_t  : basics__make_any_i64,                                        \
      uint64_t  : basics__make_any_u64,                                        \
       int64_t  : basics__make_any_i64,                                        \
     long long  : basics__make_any_i64,                                        \
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

#define _read_register1(reg)     _read_register3(reg, uint64_t, "")
#define _read_register2(reg, ty) _read_register3(reg, ty, "")
#define _read_register3(reg, ty, suffix)                                                           \
  ({                                                                                               \
    _Static_assert(sizeof(ty) <= 8, "read_register only takes a type with size <= 8");             \
    ty value;                                                                                      \
    asm("mov" suffix " %%" #reg ", %0" : "=r"(value));                                             \
    value;                                                                                         \
  })
#define read_register(...) PASTE(_read_register, NARG(__VA_ARGS__))(__VA_ARGS__)

#define write_register(reg, value)                                                                 \
  ({                                                                                               \
    typeof(value) v = value;                                                                       \
    asm("mov %0, %%" #reg : : "g"(v) : #reg);                                                      \
    v;                                                                                             \
  })

#define out8(port, val)                                                                            \
  ({                                                                                               \
    uint8_t v = val;                                                                               \
    asm volatile("outb %0, %1" : : "a"(v), "Nd"(port));                                            \
    v;                                                                                             \
  })

#define in8(port)                                                                                  \
  ({                                                                                               \
    uint8_t ret;                                                                                   \
    uint16_t p = port;                                                                             \
    asm volatile("inb %1, %0" : "=a"(ret) : "Nd"(p));                                              \
    ret;                                                                                           \
  })

#define min(x, y) ((x) > (y) ? (y) : (x))
#define max(x, y) ((x) < (y) ? (y) : (x))
int64_t smallest_greater_power2(int64_t value);
uint64_t align_up(uint64_t value, uint64_t alignment);
uint64_t align_down(uint64_t value, uint64_t alignment);

String Str__new(char *data, int64_t size);
bool Str__is_null(String str);
String Str__slice(String str, int64_t begin, int64_t end);
String Str__suffix(String str, int64_t begin);

BitSet BitSet__new(uint64_t *data, int64_t size);
bool BitSet__get(BitSet bits, int64_t idx);
void BitSet__set(BitSet bits, int64_t idx, bool value);
void BitSet__set_all(BitSet bits, bool value);
void BitSet__set_range(BitSet bits, int64_t begin, int64_t end, bool value);

static inline void asm_hlt(void) {
  asm volatile("hlt");
}

static any inline basics__make_any_bool(bool value) {
  return (any){.bool_value = value, .type = type_id_bool};
}

static any inline basics__make_any_u64(uint64_t value) {
  return (any){.u64_value = (uint64_t)value, .type = type_id_u64};
}

static any inline basics__make_any_i64(int64_t value) {
  return (any){.i64_value = (int64_t)value, .type = type_id_i64};
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
int64_t basics__fmt_any(String out, any value);

// If return value is positive, formatter tried to write that many bytes to
// provided buffer; If negative, the formatter failed on that argument
// (one-indexed)
int64_t basics__fmt(String out, const char *fmt, int32_t count, const any *args);

// Panic the kernel
_Noreturn void shutdown(void);

void memset(void *buffer, uint8_t byte, int64_t len);

// Formats a u64. Returns the length of buffer needed to output this number. If
// return value is smaller than `size`, then the writing succedded
int64_t fmt_u64(String out, uint64_t value);

// Formats a i64. Returns the length of buffer needed to output this number. If
// return value is smaller than size parameter, then the writing succedded
int64_t fmt_i64(String out, int64_t value);

// Returns the length of a null-terminated string
int64_t strlen(const char *str);

// Tries to write null-terminated string `src` into `dest`
// char *strcpy(char *dest, const char *src);

// Tries to write null-terminated string `src` into `dest`, stopping when
// `dest.size` is reached, and returning the amount of data written
int64_t strcpy_s(String dest, const char *src);
