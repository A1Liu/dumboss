#ifndef __LIB_MACROS__
#define __LIB_MACROS__
#include <macro_utils.h>
#include <types.h>

// TODO Should the formatting stuff in basics be moved to logging? I'm not sure
// where else in the codebase it would be used, and right now (Jun 27 2021) its
// only being used here.
#define SWAP(left, right)                                                                          \
  ({                                                                                               \
    typeof(*left) value = *left;                                                                   \
    *left = *right;                                                                                \
    *right = value;                                                                                \
  })

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
#define _NAMED_BREAK_HELPER(label, _internal_label)                                                \
  for (const struct LABEL_T_DO_NOT_USE *const _LABEL(label) =                                      \
           (const struct LABEL_T_DO_NOT_USE *const)&&_internal_label;                              \
       ; ({                                                                                        \
         _Pragma("clang diagnostic push \"-Wno-unused-label\"");                                   \
         (void)_LABEL(label);                                                                      \
       _internal_label:                                                                            \
         break;                                                                                    \
         _Pragma("clang diagnostic pop");                                                          \
       }))
#define NAMED_BREAK(label) _NAMED_BREAK_HELPER(label, _LABEL(__COUNTER__))

#define FOR_PTR(...)                   PASTE(_FOR_PTR, NARG(__VA_ARGS__))(__VA_ARGS__)
#define _FOR_PTR2(ptr, len)            _FOR_PTR(PASTE(M_FOR_, __COUNTER__), ptr, len, it, index)
#define _FOR_PTR3(ptr, len, it)        _FOR_PTR(PASTE(M_FOR_, __COUNTER__), ptr, len, it, index)
#define _FOR_PTR4(ptr, len, it, index) _FOR_PTR(PASTE(M_FOR_, __COUNTER__), ptr, len, it, index)
#define _FOR_PTR(_uniq, ptr, len, it, it_index)                                                    \
  NAMED_BREAK(it)                                                                                  \
  DECLARE_SCOPED(s64 PASTE(_uniq, M_idx) = 0, PASTE(_uniq, M_len) = (len))                         \
  DECLARE_SCOPED(s64 it_index = 0)                                                                 \
  DECLARE_SCOPED(typeof(&ptr[0]) PASTE(_uniq, M_ptr) = (ptr))                                      \
  for (typeof(&ptr[0]) it = PASTE(_uniq, M_ptr); PASTE(_uniq, M_idx) < PASTE(_uniq, M_len);        \
       PASTE(_uniq, M_ptr)++, PASTE(_uniq, M_idx)++, it = PASTE(_uniq, M_ptr),                     \
                       it_index = PASTE(_uniq, M_idx))

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
  NAMED_BREAK(it)                                                                                  \
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

#define make_any_array(...)                                                                        \
  (any[]) {                                                                                        \
    FOR_ARGS(make_any, __VA_ARGS__)                                                                \
  }

#define log(...)          ext__log(__LOC__, NARG(__VA_ARGS__), make_any_array(__VA_ARGS__))
#define log_fmt(fmt, ...) ext__log_fmt(__LOC__, fmt, NARG(__VA_ARGS__), make_any_array(__VA_ARGS__))

#define panic(...) ((NARG(__VA_ARGS__) ? log_fmt("" __VA_ARGS__) : log_fmt("panicked!")), exit(1))

#define __DEBUG_FORMAT(x) "%f"
#define __COMMA_STRING()  ", "

// pragmas here are to disable warnings for the debug output, because it doesn't
// really matter whether the last result is used when you're debugging.
// clang-format off
#define dbg(...)                                                               \
  ({                                                                           \
    _Pragma("clang diagnostic push \"-Wno-unused-value\"");                    \
    const char *const fmt = "dbg(%f) = ("                                      \
        FOR_ARGS_SEP(__DEBUG_FORMAT, __COMMA_STRING, __VA_ARGS__) ")";         \
    const any args[] = { FOR_ARGS(make_any, #__VA_ARGS__, ##__VA_ARGS__) };    \
    const s32 nargs = 1 + NARG(__VA_ARGS__);                                   \
    ext__log_fmt(__LOC__, fmt, nargs, args);                               \
    _Pragma("clang diagnostic pop");                                           \
  })
// clang-format on

#define assume(expression, ...)                                                                    \
  ((expression)                                                                                    \
       ?: (NARG(__VA_ARGS__) ? panic("false assumption: " __VA_ARGS__)                             \
                             : panic("false assumption: `%f`", #expression)))

#define assert(expression, ...)                                                                    \
  ((expression)                                                                                    \
       ?: (NARG(__VA_ARGS__) ? panic("failed assertion: " __VA_ARGS__)                             \
                             : panic("failed assertion: `%f`", #expression)))

/*
Ensure something is true.
Examples:

  ensure(x < 10); // this will assert if x >= 10
  ensure(x < 10) x = 2; // this will set x = 2 if x >= 10 (and also assert for safety
  ensure(x < 10) { // this will return NULL if x < 10
    return NULL;
    // it will also assert for safety if the block doesn't return
  }
*/
#define _ensure(_uniq, expr)                                                                       \
  for (typeof(expr) PASTE(_uniq, M_expr) = (expr); !PASTE(_uniq, M_expr); assert(expr))
#define ensure(expr) _ensure(PASTE(M_ENSURE_, __COUNTER__), expr)

/*
Prevent something from being true.
Examples:

  prevent(x < 10); // this will panic if x < 10
  prevent(x < 10) x = 10; // this will set x = 10 if x < 10 (and also assert for safety)
  prevent(x < 10) { // this will return NULL if x >= 10
    return NULL;
    // it will also assert for safety if the block doesn't return
  }
*/
#define _prevent(_uniq, expr)                                                                      \
  for (typeof(expr) PASTE(_uniq, M_expr) = (expr); PASTE(_uniq, M_expr); assert(!(expr)))
#define prevent(expr) _prevent(PASTE(M_PREVENT_, __COUNTER__), expr)

#endif
