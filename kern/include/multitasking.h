#pragma once
#include <types.h>

typedef enum { Done, Blocked } TaskProgress;
typedef void (*TaskCode)(void *data, s64 size);
typedef struct {
  TaskCode code;
  void *data;
  s64 data_size;
} TaskData;

#define add_task(...)  PASTE(_add_task, NARG(__VA_ARGS__))(__VA_ARGS__)
#define _add_task1(fn) (_Static_assert(CMP_TYPE(typeof(fn())), void), _add_task(fn, NULL, 0))
#define _add_task2(fn, data)                                                                       \
  (_Static_assert(CMP_TYPE(typeof(fn(data))), void),                                               \
   (sizeof() > 8) ? _add_task(fn, &(data), sizeof(data)) : _add_task(fn, data, 0))
#define _add_task3(fn, data_ptr, size)                                                             \
  (_Static_assert(CMP_TYPE(typeof(fn(data, size))), void), _add_task(fn, data_ptr, size))

#define _add_task(fn, data, size)                                                                  \
  add_task_inner((TaskData){.code = (TaskCode)(fn), .data = (void *)(data), .data_size = size})

bool add_task_inner(TaskData data);

_Noreturn void task_begin(void);
_Noreturn void task_main(void);
