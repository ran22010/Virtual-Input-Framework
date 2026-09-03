#ifndef VINPUT_INPUT_H
#define VINPUT_INPUT_H

#include <linux/input.h>

#include "vinput_ringbuf.h"

#define MAX_INPUT_DEVICES 256

struct vinput_device_manager {
    struct input_dev *devices[MAX_INPUT_DEVICES];
    unsigned int count;
};

int vinput_input_init(struct vinput_ringbuf *input_rb);

void vinput_input_exit(void);

#endif