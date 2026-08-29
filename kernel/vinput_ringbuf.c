#ifdef __KERNEL__
#include <linux/module.h>
#include <linux/errno.h>
#include <linux/smp.h>
#else
#include <errno.h>
#include <stdbool.h>

#ifndef READ_ONCE
#define READ_ONCE(x) (*(volatile typeof(x) *)&(x))
#endif

#ifndef WRITE_ONCE
#define WRITE_ONCE(x, val) (*(volatile typeof(x) *)&(x) = (val))
#endif

#ifndef smp_wmb
#define smp_wmb() __atomic_thread_fence(__ATOMIC_RELEASE)
#endif

#ifndef smp_rmb
#define smp_rmb() __atomic_thread_fence(__ATOMIC_ACQUIRE)
#endif
#endif

#include "vinput_ringbuf.h"

void vinput_ringbuf_init(struct vinput_ringbuf *rb){
    rb->head = 0;
    rb->tail = 0;
    spin_lock_init(&rb->push_lock);
}

bool vinput_ringbuf_is_empty(const struct vinput_ringbuf *rb){
    return READ_ONCE(rb->head) == READ_ONCE(rb->tail);
}

bool vinput_ringbuf_is_full(const struct vinput_ringbuf *rb){
    unsigned int head = READ_ONCE(rb->head);
    unsigned int tail = READ_ONCE(rb->tail);

    return (head - tail) >= BUFFER_SIZE;
}

int vinput_ringbuf_push(struct vinput_ringbuf *rb, const struct vinput_event *event){
    unsigned long flags;
    unsigned int head;

    spin_lock_irqsave(&rb->push_lock, flags);

    head = rb->head;

    if (vinput_ringbuf_is_full(rb)){
        spin_unlock_irqrestore(&rb->push_lock, flags);
        return -ENOSPC;
    }

    rb->buffer[head & BUFFER_MASK] = *event;

    smp_wmb();

    WRITE_ONCE(rb->head, head + 1);

    spin_unlock_irqrestore(&rb->push_lock, flags);
    
    return 0;
}

int vinput_ringbuf_pop(struct vinput_ringbuf *rb, struct vinput_event *event){
    //check if empty
    //remove event from buffer add it to event
    //increment tail
    //unsigned int head = READ_ONCE(rb->head);
    unsigned int tail = rb->tail;

    if (vinput_ringbuf_is_empty(rb)){
        return -EAGAIN;
    }

    *event = rb->buffer[tail & BUFFER_MASK];

    smp_rmb();

    WRITE_ONCE(rb->tail, tail + 1);

    return 0;
}
