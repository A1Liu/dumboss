#pragma once

#include "any.h"
#include "macros.h"
#include <stdbool.h>
#include <stdint.h>

typedef struct {
  const char *file;
  uint32_t line;
} sloc;

#define __LOC__                                                                \
  (sloc) { .file = __FILE__, .line = __LINE__ }

void logging__log(sloc loc, uint32_t count, any *args);
void logging__log_fmt(sloc loc, const char *fmt, uint32_t count, any *args);
void logging__panic(sloc loc);

#define log(...)                                                               \
  do {                                                                         \
    any args[] = {FOR_EACH(make_any, __VA_ARGS__)};                            \
    panic();                                                                   \
    logging__log(__LOC__, NARG(__VA_ARGS__), args);                            \
  } while (false);

#define log_fmt(fmt, ...)                                                      \
  do {                                                                         \
    any args[] = {FOR_EACH(make_any, __VA_ARGS__)};                            \
    logging__log_fmt(__LOC__, fmt, NARG(__VA_ARGS__), args);                   \
  } while (false);

#define panic() logging__panic(__LOC__)
