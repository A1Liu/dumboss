#pragma once
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#define min(x, y) ((x) > (y) ? (y) : (x))
#define max(x, y) ((x) < (y) ? (y) : (x))

static inline uint64_t align_up(uint64_t value, uint64_t alignment) {
  int64_t bits = 64 - __builtin_clzl(alignment - 1);
  return (((value - 1) >> bits) + 1) << bits;
}

static inline uint64_t align_down(uint64_t value, uint64_t alignment) {
  int64_t bits = 64 - __builtin_clzl(alignment - 1);
  return value >> bits << bits;
}

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

static inline BitSet BitSet__new(uint64_t *data, int64_t size) {
  return (BitSet){.data = data, .size = size};
}

static inline bool BitSet__get(BitSet bits, int64_t idx) {
  const uint64_t v = bits.data[idx / 64];
  uint32_t bit_offset = idx % 64;
  return 0 != ((((uint64_t)1) << bit_offset) & v);
}

static inline void BitSet__set(BitSet bits, int64_t idx, bool value) {
  uint64_t *v = &bits.data[idx / 64];
  uint32_t bit_offset = idx % 64;
  if (value) {
    *v |= ((uint64_t)1) << bit_offset;
  } else {
    *v &= ~(((uint64_t)1) << bit_offset);
  }
}

static inline void BitSet__set_all(BitSet bits, bool value) {
  for (int64_t i = 0, idx = 0; idx < bits.size; i++, idx += 64) {
    bits.data[i] = 0;
    if (value)
      bits.data[i] = ~bits.data[i];
  }
}
static inline void BitSet__set_range(BitSet bits, int64_t begin, int64_t end,
                                     bool value) {
  if (begin < 0 || end < 0 || end > begin)
    return;
  int64_t fast_begin = (int64_t)align_up((uint64_t)begin, 64),
          fast_end = (int64_t)align_down((uint64_t)end, 64);
  if (fast_begin >= fast_end) {
    for (int64_t i = begin; i < end; i++)
      BitSet__set(bits, i, value);
    return;
  }

  for (int64_t i = begin; i < fast_begin; i++)
    BitSet__set(bits, i, value);
  for (int64_t i = fast_begin / 64; i < fast_end / 64; i++) {
    bits.data[i] = 0;
    if (value)
      bits.data[i] = ~bits.data[i];
  }
  for (int64_t i = fast_end; i < end; i++)
    BitSet__set(bits, i, value);
}

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
