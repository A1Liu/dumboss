#ifndef __LIB_BITSET__
#define __LIB_BITSET__
#include <types.h>

typedef struct {
  u64 *data;
  s64 count;
} BitSet;

BitSet BitSet__from_raw(u64 *data, s64 size);
bool BitSet__get(BitSet bits, s64 idx);
bool BitSet__get_all(BitSet bits, s64 begin, s64 end);
bool BitSet__get_any(BitSet bits, s64 begin, s64 end);
void BitSet__set(BitSet bits, s64 idx, bool value);
void BitSet__set_all(BitSet bits, bool value);
void BitSet__set_range(BitSet bits, s64 begin, s64 end, bool value);
#endif

#ifdef __DUMBOSS_IMPL__
#undef __DUMBOSS_IMPL__
#include <log.h>
#include <macros.h>
#define __DUMBOSS_IMPL__

BitSet BitSet__from_raw(u64 *data, s64 count) {
  return (BitSet){.data = data, .count = count};
}

bool BitSet__get(const BitSet bits, s64 idx) {
  assert(0 <= idx);
  assert(idx < bits.count);

  const u64 v = bits.data[idx / 64];
  u32 bit_offset = idx % 64;
  return 0 != ((U64(1) << bit_offset) & v);
}

bool BitSet__get_all(const BitSet bits, s64 begin, s64 end) {
  assert(0 <= begin);
  assert(begin <= end);
  assert(end <= bits.count);

  s64 fast_begin = (s64)align_up((u64)begin, 64), fast_end = (s64)align_down((u64)end, 64);

  if (fast_begin >= fast_end) {
    for (s64 i = begin; i < end; i++) {
      if (!BitSet__get(bits, i)) return false;
    }

    return true;
  }

  for (s64 i = begin; i < fast_begin; i++)
    if (!BitSet__get(bits, i)) return false;
  for (s64 i = fast_begin / 64; i < fast_end / 64; i++)
    if (bits.data[i] != ~U64(0)) return false;
  for (s64 i = fast_end; i < end; i++)
    if (!BitSet__get(bits, i)) return false;

  return true;
}

bool BitSet__get_any(const BitSet bits, s64 begin, s64 end) {
  assert(0 <= begin);
  assert(begin <= end);
  assert(end <= bits.count);

  s64 fast_begin = (s64)align_up((u64)begin, 64), fast_end = (s64)align_down((u64)end, 64);

  if (fast_begin >= fast_end) {
    for (s64 i = begin; i < end; i++) {
      if (BitSet__get(bits, i)) return true;
    }

    return false;
  }

  for (s64 i = begin; i < fast_begin; i++)
    if (BitSet__get(bits, i)) return true;
  for (s64 i = fast_begin / 64; i < fast_end / 64; i++)
    if (bits.data[i]) return true;
  for (s64 i = fast_end; i < end; i++)
    if (BitSet__get(bits, i)) return true;

  return false;
}

void BitSet__set(const BitSet bits, s64 idx, bool value) {
  assert(0 <= idx);
  assert(idx < bits.count);

  u64 *v = &bits.data[idx / 64];
  u32 bit_offset = idx % 64;
  if (value) {
    *v |= ((u64)1) << bit_offset;
  } else {
    *v &= ~(((u64)1) << bit_offset);
  }
}

void BitSet__set_all(const BitSet bits, bool value) {
  for (s64 i = 0, idx = 0; idx < bits.count; i++, idx += 64) {
    bits.data[i] = 0;
    if (value) bits.data[i] = ~bits.data[i];
  }
}

void BitSet__set_range(const BitSet bits, s64 begin, s64 end, bool value) {
  assert(0 <= begin);
  assert(begin <= end);
  assert(end <= bits.count);

  s64 fast_begin = (s64)align_up((u64)begin, 64), fast_end = (s64)align_down((u64)end, 64);

  if (fast_begin >= fast_end) {
    for (s64 i = begin; i < end; i++)
      BitSet__set(bits, i, value);
    return;
  }

  for (s64 i = begin; i < fast_begin; i++)
    BitSet__set(bits, i, value);
  for (s64 i = fast_begin / 64; i < fast_end / 64; i++) {
    bits.data[i] = 0;
    if (value) bits.data[i] = ~bits.data[i];
  }
  for (s64 i = fast_end; i < end; i++)
    BitSet__set(bits, i, value);
}

#endif
