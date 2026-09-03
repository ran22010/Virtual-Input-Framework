#ifdef __KERNEL__
#include <linux/types.h>
#include <linux/compiler.h>
#else
#include <stdbool.h>
#include <stdint.h>
#endif

#include <linux/delay.h>
#include <linux/ktime.h>
#include <linux/slab.h>

#include "uapi/vinput_uapi.h"
#include "vinput_macro.h"
#include "vinput_ringbuf.h"


void vinput_play_macro(struct vinput_ringbuf *rb, struct vinput_macro_event *head){
    struct vinput_macro_event *curr_event = head;

    while (curr_event) { //if null stops
        usleep_range(curr_event->delay_ns / 1000, curr_event->delay_ns / 1000 + 1);
        struct vinput_event event = curr_event->event;
        event.timestamp_ns = ktime_get_ns();
        vinput_ringbuf_push(rb, &event); 
        curr_event = curr_event->next;
    }
}

int vinput_create_macro(struct vinput_macros *macros, struct vinput_event *buffer, int length, int index){
    if (length <= 0){
        return -EINVAL;
    }

    if (index < 0 || index >= MAX_MACROS){
        return -EINVAL;
    }

    if (macros == NULL || buffer == NULL){
        return -EINVAL;
    }

    if (macros->heads[index] != NULL){
        return -EEXIST;
    }

    struct vinput_macro_event *head = NULL;
    struct vinput_macro_event *curr = NULL;
    struct vinput_macro_event *prev = NULL;

    __u64 prev_time = 0;

    for (int i = 0; i < length; i++){
        curr = (struct vinput_macro_event *)kzalloc(sizeof(struct vinput_macro_event), GFP_KERNEL);
        
        if (curr == NULL){
            return -ENOMEM;
        }

        if (i == 0){
            head = curr;
        }

        if (prev){
            prev->next = curr;
        }

        curr->event = buffer[i];
        curr->next = NULL;

        __u64 curr_time = buffer[i].timestamp_ns;

        if (prev_time > 0){
            curr->delay_ns = curr_time - prev_time;
        }
        prev_time = curr_time;
        prev = curr;
    }

    macros->heads[index] = head;
    macros->length++;
    
    return 0;
}

int vinput_free_macro(struct vinput_macro_event *head){
    if (head == NULL){
        return -EINVAL;
    }

    while (head != NULL){
        struct vinput_macro_event *next = head->next;
        kfree(head);
        head = next;
    }

    return 0;
}
