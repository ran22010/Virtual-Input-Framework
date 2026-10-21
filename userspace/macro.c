#include "macro.h"

#include <errno.h>
#include <linux/input-event-codes.h>
#include <stdlib.h>

static void free_event_list(struct vinput_macro_event *head)
{
    while (head) {
        struct vinput_macro_event *next = head->next;
        free(head);
        head = next;
    }
}

int vinput_event_playable(const struct vinput_event *event)
{
    if (!event || event->reserved != 0 || event->dev_id > VINPUT_MOUSE_ID)
        return 0;

    switch (event->type) {
    case EV_SYN:
        return event->code == SYN_REPORT && event->value == 0;
    case EV_REL:
        return event->dev_id == VINPUT_MOUSE_ID &&
               (event->code == REL_X || event->code == REL_Y);
    case EV_KEY:
        if (event->dev_id == VINPUT_KEYBOARD_ID)
            return event->code > KEY_RESERVED && event->code < KEY_MAX &&
                   event->value >= 0 && event->value <= 2;
        return (event->code == BTN_LEFT || event->code == BTN_RIGHT ||
                event->code == BTN_MIDDLE) &&
               (event->value == 0 || event->value == 1);
    default:
        return 0;
    }
}

int vinput_create_macro(struct vinput_macros *macros,
                        const struct vinput_event *buffer,
                        size_t length, int index)
{
    struct vinput_macro_event *head = NULL;
    struct vinput_macro_event *prev = NULL;
    __u64 prev_time = 0;
    size_t i;

    if (!macros || !buffer || length == 0 || length > VINPUT_MAX_EVENTS ||
        index < 0 || index >= MAX_MACROS)
        return -EINVAL;
    if (macros->heads[index])
        return -EEXIST;

    for (i = 0; i < length; ++i) {
        struct vinput_macro_event *curr;
        __u64 curr_time = buffer[i].timestamp_ns;

        if (!vinput_event_playable(&buffer[i]) ||
            (i && curr_time < prev_time)) {
            free_event_list(head);
            return -EINVAL;
        }

        curr = calloc(1, sizeof(*curr));
        if (!curr) {
            free_event_list(head);
            return -ENOMEM;
        }

        curr->event = buffer[i];
        if (i)
            curr->delay_ns = curr_time - prev_time;
        if (prev)
            prev->next = curr;
        else
            head = curr;
        prev = curr;
        prev_time = curr_time;
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

    free_event_list(macros->heads[index]);
    macros->heads[index] = NULL;
    macros->length--;
    return 0;
}

void vinput_clear_macros(struct vinput_macros *macros)
{
    int i;
    if (!macros)
        return;
    for (i = 0; i < MAX_MACROS; ++i) {
        free_event_list(macros->heads[i]);
        macros->heads[i] = NULL;
    }
    macros->length = 0;
}
