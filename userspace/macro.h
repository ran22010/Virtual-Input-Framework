#ifndef VINPUT_USER_MACRO_H
#define VINPUT_USER_MACRO_H

#include <stddef.h>
#include "uapi/vinput_uapi.h"

#define MAX_MACROS 256
#define VINPUT_MAX_EVENTS 100000U
#define VINPUT_KEYBOARD_ID 0U
#define VINPUT_MOUSE_ID 1U

/* Preserved from the kernel implementation: each node holds one timed event. */
struct vinput_macro_event {
    __u64 delay_ns;
    struct vinput_event event;
    struct vinput_macro_event *next;
};

struct vinput_macros {
    struct vinput_macro_event *heads[MAX_MACROS];
    int length;
};

/* Return 0 on success, or a negative errno value on failure. */
int vinput_create_macro(struct vinput_macros *macros,
                        const struct vinput_event *buffer,
                        size_t length, int index);
int vinput_free_macro(struct vinput_macros *macros, int index);
void vinput_clear_macros(struct vinput_macros *macros);

/* Conservative, known-supported subset of the current kernel write ABI. */
int vinput_event_playable(const struct vinput_event *event);

#endif
