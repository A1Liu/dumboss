#ifndef __LIB_ASM__
#define __LIB_ASM__
#include <macro_utils.h>
#include <types.h>

#define read_register(...)       PASTE(_read_register, NARG(__VA_ARGS__))(__VA_ARGS__)
#define _read_register1(reg)     _read_register3(reg, u64, "")
#define _read_register2(reg, ty) _read_register3(reg, ty, "")
#define _read_register3(reg, ty, suffix)                                                           \
  ({                                                                                               \
    _Static_assert(sizeof(ty) <= 8, "read_register only takes a type with size <= 8");             \
    ty value;                                                                                      \
    asm("mov" suffix " %%" #reg ", %0" : "=r"(value));                                             \
    value;                                                                                         \
  })

#define write_register(...)          PASTE(_write_register, NARG(__VA_ARGS__))(__VA_ARGS__)
#define _write_register2(reg, value) _write_register3(reg, value, "")
#define _write_register3(reg, value, suffix)                                                       \
  ({                                                                                               \
    _Static_assert(sizeof(value) <= 8, "read_register only takes a type with size <= 8");          \
    typeof(value) v = value;                                                                       \
    asm("mov" suffix " %0, %%" #reg : : "r"(v) : #reg);                                            \
    v;                                                                                             \
  })

#define out8(port, val)                                                                            \
  ({                                                                                               \
    u8 v = val;                                                                                    \
    asm volatile("outb %0, %1" : : "a"(v), "Nd"(port));                                            \
    v;                                                                                             \
  })

#define in8(port)                                                                                  \
  ({                                                                                               \
    u8 ret;                                                                                        \
    u16 p = port;                                                                                  \
    asm volatile("inb %1, %0" : "=a"(ret) : "Nd"(p));                                              \
    ret;                                                                                           \
  })

#endif
