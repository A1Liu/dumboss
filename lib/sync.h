#ifndef __LIB_SYNC__
#define __LIB_SYNC__
#include <stdatomic.h>
#include <stddef.h>
#include <types.h>

// NOTE: This ALSO breaks compatibility with GCC.
//                                    - Albert Liu, Nov 15, 2021 Mon 19:03 EST
#define spin_or(expr, spin)                                                                        \
  for (s64 M_reps = 0; ({                                                                          \
         if (expr) break;                                                                          \
         if (M_reps++ < spin) continue;                                                            \
         true;                                                                                     \
       });                                                                                         \
       ({ break; }))

bool Mutex__try_lock(_Atomic(u8) *mtx);
void Mutex__unlock(_Atomic(u8) *mtx);

// NOTE: This breaks compatibility with GCC. I guess that's fine, but like, seems
// weird and a bit uncomfy.
//                                    - Albert Liu, Nov 15, 2021 Mon 18:36 EST
#define CRITICAL(mtx)                                                                              \
  for (u8 *M_mtx = (mtx); Mutex__try_lock(M_mtx); ({                                               \
         Mutex__unlock(M_mtx);                                                                     \
         break;                                                                                    \
       }))

typedef struct _QueueBlock _QueueBlock;

typedef struct {
  struct _QueueBlock *const ptr;
  const s64 count;     // size in elements
  const s32 elem_size; // having a larger elem_size doesn't make sense.
} Queue;

typedef enum __attribute__((packed)) {
  Queue__Success = 0,
  Queue__Blocked,
  Queue__TypeError,
} Queue__ResultKind;

typedef struct {
  Queue__ResultKind kind;
  s64 count;
} Queue__Result;

#define NULL_QUEUE ((Queue){.ptr = NULL, .elem_size = 0, .count = 0})

// Creates a queue using the provided buffer.
Queue Queue__create(Buffer buffer, s32 elem_size);

// Attempts to enqueue `count` elements starting at `buffer`. Returns the number
// of elements actually enqueued.
Queue__Result Queue__enqueue_internal(Queue queue, const void *buffer, s64 count);

// Attempts to dequeue `count` elements, writing to `buffer`. Returns the number
// of elements actually dequeued.
Queue__Result Queue__dequeue_internal(Queue queue, void *buffer, s64 count);

#define Queue__enqueue(q, buffer)                                                                  \
  ({                                                                                               \
    const typeof(buffer) M_buf = (buffer);                                                         \
    assert(q.elem_size == sizeof(*M_buf.data));                                                    \
    Queue__enqueue_internal(q, M_buf.data, M_buf.count);                                           \
  })

#define Queue__dequeue(q, buffer)                                                                  \
  ({                                                                                               \
    const typeof(buffer) M_buf = (buffer);                                                         \
    assert(q.elem_size == sizeof(*M_buf.data));                                                    \
    Queue__dequeue_internal(q, M_buf.data, M_buf.count);                                           \
  })

// Returns the length of the queue in elements.
s64 Queue__len(Queue queue);

// Returns the capacity of the queue in elements.
s64 Queue__capacity(Queue queue);
#endif

#ifdef __DUMBOSS_IMPL__
#undef __DUMBOSS_IMPL__
#include <macros.h>
#define __DUMBOSS_IMPL__

#define Queue__BLOCKED                                                                             \
  (Queue__Result) {                                                                                \
    .kind = Queue__Blocked, .count = 0,                                                            \
  }

#define Queue__TYPE_ERROR                                                                          \
  (Queue__Result) {                                                                                \
    .kind = Queue__TypeError, .count = 0,                                                          \
  }

// TODO: Add more macros for clang builtin as needed. Builtins list is here:
// https://releases.llvm.org/10.0.0/tools/clang/docs/LanguageExtensions.html
//                                      - Albert Liu, Nov 03, 2021 Wed 00:50 EDT
#define a_init(ptr_val, initial) __c11_atomic_init(ptr_val, initial)
#define a_load(obj)              __c11_atomic_load(obj, __ATOMIC_SEQ_CST)
#define a_store(obj, value)      __c11_atomic_store(obj, value, __ATOMIC_SEQ_CST)
#define a_cxweak(obj, expected, desired)                                                           \
  __c11_atomic_compare_exchange_weak(obj, expected, desired, __ATOMIC_SEQ_CST, __ATOMIC_SEQ_CST)
#define a_cxstrong(obj, expected, desired)                                                         \
  __c11_atomic_compare_exchange_strong(obj, expected, desired, __ATOMIC_SEQ_CST, __ATOMIC_SEQ_CST)

bool Mutex__try_lock(_Atomic(u8) *mtx) {
  u8 current = 0;
  return a_cxweak(mtx, &current, 1);
}

void Mutex__unlock(_Atomic(u8) *mtx) {
  a_store(mtx, 0);
}

// TODO: overflow might happen in certain places idk
//                                      - Albert Liu, Nov 03, 2021 Wed 00:51 EDT

// should we be using u64 or s64 here? u64 has the semantic
// meaning that all values will be positive, which we want, but makes it harder
// to subtract numbers correctly, and has required wrapping behavior, which we
// might not want.
struct _QueueBlock {
  // eventually we should add a `next` buffer that points to the queue to be
  // used for writing. Also, probably should add a 32-bit `ref_count` field for
  // when multiple people have a reference to the same queue.
  _Atomic(s64) read_head;  // first element index that's safe to read from
  _Atomic(s64) write_head; // first element index that's safe to write to
  _Atomic(u8) read_mutex;  // Reading right now
  _Atomic(u8) write_mutex; // Writing right now
  s16 _unused0;
  s32 _unused1;
  s64 _unused2;
  s64 _unused3;
  s64 _unused4;
  s64 _unused5;
  s64 _unused6;

  // This is the beginning of the second cache line
  u8 data[];
};

_Static_assert(sizeof(_Atomic(s64)) == 8, "atomics are zero-cost right?? :)");
_Static_assert(sizeof(_QueueBlock) == 64, "Queue should have 64 byte control structure");

Queue Queue__create(const Buffer buffer, const s32 elem_size) {
  const u64 address = (u64)buffer.data;
  if (address != align_down(address, 64)) return NULL_QUEUE; // align to cache line

  s64 count = buffer.count - (s64)sizeof(Queue);
  count -= count % elem_size;
  if (count <= 0 || elem_size == 0) return NULL_QUEUE;

  _QueueBlock *ptr = (_QueueBlock *)buffer.data;
  a_init(&ptr->read_head, 0);
  a_init(&ptr->write_head, 0);
  a_init(&ptr->read_mutex, 0);
  a_init(&ptr->write_mutex, 0);
  ptr->_unused1 = 0x1ead;
  ptr->_unused1 = 0x1eadbeef;
  ptr->_unused2 = 0x1eadbeef1eadbeef;
  ptr->_unused3 = 0x1eadbeef1eadbeef;
  ptr->_unused4 = 0x1eadbeef1eadbeef;
  ptr->_unused5 = 0x1eadbeef1eadbeef;
  ptr->_unused6 = 0x1eadbeef1eadbeef;

  return (Queue){.ptr = ptr, .elem_size = elem_size, .count = count};
}

Queue__Result Queue__enqueue_internal(Queue queue, const void *buffer, s64 count) {
  if (count == 0) return (Queue__Result){.kind = Queue__Success, .count = 0};

  _QueueBlock *const ptr = queue.ptr;
  spin_or(Mutex__try_lock(&ptr->write_mutex), 5) return Queue__BLOCKED;

  s64 read_head = a_load(&ptr->read_head), write_head = a_load(&ptr->write_head);
  s64 new_head = min(write_head + count, read_head + queue.count);
  s64 write_count = new_head - write_head, modular_index = write_head % queue.count;

  memcpy(&ptr->data[modular_index * queue.elem_size], buffer, write_count * queue.elem_size);
  a_store(&ptr->write_head, new_head);

  Mutex__unlock(&ptr->write_mutex);

  return (Queue__Result){.kind = Queue__Success, .count = write_count};
}

Queue__Result Queue__dequeue_internal(Queue queue, void *buffer, s64 count) {
  if (count == 0) return (Queue__Result){.kind = Queue__Success, .count = 0};

  _QueueBlock *const ptr = queue.ptr;
  spin_or(Mutex__try_lock(&ptr->read_mutex), 5) return Queue__BLOCKED;

  s64 read_head = a_load(&ptr->read_head), write_head = a_load(&ptr->write_head);
  s64 new_head = min(read_head + count, write_head);
  s64 read_count = new_head - read_head, modular_index = read_head % queue.count;

  memcpy(buffer, &ptr->data[modular_index * queue.elem_size], read_count * queue.elem_size);
  a_store(&ptr->read_head, new_head);

  Mutex__unlock(&ptr->read_mutex);

  return (Queue__Result){.kind = Queue__Success, .count = read_count};
}

s64 Queue__len(Queue queue) {
  s64 read_head = a_load(&queue.ptr->read_head);
  s64 write_head = a_load(&queue.ptr->write_head);

  return write_head - read_head;
}

s64 Queue__capacity(Queue queue) {
  return queue.count;
}

#endif
