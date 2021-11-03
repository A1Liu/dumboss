#pragma once
#include "int.h"
#include <stdbool.h>
#include <stddef.h>

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
#define _FOR_ARGS_SEP1(func, sep, _a0)           func(_a0)
#define _FOR_ARGS_SEP2(func, sep, _a0, _a1)      func(_a0) sep() func(_a1)
#define _FOR_ARGS_SEP3(func, sep, _a0, _a1, _a2) func(_a0) sep() func(_a1) sep() func(_a2)
#define _FOR_ARGS_SEP4(func, sep, _a0, _a1, _a2, _a3)                                              \
  func(_a0) sep() func(_a1) sep() func(_a2) sep() func(_a3)
#define _FOR_ARGS_SEP5(func, sep, _a0, _a1, _a2, _a3, _a4)                                         \
  func(_a0) sep() func(_a1) sep() func(_a2) sep() func(_a3) sep() func(_a4)
#define _FOR_ARGS_SEP6(func, sep, _a0, _a1, _a2, _a3, _a4, _a5)                                    \
  func(_a0) sep() func(_a1) sep() func(_a2) sep() func(_a3) sep() func(_a4) sep() func(_a5)
#define _FOR_ARGS_SEP7(func, sep, _a0, _a1, _a2, _a3, _a4, _a5, _a6)                               \
  func(_a0) sep() func(_a1) sep() func(_a2) sep() func(_a3) sep() func(_a4) sep() func(_a5) sep()  \
      func(_a6)
#define _FOR_ARGS_SEP8(func, sep, _a0, _a1, _a2, _a3, _a4, _a5, _a6, _a7)                          \
  func(_a0) sep() func(_a1) sep() func(_a2) sep() func(_a3) sep() func(_a4) sep() func(_a5) sep()  \
      func(_a6) sep() func(_a7)
#define _FOR_ARGS_SEP9(func, sep, _a0, _a1, _a2, _a3, _a4, _a5, _a6, _a7, _a8)                     \
  func(_a0) sep() func(_a1) sep() func(_a2) sep() func(_a3) sep() func(_a4) sep() func(_a5) sep()  \
      func(_a6) sep() func(_a7) sep() func(_a8)
#define FOR_ARGS_SEP(func, sep, ...)                                                               \
  PASTE(_FOR_ARGS_SEP, NARG(__VA_ARGS__))                                                          \
  (func, sep, ##__VA_ARGS__)

// NOTE: This breaks compatibility with GCC. I guess that's fine, but like, seems
// weird and a bit uncomfy.
//                                    - Albert Liu, Oct 24, 2021 Sun 04:20 EDT
#define DECLARE_SCOPED(...) for (__VA_ARGS__;; ({ break; }))

#define _CMP_TYPE_HELPER(T1, T2) _Generic(((T1){0}), T2 : 1, default : 0)
#define CMP_TYPE(T1, T2)         (_CMP_TYPE_HELPER(T1, T2) && _CMP_TYPE_HELPER(T2, T1))

struct LABEL_T_DO_NOT_USE;

#define _LABEL(name) PASTE(M_LABEL_, name)
#define break(label)                                                                               \
  ({                                                                                               \
    _Static_assert(CMP_TYPE(typeof(&_LABEL(label)), const struct LABEL_T_DO_NOT_USE *const *),     \
                   "called break on something that's not a label");                                \
    goto *(void *)_LABEL(label);                                                                   \
  })

// NOTE: This ALSO breaks compatibility with GCC.
//                                    - Albert Liu, Nov 02, 2021 Tue 01:19 EDT
#define _BREAK_BLOCK_HELPER(label, _internal_label)                                                \
  for (const struct LABEL_T_DO_NOT_USE *const _LABEL(label) =                                      \
           (const struct LABEL_T_DO_NOT_USE *const)&&_internal_label;                              \
       ; ({                                                                                        \
         _Pragma("clang diagnostic push \"-Wno-unused-label\"");                                   \
         (void)_LABEL(label);                                                                      \
       _internal_label:                                                                            \
         break;                                                                                    \
         _Pragma("clang diagnostic pop");                                                          \
       }))
#define BREAK_BLOCK(label) _BREAK_BLOCK_HELPER(label, _LABEL(__COUNTER__))

#define _FOR_PTR(_uniq, ptr, len, it, it_index)                                                    \
  BREAK_BLOCK(it)                                                                                  \
  DECLARE_SCOPED(s64 PASTE(_uniq, M_idx) = 0, PASTE(_uniq, M_len) = len)                           \
  DECLARE_SCOPED(s64 it_index = 0)                                                                 \
  DECLARE_SCOPED(typeof(&ptr[0]) PASTE(_uniq, M_ptr) = ptr)                                        \
  DECLARE_SCOPED(typeof(&ptr[0]) it = NULL)                                                        \
  for (it = PASTE(_uniq, M_ptr); PASTE(_uniq, M_idx) < PASTE(_uniq, M_len); PASTE(_uniq, M_ptr)++, \
      PASTE(_uniq, M_idx)++, it = PASTE(_uniq, M_ptr), it_index = PASTE(_uniq, M_idx))

#define _FOR_PTR2(ptr, len)            _FOR_PTR(PASTE(M_FOR_, __COUNTER__), ptr, len, it, index)
#define _FOR_PTR3(ptr, len, it)        _FOR_PTR(PASTE(M_FOR_, __COUNTER__), ptr, len, it, index)
#define _FOR_PTR4(ptr, len, it, index) _FOR_PTR(PASTE(M_FOR_, __COUNTER__), ptr, len, it, index)
#define FOR_PTR(...)                   PASTE(_FOR_PTR, NARG(__VA_ARGS__))(__VA_ARGS__)

#define _FOR(_uniq, array, it, it_index)                                                           \
  DECLARE_SCOPED(typeof(array) PASTE(_uniq, M_array) = array)                                      \
  _FOR_PTR(_uniq, PASTE(_uniq, M_array).data, PASTE(_uniq, M_array).count, it, it_index)

#define _FOR1(array)               _FOR(PASTE(M_FOR_, __COUNTER__), array, it, it_index)
#define _FOR2(array, it)           _FOR(PASTE(M_FOR_, __COUNTER__), array, it, it_index)
#define _FOR3(array, it, it_index) _FOR(PASTE(M_FOR_, __COUNTER__), array, it, it_index)
#define FOR(...)                   PASTE(_FOR, NARG(__VA_ARGS__))(__VA_ARGS__)

#define _REPEAT(_uniq, times, it)                                                                  \
  BREAK_BLOCK(it)                                                                                  \
  DECLARE_SCOPED(s64 PASTE(_uniq, M_it) = 0, PASTE(_uniq, M_end) = times)                          \
  for (s64 it = PASTE(_uniq, M_it); PASTE(_uniq, M_it) < PASTE(_uniq, M_end);                      \
       PASTE(_uniq, M_it)++, it = PASTE(_uniq, M_it))

#define _REPEAT1(times)     _REPEAT(PASTE(M_REPEAT_, __COUNTER__), times, it)
#define _REPEAT2(times, it) _REPEAT(PASTE(M_REPEAT_, __COUNTER__), times, it)
#define REPEAT(...)         PASTE(_REPEAT, NARG(__VA_ARGS__))(__VA_ARGS__)

// Does bubble sort stuff; user has to make the swaps themself
#define SLOW_SORT(array)                                                                           \
  DECLARE_SCOPED(typeof(array) M_array = array)                                                    \
  for (s64 M_right_bound = M_array.count - 1, M_left_index = 0; M_right_bound > 0;                 \
       M_right_bound--, M_left_index = 0)                                                          \
    for (typeof(M_array.data) left = M_array.data + M_left_index,                                  \
                              right = M_array.data + M_left_index + 1;                             \
         M_left_index < M_right_bound; M_left_index++, left++, right++)

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

#define _read_register1(reg)     _read_register3(reg, u64, "")
#define _read_register2(reg, ty) _read_register3(reg, ty, "")
#define _read_register3(reg, ty, suffix)                                                           \
  ({                                                                                               \
    _Static_assert(sizeof(ty) <= 8, "read_register only takes a type with size <= 8");             \
    ty value;                                                                                      \
    asm("mov" suffix " %%" #reg ", %0" : "=r"(value));                                             \
    value;                                                                                         \
  })
#define read_register(...) PASTE(_read_register, NARG(__VA_ARGS__))(__VA_ARGS__)

#define _write_register2(reg, value) _write_register3(reg, value, "")
#define _write_register3(reg, value, suffix)                                                       \
  ({                                                                                               \
    _Static_assert(sizeof(value) <= 8, "read_register only takes a type with size <= 8");          \
    typeof(value) v = value;                                                                       \
    asm("mov" suffix " %0, %%" #reg : : "r"(v) : #reg);                                            \
    v;                                                                                             \
  })
#define write_register(...) PASTE(_write_register, NARG(__VA_ARGS__))(__VA_ARGS__)

#define out8(port, val)                                                                            \
  ({                                                                                               \
    u8 v = val;                                                                                    \
    asm volatile("outb %0, %1" : : "a"(v), "Nd"(port));                                            \
    v;                                                                                             \
  })

#define in8(port)                                                                                  \
  ({                                                                                               \
    u8 ret;                                                                                        \
    u16 p = port;                                                                                  \
    asm volatile("inb %1, %0" : "=a"(ret) : "Nd"(p));                                              \
    ret;                                                                                           \
  })

#define min(x, y)                                                                                  \
  ({                                                                                               \
    typeof(x + y) _x = x, _y = y;                                                                  \
    (_x > _y) ? _y : _x;                                                                           \
  })
#define max(x, y)                                                                                  \
  ({                                                                                               \
    typeof(x + y) _x = x, _y = y;                                                                  \
    (_x > _y) ? _x : _y;                                                                           \
  })

s64 smallest_greater_power2(s64 value);
u64 align_up(u64 value, u64 alignment);
u64 align_down(u64 value, u64 alignment);

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

void memset(void *buffer, u8 byte, s64 len);

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
