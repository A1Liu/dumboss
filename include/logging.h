#pragma once

#include "basics.h"

void logging__log(sloc loc, int32_t count, any *args);
void logging__log_fmt(sloc loc, const char *fmt, int32_t count, any *args);
_Noreturn void logging__panic(sloc loc, const char *message);

#define __DEBUG_STRINGIFY(x) "`" #x "` = %"
#define __COMMA_STRING() ", "

#define log(...)                                                               \
  logging__log(__LOC__, NARG(__VA_ARGS__),                                     \
               (any[]){FOR_EACH(make_any, __VA_ARGS__)})

#define debug(...)                                                             \
  logging__log_fmt(__LOC__,                                                    \
                   "debug(" FOR_EACH_SEP(__DEBUG_STRINGIFY, __COMMA_STRING,    \
                                         __VA_ARGS__) ")",                     \
                   NARG(__VA_ARGS__),                                          \
                   (any[]){FOR_EACH(make_any, __VA_ARGS__)})

#define log_fmt(fmt, ...)                                                      \
  logging__log_fmt(__LOC__, fmt, NARG(__VA_ARGS__),                            \
                   (any[]){FOR_EACH(make_any, __VA_ARGS__)})

#define panic() logging__panic(__LOC__, "panicked!")

#define assert(expression)                                                     \
  ((expression)                                                                \
       ?: (logging__panic(__LOC__,                                             \
                          "assertion failed: `" #expression "` = false"),      \
           expression))
