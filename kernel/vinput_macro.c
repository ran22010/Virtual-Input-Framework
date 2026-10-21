#include <linux/delay.h>
#include <linux/ktime.h>
#include <linux/slab.h>
#include <linux/errno.h>

#include "uapi/vinput_uapi.h"
#include "vinput_macro.h"
#include "vinput_ringbuf.h"

static void vinput_free_macro_events(struct vinput_macro_event *head)
{
    while (head) {
        struct vinput_macro_event *next = head->next;
        kfree(head);
        head = next;
    }
}

int vinput_play_macro(struct vinput_ringbuf *rb, struct vinput_macro_event *head)
{
    struct vinput_macro_event *curr_event = head;

    if (!rb)
        return -EINVAL;

    while (curr_event) {
        struct vinput_event event = curr_event->event;
        int ret;

        usleep_range(curr_event->delay_ns / 1000,
                     curr_event->delay_ns / 1000 + 1);
        event.timestamp_ns = ktime_get_ns();
        ret = vinput_ringbuf_push(rb, &event);
        if (ret)
            return ret;
        curr_event = curr_event->next;
    }
    return 0;
}

int vinput_create_macro(struct vinput_macros *macros, struct vinput_event *buffer,
                        int length, int index)
{
    struct vinput_macro_event *head = NULL;
    struct vinput_macro_event *prev = NULL;
    __u64 prev_time = 0;
    int i;

    if (!macros || !buffer || length <= 0 || index < 0 || index >= MAX_MACROS)
        return -EINVAL;

    if (macros->heads[index])
        return -EEXIST;

    for (i = 0; i < length; i++) {
        struct vinput_macro_event *curr;
        __u64 curr_time = buffer[i].timestamp_ns;

        if (i && curr_time < prev_time) {
            vinput_free_macro_events(head);
            return -EINVAL;
        }

        curr = kzalloc(sizeof(*curr), GFP_KERNEL);
        if (!curr) {
            vinput_free_macro_events(head);
            return -ENOMEM;
        }

        if (!head)
            head = curr;
        if (prev)
            prev->next = curr;

        curr->event = buffer[i];
        if (i)
            curr->delay_ns = curr_time - prev_time;
        prev_time = curr_time;
        prev = curr;
    }

    macros->heads[index] = head;
    macros->length++;
    return 0;
}

int vinput_free_macro(struct vinput_macros *macros, int index)
{
    if (!macros || index < 0 || index >= MAX_MACROS)
        return -EINVAL;
    if (!macros->heads[index])
        return -ENOENT;

    vinput_free_macro_events(macros->heads[index]);
    macros->heads[index] = NULL;
    macros->length--;
    return 0;
}
