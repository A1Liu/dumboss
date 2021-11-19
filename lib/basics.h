#ifndef __LIB_BASICS__
#define __LIB_BASICS__
#include <types.h>

typedef struct {
  u8 *const begin;
  const s64 count;
  s64 index;
} Bump;

Bump Bump__new(s64 count);
void *Bump__bump_impl(Bump *bump, s64 size, s64 align);
#define Bump__bump(bump, ty) ((ty *)Bump__bump_impl(bump, sizeof(ty), _Alignof(ty)))

String Str__new(char *data, s64 count);
bool Str__is_null(String str);
String Str__slice(String str, s64 begin, s64 end);
String Str__suffix(String str, s64 begin);

// If return value is positive, formatter tried to write that many bytes to
// provided buffer; If negative, the formatter to format the argument
s64 any__fmt_any(String out, any value);

// If return value is positive, formatter tried to write that many bytes to
// provided buffer; If negative, the formatter failed on that argument
// (one-indexed)
s64 any__fmt(String out, const char *fmt, s32 count, const any *args);

// Formats a u64. Returns the length of buffer needed to output this number. If
// return value is smaller than `size`, then the writing succedded
s64 fmt_u64(String out, u64 value);

// Formats a i64. Returns the length of buffer needed to output this number. If
// return value is smaller than size parameter, then the writing succedded
s64 fmt_i64(String out, s64 value);

s64 smallest_greater_power2(s64 _value);

// Tries to write null-terminated string `src` into `dest`
// char *strcpy(char *dest, const char *src);

// Tries to write null-terminated string `src` into `dest`, stopping when
// `dest.size` is reached, and returning the amount of data written
s64 strcpy_s(String dest, const char *src);
// Returns the length of a null-terminated string
s64 strlen(const char *str);
void memcpy(void *dest, const void *src, s64 count);
void memset(void *buffer, u8 value, s64 len);

#define min(x, y)                                                                                  \
  ({                                                                                               \
    typeof(x + y) _x = x, _y = y;                                                                  \
    (_x > _y) ? _y : _x;                                                                           \
  })
#define max(x, y)                                                                                  \
  ({                                                                                               \
    typeof(x + y) _x = x, _y = y;                                                                  \
    (_x > _y) ? _x : _y;                                                                           \
  })

#define align_up(value, _align)                                                                    \
  ({                                                                                               \
    const u64 M_align = _align;                                                                    \
    assert(M_align && (__builtin_popcountl(M_align) == 1), "alignment wasn't a power of 2");       \
    __builtin_align_up(value, M_align);                                                            \
  })
#define align_down(value, _align)                                                                  \
  ({                                                                                               \
    const u64 M_align = _align;                                                                    \
    assert(M_align && (__builtin_popcountl(M_align) == 1), "alignment wasn't a power of 2");       \
    __builtin_align_down(value, M_align);                                                          \
  })
#define is_aligned(value, _align)                                                                  \
  ({                                                                                               \
    const u64 M_align = _align;                                                                    \
    assert(M_align && (__builtin_popcountl(M_align) == 1), "alignment wasn't a power of 2");       \
    __builtin_is_aligned(value, M_align);                                                          \
  })

#endif

#ifdef __DUMBOSS_IMPL__
#undef __DUMBOSS_IMPL__
#include <external.h>
#include <macros.h>
#include <types.h>
#define __DUMBOSS_IMPL__

Bump Bump__new(s64 count) {
  void *base = ext__alloc_pages(count);
  assert(base);

  return (Bump){.begin = base, .count = count * _4KB, .index = 0};
}

void *Bump__bump_impl(Bump *bump, s64 size, s64 align) {
  s64 aligned_index = align_up(bump->index, align);
  s64 end_index = aligned_index + size;
  ensure(end_index <= bump->count) return NULL;

  u8 *ptr = bump->begin + aligned_index;
  bump->index = end_index;
  return ptr;
}

s64 smallest_greater_power2(s64 _value) {
  assert(_value >= 0);
  u64 value = (u64)_value;

  if (value <= 1) return 0;
  return 64 - __builtin_clzl(value - 1);
}

void memcpy(void *_dest, const void *_src, s64 count) {
  u8 *dest = _dest;
  const u8 *src = _src;
  FOR_PTR(src, count) {
    dest[index] = *it;
  }
}

void memset(void *_buffer, u8 byte, s64 len) {
  u8 *buffer = _buffer;
  for (s64 i = 0; i < len; i++)
    buffer[i] = byte;
}

String Str__new(char *data, s64 count) {
  if (count < 0) return (String){.data = NULL, .count = 0};
  return (String){.data = data, .count = count};
}

bool Str__is_null(String str) {
  return str.data == NULL || str.count == 0;
}

String Str__slice(String str, s64 begin, s64 end) {
  assert(0 <= begin);
  assert(begin <= end);
  assert(end <= str.count);

  return (String){.data = str.data + begin, .count = end - begin};
}

String Str__suffix(String str, s64 begin) {
  assert(0 <= begin);
  assert(begin <= str.count);

  return (String){.data = str.data + begin, .count = str.count - begin};
}

// If return value is positive, formatter tried to write that many bytes to
// provided buffer; If negative, the formatter to format the argument
s64 any__fmt_any(String out, any value) {
  switch (value.type) {
  case type_id_bool: {
    const char *const src = value.bool_value ? "true" : "false";

    // @Safety there better not be any C-strings that are over 2^63 bytes long
    s64 written = (s64)strcpy_s(out, src);
    return written + (s64)strlen(src + written);
  }
    return (s64)fmt_u64(out, value.u64_value);
  case type_id_u8:
    return (s64)fmt_u64(out, value.u64_value);
  case type_id_i8:
    return (s64)fmt_i64(out, value.i64_value);
  case type_id_u16:
    return (s64)fmt_u64(out, value.u64_value);
  case type_id_i16:
    return (s64)fmt_i64(out, value.i64_value);
  case type_id_u32:
    return (s64)fmt_u64(out, value.u64_value);
  case type_id_i32:
    return (s64)fmt_i64(out, value.i64_value);
  case type_id_u64:
    return (s64)fmt_u64(out, value.u64_value);
  case type_id_i64:
    return (s64)fmt_i64(out, value.i64_value);

  case type_id_char: {
    if (out.count >= 1) *out.data = value.char_value;
    return 1;
  }

  case type_id_char_ptr: {
    char *src = (char *)value.ptr;
    if (src == NULL) return 0;

    // @Safety there better not be any C-strings that are over 2^63 bytes long
    s64 written = (s64)strcpy_s(out, src);
    return written + (s64)strlen(src + written);
  }

  default: // TODO what should we do here?
    return -1;
  }
}

// If return value is positive, formatter tried to write that many bytes to
// provided buffer; If negative, the formatter failed on that argument
// (one-indexed)
s64 any__fmt(String out, const char *fmt, s32 count, const any *args) {
  // TODO why would a string be larger than s64's max value?
  const s64 len = (s64)out.count;
  s64 format_count = 0, written = 0;

  while (*fmt) {
    if (*fmt != '%') {
      if (written < len) out.data[written] = *fmt;
      written++;
      fmt++;
      continue;
    }

    fmt++;
    if (*fmt == '%') {
      if (written < len) out.data[written] = '%';
      written++;
      fmt++;
      continue;
    }

    if (*fmt != 'f') return -format_count - 1;
    fmt++;

    // TODO how should we handle this? It's definitely a bug, and this case is
    // the scary one we don't ever want to happen
    if (format_count >= count) return -format_count - 1;

    s64 fmt_try = any__fmt_any(Str__suffix(out, min(written, len)), args[format_count]);
    if (fmt_try < 0) return -format_count - 1;
    written += fmt_try;
    format_count++;
  }

  // TODO how should we handle this? It's a bug, but it's kinda fine
  if (format_count != count) return -format_count - 1;
  return written;
}

///////////////////////////////////////////////////////////////////////////////
// \author (c) Marco Paland (info@paland.com)
//             2014-2019, PALANDesign Hannover, Germany
//
// \license The MIT License (MIT)
//
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:
//
// The above copyright notice and this permission notice shall be included in
// all copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
// THE SOFTWARE.
//
// \brief Tiny printf, sprintf and (v)snprintf implementation, optimized for
// speed on
//        embedded systems with a very limited resources. These routines are
//        thread safe and reentrant! Use this instead of the bloated
//        standard/newlib printf cause these use malloc for printf (and may not
//        be thread safe).
//
///////////////////////////////////////////////////////////////////////////////

// output function type
typedef void (*out_fct_type)(char character, void *buffer, u64 idx, u64 maxlen);

// internal buffer output
static inline void _out_buffer(char character, void *buffer, u64 idx, u64 maxlen) {
  if (idx < maxlen) {
    ((char *)buffer)[idx] = character;
  }
}

static u64 _ntoa_long(out_fct_type out, char *buffer, u64 idx, u64 maxlen, u64 value, bool negative,
                      unsigned long base, u32 prec, u32 width, u32 flags);

s64 fmt_u64(String out, u64 value) {
  return (s64)_ntoa_long(_out_buffer, out.data, 0, (u64)out.count, value, false, 10, 0, 0, 0);
}
s64 fmt_i64(String out, s64 value) {
  return (s64)_ntoa_long(_out_buffer, out.data, 0, (u64)out.count,
                         (u64)(value < 0 ? -value : value), value < 0, 10, 0, 0, 0);
}

s64 strlen(const char *str) {
  if (str == NULL) return 0;

  s64 i = 0;
  for (; *str; i++, str++)
    ;

  return i;
}

// char *strcpy(char *dest, const char *src) {
//   if (src == NULL)
//     return dest;
//
//   for (; *src; dest++, src++)
//     *dest = *src;
//
//   return dest;
// }

s64 strcpy_s(String dest, const char *src) {
  if (src == NULL) return 0;

  s64 written = 0;
  for (; written < dest.count && src[written]; written++)
    dest.data[written] = src[written];

  return written;
}

// 'ntoa' conversion buffer size, this must be big enough to hold one converted
// numeric number including padded zeros (dynamically created on stack)
// default: 32 byte
#define PRINTF_NTOA_BUFFER_SIZE 32U

// 'ftoa' conversion buffer size, this must be big enough to hold one converted
// float number including padded zeros (dynamically created on stack)
// default: 32 byte
// #define PRINTF_FTOA_BUFFER_SIZE 32U

// support for the floating point type (%f)
// default: activated
// #define PRINTF_SUPPORT_FLOAT

// support for exponential floating point notation (%e/%g)
// default: activated
// #define PRINTF_SUPPORT_EXPONENTIAL

// define the default floating point precision
// default: 6 digits
// #define PRINTF_DEFAULT_FLOAT_PRECISION 6U

// define the largest float suitable to print with %f
// default: 1e9
// #define PRINTF_MAX_FLOAT 1e9

// support for the long long types (%llu or %p)
// default: activated
// #define PRINTF_SUPPORT_LONG_LONG

// support for the ptrdiff_t type (%t)
// ptrdiff_t is normally defined in <stddef.h> as long or long long type
// default: activated
// #define PRINTF_SUPPORT_PTRDIFF_T

///////////////////////////////////////////////////////////////////////////////

// internal flag definitions
#define FLAGS_ZEROPAD   (1U << 0U)
#define FLAGS_LEFT      (1U << 1U)
#define FLAGS_PLUS      (1U << 2U)
#define FLAGS_SPACE     (1U << 3U)
#define FLAGS_HASH      (1U << 4U)
#define FLAGS_UPPERCASE (1U << 5U)
#define FLAGS_CHAR      (1U << 6U)
#define FLAGS_SHORT     (1U << 7U)
#define FLAGS_LONG      (1U << 8U)
#define FLAGS_LONG_LONG (1U << 9U)
#define FLAGS_PRECISION (1U << 10U)
#define FLAGS_ADAPT_EXP (1U << 11U)

// internal output function wrapper
// internal secure strlen
// \return The length of the string (excluding the terminating 0) limited by
// 'maxsize'
// static inline u32 _strnlen_s(const char *str, u64 maxsize) {
//   const char *s;
//   for (s = str; *s && maxsize--; ++s)
//     ;
//   return (u32)(s - str);
// }

// internal test if char is a digit (0-9)
// \return true if char is a digit
// static inline bool _is_digit(char ch) { return (ch >= '0') && (ch <= '9'); }

// internal ASCII string to u32 conversion
// static u32 _atoi(const char **str) {
//   u32 i = 0U;
//   while (_is_digit(**str)) {
//     i = i * 10U + (u32)(*((*str)++) - '0');
//   }
//   return i;
// }

// output the specified string in reverse, taking care of any zero-padding
static u64 _out_rev(out_fct_type out, char *buffer, u64 idx, u64 maxlen, const char *buf, u64 len,
                    u32 width, u32 flags) {
  const u64 start_idx = idx;

  // pad spaces up to given width
  if (!(flags & FLAGS_LEFT) && !(flags & FLAGS_ZEROPAD)) {
    for (u64 i = len; i < width; i++) {
      out(' ', buffer, idx++, maxlen);
    }
  }

  // reverse string
  while (len) {
    out(buf[--len], buffer, idx++, maxlen);
  }

  // append pad spaces up to given width
  if (flags & FLAGS_LEFT) {
    while (idx - start_idx < width) {
      out(' ', buffer, idx++, maxlen);
    }
  }

  return idx;
}

// internal itoa format
static u64 _ntoa_format(out_fct_type out, char *buffer, u64 idx, u64 maxlen, char *buf, u64 len,
                        bool negative, u32 base, u32 prec, u32 width, u32 flags) {
  // pad leading zeros
  if (!(flags & FLAGS_LEFT)) {
    if (width && (flags & FLAGS_ZEROPAD) && (negative || (flags & (FLAGS_PLUS | FLAGS_SPACE)))) {
      width--;
    }
    while ((len < prec) && (len < PRINTF_NTOA_BUFFER_SIZE)) {
      buf[len++] = '0';
    }
    while ((flags & FLAGS_ZEROPAD) && (len < width) && (len < PRINTF_NTOA_BUFFER_SIZE)) {
      buf[len++] = '0';
    }
  }

  // handle hash
  if (flags & FLAGS_HASH) {
    if (!(flags & FLAGS_PRECISION) && len && ((len == prec) || (len == width))) {
      len--;
      if (len && (base == 16U)) {
        len--;
      }
    }
    if ((base == 16U) && !(flags & FLAGS_UPPERCASE) && (len < PRINTF_NTOA_BUFFER_SIZE)) {
      buf[len++] = 'x';
    } else if ((base == 16U) && (flags & FLAGS_UPPERCASE) && (len < PRINTF_NTOA_BUFFER_SIZE)) {
      buf[len++] = 'X';
    } else if ((base == 2U) && (len < PRINTF_NTOA_BUFFER_SIZE)) {
      buf[len++] = 'b';
    }
    if (len < PRINTF_NTOA_BUFFER_SIZE) {
      buf[len++] = '0';
    }
  }

  if (len < PRINTF_NTOA_BUFFER_SIZE) {
    if (negative) {
      buf[len++] = '-';
    } else if (flags & FLAGS_PLUS) {
      buf[len++] = '+'; // ignore the space if the '+' exists
    } else if (flags & FLAGS_SPACE) {
      buf[len++] = ' ';
    }
  }

  return _out_rev(out, buffer, idx, maxlen, buf, len, width, flags);
}

// internal itoa for 'long' type
// TODO change this `u32 value` to a `u64 value` once we're on x86_64
static u64 _ntoa_long(out_fct_type out, char *buffer, u64 idx, u64 maxlen, u64 value, bool negative,
                      unsigned long base, u32 prec, u32 width, u32 flags) {
  char buf[PRINTF_NTOA_BUFFER_SIZE];
  u64 len = 0U;

  // no hash for 0 values
  if (!value) {
    flags &= ~FLAGS_HASH;
  }

  // write if precision != 0 and value is != 0
  if (!(flags & FLAGS_PRECISION) || value) {
    do {
      const char digit = (char)(value % base);
      buf[len++] = digit < 10 ? '0' + digit : (flags & FLAGS_UPPERCASE ? 'A' : 'a') + digit - 10;
      value /= base;
    } while (value && (len < PRINTF_NTOA_BUFFER_SIZE));
  }

  return _ntoa_format(out, buffer, idx, maxlen, buf, len, negative, (u32)base, prec, width, flags);
}

#if defined(PRINTF_SUPPORT_FLOAT)

#if defined(PRINTF_SUPPORT_EXPONENTIAL)
// forward declaration so that _ftoa can switch to exp notation for values >
// PRINTF_MAX_FLOAT
static u64 _etoa(out_fct_type out, char *buffer, u64 idx, u64 maxlen, double value, u32 prec,
                 u32 width, u32 flags);
#endif

// internal ftoa for fixed decimal floating point
static u64 _ftoa(out_fct_type out, char *buffer, u64 idx, u64 maxlen, double value, u32 prec,
                 u32 width, u32 flags) {
  char buf[PRINTF_FTOA_BUFFER_SIZE];
  u64 len = 0U;
  double diff = 0.0;

  // powers of 10
  static const double pow10[] = {1,      10,      100,      1000,      10000,
                                 100000, 1000000, 10000000, 100000000, 1000000000};

  // test for special values
  if (value != value) return _out_rev(out, buffer, idx, maxlen, "nan", 3, width, flags);
  if (value < -DBL_MAX) return _out_rev(out, buffer, idx, maxlen, "fni-", 4, width, flags);
  if (value > DBL_MAX)
    return _out_rev(out, buffer, idx, maxlen, (flags & FLAGS_PLUS) ? "fni+" : "fni",
                    (flags & FLAGS_PLUS) ? 4U : 3U, width, flags);

  // test for very large values
  // standard printf behavior is to print EVERY whole number digit -- which
  // could be 100s of characters overflowing your buffers == bad
  if ((value > PRINTF_MAX_FLOAT) || (value < -PRINTF_MAX_FLOAT)) {
#if defined(PRINTF_SUPPORT_EXPONENTIAL)
    return _etoa(out, buffer, idx, maxlen, value, prec, width, flags);
#else
    return 0U;
#endif
  }

  // test for negative
  bool negative = false;
  if (value < 0) {
    negative = true;
    value = 0 - value;
  }

  // set default precision, if not set explicitly
  if (!(flags & FLAGS_PRECISION)) {
    prec = PRINTF_DEFAULT_FLOAT_PRECISION;
  }
  // limit precision to 9, cause a prec >= 10 can lead to overflow errors
  while ((len < PRINTF_FTOA_BUFFER_SIZE) && (prec > 9U)) {
    buf[len++] = '0';
    prec--;
  }

  s32 whole = (s32)value;
  double tmp = (value - whole) * pow10[prec];
  unsigned long frac = (u64)tmp;
  diff = tmp - frac;

  if (diff > 0.5) {
    ++frac;
    // handle rollover, e.g. case 0.99 with prec 1 is 1.0
    if (frac >= pow10[prec]) {
      frac = 0;
      ++whole;
    }
  } else if (diff < 0.5) {
  } else if ((frac == 0U) || (frac & 1U)) {
    // if halfway, round up if odd OR if last digit is 0
    ++frac;
  }

  if (prec == 0U) {
    diff = value - (double)whole;
    if ((!(diff < 0.5) || (diff > 0.5)) && (whole & 1)) {
      // exactly 0.5 and ODD, then round up
      // 1.5 -> 2, but 2.5 -> 2
      ++whole;
    }
  } else {
    u32 count = prec;
    // now do fractional part, as an unsigned number
    while (len < PRINTF_FTOA_BUFFER_SIZE) {
      --count;
      buf[len++] = (char)(48U + (frac % 10U));
      if (!(frac /= 10U)) {
        break;
      }
    }
    // add extra 0s
    while ((len < PRINTF_FTOA_BUFFER_SIZE) && (count-- > 0U)) {
      buf[len++] = '0';
    }
    if (len < PRINTF_FTOA_BUFFER_SIZE) {
      // add decimal
      buf[len++] = '.';
    }
  }

  // do whole part, number is reversed
  while (len < PRINTF_FTOA_BUFFER_SIZE) {
    buf[len++] = (char)(48 + (whole % 10));
    if (!(whole /= 10)) {
      break;
    }
  }

  // pad leading zeros
  if (!(flags & FLAGS_LEFT) && (flags & FLAGS_ZEROPAD)) {
    if (width && (negative || (flags & (FLAGS_PLUS | FLAGS_SPACE)))) {
      width--;
    }
    while ((len < width) && (len < PRINTF_FTOA_BUFFER_SIZE)) {
      buf[len++] = '0';
    }
  }

  if (len < PRINTF_FTOA_BUFFER_SIZE) {
    if (negative) {
      buf[len++] = '-';
    } else if (flags & FLAGS_PLUS) {
      buf[len++] = '+'; // ignore the space if the '+' exists
    } else if (flags & FLAGS_SPACE) {
      buf[len++] = ' ';
    }
  }

  return _out_rev(out, buffer, idx, maxlen, buf, len, width, flags);
}

#if defined(PRINTF_SUPPORT_EXPONENTIAL)
// internal ftoa variant for exponential floating-point type, contributed by
// Martijn Jasperse <m.jasperse@gmail.com>
static u64 _etoa(out_fct_type out, char *buffer, u64 idx, u64 maxlen, double value, u32 prec,
                 u32 width, u32 flags) {
  // check for NaN and special values
  if ((value != value) || (value > DBL_MAX) || (value < -DBL_MAX)) {
    return _ftoa(out, buffer, idx, maxlen, value, prec, width, flags);
  }

  // determine the sign
  const bool negative = value < 0;
  if (negative) {
    value = -value;
  }

  // default precision
  if (!(flags & FLAGS_PRECISION)) {
    prec = PRINTF_DEFAULT_FLOAT_PRECISION;
  }

  // determine the decimal exponent
  // based on the algorithm by David Gay (https://www.ampl.com/netlib/fp/dtoa.c)
  union {
    u64 U;
    double F;
  } conv;

  conv.F = value;
  s32 exp2 = (s32)((conv.U >> 52U) & 0x07FFU) - 1023; // effectively log2
  conv.U = (conv.U & ((1ULL << 52U) - 1U)) |
           (1023ULL << 52U); // drop the exponent so conv.F is now in [1,2)
  // now approximate log10 from the log2 integer part and an expansion of ln
  // around 1.5
  int expval =
      (int)(0.1760912590558 + exp2 * 0.301029995663981 + (conv.F - 1.5) * 0.289529654602168);
  // now we want to compute 10^expval but we want to be sure it won't overflow
  exp2 = (int)(expval * 3.321928094887362 + 0.5);
  const double z = expval * 2.302585092994046 - exp2 * 0.6931471805599453;
  const double z2 = z * z;
  conv.U = (u64)(exp2 + 1023) << 52U;
  // compute exp(z) using continued fractions, see
  // https://en.wikipedia.org/wiki/Exponential_function#Continued_fractions_for_ex
  conv.F *= 1 + 2 * z / (2 - z + (z2 / (6 + (z2 / (10 + z2 / 14)))));
  // correct for rounding errors
  if (value < conv.F) {
    expval--;
    conv.F /= 10;
  }

  // the exponent format is "%+03d" and largest value is "307", so set aside 4-5
  // characters
  u32 minwidth = ((expval < 100) && (expval > -100)) ? 4U : 5U;

  // in "%g" mode, "prec" is the number of *significant figures* not decimals
  if (flags & FLAGS_ADAPT_EXP) {
    // do we want to fall-back to "%f" mode?
    if ((value >= 1e-4) && (value < 1e6)) {
      if ((s32)prec > expval) {
        prec = (u32)((s32)prec - expval - 1);
      } else {
        prec = 0;
      }
      flags |= FLAGS_PRECISION; // make sure _ftoa respects precision
      // no characters in exponent
      minwidth = 0U;
      expval = 0;
    } else {
      // we use one sigfig for the whole part
      if ((prec > 0) && (flags & FLAGS_PRECISION)) {
        --prec;
      }
    }
  }

  // will everything fit?
  u32 fwidth = width;
  if (width > minwidth) {
    // we didn't fall-back so subtract the characters required for the exponent
    fwidth -= minwidth;
  } else {
    // not enough characters, so go back to default sizing
    fwidth = 0U;
  }
  if ((flags & FLAGS_LEFT) && minwidth) {
    // if we're padding on the right, DON'T pad the floating part
    fwidth = 0U;
  }

  // rescale the float value
  if (expval) {
    value /= conv.F;
  }

  // output the floating part
  const u64 start_idx = idx;
  idx = _ftoa(out, buffer, idx, maxlen, negative ? -value : value, prec, fwidth,
              flags & ~FLAGS_ADAPT_EXP);

  // output the exponent part
  if (minwidth) {
    // output the exponential symbol
    out((flags & FLAGS_UPPERCASE) ? 'E' : 'e', buffer, idx++, maxlen);
    // output the exponent value
    idx = _ntoa_long(out, buffer, idx, maxlen, (expval < 0) ? -expval : expval, expval < 0, 10, 0,
                     minwidth - 1, FLAGS_ZEROPAD | FLAGS_PLUS);
    // might need to right-pad spaces
    if (flags & FLAGS_LEFT) {
      while (idx - start_idx < width)
        out(' ', buffer, idx++, maxlen);
    }
  }
  return idx;
}
#endif // PRINTF_SUPPORT_EXPONENTIAL
#endif // PRINTF_SUPPORT_FLOAT

#endif
