#ifndef VINPUT_RINGBUF_H
#define VINPUT_RINGBUF_H

#ifdef __KERNEL__
#include <linux/types.h>
#include <linux/compiler.h>
#else
#include <stdbool.h>
#include <stdint.h>
#endif

#include "uapi/vinput_uapi.h"

#define BUFFER_SIZE 256
#define BUFFER_MASK (BUFFER_SIZE - 1)

struct vinput_ringbuf{
    struct vinput_event buffer[BUFFER_SIZE];
    unsigned int head;
    unsigned int tail;
};

void vinput_ringbuf_init(struct vinput_ringbuf *rb);
bool vinput_ringbuf_is_empty(const struct vinput_ringbuf *rb);
bool vinput_ringbuf_is_full(const struct vinput_ringbuf *rb);
int vinput_ringbuf_push(struct vinput_ringbuf *rb, const struct vinput_event *event);
int vinput_ringbuf_pop(struct vinput_ringbuf *rb, struct vinput_event *event);

#endif 