#pragma once
#include <types.h>

typedef enum { Done, Blocked } TaskProgress;
typedef void (*TaskCode)(void *data);

#define schedule_task(fn, data)                                                                    \
  (_Static_assert(CMP_TYPE(typeof(fn(&data))), void),                                              \
   schedule_task_inner(fn, &(data), sizeof(data)))
void schedule_task_inner(void *fn, void *data, s64 size);

_Noreturn void task_begin(void);
_Noreturn void task_main(void);
