#include "ring_buffer.h"

void ring_buffer_init(ring_buffer_t *rb)
{
    rb->head = 0;
    rb->tail = 0;
    rb->count = 0;
}

bool ring_buffer_is_empty(ring_buffer_t *rb)
{
    return rb->count == 0;
}

bool ring_buffer_is_full(ring_buffer_t *rb)
{
    return rb->count >= RING_BUFFER_SIZE;
}

bool ring_buffer_put(ring_buffer_t *rb, char c)
{
    if (ring_buffer_is_full(rb))
    {
        return false;
    }

    rb->data[rb->head] = c;
    rb->head = (rb->head + 1U) % RING_BUFFER_SIZE;
    rb->count++;
    return true;
}

bool ring_buffer_get(ring_buffer_t *rb, char *c)
{
    if (ring_buffer_is_empty(rb))
    {
        return false;
    }

    *c = rb->data[rb->tail];
    rb->tail = (rb->tail + 1U) % RING_BUFFER_SIZE;
    rb->count--;
    return true;
}