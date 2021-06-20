#pragma once
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#define min(x, y) ((x) > (y) ? (y) : (x))

typedef struct {
  uint8_t *data;
  uint64_t size;
} Buffer;

typedef struct {
  char *data;
  uint64_t size;
} String;

String string__slice(String str, int64_t begin, int64_t end);
String string__suffix(String str, int64_t begin);
String string__new(char *data, int64_t size);

// Formats a u64. Returns the length of buffer needed to output this number. If
// return value is smaller than `size`, then the writing succedded
uint64_t fmt_u64(String out, uint64_t value);

// Formats a i64. Returns the length of buffer needed to output this number. If
// return value is smaller than size parameter, then the writing succedded
uint64_t fmt_i64(String out, int64_t value);

// Returns the length of a null-terminated string
uint64_t strlen(const char *str);

// Tries to write null-terminated string `src` into `dest`
// char *strcpy(char *dest, const char *src);

// Tries to write null-terminated string `src` into `dest`, stopping when
// `dest.size` is reached, and returning the amount of data written
uint64_t strcpy_s(String dest, const char *src);
