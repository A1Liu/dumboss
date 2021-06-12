#pragma once
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

// Formats a u64. Returns the length of buffer needed to output this number. If
// return value is smaller than `size`, then the writing succedded
uint64_t fmt_u64(uint64_t value, char *out, uint64_t size);

// Formats a i64. Returns the length of buffer needed to output this number. If
// return value is smaller than size parameter, then the writing succedded
uint64_t fmt_i64(int64_t value, char *out, uint64_t size);

// Returns the length of a null-terminated string
uint64_t strlen(char *str);

// Tries to write null-terminated string `src` into `dest`
char *strcpy(char *dest, const char *src);

// Tries to write null-terminated string `src` into `dest`, stopping after
// writing `size` bytes
uint64_t strcpy_s(char *dest, const char *src, uint64_t size);
