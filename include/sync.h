#pragma once

#include "basics.h"
#include <stdatomic.h>

// A single producer single consumer Concurrent Queue. In the future it'll
// pseudo-support multiple consumer by allowing reads that don't alter the begin
// index. The returned pointer will either be NULL (on failure) or `buffer.data`
// on success.
typedef struct Queue Queue;

// Creates a queue using the provided buffer.
Queue *Queue__create(Buffer buffer, int32_t elem_size);

// Attempts to enqueue `count` elements starting at `buffer`. Returns the number
// of elements actually enqueued.
int64_t Queue__enqueue(Queue *queue, const void *buffer, int64_t count);

// Attempts to dequeue `count` elements, writing to `buffer`. Returns the number
// of elements actually dequeued.
int64_t Queue__dequeue(Queue *queue, void *buffer, int64_t count);

// THIS FUNCTION DOES NOT DEQUEUE
// Attempts to copy `count` elements from the beginning of the queue, writing to
// `buffer`. Returns the number of elements actually copied.
// THIS FUNCTION DOES NOT DEQUEUE
int64_t Queue__read(Queue *queue, void *buffer, int64_t count);

// Returns the length of the queue in elements.
int64_t Queue__len(const Queue *queue);

// Returns the capacity of the queue in elements.
int64_t Queue__capacity(const Queue *queue);
