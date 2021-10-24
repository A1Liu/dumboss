#include "sync.h"
#include "memory.h"

// TODO These are the clang builtins we need to use to do atomics, because we
// don't link against the C standard library.
//
// __c11_atomic_init
// __c11_atomic_thread_fence
// __c11_atomic_signal_fence
// __c11_atomic_is_lock_free
// __c11_atomic_store
// __c11_atomic_load
// __c11_atomic_exchange
// __c11_atomic_compare_exchange_strong
// __c11_atomic_compare_exchange_weak
// __c11_atomic_fetch_add
// __c11_atomic_fetch_sub
// __c11_atomic_fetch_and
// __c11_atomic_fetch_or
// __c11_atomic_fetch_xor
// __c11_atomic_fetch_max
// __c11_atomic_fetch_min

// should we be using uint64_t or int64_t here? uint64_t has the semantic
// meaning that all values will be positive, which we want, but makes it harder
// to subtract numbers correctly, and has required wrapping behavior, which we
// might not want.
struct Queue {
  // eventually we should add a `next` buffer that points to the queue to be
  // used for writing. Also, probably should add a 32-bit `ref_count` field for
  // when multiple people have a reference to the same queue.
  _Atomic(int64_t) begin;
  _Atomic(int64_t) end_read;
  _Atomic(int64_t) end_write;
  int64_t elem_count;
  int32_t elem_size; // Having a larger elem_size doesn't make sense.
  int32_t _unused0;
  int64_t _unused1;
  int64_t _unused2;
  int64_t _unused3;
  uint8_t buffer[];
};

_Static_assert(sizeof(Queue) == 64, "Queue should have 64 byte control structure");

Queue *Queue__create(const Buffer buffer, const int32_t elem_size) {
  const uint64_t address = (uint64_t)buffer.data;
  if (address == align_down(address, 8)) return NULL;

  const int64_t buffer_bytes = buffer.count - (int64_t)sizeof(Queue);
  if (buffer_bytes <= 0 || elem_size == 0) return NULL;

  const int64_t elem_count = buffer_bytes / elem_size;
  if (elem_count <= 0) return NULL;

  Queue *queue = (Queue *)buffer.data;
  queue->begin = 0;
  queue->end_read = 0;
  queue->end_write = 0;
  queue->elem_count = elem_count;
  queue->elem_size = elem_size;
  queue->_unused0 = 0x1eadbeef;
  queue->_unused1 = 0x1eadbeef1eadbeef;
  queue->_unused2 = 0x1eadbeef1eadbeef;
  queue->_unused3 = 0x1eadbeef1eadbeef;

  return queue;
}

int64_t Queue__enqueue(Queue *queue, const void *buffer, int64_t count) {
  (void)queue;
  (void)buffer;
  (void)count;
  return 0;
}

int64_t Queue__dequeue(Queue *queue, void *buffer, int64_t count) {
  (void)queue;
  (void)buffer;
  (void)count;
  return 0;
}

int64_t Queue__read(Queue *queue, void *buffer, int64_t count) {
  (void)queue;
  (void)buffer;
  (void)count;
  return 0;
}

int64_t Queue__len(const Queue *queue) {
  return queue->end_read - queue->begin;
}

int64_t Queue__capacity(const Queue *queue) {
  return queue->elem_count;
}
