#ifndef RING_BUFFER_H
#define RING_BUFFER_H

#include <stdint.h>
#include <stdbool.h>

#define RING_BUFFER_SIZE 64

typedef struct
{
    volatile uint8_t head;
    volatile uint8_t tail;
    volatile uint8_t count;
    char data[RING_BUFFER_SIZE];
} ring_buffer_t;

void ring_buffer_init(ring_buffer_t *rb);
bool ring_buffer_is_empty(ring_buffer_t *rb);
bool ring_buffer_is_full(ring_buffer_t *rb);
bool ring_buffer_put(ring_buffer_t *rb, char c);
bool ring_buffer_get(ring_buffer_t *rb, char *c);

#endif