#include <types.h>

void logging__log(sloc loc, int32_t count, const any *args);
void logging__log_fmt(sloc loc, const char *fmt, s32 count, const any *args);

#define log(...) logging__log(__LOC__, NARG(__VA_ARGS__), make_any_array(__VA_ARGS__))

#define log_fmt(fmt, ...)                                                                          \
  logging__log_fmt(__LOC__, fmt, NARG(__VA_ARGS__), make_any_array(__VA_ARGS__))

#define panic(...)                                                                                 \
  ((NARG(__VA_ARGS__) ? log_fmt("" __VA_ARGS__) : log_fmt("panicked!")), shutdown())

#define assert(expression, ...)                                                                    \
  ((expression)                                                                                    \
       ?: (NARG(__VA_ARGS__) ? panic("assertion failed: " __VA_ARGS__)                             \
                             : panic("assertion failed: `%f`", #expression)))

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
    logging__log_fmt(__LOC__, fmt, nargs, args);                               \
    _Pragma("clang diagnostic pop");                                           \
  })
// clang-format on
