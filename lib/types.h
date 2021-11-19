#ifndef __LIB_TYPES__
#define __LIB_TYPES__
#include <stdbool.h>
#include <stdint.h>

#define _KB (1024)
#define _MB (1024 * 1024)
#define _GB (1024 * 1024 * 1024)

#define _4KB (4096)
#define _2MB (2 * 1024 * 1024)
#define _1GB (1024 * 1024 * 1024)

typedef uint64_t u64;
typedef int64_t s64;
typedef uint32_t u32;
typedef int32_t s32;
typedef uint16_t u16;
typedef int16_t s16;
typedef uint8_t u8;
typedef int8_t s8;

typedef struct {
  u8 *data;
  s64 count;
} Buffer;

typedef struct {
  char *data;
  s64 count;
} String;

typedef struct {
  const char *file;
  u32 line;
} sloc;

typedef enum __attribute__((packed)) {
  type_id_bool,
  type_id_u8,
  type_id_i8,
  type_id_u16,
  type_id_i16,
  type_id_u32,
  type_id_i32,
  type_id_u64,
  type_id_i64,
  type_id_char,
  type_id_char_ptr,
} type_id;

typedef struct {
  union {
    u64 u64_value;
    s64 i64_value;
    char char_value;
    bool bool_value;
    void *ptr;
  };
  type_id type;
} any;

#define U64(i) ((u64)(i))
#define U32(i) ((u32)(i))

#define S64(i) ((s64)(i))

#define NULL ((void *)0)

#define __LOC__                                                                                    \
  (sloc) {                                                                                         \
    .file = __FILE__, .line = __LINE__                                                             \
  }

#endif
