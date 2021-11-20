#include "multitasking.h"
#include "asm_kern.h"
#include "bootboot.h"
#include "init.h"
#include "memory.h"
#include <basics.h>
#include <macros.h>
#include <sync.h>

typedef enum __attribute__((packed)) { Empty = 0, Queing, Queued, Taken } TaskSync;

typedef struct {
  TaskSync sync_info;
  s32 id;

  TaskCode code;
  void *data;
} Task;

typedef struct {
  Task *tasks;
  s64 count; // capacity of buffer in term of tasks (should always be a power of 2)

  // x modulo y = (x & (y − 1))
  // Get amount queued using `write_to - read_from`
  s64 read_from; // always-increasing task index to read from (need to take mod before indexing)
  s64 write_to;  // always-increasing task index to write to (need to take mod before indexing)

  u16 core_id;
} WorkerState;

static struct {
  WorkerState *workers;
  u16 count;
  _Atomic u16 init_count;

  // TODO: this can be garbage collected, as long as we make tasks movable.
  Bump task_data_alloc;
} TaskGlobals;

void tasks__init(void) {
  TaskGlobals.workers = Bump__array(&InitAlloc, WorkerState, bb.numcores);
  TaskGlobals.count = bb.numcores;
  TaskGlobals.init_count = 0;
  TaskGlobals.task_data_alloc = Bump__new(4);

  FOR_PTR(TaskGlobals.workers, TaskGlobals.count) {
    it->tasks = zeroed_pages(2);
    it->count = 2 * _4KB / sizeof(Task);
  }
}

static WorkerState *get_state(void);

_Noreturn void task_begin(void) {
  u16 index = a_add(&TaskGlobals.init_count, 1);
  WorkerState *state = &TaskGlobals.workers[index];
  state->core_id = core_id();

  // TODO switch kernel stacks?

  // TODO run tasks?

  log_fmt("Kernel main end");
  exit(0);
}

_Noreturn void task_main(void) {
  log_fmt("Kernel main end");
  exit(0);
}

// TODO schedule(task) add task to the workers

// TODO task_switch()
