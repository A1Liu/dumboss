#include "multitasking.h"
#include "asm.h"
#include "bootboot.h"
#include "init.h"
#include "interrupts.h"
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
// `write_to` and `read_from` need to take mod before indexing
typedef struct {
  Task *tasks;
  s32 task_count;
  u16 core_id;

  // void* stack_pointer;
  Task running_task;

  _Atomic s64 read_from; // always-increasing index to read from
  _Atomic s64 write_to;  // always-increasing index to write to
} WorkerState;

static struct {
  WorkerState *workers;
  u16 worker_count;
  _Atomic u16 init_count;
  _Atomic u16 init_finish_count;

  // This could probably be s32 or something, idk
  _Atomic s64 next_id;

  // TODO: this can be garbage collected, as long as we make tasks movable.
  Bump task_data_alloc;
} TaskGlobals;

void tasks__init(void) {
  TaskGlobals.workers = Bump__array(&InitAlloc, WorkerState, bb.numcores);
  TaskGlobals.worker_count = bb.numcores;
  TaskGlobals.task_data_alloc = Bump__new(4);

  FOR_PTR(TaskGlobals.workers, TaskGlobals.worker_count) {
    it->tasks = zeroed_pages(2);
    it->task_count = 2 * _4KB / sizeof(Task);

    // u8* stack_begin = zeroed_pages(2);
    // it->stack_pointer = stack_begin + _4KB * 2;
  }

  log_fmt("tasks INIT_COMPLETE");
}

static s64 get_worker_index(void);
static Task dequeue_task(WorkerState *worker);
static bool enqueue_task(WorkerState *worker, TaskData data);
static _Noreturn void task_main_inner(WorkerState *self, s64 self_index);

bool add_task_inner(TaskData data) {
  FOR_PTR(TaskGlobals.workers, TaskGlobals.worker_count) {
    if (enqueue_task(it, data)) return true;
  }

  return false;
}
// TODO Use https://wiki.osdev.org/APIC (and maybe Phil Opperman's Blog?) to set
// up the APIC and handle logging through serial interrupts
_Noreturn void task_begin(void) {
  u16 index = a_add(&TaskGlobals.init_count, 1);
  WorkerState *state = &TaskGlobals.workers[index];
  state->core_id = core_id();

  s64 self_index = get_worker_index();
  WorkerState *self = &TaskGlobals.workers[self_index];

  // TODO switch kernel stacks?

  load_idt();

  u16 tss = tss_segment(self_index);
  asm volatile("ltr %0" : : "r"(tss));

  a_add(&TaskGlobals.init_finish_count, 1);

  // divide_by_zero();

  // TODO Need to enable multicore stuff first.
  // Wait for all cores to be initialized.
  // while (a_load(&TaskGlobals.init_finish_count) != TaskGlobals.worker_count)
  //   pause();

  task_main();
}

_Noreturn void task_main(void) {
  s64 self_index = get_worker_index();
  WorkerState *self = &TaskGlobals.workers[self_index];
  Task *task = &self->running_task;

  while (true) {
    RANGE(0, TaskGlobals.worker_count) {
      s64 worker_index = (it + self_index) % TaskGlobals.worker_count;
      *task = dequeue_task(&TaskGlobals.workers[worker_index]);
      if (task->sync_info != Task__Empty) break(it);
    }

    if (task->sync_info == Task__Empty) log_fmt("Found no tasks");
    else
      log_fmt("Found a task");

    // TODO run task

    log_fmt("Kernel main end");
    exit(0);

    pause();
  }
}

static s64 get_worker_index(void) {
  const u16 id = core_id();
  FOR_PTR(TaskGlobals.workers, TaskGlobals.worker_count) {
    if (it->core_id == id) return index;
  }

  log_fmt("Unreachable?");
  exit(1);
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
      target->id = a_add(&TaskGlobals.next_id, 1);
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
