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
  s32 count;
  u16 core_id;

  // Get amount queued using `write_to - read_from`
  s64 read_from; // always-increasing task index to read from (need to take mod before indexing)
  s64 write_to;  // always-increasing task index to write to (need to take mod before indexing)
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

  task_main();
}

_Noreturn void task_main(void) {
  log_fmt("Kernel main end");
  exit(0);

  while (true) {
    // TODO dequeue a task and run it
  }
}

static WorkerState *get_state() {
  const u16 id = core_id();
  FOR_PTR(TaskGlobals.workers, TaskGlobals.count) {
    if (it->core_id == id) return it;
  }

  // Unreachable
  return NULL;
}

// TODO add_task(code, data) add task to the workers
