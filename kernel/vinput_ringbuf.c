#include <linux/errno.h>
#include <linux/smp.h>
#include <linux/sched.h>

#include "vinput_ringbuf.h"

void vinput_ringbuf_init(struct vinput_ringbuf *rb)
{
    rb->head = 0;
    rb->tail = 0;
    spin_lock_init(&rb->push_lock);
    init_waitqueue_head(&rb->not_empty);
}

bool vinput_ringbuf_is_empty(const struct vinput_ringbuf *rb)
{
    return READ_ONCE(rb->head) == READ_ONCE(rb->tail);
}

bool vinput_ringbuf_is_full(const struct vinput_ringbuf *rb)
{
    unsigned int head = READ_ONCE(rb->head);
    unsigned int tail = READ_ONCE(rb->tail);

    return (head - tail) >= BUFFER_SIZE;
}

int vinput_ringbuf_push(struct vinput_ringbuf *rb, const struct vinput_event *event)
{
    unsigned long flags;
    unsigned int head;
    unsigned int tail;

    spin_lock_irqsave(&rb->push_lock, flags);
    head = rb->head;
    tail = smp_load_acquire(&rb->tail);

    if (head - tail >= BUFFER_SIZE) {
        spin_unlock_irqrestore(&rb->push_lock, flags);
        return -ENOSPC;
    }

    rb->buffer[head & BUFFER_MASK] = *event;
    smp_store_release(&rb->head, head + 1);
    spin_unlock_irqrestore(&rb->push_lock, flags);

    wake_up_interruptible(&rb->not_empty);
    return 0;
}

/* One consumer; producers are serialized by push_lock. */
int vinput_ringbuf_pop(struct vinput_ringbuf *rb, struct vinput_event *event)
{
    unsigned int tail = READ_ONCE(rb->tail);
    unsigned int head = smp_load_acquire(&rb->head);

    if (head == tail)
        return -EAGAIN;

    *event = rb->buffer[tail & BUFFER_MASK];
    smp_store_release(&rb->tail, tail + 1);
    return 0;
}
