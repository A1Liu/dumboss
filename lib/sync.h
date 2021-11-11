#pragma once
#include "basics.h"
#include "logging.h"
#include "memory.h"
#include <stdatomic.h>
#include <stddef.h>

typedef struct _QueueBlock _QueueBlock;
typedef struct {
  struct _QueueBlock *const ptr;
  const s64 count;     // size in elements
  const s32 elem_size; // having a larger elem_size doesn't make sense.
} Queue;

#define NULL_QUEUE ((Queue){.ptr = NULL, .elem_size = 0, .count = 0})

// Creates a queue using the provided buffer.
Queue Queue__create(Buffer buffer, s32 elem_size);

// Attempts to enqueue `count` elements starting at `buffer`. Returns the number
// of elements actually enqueued.
s64 Queue__enqueue(Queue queue, const void *buffer, s64 count, s32 elem_size);

// Attempts to dequeue `count` elements, writing to `buffer`. Returns the number
// of elements actually dequeued.
s64 Queue__dequeue(Queue queue, void *buffer, s64 count, s32 elem_size);

// Returns the length of the queue in elements.
s64 Queue__len(const Queue queue, s32 elem_size);

// Returns the capacity of the queue in elements.
s64 Queue__capacity(const Queue queue, s32 elem_size);

#ifdef __DUMBOSS_IMPL__

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

#define READ_MUTEX  U32(1 << 0)
#define WRITE_MUTEX U32(1 << 1)

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
  _Atomic(u32) flags;      // Flags
  s32 _unused0;
  s64 _unused1;
  s64 _unused2;
  s64 _unused3;
  s64 _unused4;
  s64 _unused5;

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
  a_init(&ptr->flags, 0);
  ptr->_unused0 = 0x1eadbeef;
  ptr->_unused1 = 0x1eadbeef1eadbeef;
  ptr->_unused2 = 0x1eadbeef1eadbeef;
  ptr->_unused3 = 0x1eadbeef1eadbeef;
  ptr->_unused3 = 0x1eadbeef1eadbeef;
  ptr->_unused3 = 0x1eadbeef1eadbeef;

  Queue queue = (Queue){.ptr = ptr, .elem_size = elem_size, .count = count};

  return queue;
}

s64 Queue__enqueue(Queue queue, const void *buffer, s64 count, s32 elem_size) {
  assert(elem_size == queue.elem_size);
  if (count == 0) return 0;

  NAMED_BREAK(set_mutex) {
    u32 flags = a_load(&queue.ptr->flags) & ~WRITE_MUTEX;
    REPEAT(5) {
      if (a_cxweak(&queue.ptr->flags, &flags, flags | WRITE_MUTEX)) break(set_mutex);
      flags &= ~WRITE_MUTEX;
    }

    return -1;
  }

  const s64 queue_size = queue.count;
  s64 read_head = a_load(&queue.ptr->read_head), write_head = a_load(&queue.ptr->write_head);
  s64 new_head = min(write_head + count, read_head + queue_size);
  s64 write_count = new_head - write_head, modular_index = write_head % queue_size;

  memcpy(&queue.ptr->data[modular_index * elem_size], buffer, write_count * elem_size);
  a_store(&queue.ptr->write_head, new_head);

  u32 flags = a_load(&queue.ptr->flags);
  while (a_cxweak(&queue.ptr->flags, &flags, flags & ~WRITE_MUTEX))
    ;

  return write_count;
}

s64 Queue__dequeue(Queue queue, void *buffer, s64 count, s32 elem_size) {
  assert(elem_size == queue.elem_size);
  if (count == 0) return 0;

  NAMED_BREAK(set_mutex) {
    u32 flags = a_load(&queue.ptr->flags) & ~READ_MUTEX;
    REPEAT(5) {
      if (a_cxweak(&queue.ptr->flags, &flags, flags | READ_MUTEX)) break(set_mutex);
      flags &= ~READ_MUTEX;
    }

    return -1;
  }

  const s64 queue_size = queue.count;
  s64 read_head = a_load(&queue.ptr->read_head), write_head = a_load(&queue.ptr->write_head);
  s64 new_head = min(read_head + count, write_head);
  s64 read_count = new_head - read_head, modular_index = read_head % queue_size;

  memcpy(buffer, &queue.ptr->data[modular_index * elem_size], read_count * elem_size);
  a_store(&queue.ptr->read_head, new_head);

  u32 flags = a_load(&queue.ptr->flags);
  while (a_cxweak(&queue.ptr->flags, &flags, flags & ~READ_MUTEX))
    ;

  return read_count;
}

s64 Queue__len(const Queue queue, s32 elem_size) {
  assert(elem_size == queue.elem_size);

  s64 read_head = queue.ptr->read_head;
  s64 write_head = queue.ptr->write_head;

  return (write_head - read_head) / elem_size;
}

s64 Queue__capacity(const Queue queue, s32 elem_size) {
  assert(elem_size == queue.elem_size);

  return queue.count / elem_size;
}

#endif
