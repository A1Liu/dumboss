#pragma once
#include "basics.h"

// A single producer single consumer Concurrent Queue. In the future it'll
// pseudo-support multiple consumer by allowing reads that don't alter the begin
// index. The returned pointer will either be NULL (on failure) or `buffer.data`
// on success.
typedef struct Queue Queue;

// Creates a queue using the provided buffer.
Queue *Queue__create(Buffer buffer, s32 elem_size);

// Attempts to enqueue `count` elements starting at `buffer`. Returns the number
// of elements actually enqueued.
s64 Queue__enqueue(Queue *queue, const void *buffer, s64 count);

// Attempts to dequeue `count` elements, writing to `buffer`. Returns the number
// of elements actually dequeued.
s64 Queue__dequeue(Queue *queue, void *buffer, s64 count);

// THIS FUNCTION DOES NOT DEQUEUE
// Attempts to copy `count` elements from the beginning of the queue, writing to
// `buffer`. Returns the number of elements actually copied.
// THIS FUNCTION DOES NOT DEQUEUE
s64 Queue__read(Queue *queue, void *buffer, s64 count);

// Returns the length of the queue in elements.
s64 Queue__len(const Queue *queue);

// Returns the capacity of the queue in elements.
s64 Queue__capacity(const Queue *queue);
