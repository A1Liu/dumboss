#pragma once

// TODO Should the formatting stuff in basics be moved to logging? I'm not sure
// where else in the codebase it would be used, and right now (Jun 27 2021) its
// only being used here.
#include "basics.h"

void logging__log(sloc loc, int32_t count, any *args);
void logging__log_fmt(sloc loc, const char *fmt, int32_t count, any *args);

#define log(...)                                                               \
  logging__log(__LOC__, NARG(__VA_ARGS__), make_any_array(__VA_ARGS__))

#define log_fmt(fmt, ...)                                                      \
  logging__log_fmt(__LOC__, fmt, NARG(__VA_ARGS__), make_any_array(__VA_ARGS__))

#define panic(...)                                                             \
  ((NARG(__VA_ARGS__) ? log_fmt("" __VA_ARGS__) : log_fmt("panicked!")), exit())

#define assert(expression, ...)                                                \
  ((expression)                                                                \
       ?: ((NARG(__VA_ARGS__) ? panic("assertion failed:" __VA_ARGS__)         \
                              : panic("assertion failed: `%`", #expression)),  \
           expression))

#define __DEBUG_FORMAT(x) "%"
#define __COMMA_STRING() ", "

// pragmas here are to disable warnings for the debug output, because it doesn't
// really matter whether the last result is used when you're debugging.
#define dbg(...)                                                               \
  (_Pragma("clang diagnostic push") _Pragma(                                   \
       "clang diagnostic ignored \"-Wunused-value\"")                          \
       logging__log_fmt(__LOC__,                                               \
                        "dbg(%) = (" FOR_EACH_SEP(                             \
                            __DEBUG_FORMAT, __COMMA_STRING, __VA_ARGS__) ")",  \
                        1 + NARG(__VA_ARGS__),                                 \
                        make_any_array(#__VA_ARGS__, ##__VA_ARGS__)),          \
   ##__VA_ARGS__ _Pragma("clang diagnostic pop"))
