#include "sync.h"
#include "basics.h"
#include "memory.h"
#include <stdatomic.h>

// TODO: Add more macros for clang builtin as needed. Builtins list is here:
// https://releases.llvm.org/3.7.0/tools/clang/docs/LanguageExtensions.html
//                                      - Albert Liu, Nov 03, 2021 Wed 00:50 EDT
#define a_init(ptr_val, initial) __c11_atomic_init(ptr_val, initial)
#define a_load(obj)              __c11_atomic_load(obj, __ATOMIC_SEQ_CST)
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
  _Atomic(s64) begin;     // First index that's safe to read from
  _Atomic(s64) end_read;  // last index of data that's safe to read
  _Atomic(s64) end_write; // first index of data that's safe to write
  const s64 count;        // size in bytes
  const s32 elem_size;    // Having a larger elem_size doesn't make sense.
  s32 _unused0;
  s64 _unused1;
  s64 _unused2;
  s64 _unused3;

  // This is the beginning of the second cache line
  u8 data[];
};

_Static_assert(sizeof(_Atomic(s64)) == 8, "atomics are zero-cost right?? :)");
_Static_assert(sizeof(Queue) == 64, "Queue should have 64 byte control structure");

Queue *Queue__create(const Buffer buffer, const s32 elem_size) {
  const u64 address = (u64)buffer.data;
  if (address != align_down(address, 64)) return NULL;

  const s64 count = buffer.count - (s64)sizeof(Queue);
  if (count <= 0 || elem_size == 0) return NULL;

  Queue *queue = (Queue *)buffer.data;
  queue->begin = 0;
  a_init(&queue->end_read, 0);
  a_init(&queue->end_write, 0);
  *((s64 *)&queue->count) = count;
  *((s32 *)&queue->elem_size) = elem_size;
  queue->_unused0 = 0x1eadbeef;
  queue->_unused1 = 0x1eadbeef1eadbeef;
  queue->_unused2 = 0x1eadbeef1eadbeef;
  queue->_unused3 = 0x1eadbeef1eadbeef;

  return queue;
}

s64 Queue__enqueue(Queue *queue, const void *buffer, s64 count) {
  s32 elem_size = queue->elem_size;
  s64 write_head = a_load(&queue->end_write);
  s64 new_head = write_head + count * elem_size;

  while (!a_cxweak(&queue->end_write, &write_head, new_head)) {
    new_head = write_head + count * elem_size;
  }
  (void)queue;
  (void)buffer;
  (void)count;
  return 0;
}

s64 Queue__dequeue(Queue *queue, void *buffer, s64 count) {
  (void)queue;
  (void)buffer;
  (void)count;
  return 0;
}

s64 Queue__read(Queue *queue, void *buffer, s64 count) {
  (void)queue;
  (void)buffer;
  (void)count;
  return 0;
}

s64 Queue__len(const Queue *queue) {
  return queue->end_read - queue->begin;
}

s64 Queue__capacity(const Queue *queue) {
  return queue->count;
}
