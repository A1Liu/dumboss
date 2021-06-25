#include "basics.h"

String string__new(char *data, int64_t size) {
  if (size < 0)
    return (String){.data = NULL, .size = 0};
  return (String){.data = data, .size = size};
}

String string__slice(String str, int64_t begin, int64_t end) {
  int64_t len = str.size;
  if (begin < 0 || end < 0 || begin >= len || end >= len || begin > end ||
      str.data == NULL)
    return (String){.data = NULL, .size = 0};
  return (String){.data = str.data + begin, .size = end - begin};
}
String string__suffix(String str, int64_t begin) {
  int64_t len = str.size;
  if (begin < 0 || begin >= len || str.data == NULL)
    return (String){.data = NULL, .size = 0};
  return (String){.data = str.data + begin, .size = len - begin};
}

// output function type
typedef void (*out_fct_type)(char character, void *buffer, uint64_t idx,
                             uint64_t maxlen);

// internal buffer output
static inline void _out_buffer(char character, void *buffer, uint64_t idx,
                               uint64_t maxlen) {
  if (idx < maxlen) {
    ((char *)buffer)[idx] = character;
  }
}

static uint64_t _ntoa_long(out_fct_type out, char *buffer, uint64_t idx,
                           uint64_t maxlen, uint64_t value, bool negative,
                           unsigned long base, uint32_t prec, uint32_t width,
                           uint32_t flags);

int64_t fmt_u64(String out, uint64_t value) {
  return (int64_t)_ntoa_long(_out_buffer, out.data, 0, (uint64_t)out.size,
                             value, false, 10, 0, 0, 0);
}
int64_t fmt_i64(String out, int64_t value) {
  return (int64_t)_ntoa_long(_out_buffer, out.data, 0, (uint64_t)out.size,
                             (uint64_t)(value < 0 ? -value : value), value < 0,
                             10, 0, 0, 0);
}

int64_t strlen(const char *str) {
  if (str == NULL)
    return 0;

  int64_t i = 0;
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

int64_t strcpy_s(String dest, const char *src) {
  if (src == NULL)
    return 0;

  int64_t written = 0;
  for (; written < dest.size && src[written]; written++)
    dest.data[written] = src[written];

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
#define FLAGS_ZEROPAD (1U << 0U)
#define FLAGS_LEFT (1U << 1U)
#define FLAGS_PLUS (1U << 2U)
#define FLAGS_SPACE (1U << 3U)
#define FLAGS_HASH (1U << 4U)
#define FLAGS_UPPERCASE (1U << 5U)
#define FLAGS_CHAR (1U << 6U)
#define FLAGS_SHORT (1U << 7U)
#define FLAGS_LONG (1U << 8U)
#define FLAGS_LONG_LONG (1U << 9U)
#define FLAGS_PRECISION (1U << 10U)
#define FLAGS_ADAPT_EXP (1U << 11U)

// internal output function wrapper
// internal secure strlen
// \return The length of the string (excluding the terminating 0) limited by
// 'maxsize'
// static inline uint32_t _strnlen_s(const char *str, uint64_t maxsize) {
//   const char *s;
//   for (s = str; *s && maxsize--; ++s)
//     ;
//   return (uint32_t)(s - str);
// }

// internal test if char is a digit (0-9)
// \return true if char is a digit
// static inline bool _is_digit(char ch) { return (ch >= '0') && (ch <= '9'); }

// internal ASCII string to uint32_t conversion
// static uint32_t _atoi(const char **str) {
//   uint32_t i = 0U;
//   while (_is_digit(**str)) {
//     i = i * 10U + (uint32_t)(*((*str)++) - '0');
//   }
//   return i;
// }

// output the specified string in reverse, taking care of any zero-padding
static uint64_t _out_rev(out_fct_type out, char *buffer, uint64_t idx,
                         uint64_t maxlen, const char *buf, uint64_t len,
                         uint32_t width, uint32_t flags) {
  const uint64_t start_idx = idx;

  // pad spaces up to given width
  if (!(flags & FLAGS_LEFT) && !(flags & FLAGS_ZEROPAD)) {
    for (uint64_t i = len; i < width; i++) {
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
static uint64_t _ntoa_format(out_fct_type out, char *buffer, uint64_t idx,
                             uint64_t maxlen, char *buf, uint64_t len,
                             bool negative, uint32_t base, uint32_t prec,
                             uint32_t width, uint32_t flags) {
  // pad leading zeros
  if (!(flags & FLAGS_LEFT)) {
    if (width && (flags & FLAGS_ZEROPAD) &&
        (negative || (flags & (FLAGS_PLUS | FLAGS_SPACE)))) {
      width--;
    }
    while ((len < prec) && (len < PRINTF_NTOA_BUFFER_SIZE)) {
      buf[len++] = '0';
    }
    while ((flags & FLAGS_ZEROPAD) && (len < width) &&
           (len < PRINTF_NTOA_BUFFER_SIZE)) {
      buf[len++] = '0';
    }
  }

  // handle hash
  if (flags & FLAGS_HASH) {
    if (!(flags & FLAGS_PRECISION) && len &&
        ((len == prec) || (len == width))) {
      len--;
      if (len && (base == 16U)) {
        len--;
      }
    }
    if ((base == 16U) && !(flags & FLAGS_UPPERCASE) &&
        (len < PRINTF_NTOA_BUFFER_SIZE)) {
      buf[len++] = 'x';
    } else if ((base == 16U) && (flags & FLAGS_UPPERCASE) &&
               (len < PRINTF_NTOA_BUFFER_SIZE)) {
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
// TODO change this `uint32_t value` to a `uint64_t value` once we're on x86_64
static uint64_t _ntoa_long(out_fct_type out, char *buffer, uint64_t idx,
                           uint64_t maxlen, uint64_t value, bool negative,
                           unsigned long base, uint32_t prec, uint32_t width,
                           uint32_t flags) {
  char buf[PRINTF_NTOA_BUFFER_SIZE];
  uint64_t len = 0U;

  // no hash for 0 values
  if (!value) {
    flags &= ~FLAGS_HASH;
  }

  // write if precision != 0 and value is != 0
  if (!(flags & FLAGS_PRECISION) || value) {
    do {
      const char digit = (char)(value % base);
      buf[len++] = digit < 10
                       ? '0' + digit
                       : (flags & FLAGS_UPPERCASE ? 'A' : 'a') + digit - 10;
      value /= base;
    } while (value && (len < PRINTF_NTOA_BUFFER_SIZE));
  }

  return _ntoa_format(out, buffer, idx, maxlen, buf, len, negative,
                      (uint32_t)base, prec, width, flags);
}

#if defined(PRINTF_SUPPORT_FLOAT)

#if defined(PRINTF_SUPPORT_EXPONENTIAL)
// forward declaration so that _ftoa can switch to exp notation for values >
// PRINTF_MAX_FLOAT
static uint64_t _etoa(out_fct_type out, char *buffer, uint64_t idx,
                      uint64_t maxlen, double value, uint32_t prec,
                      uint32_t width, uint32_t flags);
#endif

// internal ftoa for fixed decimal floating point
static uint64_t _ftoa(out_fct_type out, char *buffer, uint64_t idx,
                      uint64_t maxlen, double value, uint32_t prec,
                      uint32_t width, uint32_t flags) {
  char buf[PRINTF_FTOA_BUFFER_SIZE];
  uint64_t len = 0U;
  double diff = 0.0;

  // powers of 10
  static const double pow10[] = {1,         10,        100,     1000,
                                 10000,     100000,    1000000, 10000000,
                                 100000000, 1000000000};

  // test for special values
  if (value != value)
    return _out_rev(out, buffer, idx, maxlen, "nan", 3, width, flags);
  if (value < -DBL_MAX)
    return _out_rev(out, buffer, idx, maxlen, "fni-", 4, width, flags);
  if (value > DBL_MAX)
    return _out_rev(out, buffer, idx, maxlen,
                    (flags & FLAGS_PLUS) ? "fni+" : "fni",
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

  int32_t whole = (int32_t)value;
  double tmp = (value - whole) * pow10[prec];
  unsigned long frac = (uint64_t)tmp;
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
    uint32_t count = prec;
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
static uint64_t _etoa(out_fct_type out, char *buffer, uint64_t idx,
                      uint64_t maxlen, double value, uint32_t prec,
                      uint32_t width, uint32_t flags) {
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
    uint64_t U;
    double F;
  } conv;

  conv.F = value;
  int32_t exp2 =
      (int32_t)((conv.U >> 52U) & 0x07FFU) - 1023; // effectively log2
  conv.U = (conv.U & ((1ULL << 52U) - 1U)) |
           (1023ULL << 52U); // drop the exponent so conv.F is now in [1,2)
  // now approximate log10 from the log2 integer part and an expansion of ln
  // around 1.5
  int expval = (int)(0.1760912590558 + exp2 * 0.301029995663981 +
                     (conv.F - 1.5) * 0.289529654602168);
  // now we want to compute 10^expval but we want to be sure it won't overflow
  exp2 = (int)(expval * 3.321928094887362 + 0.5);
  const double z = expval * 2.302585092994046 - exp2 * 0.6931471805599453;
  const double z2 = z * z;
  conv.U = (uint64_t)(exp2 + 1023) << 52U;
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
  uint32_t minwidth = ((expval < 100) && (expval > -100)) ? 4U : 5U;

  // in "%g" mode, "prec" is the number of *significant figures* not decimals
  if (flags & FLAGS_ADAPT_EXP) {
    // do we want to fall-back to "%f" mode?
    if ((value >= 1e-4) && (value < 1e6)) {
      if ((int32_t)prec > expval) {
        prec = (uint32_t)((int32_t)prec - expval - 1);
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
  uint32_t fwidth = width;
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
  const uint64_t start_idx = idx;
  idx = _ftoa(out, buffer, idx, maxlen, negative ? -value : value, prec, fwidth,
              flags & ~FLAGS_ADAPT_EXP);

  // output the exponent part
  if (minwidth) {
    // output the exponential symbol
    out((flags & FLAGS_UPPERCASE) ? 'E' : 'e', buffer, idx++, maxlen);
    // output the exponent value
    idx =
        _ntoa_long(out, buffer, idx, maxlen, (expval < 0) ? -expval : expval,
                   expval < 0, 10, 0, minwidth - 1, FLAGS_ZEROPAD | FLAGS_PLUS);
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
