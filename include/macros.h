#pragma once
#include "types.h"

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
#define MAKE_COMMA()        ,
#define FOR_ARGS(func, ...) PASTE(_ARGS_SEP, NARG(__VA_ARGS__))(func, MAKE_COMMA, ##__VA_ARGS__)

#define FOR_ARGS_SEP(func, sep, ...) PASTE(_ARGS_SEP, NARG(__VA_ARGS__))(func, sep, ##__VA_ARGS__)
#define _ARGS_SEP0(func, sep, ...)
#define _ARGS_SEP1(func, sep, _a0)           func(_a0)
#define _ARGS_SEP2(func, sep, _a0, _a1)      func(_a0) sep() func(_a1)
#define _ARGS_SEP3(func, sep, _a0, _a1, _a2) func(_a0) sep() func(_a1) sep() func(_a2)
#define _ARGS_SEP4(func, sep, _a0, _a1, _a2, _a3)                                                  \
  func(_a0) sep() func(_a1) sep() func(_a2) sep() func(_a3)
#define _ARGS_SEP5(func, sep, _a0, _a1, _a2, _a3, _a4)                                             \
  func(_a0) sep() func(_a1) sep() func(_a2) sep() func(_a3) sep() func(_a4)
#define _ARGS_SEP6(func, sep, _a0, _a1, _a2, _a3, _a4, _a5)                                        \
  func(_a0) sep() func(_a1) sep() func(_a2) sep() func(_a3) sep() func(_a4) sep() func(_a5)
#define _ARGS_SEP7(func, sep, _a0, _a1, _a2, _a3, _a4, _a5, _a6)                                   \
  func(_a0) sep() func(_a1) sep() func(_a2) sep() func(_a3) sep() func(_a4) sep() func(_a5) sep()  \
      func(_a6)
#define _ARGS_SEP8(func, sep, _a0, _a1, _a2, _a3, _a4, _a5, _a6, _a7)                              \
  func(_a0) sep() func(_a1) sep() func(_a2) sep() func(_a3) sep() func(_a4) sep() func(_a5) sep()  \
      func(_a6) sep() func(_a7)
#define _ARGS_SEP9(func, sep, _a0, _a1, _a2, _a3, _a4, _a5, _a6, _a7, _a8)                         \
  func(_a0) sep() func(_a1) sep() func(_a2) sep() func(_a3) sep() func(_a4) sep() func(_a5) sep()  \
      func(_a6) sep() func(_a7) sep() func(_a8)

// NOTE: This breaks compatibility with GCC. I guess that's fine, but like, seems
// weird and a bit uncomfy.
//                                    - Albert Liu, Oct 24, 2021 Sun 04:20 EDT
#define DECLARE_SCOPED(...) for (__VA_ARGS__;; ({ break; }))

#define _CMP_TYPE_HELPER(T1, T2) _Generic(((T1){0}), T2 : 1, default : 0)
#define CMP_TYPE(T1, T2)         (_CMP_TYPE_HELPER(T1, T2) && _CMP_TYPE_HELPER(T2, T1))

// NOTE: This forward declaration is required to compile on Clang. It's not clear
// whether it SHOULD be required, but it is.
//                        - Albert Liu, Nov 02, 2021 Tue 21:16 EDT
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

#define FOR_PTR(...)                   PASTE(_FOR_PTR, NARG(__VA_ARGS__))(__VA_ARGS__)
#define _FOR_PTR2(ptr, len)            _FOR_PTR(PASTE(M_FOR_, __COUNTER__), ptr, len, it, index)
#define _FOR_PTR3(ptr, len, it)        _FOR_PTR(PASTE(M_FOR_, __COUNTER__), ptr, len, it, index)
#define _FOR_PTR4(ptr, len, it, index) _FOR_PTR(PASTE(M_FOR_, __COUNTER__), ptr, len, it, index)
#define _FOR_PTR(_uniq, ptr, len, it, it_index)                                                    \
  BREAK_BLOCK(it)                                                                                  \
  DECLARE_SCOPED(s64 PASTE(_uniq, M_idx) = 0, PASTE(_uniq, M_len) = len)                           \
  DECLARE_SCOPED(s64 it_index = 0)                                                                 \
  DECLARE_SCOPED(typeof(&ptr[0]) PASTE(_uniq, M_ptr) = ptr)                                        \
  DECLARE_SCOPED(typeof(&ptr[0]) it = NULL)                                                        \
  for (it = PASTE(_uniq, M_ptr); PASTE(_uniq, M_idx) < PASTE(_uniq, M_len); PASTE(_uniq, M_ptr)++, \
      PASTE(_uniq, M_idx)++, it = PASTE(_uniq, M_ptr), it_index = PASTE(_uniq, M_idx))

#define FOR(...)                   PASTE(_FOR, NARG(__VA_ARGS__))(__VA_ARGS__)
#define _FOR1(array)               _FOR(PASTE(M_FOR_, __COUNTER__), array, it, it_index)
#define _FOR2(array, it)           _FOR(PASTE(M_FOR_, __COUNTER__), array, it, it_index)
#define _FOR3(array, it, it_index) _FOR(PASTE(M_FOR_, __COUNTER__), array, it, it_index)
#define _FOR(_uniq, array, it, it_index)                                                           \
  DECLARE_SCOPED(typeof(array) PASTE(_uniq, M_array) = array)                                      \
  _FOR_PTR(_uniq, PASTE(_uniq, M_array).data, PASTE(_uniq, M_array).count, it, it_index)

#define REPEAT(...)         PASTE(_REPEAT, NARG(__VA_ARGS__))(__VA_ARGS__)
#define _REPEAT1(times)     _REPEAT(PASTE(M_REPEAT_, __COUNTER__), times, it)
#define _REPEAT2(times, it) _REPEAT(PASTE(M_REPEAT_, __COUNTER__), times, it)
#define _REPEAT(_uniq, times, it)                                                                  \
  BREAK_BLOCK(it)                                                                                  \
  DECLARE_SCOPED(s64 PASTE(_uniq, M_it) = 0, PASTE(_uniq, M_end) = times)                          \
  for (s64 it = PASTE(_uniq, M_it); PASTE(_uniq, M_it) < PASTE(_uniq, M_end);                      \
       PASTE(_uniq, M_it)++, it = PASTE(_uniq, M_it))

// Does bubble sort stuff; user has to make the swaps themself
#define SLOW_SORT(array)                                                                           \
  DECLARE_SCOPED(typeof(array) M_array = array)                                                    \
  for (s64 M_right_bound = M_array.count - 1, M_left_index = 0; M_right_bound > 0;                 \
       M_right_bound--, M_left_index = 0)                                                          \
    for (typeof(M_array.data) left = M_array.data + M_left_index,                                  \
                              right = M_array.data + M_left_index + 1;                             \
         M_left_index < M_right_bound; M_left_index++, left++, right++)

#define read_register(...)       PASTE(_read_register, NARG(__VA_ARGS__))(__VA_ARGS__)
#define _read_register1(reg)     _read_register3(reg, u64, "")
#define _read_register2(reg, ty) _read_register3(reg, ty, "")
#define _read_register3(reg, ty, suffix)                                                           \
  ({                                                                                               \
    _Static_assert(sizeof(ty) <= 8, "read_register only takes a type with size <= 8");             \
    ty value;                                                                                      \
    asm("mov" suffix " %%" #reg ", %0" : "=r"(value));                                             \
    value;                                                                                         \
  })

#define write_register(...)          PASTE(_write_register, NARG(__VA_ARGS__))(__VA_ARGS__)
#define _write_register2(reg, value) _write_register3(reg, value, "")
#define _write_register3(reg, value, suffix)                                                       \
  ({                                                                                               \
    _Static_assert(sizeof(value) <= 8, "read_register only takes a type with size <= 8");          \
    typeof(value) v = value;                                                                       \
    asm("mov" suffix " %0, %%" #reg : : "r"(v) : #reg);                                            \
    v;                                                                                             \
  })

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
