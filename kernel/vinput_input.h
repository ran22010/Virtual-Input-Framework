#ifndef VINPUT_INPUT_H
#define VINPUT_INPUT_H

#include <linux/input.h>

#define MAX_INPUT_DEVICES 256

struct vinput_device_manager {
    struct input_dev *devices[MAX_INPUT_DEVICES];
    unsigned int count;
};

/*
 * Initialize the input event listener.
 *
 * Registers the input_handler with the Linux input subsystem.
 */
int vinput_input_init(void);

/*
 * Stop listening for input events.
 *
 * Unregisters the input_handler and disconnects
 * all associated input handles.
 */
void vinput_input_exit(void);

#endif