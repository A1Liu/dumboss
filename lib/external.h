#ifndef __LIB_EXTERNAL__
#define __LIB_EXTERNAL__
#include <types.h>

// TODO: use `ext__shutdown(s32 value);` instead.
//                      - Albert Liu, Nov 12, 2021 Fri 20:35 EST
_Noreturn void ext__shutdown(void);

// Shuts down the program. Akin to exit.
//                  - Albert Liu, Nov 11, 2021 Thu 17:06 EST
_Noreturn static inline void exit(s32 code) {
  ext__shutdown();
}

void ext__log(sloc loc, int32_t count, const any *args);
void ext__log_fmt(sloc loc, const char *fmt, s32 count, const any *args);

#endif
