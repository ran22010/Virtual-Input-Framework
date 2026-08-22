#ifdef __KERNEL__
#include <linux/types.h>
#include <linux/compiler.h>
#else
#include <stdbool.h>
#include <stdint.h>
#endif

#include "uapi/vinput_uapi.h"
#include "vinput_macro.h"

int vinput_play_macro(struct vinput_macro_event *head){
    if (index < 0 || index >= MAX_MACROS){
        return -EINVAL;
    }

    if (macros == NULL){
        return -EINVAL;
    }

    struct vinput_macro_event *curr_event = macros->heads[index];

    while (curr_event) { //if null stops
        //push event onto some queue for execution
        //wait curr_event->ts 
        //curr_event = curr_event->next;
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
        curr = (vinput_macro_event *)kzalloc(sizeof(vinput_macro_event), GFP_KERNEL);
        
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
            curr->delay = curr_time - prev_time;
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
        vinput_macro_event *next = head->next;
        kfree(head);
        head = next;
    }

    macros->heads[index] = NULL;
    macros->length--;

    return 0;
}