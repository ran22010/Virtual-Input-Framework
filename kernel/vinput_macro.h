#ifndef VINPUT_MACRO_H
#define VINPUT_MACRO_H

#ifdef __KERNEL__
#include <linux/types.h>
#include <linux/compiler.h>
#else
#include <stdbool.h>
#include <stdint.h>
#endif

#include "uapi/vinput_uapi.h"

#define MAX_MACROS 256


struct vinput_macro_event{
    __u64 delay;
    struct vinput_event event;
    struct vinput_macro_event *next;
};

struct vinput_macros{
    struct vinput_macro_event *heads[MAX_MACROS];
    int length;
};

int vinput_play_macro(struct vinput_macros *macros, int index);
int vinput_create_macro(struct vinput_macros *macros, struct vinput_event *buffer, int length, int index);
int vinput_free_macro(struct vinput_macros *macros, int index);


#endif 