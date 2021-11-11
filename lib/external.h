#ifndef __LIB_EXTERNAL__
#define __LIB_EXTERNAL__
#include <types.h>

_Noreturn void ext__shutdown(void);
void ext__log(sloc loc, int32_t count, const any *args);
void ext__log_fmt(sloc loc, const char *fmt, s32 count, const any *args);
#endif
