#pragma once
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#define min(x, y) ((x) > (y) ? (y) : (x))
#define max(x, y) ((x) < (y) ? (y) : (x))

int64_t smallest_greater_power2(int64_t value);
uint64_t align_up(uint64_t value, uint64_t alignment);
uint64_t align_down(uint64_t value, uint64_t alignment);

typedef struct {
  char *data;
  int64_t size;
} String;

typedef struct {
  uint64_t *data;
  int64_t size;
} BitSet;

String string__new(char *data, int64_t size);
String string__slice(String str, int64_t begin, int64_t end);
String string__suffix(String str, int64_t begin);

BitSet BitSet__new(uint64_t *data, int64_t size);
bool BitSet__get(BitSet bits, int64_t idx);
void BitSet__set(BitSet bits, int64_t idx, bool value);
void BitSet__set_all(BitSet bits, bool value);
void BitSet__set_range(BitSet bits, int64_t begin, int64_t end, bool value);

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
