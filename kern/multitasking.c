#include "multitasking.h"
#include "asm_kern.h"
#include "bootboot.h"
#include "init.h"
#include "memory.h"
#include <basics.h>
#include <macros.h>
#include <sync.h>

#define Task__Empty   U8(0)
#define Task__Writing U8(1)
#define Task__Queued  U8(2)
#define Task__Reading U8(3)

typedef struct {
  _Atomic u8 sync_info;
  s64 id;

  TaskData data;
} Task;

// Get amount queued using `write_to - read_from`
typedef struct {
  Task *tasks;
  s32 task_count;
  u16 core_id;

  // void* stack_pointer;

  // always-increasing task index to read from (need to take mod before indexing)
  _Atomic s64 read_from;
  // always-increasing task index to write to (need to take mod before indexing)
  _Atomic s64 write_to;
} WorkerState;

static struct {
  WorkerState *workers;
  u16 worker_count;
  _Atomic u16 init_count;

  // This could probably be s32 or something, idk
  _Atomic s64 id;

  // TODO: this can be garbage collected, as long as we make tasks movable.
  Bump task_data_alloc;
} TaskGlobals;

void tasks__init(void) {
  TaskGlobals.workers = Bump__array(&InitAlloc, WorkerState, bb.numcores);
  TaskGlobals.worker_count = bb.numcores;
  TaskGlobals.init_count = 0;
  TaskGlobals.task_data_alloc = Bump__new(4);

  FOR_PTR(TaskGlobals.workers, TaskGlobals.worker_count) {
    it->tasks = zeroed_pages(2);
    it->task_count = 2 * _4KB / sizeof(Task);

    // u8* stack_begin = zeroed_pages(2);
    // it->stack_pointer = stack_begin + _4KB * 2;
  }
}

static WorkerState *get_state(void);
static Task dequeue_task(WorkerState *worker);
static bool enqueue_task(WorkerState *worker, TaskData data);

_Noreturn void task_begin(void) {
  u16 index = a_add(&TaskGlobals.init_count, 1);
  WorkerState *state = &TaskGlobals.workers[index];
  state->core_id = core_id();

  // TODO switch kernel stacks?

  task_main();
}

bool add_task(TaskData data) {
  FOR_PTR(TaskGlobals.workers, TaskGlobals.worker_count) {
    if (enqueue_task(it, data)) return true;
  }

  return false;
}

_Noreturn void task_main(void) {

  WorkerState *self = get_state();
  s64 self_index = self - TaskGlobals.workers;

  while (true) {
    Task task;
    RANGE(0, TaskGlobals.worker_count) {
      s64 worker_index = (it + self_index) % TaskGlobals.worker_count;
      task = dequeue_task(&TaskGlobals.workers[worker_index]);
      if (task.sync_info != Task__Empty) break(it);
    }

    if (task.sync_info == Task__Empty) log_fmt("Found no tasks");
    else
      log_fmt("Found a task");

    log_fmt("Kernel main end");
    exit(0);
  }
}

static WorkerState *get_state() {
  const u16 id = core_id();
  FOR_PTR(TaskGlobals.workers, TaskGlobals.worker_count) {
    if (it->core_id == id) return it;
  }

  // Unreachable
  return NULL;
}

static Task dequeue_task(WorkerState *worker) {
  const s64 write_to = a_load(&worker->write_to);
  s64 read_from = a_load(&worker->read_from);
  Task *const tasks = worker->tasks;
  const s32 task_count = worker->task_count;

  Task task;
  LOOP(find_task) {
    u8 current = Task__Queued;
    Task *target = &tasks[read_from % task_count];
    if (a_cxstrong(&target->sync_info, &current, Task__Reading)) {
      task = *target;

      // we have mutual exclusion here, so we just write directly
      a_store(&target->sync_info, Task__Empty);

      break(find_task);
    }

    while (read_from < write_to) {
      if (a_cxweak(&worker->read_from, &read_from, read_from + 1)) continue(find_task);
    }

    return (Task){.sync_info = Task__Empty};
  }

  return task;
}

static bool enqueue_task(WorkerState *worker, TaskData data) {
  Task *const tasks = worker->tasks;
  const s32 task_count = worker->task_count;
  const s64 read_from = a_load(&worker->read_from), upper_bound = read_from + task_count;
  s64 write_to = a_load(&worker->write_to);

  LOOP(find_slot) {
    u8 current = Task__Empty;
    Task *target = &tasks[write_to % task_count];
    if (a_cxstrong(&target->sync_info, &current, Task__Writing)) {
      target->id = a_add(&TaskGlobals.id, 1);
      target->data = data;

      // we have mutual exclusion here, so we just write directly
      a_store(&target->sync_info, Task__Queued);

      break(find_slot);
    }

    while (write_to < upper_bound) {
      if (a_cxweak(&worker->write_to, &write_to, write_to + 1)) continue(find_slot);
    }

    return false;
  }

  return true;
}
