#include "sync.h"
#include "basics.h"
#include "logging.h"
#include "memory.h"
#include <stdatomic.h>
#include <stddef.h>

// TODO: Add more macros for clang builtin as needed. Builtins list is here:
// https://releases.llvm.org/10.0.0/tools/clang/docs/LanguageExtensions.html
//                                      - Albert Liu, Nov 03, 2021 Wed 00:50 EDT
#define a_init(ptr_val, initial) __c11_atomic_init(ptr_val, initial)
#define a_load(obj)              __c11_atomic_load(obj, __ATOMIC_SEQ_CST)
#define a_store(obj, value)      __c11_atomic_store(obj, value, __ATOMIC_SEQ_CST)
#define a_cxweak(obj, expected, desired)                                                           \
  __c11_atomic_compare_exchange_weak(obj, expected, desired, __ATOMIC_SEQ_CST, __ATOMIC_SEQ_CST)

// TODO: overflow might happen in certain places idk
//                                      - Albert Liu, Nov 03, 2021 Wed 00:51 EDT

// should we be using u64 or s64 here? u64 has the semantic
// meaning that all values will be positive, which we want, but makes it harder
// to subtract numbers correctly, and has required wrapping behavior, which we
// might not want.
struct Queue {
  // eventually we should add a `next` buffer that points to the queue to be
  // used for writing. Also, probably should add a 32-bit `ref_count` field for
  // when multiple people have a reference to the same queue.
  _Atomic(s64) read_tail;  // lowest element index that's still being read from
  _Atomic(s64) read_head;  // lowest element index that's safe to read from
  _Atomic(s64) write_tail; // highest element index of data that's safe to read
  _Atomic(s64) write_head; // highest element index of data that's safe to write
  const s64 count;         // size in elements
  _Atomic(s16) flags;      // Flags
  const s16 elem_size;     // having a larger elem_size doesn't make sense.
  s32 _unused0;
  s64 _unused1;
  s64 _unused2;

  // This is the beginning of the second cache line
  u8 data[];
};

_Static_assert(sizeof(_Atomic(s64)) == 8, "atomics are zero-cost right?? :)");
_Static_assert(sizeof(Queue) == 64, "Queue should have 64 byte control structure");

Queue *Queue__create(const Buffer buffer, const s32 elem_size) {
  const u64 address = (u64)buffer.data;
  if (address != align_down(address, 64)) return NULL; // align to cache line

  s64 count = buffer.count - (s64)sizeof(Queue);
  count -= count % elem_size;
  if (count <= 0 || elem_size == 0) return NULL;

  Queue *queue = (Queue *)buffer.data;
  a_init(&queue->read_head, 0);
  a_init(&queue->read_tail, 0);
  a_init(&queue->write_head, 0);
  a_init(&queue->write_tail, 0);
  *((s64 *)&queue->count) = count;
  *((s32 *)&queue->elem_size) = elem_size;
  queue->_unused0 = 0x1eadbeef;
  queue->_unused1 = 0x1eadbeef1eadbeef;
  queue->_unused2 = 0x1eadbeef1eadbeef;

  return queue;
}

s64 Queue__enqueue(Queue *queue, const void *buffer, s64 count, s32 elem_size) {
  assert(elem_size == queue->elem_size);
  const s64 queue_size = queue->count;
  s64 read_tail = a_load(&queue->read_tail), write_head = a_load(&queue->write_head);
  s64 new_head = min(write_head + count, read_tail + queue_size);

  while (!a_cxweak(&queue->write_head, &write_head, new_head)) {
    new_head = min(write_head + count, read_tail + queue_size);
  }

  s64 write_count = new_head - write_head;
  memcpy(&queue->data[write_head * elem_size], buffer, write_count * elem_size);
  a_store(&queue->write_tail, new_head);

  return write_count;
}

s64 Queue__dequeue(Queue *queue, void *buffer, s64 count, s32 elem_size) {
  assert(elem_size == queue->elem_size);
  const s64 queue_size = queue->count;
  s64 read_head = a_load(&queue->read_head), write_tail = a_load(&queue->write_tail);
  s64 new_head = min(read_head + count, write_tail);

  while (!a_cxweak(&queue->read_head, &read_head, new_head)) {
    new_head = min(read_head + count, read_head + queue_size);
  }

  s64 read_count = new_head - read_head;
  memcpy(buffer, &queue->data[read_head * elem_size], read_count * elem_size);
  a_store(&queue->read_tail, new_head);

  return read_count;
}

s64 Queue__len(const Queue *queue, s32 elem_size) {
  assert(elem_size == queue->elem_size);

  s64 read_tail = queue->read_tail;
  s64 write_head = queue->write_head;

  return (write_head - read_tail) / elem_size;
}

s64 Queue__capacity(const Queue *queue, s32 elem_size) {
  assert(elem_size == queue->elem_size);

  return queue->count / elem_size;
}
